#ifndef NFC_OPERATIONS_H
#define NFC_OPERATIONS_H

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include "server_operations.h"

// Define the pins for SPI communication
#define PN532_SS 4
#define PN532_MOSI 23
#define PN532_MISO 19
#define PN532_SCK 18
// #define PN532_RST 5

Adafruit_PN532 nfc(PN532_SS);

uint8_t uid[7];
uint8_t uidLength;
uint8_t blockToken = 8;
uint8_t keyuniversal[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/**
 * @brief Resets the PN532 module.
 */
void resetPN532()
{
  nfc.begin();
  nfc.SAMConfig();
}

/**
 * @brief Initializes the NFC module.
 */
void initNFC()
{
  SPI.begin();
  Serial.println("SPI connection established successfully!");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  while (!versiondata)
  {
    resetPN532();
    versiondata = nfc.getFirmwareVersion();
  }

  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.SAMConfig();
}

/**
 * @brief Prints data in both hexadecimal and ASCII formats.
 *
 * @param data Pointer to the data array.
 * @param length Length of the data array.
 */
void printData(uint8_t *data, uint8_t length)
{
  Serial.print("Hex: ");
  for (uint8_t i = 0; i < length; i++)
  {
    Serial.print(" 0x");
    Serial.print(data[i], HEX);
  }
  Serial.println("");

  Serial.print("ASCII: ");
  for (uint8_t i = 0; i < length; i++)
  {
    if (data[i] >= 32 && data[i] <= 126)
    { // Printable ASCII range
      Serial.print((char)data[i]);
    }
    else
    {
      Serial.print('.');
    }
  }
  Serial.println("");
}

/**
 * @brief Reads the UID of an NFC card.
 *
 * @param timeout Timeout in milliseconds (default is 10 seconds).
 * @return true if the UID was read successfully.
 * @return false if there was an error or timeout.
 */
bool readUID(unsigned long timeout = 10000)
{
  uint8_t success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, timeout);

  if (success)
  {
    Serial.println("Found an NFC card!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      Serial.print(" 0x");
      Serial.print(uid[i], HEX);
    }
    Serial.println("");
    return true;
  }

  Serial.println("Timeout waiting for an NFC card.");
  return false;
}

/**
 * @brief Dumps data from an NFC card.
 */
void dumpData()
{
  // Check if an NFC card is present
  if (!readUID())
  {
    Serial.println("No NFC card found.");
    return;
  }

  uint8_t success;
  bool authenticated = false;
  uint8_t data[16];
  Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

  for (uint8_t currentblock = 0; currentblock < 64; currentblock++)
  {
    if (nfc.mifareclassic_IsFirstBlock(currentblock))
      authenticated = false;

    if (!authenticated)
    {
      Serial.print("------------------------Sector ");
      Serial.print(currentblock / 4, DEC);
      Serial.println("-------------------------");
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, currentblock, 1, keyuniversal);
      if (success)
      {
        authenticated = true;
      }
      else
      {
        Serial.println("Authentication error");
      }
    }
    if (!authenticated)
    {
      Serial.print("Block ");
      Serial.print(currentblock, DEC);
      Serial.println(" unable to authenticate");
    }
    else
    {
      success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
      if (success)
      {
        Serial.print("Block ");
        Serial.print(currentblock, DEC);
        if (currentblock < 10)
        {
          Serial.print("  ");
        }
        else
        {
          Serial.print(" ");
        }
        nfc.PrintHexChar(data, 16);
      }
      else
      {
        Serial.print("Block ");
        Serial.print(currentblock, DEC);
        Serial.println(" unable to read this block");
      }
    }
  }

  Serial.flush();
  resetPN532();
}

/**
 * @brief Writes a token to an NFC card.
 *
 * @param data The token data to write.
 * @param block The block number to write to (default is blockToken).
 */
void writeToken(const uint8_t *data, uint8_t block = blockToken)
{
  if (!readUID())
  {
    Serial.println("No NFC card found.");
    return;
  }

  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block, 1, keyuniversal))
  {
    Serial.println("Authentication failed");
    while (1)
      ; // halt
  }

  uint8_t dataBytes[16] = {0};                                     // Ensure dataBytes is 16 bytes long
  memcpy(dataBytes, data, sizeof(dataBytes)); // Copy data to dataBytes

  uint8_t success = nfc.mifareclassic_WriteDataBlock(block, dataBytes);
  if (success)
  {
    Serial.println("Data written to NFC card successfully");
  }
  else
  {
    Serial.println("Failed to write data to NFC card");
  }
}

