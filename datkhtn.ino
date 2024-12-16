#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "nfc_operations.h"
#include "wifi_setup.h"
#include "server_operations.h"
#include "display_operations.h"

#define RELAY_PIN 13
#define BUZZER_PIN 12

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Hello!");

  // Initialize the display
  initDisplay();

  // Connect to WiFi
  connectToWiFi();

  // Initialize SPI for PN532
  initNFC();
  // Initialize I2C for SSD1306 OLED display
  initDisplay();

  // Initialize relay and buzzer pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("Enter 1 for login, 2 for create, 3 for remove, 4 for dump data, 5 write data, 6 for read data, 7 test door");
  Serial.println("Waiting for number");
  displayResponse((uint8_t *)"Waiting for number");
}

String test;

/**
 * @brief Controls the relay.
 *
 * @param time The time to keep the relay on in seconds.
 */
void controlRelay(uint8_t time = 10)
{
  digitalWrite(RELAY_PIN, LOW);
  tone(BUZZER_PIN, 1000, 1000);
  noTone(BUZZER_PIN);
  delay(time * 1000);
  digitalWrite(RELAY_PIN, HIGH);
}

void loop(void)
{
  if (Serial.available() > 0)
  {
    int action = Serial.parseInt();
    char response[64];
    switch (action)
    {
    case 1:
      Serial.println("Login");
      if (login())
      {
        strcpy(response, "Login successful");
        controlRelay(10); // Turn on relay for 10 seconds
      }
      else
      {
        strcpy(response, "Login failed");
      }
      break;
    case 2:
      Serial.println("Create");
      if (create())
      {
        strcpy(response, "Create successful");
      }
      else
      {
        strcpy(response, "Create failed");
      }
      break;
    case 3:
      Serial.println("Remove");
      if (remove())
      {
        strcpy(response, "Remove successful");
      }
      else
      {
        strcpy(response, "Remove failed");
      }
      break;
    case 4:
      Serial.println("Dump data from card");
      dumpData();
      strcpy(response, "Data dumped");
      break;
    case 5:
      Serial.println("Write data");
      writeToken((const uint8_t *)"Tokens");
      strcpy(response, "Data written");
      break;
    case 6:
      Serial.println("Read data");
      readToken();
      strcpy(response, "Data read");
      break;
    case 7:
      controlRelay(10);
      strcpy(response, "Door test");
      break;
    case 8:
      Serial.println("Format card to default");
      reformatMifareClassicCard();
      strcpy(response, "Card formatted to default");
      break;
    case 9:
      Serial.println("test phone");
      test = readToken();
      Serial.print("token from read ");
      Serial.println(test);
      break;
    default:
      strcpy(response, "Invalid action");
      Serial.println("Invalid action. Enter 1 for login, 2 for create, 3 for remove, 4 for dump data, 5 for write Token, 6 for read Token, 7 for test door");
      break;
    }
    Serial.println(response);
    displayResponse((uint8_t *)response);
    Serial.println("Waiting for number");
  }
  Serial.flush();
}
