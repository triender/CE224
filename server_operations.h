#ifndef SERVER_OPERATIONS_H
#define SERVER_OPERATIONS_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <NetworkClientSecure.h>
#include "display_operations.h"
#include "nfc_operations.h" // Include the NFC operations header

const char *serverURL = "https://ch1cken.mywire.org:8443/";
// const char *IP = "171.247.34.253";
const char *rootCACertificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIID0zCCArugAwIBAgIQVmcdBOpPmUxvEIFHWdJ1lDANBgkqhkiG9w0BAQwFADB7\n"
    "MQswCQYDVQQGEwJHQjEbMBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYD\n"
    "VQQHDAdTYWxmb3JkMRowGAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UE\n"
    "AwwYQUFBIENlcnRpZmljYXRlIFNlcnZpY2VzMB4XDTE5MDMxMjAwMDAwMFoXDTI4\n"
    "MTIzMTIzNTk1OVowgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpOZXcgSmVyc2V5\n"
    "MRQwEgYDVQQHEwtKZXJzZXkgQ2l0eTEeMBwGA1UEChMVVGhlIFVTRVJUUlVTVCBO\n"
    "ZXR3b3JrMS4wLAYDVQQDEyVVU0VSVHJ1c3QgRUNDIENlcnRpZmljYXRpb24gQXV0\n"
    "aG9yaXR5MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAEGqxUWqn5aCPnetUkb1PGWthL\n"
    "q8bVttHmc3Gu3ZzWDGH926CJA7gFFOxXzu5dP+Ihs8731Ip54KODfi2X0GHE8Znc\n"
    "JZFjq38wo7Rw4sehM5zzvy5cU7Ffs30yf4o043l5o4HyMIHvMB8GA1UdIwQYMBaA\n"
    "FKARCiM+lvEH7OKvKe+CpX/QMKS0MB0GA1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1\n"
    "xmNjmjAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zARBgNVHSAECjAI\n"
    "MAYGBFUdIAAwQwYDVR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5j\n"
    "b20vQUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNAYIKwYBBQUHAQEEKDAmMCQG\n"
    "CCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21vZG9jYS5jb20wDQYJKoZIhvcNAQEM\n"
    "BQADggEBABns652JLCALBIAdGN5CmXKZFjK9Dpx1WywV4ilAbe7/ctvbq5AfjJXy\n"
    "ij0IckKJUAfiORVsAYfZFhr1wHUrxeZWEQff2Ji8fJ8ZOd+LygBkc7xGEJuTI42+\n"
    "FsMuCIKchjN0djsoTI0DQoWz4rIjQtUfenVqGtF8qmchxDM6OW1TyaLtYiKou+JV\n"
    "bJlsQ2uRl9EMC5MCHdK8aXdJ5htN978UeAOwproLtOGFfy/cQjutdAFI3tZs4RmY\n"
    "CV4Ks2dH/hzg1cEo70qLRDEmBDeNiXQ2Lu+lIg+DdEmSx/cQwgwp+7e9un/jX9Wf\n"
    "8qn0dNW44bOwgeThpWOjzOoEeJBuv/c=\n"
    "-----END CERTIFICATE-----\n";

WiFiClientSecure httpsClient;
NetworkClientSecure *client = new NetworkClientSecure;
HTTPClient https;

/**
 * @brief Sets the system clock using NTP (Network Time Protocol) to synchronize with a time server.
 *
 * This function configures the system time to GMT+7 by using the NTP server "pool.ntp.org".
 * It waits until the time is synchronized and then prints the current time.
 */
void setClock()
{
  const long gmtOffset_sec = 0;     // GMT+7 offset in seconds
  const int daylightOffset_sec = 0; // No daylight saving time offset

  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2)
  {
    delay(100);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

/**
 * @brief Connects to the server.
 *
 * @return True if the connection was successful, false otherwise.
 */
bool connectToServer()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected");
    return false;
  }

  if (!client)
  {
    Serial.println("Unable to create client");
    return false;
  }

  client->setCACert(rootCACertificate);

  if (!https.begin(*client, serverURL))
  {
    Serial.println("Connection to server failed!");
    return false;
  }

  Serial.println("Connected to server successfully");
  return true;
}

/**
 * @brief Sends a request to the server with the specified action and token.
 *
 * @param token The token to send with the request.
 * @param device The device type (e.g., "phone").
 * @param response The server's response as a uint8_t array.
 */
void sendRequest(const uint8_t *token, const char *device, uint8_t *response)
{
  if (!connectToServer())
  {
    strcpy((char *)response, "Connection failed");
    return;
  }

  https.begin(*client, serverURL);
  https.addHeader("Content-Type", "application/json");

  // Construct the JSON payload
  String payload = "{\"loginDoor\": {\"token\": \"" + String((char *)token) + "\", \"device\": \"" + String(device) + "\"}}";

  Serial.println(payload);
  int httpResponseCode = https.POST(payload);

  if (httpResponseCode > 0)
  {
    String serverResponse = https.getString();
    Serial.println("Server response: " + serverResponse);
    strncpy((char *)response, serverResponse.c_str(), 63);
    response[63] = '\0'; // Ensure null-termination
  }
  else
  {
    Serial.println("Error sending POST request");
    strcpy((char *)response, "Error sending POST request");
  }
  https.end();
}

/**
 * @brief Sends a request to the server with the specified action and token.
 *
 * @param message Response message: 'success' if write succeeded, otherwise 'error'.
 */
void sendResponseConfirmWrite(const char *message)
{
  if (!connectToServer()) // Kiểm tra kết nối tới WiFi
  {
    Serial.println("Failed to connect to server.");
    return;
  }

  // Kết hợp base URL và endpoint
  String url = String(serverURL) + "esp32-response";

  https.begin(*client, url); // Thiết lập kết nối HTTPS
  https.addHeader("Content-Type", "application/json"); // Thêm header JSON

  // Tạo payload JSON
  String payload = String("{\"message\":\"") + message + "\"}";
  // String payload = "{\"message\":\"success\"}";
  Serial.println("Sending payload: " + payload);

  // Gửi POST request
  int httpResponseCode = https.POST(payload);

  // Xử lý phản hồi
  if (httpResponseCode > 0)
  {
    String serverResponse = https.getString(); // Đọc phản hồi từ server
    Serial.println("Server response: " + serverResponse);
  }
  else
  {
    Serial.printf("Error sending POST request, HTTP code: %d\n", httpResponseCode);
  }

  https.end(); // Đóng kết nối
}

#endif // SERVER_OPERATIONS_H