/**
 * @brief Reads a token from an NFC card.
 *
 * @return The token read from the NFC card.
 */
uint8_t* readToken()
{
  static uint8_t result[16] = {0};
  // APDU Command: SELECT AID
  uint8_t command[] = {
      0x00, 0xA4, 0x04, 0x00,                   // CLA, INS, P1, P2 (SELECT AID command)
      0x07,                                     // Length of AID (7 bytes)
      0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // AID (example)
      0x00                                      // Le (expected length of response)
  };

  uint8_t response[17];
  uint8_t responseLength;

  // Wait until a device is in range
  Serial.println("Waiting for device...");
  while (!nfc.inListPassiveTarget())
  {
    delay(100);
  }
  Serial.println("Device detected!");

  // Send the SELECT AID command
  if (nfc.inDataExchange(command, sizeof(command), response, &responseLength))
  {
    Serial.println("Data read from phone successfully:");
    printData(response, responseLength);
    memcpy(result, response, 16); // Ensure only 16 bytes are copied
  }
  else
  {
    if (!readUID())
    {
      Serial.println("Failed to read UID");
      resetPN532();
      return result;
    }

    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockToken, 1, keyuniversal))
    {
      Serial.println("Authentication failed");
      resetPN532();
      return result;
    }

    uint8_t data[16] = {0}; // Ensure data is 16 bytes long
    uint8_t success = nfc.mifareclassic_ReadDataBlock(blockToken, data);
    if (success)
    {
      Serial.println("Data read from NFC card successfully:");
      printData(data, 16);
      memcpy(result, data, 16);
    }
    else
    {
      Serial.println("Failed to read data from NFC card");
    }
  }

  // Reset the PN532
  resetPN532();

  return result;
}

/**
 * @brief Creates a new token on the NFC card and sends it to the server.
 *
 * @return true if the token was created and sent successfully.
 * @return false if there was an error during the process.
 */
bool create()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Lost connected to WiFi");
    return false;
  }

  if (!readUID())
  {
    Serial.println("No NFC card found.");
    return false;
  }

  uint8_t response[64];
  sendRequest("create", nullptr, response);
  if (strncmp((char*)response, "Create processed. Token: ", 25) == 0)
  {
    writeToken(response + 25, blockToken);
    return true;
  }
  return false;
}

/**
 * @brief Logs in using the token read from the NFC card.
 *
 * @return true if login was successful.
 * @return false if there was an error during the process.
 */
bool login()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Lost connection to WiFi");
    return false;
  }

  uint8_t* token = readToken();
  Serial.print("Token: ");
  for (int i = 0; i < 16; i++) {
    Serial.print(token[i], HEX);
  }
  Serial.println();

  if (strlen((char*)token) == 0)
  {
    Serial.println("Failed to read token from NFC card.");
    return false;
  }

  uint8_t response[64];
  sendRequest("login", token, response);
  if (strcmp((char*)response, "Login processed") == 0)
  {
    return true;
  }
  else
  {
    Serial.print("Login failed: ");
    Serial.println((char*)response);
    return false;
  }
  resetPN532();
}

/**
 * @brief Removes the token from the server using the token read from the NFC card.
 *
 * @return true if the token was removed successfully.
 * @return false if there was an error during the process.
 */
bool remove()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Lost connected to WiFi");
    return false;
  }

  if (!readUID())
  {
    Serial.println("No NFC card found.");
    return false;
  }

  uint8_t* token = readToken();
  if (strlen((char*)token) == 0)
  {
    Serial.println("Failed to read token from NFC card.");
    return false;
  }

  uint8_t response[64];
  sendRequest("remove", token, response);
  if (strcmp((char*)response, "Remove processed") == 0)
  {
    return true;
  }
  else
  {
    Serial.print("Remove failed: ");
    Serial.println((char*)response);
    return false;
  }
}

#endif // NFC_OPERATIONS_H