#ifndef NFC_OPERATIONS_H
#define NFC_OPERATIONS_H

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include "server_operations.h"


// Define the pins for SPI communication
#define PN532_SS 15
#define PN532_MOSI 23
#define PN532_MISO 19
#define PN532_SCK 18
// #define PN532_RST 5
String token;

//--------------------------------------------------------------------------------------------------------------------------
// Define the key for Mifare Classic cards
#define NR_SHORTSECTOR (32)         // Number of short sectors on Mifare 1K/4K
#define NR_LONGSECTOR (8)           // Number of long sectors on Mifare 4K
#define NR_BLOCK_OF_SHORTSECTOR (4) // Number of blocks in a short sector
#define NR_BLOCK_OF_LONGSECTOR (16) // Number of blocks in a long sector

// Determine the sector trailer block based on sector number
#define BLOCK_NUMBER_OF_SECTOR_TRAILER(sector) (((sector) < NR_SHORTSECTOR) ? ((sector) * NR_BLOCK_OF_SHORTSECTOR + NR_BLOCK_OF_SHORTSECTOR - 1) : (NR_SHORTSECTOR * NR_BLOCK_OF_SHORTSECTOR + (sector - NR_SHORTSECTOR) * NR_BLOCK_OF_LONGSECTOR + NR_BLOCK_OF_LONGSECTOR - 1))

// Determine the sector's first block based on the sector number
#define BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector) (((sector) < NR_SHORTSECTOR) ? ((sector) * NR_BLOCK_OF_SHORTSECTOR) : (NR_SHORTSECTOR * NR_BLOCK_OF_SHORTSECTOR + (sector - NR_SHORTSECTOR) * NR_BLOCK_OF_LONGSECTOR))

// The default Mifare Classic key
static const uint8_t KEY_DEFAULT_KEYAB[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//--------------------------------------------------------------------------------------------------------------------------

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
      Serial.print((char)data[i]);
    else
      Serial.print('.');
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
        authenticated = true;
      else
        Serial.println("Authentication error");
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
          Serial.print("  ");
        else
          Serial.print(" ");
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
bool writeToken(const uint8_t *data, uint8_t block = blockToken)
{
  if (!readUID())
  {
    Serial.println("No NFC card found.");
    return false;
  }

  if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, block, 1, keyuniversal))
  {
    Serial.println("Authentication failed");
    resetPN532();
    return false;
  }

  uint8_t dataBytes[16] = {0}; // Ensure dataBytes is 16 bytes long
  memcpy(dataBytes, data, 16); // Copy data to dataBytes

  if (nfc.mifareclassic_WriteDataBlock(block, dataBytes))
  {
    Serial.println("Data written to NFC card successfully");
    return true;
  }
  else
  {
    Serial.println("Failed to write data to NFC card");
    return false;
  }
  resetPN532();
}

/**
 * @brief Reads a token from an NFC card.
 *
 * @return The token read from the NFC card.
 */
String readToken()
{
  String result = "";
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
  uint8_t timer = 500;
  while (!nfc.inListPassiveTarget() && timer-- > 0)
  {
    delay(10);
  }
  if (timer == 0)
  {
    Serial.println("No device detected.");
    return "";
  }

  Serial.println("Device detected!");

  // Send the SELECT AID command
  if (nfc.inDataExchange(command, sizeof(command), response, &responseLength))
  {
    Serial.println("Data read from phone successfully:");
    printData(response, responseLength);
    result = String((char *)response, responseLength);
  }
  else
  {
    if (!readUID())
    {
      Serial.println("Failed to read UID");
      resetPN532();
      return "";
    }

    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockToken, 1, keyuniversal))
    {
      Serial.println("Authentication failed");
      resetPN532();
      return "";
    }

    uint8_t data[16] = {0}; // Ensure data is 16 bytes long
    uint8_t success = nfc.mifareclassic_ReadDataBlock(blockToken, data);
    if (success)
    {
      Serial.println("Data read from NFC card successfully:");
      printData(data, 16);
      result = String((char *)data, 16);
    }
    else
    {
      Serial.println("Failed to read data from NFC card");
      result = "";
    }
  }

  // Reset the PN532
  resetPN532();

  return result;
}

/**
 * @brief Reads a token from an NFC card.
 *
 * @param token Pointer to a String to store the token.
 * @param device Pointer to a String to store the device type.
 * @return True if the token was read successfully, false otherwise.
 */
bool readTokenLoop(String* token, String* device) {
  *token = "";
  *device = "";
  
  // APDU Command: SELECT AID
  uint8_t command[] = {
      0x00, 0xA4, 0x04, 0x00,                   // CLA, INS, P1, P2 (SELECT AID command)
      0x07,                                     // Length of AID (7 bytes)
      0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // AID (example)
      0x00                                      // Le (expected length of response)
  };

  uint8_t response[17];
  uint8_t responseLength;

  // Send the SELECT AID command
  if (nfc.inDataExchange(command, sizeof(command), response, &responseLength)) {
    Serial.println("Data read from phone successfully:");
    printData(response, responseLength);
    *token = String((char *)response, responseLength);
    *device = "phone";
    return true;
  } else {
    if (!readUID()) {
      Serial.println("Failed to read UID");
      resetPN532();
      return false;
    }

    if (!nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockToken, 1, keyuniversal)) {
      Serial.println("Authentication failed");
      resetPN532();
      return false;
    }

    uint8_t data[16] = {0}; // Ensure data is 16 bytes long
    uint8_t success = nfc.mifareclassic_ReadDataBlock(blockToken, data);
    if (success) {
      Serial.println("Data read from NFC card successfully:");
      printData(data, 16);
      *token = String((char *)data, 16);
      *device = "card";
      return true;
    } else {
      Serial.println("Failed to read data from NFC card");
      *token = "";
      *device = "";
      return false;
    }
  }

  // Reset the PN532
  resetPN532();
  return false;
}

#endif // NFC_OPERATIONS_H