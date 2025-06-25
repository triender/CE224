#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <WiFi.h>
#include <WiFiMulti.h>
#include "display_operations.h" // Include the display operations header

WiFiMulti WiFiMulti;

// List of known networks
struct WiFiNetwork {
  const char* ssid;
  const char* password;
};

WiFiNetwork knownNetworks[] = {
  {"Kim Cuc lau 1", nullptr},
  {"WifiEsp", "12345678"},
  {"UIT Public", nullptr},
  {"KTMT - SinhVien", "sinhvien"},
  // {"Network4", nullptr}
};

/**
 * @brief Connects to one of the known WiFi networks.
 */
void connectToWiFi() {
  WiFi.mode(WIFI_STA);

  Serial.println("Trying to connect to WiFi networks...");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  for (WiFiNetwork network : knownNetworks) {
    Serial.print("Connecting to ");
    Serial.println(network.ssid);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Connecting to ");
    display.println(network.ssid);
    display.display();

    if (network.password) {
      WiFiMulti.addAP(network.ssid, network.password);
    } else {
      WiFiMulti.addAP(network.ssid);
    }

    int attempts = 0;
    while (WiFiMulti.run() != WL_CONNECTED && attempts < 5) {
      delay(100);
      Serial.print(".");
      display.print(".");
      display.display();
      attempts++;
    }

    if (WiFiMulti.run() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Connected to WiFi");
      display.display();
      return;
    } else {
      Serial.println("\nFailed to connect to WiFi");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Failed to connect to WiFi");
      display.display();
    }
  }

  Serial.println("Could not connect to any known WiFi networks.");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Could not connect to any known WiFi networks.");
  display.display();
}

#endif // WIFI_SETUP_H