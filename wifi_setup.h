#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

// const char* ssid = "KTMT - SinhVien";
// const char* password = "sinhvien";
const char* ssid = "Kim Cuc lau 1";
// const char* password = "123456789";

/**
 * @brief Connects to the specified WiFi network.
 */
void connectToWiFi() {
  // WiFi.begin(ssid, password);
  WiFi.begin(ssid);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

#endif // WIFI_SETUP_H