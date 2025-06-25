#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <WiFi.h>
#include <WiFiMulti.h>
#include "display_operations.h" // Include the display operations header
#include "secrets.h"            // Include the secrets file

WiFiMulti WiFiMulti;

/**
 * @brief Connects to one of the known WiFi networks.
 */
void connectToWiFi() {
  WiFi.mode(WIFI_STA);

  // Add known networks from secrets.h
  WiFiMulti.addAP(WIFI_SSID_1, WIFI_PASS_1);
  WiFiMulti.addAP(WIFI_SSID_2, WIFI_PASS_2);
  WiFiMulti.addAP(WIFI_SSID_3, WIFI_PASS_3);
  WiFiMulti.addAP(WIFI_SSID_4, WIFI_PASS_4);

  Serial.println("Trying to connect to WiFi networks...");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();

  // Wait for connection
  if (WiFiMulti.run(10000) == WL_CONNECTED) {
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(WiFi.SSID());
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("WiFi Connected");
      display.setCursor(0, 10);
      display.print(WiFi.SSID());
      display.display();
  } else {
      Serial.println("Could not connect to any known WiFi networks.");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("WiFi connection failed.");
      display.display();
  }
}

#endif // WIFI_SETUP_H