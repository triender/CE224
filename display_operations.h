#ifndef DISPLAY_OPERATIONS_H
#define DISPLAY_OPERATIONS_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/**
 * @brief Initializes the SSD1306 OLED display.
 */
void initDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  else{
    Serial.println(F("SSD1306 allocation success"));
  }
  
  display.clearDisplay();
  display.display();
  delay(500);

  // Set rotation to 180 degrees
  display.setRotation(2);

  // Clear the buffer
  display.clearDisplay();
}

/**
 * @brief Displays the response on the OLED screen.
 * 
 * @param response The response string to display.
 */
void displayResponse(const uint8_t *response) {
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text

  // Calculate the width of the message in pixels
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds((char*)response, 0, 0, &x1, &y1, &w, &h);

  // Calculate the starting position to center the text
  int16_t x = (display.width() - w) / 2;
  int16_t y = (display.height() - h) / 2;

  display.setCursor(x, y);     // Set cursor to calculated position
  display.println((char*)response);
  display.display();
}

#endif // DISPLAY_OPERATIONS_H