#ifndef SERVER_OPERATIONS_H
#define SERVER_OPERATIONS_H

#include <WiFi.h>
#include <HTTPClient.h>

const char *serverURL = "http://ch1cken.mywire.org:3000/";

/**
 * @brief Sends a request to the server with the specified action and token.
 * 
 * @param action The action to perform (e.g., "create", "login", "remove").
 * @param token The token to send with the request.
 * @param response The server's response as a uint8_t array.
 */
void sendRequest(const char* action, const uint8_t* token, uint8_t* response) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    strcpy((char*)response, "");
    return;
  }
  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String payload;
  if (strcmp(action, "create") == 0) {
    payload = "{\"create\":true}";
  } else {
    payload = "{\"" + String(action) + "\":\"" + String((char*)token) + "\"}";
  }
  Serial.println(payload);
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    String serverResponse = http.getString();
    Serial.println("Server response: " + serverResponse);
    strncpy((char*)response, serverResponse.c_str(), 63);
    response[63] = '\0'; // Ensure null-termination
  } else {
    Serial.println("Error sending POST request");
    strcpy((char*)response, "");
  }
  http.end();
}

#endif // SERVER_OPERATIONS_H