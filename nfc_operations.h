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
String token;

//--------------------------------------------------------------------------------------------------------------------------
// Define the key for Mifare Classic cards
#define NR_SHORTSECTOR          (32)    // Number of short sectors on Mifare 1K/4K
#define NR_LONGSECTOR           (8)     // Number of long sectors on Mifare 4K
#define NR_BLOCK_OF_SHORTSECTOR (4)     // Number of blocks in a short sector
#define NR_BLOCK_OF_LONGSECTOR  (16)    // Number of blocks in a long sector

// Determine the sector trailer block based on sector number
#define BLOCK_NUMBER_OF_SECTOR_TRAILER(sector) (((sector)<NR_SHORTSECTOR)? \
  ((sector)*NR_BLOCK_OF_SHORTSECTOR + NR_BLOCK_OF_SHORTSECTOR-1):\
  (NR_SHORTSECTOR*NR_BLOCK_OF_SHORTSECTOR + (sector-NR_SHORTSECTOR)*NR_BLOCK_OF_LONGSECTOR + NR_BLOCK_OF_LONGSECTOR-1))

// Determine the sector's first block based on the sector number
#define BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector) (((sector)<NR_SHORTSECTOR)? \
  ((sector)*NR_BLOCK_OF_SHORTSECTOR):\
  (NR_SHORTSECTOR*NR_BLOCK_OF_SHORTSECTOR + (sector-NR_SHORTSECTOR)*NR_BLOCK_OF_LONGSECTOR))

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
 * @brief Creates a new token on the NFC card and sends it to the server.
 *
 * @return true if the token was created and sent successfully.
 * @return false if there was an error during the process.
 */
bool create()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Lost connection to WiFi");
    return false;
  }
  if (!writeToken((const uint8_t *)"Tokens", 8))
  {
    return false;
  }
  
  uint8_t response[64] = {0};
  sendRequest("create", nullptr, response);

  const char *expectedResponse = "Create processed. Token: ";
  if (strncmp((char *)response, expectedResponse, strlen(expectedResponse)) == 0)
  {
    Serial.print("Token: ");
    Serial.println((char *)response + strlen(expectedResponse));
    printData(response + strlen(expectedResponse), 16);
    if (writeToken(response + strlen(expectedResponse)))
    {
      Serial.println("Token written to NFC card successfully");
      return true;
    }
  }
  Serial.println("Failed to write token to NFC card");
  Serial.println("Failed to create token");
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
  resetPN532();
  token = readToken();
  Serial.println(token);
  if (token.length() == 0)
  {
    Serial.println("Failed to read token from NFC card.");
    return false;
  }

  uint8_t response[64];
  sendRequest("login", (uint8_t *)token.c_str(), response);
  if (strcmp((char *)response, "Login processed") == 0)
    return true;
  else
  {
    Serial.print("Login failed: ");
    Serial.println((char *)response);
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
    Serial.println("Lost connection to WiFi");
    return false;
  }

  if (!readUID())
  {
    Serial.println("No NFC card found.");
    return false;
  }

  String token = readToken();
  if (token.length() == 0)
  {
    Serial.println("Failed to read token from NFC card.");
    return false;
  }

  uint8_t response[64];
  sendRequest("remove", (uint8_t *)token.c_str(), response);
  if (strcmp((char *)response, "Remove processed") == 0)
  {
    return true;
  }
  else
  {
    Serial.print("Remove failed: ");
    Serial.println((char *)response);
    return false;
  }
}

/**
 * @brief Reformats a Mifare Classic card.
 */
void reformatMifareClassicCard()
{
  uint8_t success;
  uint8_t uid[7] = {0}; // Buffer to store the returned UID
  uint8_t uidLength;    // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t blockBuffer[16]; // Buffer to store block contents
  uint8_t blankAccessBits[3] = {0xff, 0x07, 0x80};
  uint8_t idx = 0;
  uint8_t numOfSector = 16; // Assume Mifare Classic 1K for now (16 4-block sectors)

  Serial.println("Place your NDEF formatted Mifare Classic 1K card on the reader");
  Serial.println("and press any key to continue ...");

  // Wait for user input before proceeding
  while (!Serial.available());
  while (Serial.available()) Serial.read();

  // Wait for an ISO14443A type card (Mifare, etc.)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success)
  {
    Serial.println("Found an ISO14443A card/tag");
    Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (uidLength != 4)
    {
      Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!");
      return;
    }

    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
    Serial.println("");
    Serial.println("Reformatting card for Mifare Classic (please don't touch it!) ... ");

    // Run through the card sector by sector
    for (idx = 0; idx < numOfSector; idx++)
    {
      // Authenticate the current sector using key B
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, BLOCK_NUMBER_OF_SECTOR_TRAILER(idx), 1, (uint8_t *)KEY_DEFAULT_KEYAB);
      if (!success)
      {
        Serial.print("Authentication failed for sector "); Serial.println(idx);
        return;
      }

      // Write to the other blocks
      memset(blockBuffer, 0, sizeof(blockBuffer));
      if (!(nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(idx), blockBuffer)))
      {
        Serial.print("Unable to write to sector "); Serial.println(idx);
        return;
      }
      if (!(nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(idx) + 1, blockBuffer)))
      {
        Serial.print("Unable to write to sector "); Serial.println(idx);
        return;
      }
      if (!(nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(idx) + 2, blockBuffer)))
      {
        Serial.print("Unable to write to sector "); Serial.println(idx);
        return;
      }

      // Reset both keys to default
      memcpy(blockBuffer, KEY_DEFAULT_KEYAB, sizeof(KEY_DEFAULT_KEYAB));
      memcpy(blockBuffer + 6, blankAccessBits, sizeof(blankAccessBits));
      blockBuffer[9] = 0x69;
      memcpy(blockBuffer + 10, KEY_DEFAULT_KEYAB, sizeof(KEY_DEFAULT_KEYAB));

      // Write the trailer block
      if (!(nfc.mifareclassic_WriteDataBlock(BLOCK_NUMBER_OF_SECTOR_TRAILER(idx), blockBuffer)))
      {
        Serial.print("Unable to write trailer block of sector "); Serial.println(idx);
        return;
      }
    }
  }
  else
  {
    Serial.println("No card found.");
  }

  Serial.println("\n\nDone!");
  delay(1000);
  Serial.flush();
}

#endif // NFC_OPERATIONS_H