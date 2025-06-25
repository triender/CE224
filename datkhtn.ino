#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <PubSubClient.h>
#include "nfc_operations.h"
#include "wifi_setup.h"
#include "server_operations.h"
#include "display_operations.h"
#include <ArduinoJson.h>

#define RELAY_PIN 13
#define BUZZER_PIN 14

const char *mqtt_server = "ch1cken.mywire.org";
const char *mqtt_topic = "esp32/toDoor";

WiFiClient espClient;
PubSubClient clients(espClient);

/**
 * @brief Controls the relay.
 *
 * @param timeLed The time to keep the led on in seconds.
 * @param timeBuzzer The time to keep the buzzer on in seconds.
 * @param f The frequency of the buzzer.
 */
/**
 * @param timeLed The duration in seconds for which the LED should remain on.
 */
void controlRelayAndBuzzer(uint8_t timeLed = 5, uint8_t timeBuzzer = 1, uint8_t f = 1000)
{
  tone(BUZZER_PIN, f, timeBuzzer * 1000);
  noTone(BUZZER_PIN);
  digitalWrite(RELAY_PIN, HIGH);
  delay((timeLed - timeBuzzer) * 1000);
  digitalWrite(RELAY_PIN, LOW);
}

/**
 * @brief Callback function to handle incoming MQTT messages.
 *
 * This function is called whenever a message is received on a subscribed topic.
 * It parses the JSON message, extracts the token, and writes it to the NFC card.
 *
 * @param topic The topic on which the message was received.
 * @param payload The message payload.
 * @param length The length of the message payload.
 */
void callback(char *topic, byte *payload, unsigned int length)
{
  String message;
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (String(topic) == mqtt_topic)
  {
    // Parse the JSON message
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // Extract the token from either "copy" or "create"
    const char *token = doc["copy"] | doc["create"];
    if (token)
    {
      displayResponse((uint8_t *)"Message received");
      Serial.print("Token: ");
      Serial.println(token);

      // Write the token to the NFC card
      if (writeToken((const uint8_t *)token))
      {
        Serial.println("Token written to NFC card successfully");
        displayResponse((uint8_t *)"Write success");
        sendResponseConfirmWrite("success");
      }
      else
      {
        Serial.println("Failed to write token to NFC card");
        displayResponse((uint8_t *)"Write fail");
        sendResponseConfirmWrite("error");
      }
      delay(5000);
      displayResponse((uint8_t *)"Insert NFC device...");
    }
    else
    {
      Serial.println("Token not found in the message");
    }
  }
}

/**
 * @brief Reconnects to the MQTT server if the connection is lost.
 *
 * This function attempts to reconnect to the MQTT server and subscribes to the necessary topics.
 */
void reconnect()
{
  displayResponse((uint8_t *)"reconecting...");
  while (!clients.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (clients.connect("ESP32Client"))
    {
      Serial.println("connected");
      clients.subscribe(mqtt_topic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(clients.state());
      delay(500);
    }
  }
  displayResponse((uint8_t *)"Insert NFC device...");
}

void setup()
{
  Serial.begin(115200);

  // Initialize the display
  initDisplay();

  // Connect to WiFi
  connectToWiFi();

  // Synchronize time
  // displayResponse((uint8_t *)"Synch Time...");
  // setClock();
  // displayResponse((uint8_t *)"Synch Time Done");

  // Initialize relay and buzzer pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Setup HTTPS client
  displayResponse((uint8_t *)"Connecting to server...");
  if (connectToServer())
    displayResponse((uint8_t *)"Connected to server");

  // Setup MQTT
  clients.setServer(mqtt_server, 1883);
  clients.setCallback(callback);
  displayResponse((uint8_t *)"Connecting to MQTT...");

  // Connect to MQTT
  while (!clients.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (clients.connect("ESP8266Client"))
    {
      Serial.println("connected");
      displayResponse((uint8_t *)"Connected to MQTT");
      clients.subscribe(mqtt_topic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(clients.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }

  // Init NFC
  displayResponse((uint8_t *)"Init NFC module...");
  initNFC();

  displayResponse((uint8_t *)"Insert NFC device...");
}

/**
 * @brief Handles the detection of an NFC device and processes the token.
 */
void handleNFCDevice()
{
  displayResponse((uint8_t *)"Device detected");
  String token, device;
  uint8_t response[64];

  // Read the token and device information from the NFC card
  if (readTokenLoop(&token, &device))
  {
    // Send the token and device information to the server
    sendRequest((uint8_t *)token.c_str(), device.c_str(), response);

    // Check the server's response
    if (strcmp((char *)response, "Login processed") == 0)
    {
      Serial.println("Login successful");
      displayResponse((uint8_t *)"Login successful");
      controlRelayAndBuzzer(); // Open the door and activate the buzzer
    }
    else
    {
      Serial.println("Login failed");
      displayResponse((uint8_t *)"Login failed");
      controlRelayAndBuzzer(1, 1, 4000); // Activate the buzzer with a different tone
    }
  }
  else
  {
    Serial.println("Login failed");
    displayResponse((uint8_t *)"Login failed");
    controlRelayAndBuzzer(1, 1, 4000); // Activate the buzzer with a different tone
  }

  delay(1000); // Wait for 1 second before resetting the NFC reader
  resetPN532();
  displayResponse((uint8_t *)"Insert NFC device...");
}

void loop()
{
  if (!clients.connected())
  {
    reconnect();
  }
  clients.loop();

  if (nfc.inListPassiveTarget())
    handleNFCDevice();
}