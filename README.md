# ESP32-NFC Smart Door Lock

## Project Overview

This project is an highly automated door access control system built on the ESP32 microcontroller platform. The system authenticates users via Near Field Communication (NFC), supporting both physical cards and smartphones. The core of the project lies in the seamless integration of on-site hardware with a backend server, communicating securely over HTTPS for authentication and via MQTT for remote management, creating a flexible and modern access control solution.

## Key Features

*   **Versatile Authentication**: Supports unlocking with Mifare Classic NFC cards and Android mobile devices featuring Host Card Emulation (HCE).
*   **Secure Server Communication**: Utilizes the HTTPS protocol to send authentication tokens to a backend server, ensuring data integrity and security.
*   **Remote Card Management**: Integrates the MQTT protocol to receive commands from the server, enabling administrators to conveniently create or copy new card credentials remotely.
*   **Intuitive User Feedback**: An SSD1306 OLED display is used to show real-time status messages, such as "Connected," "Login Successful," or "Write Failed."
*   **Audible Alerts**: A buzzer emits distinct sound signals to notify of successful door access or authentication errors.
*   **Electronic Lock Control**: The system drives a relay to operate an electronic door lock mechanism.

## System Architecture and Technology

### Hardware Platform
*   **Microcontroller**: ESP32, chosen for its powerful processing capabilities and built-in Wi-Fi.
*   **NFC Interface**: A PN532 NFC reader/writer module, which serves as the interface for interacting with cards and phones.
*   **Display**: A 128x64 OLED screen communicating over I2C.
*   **Peripherals**: A 5V relay and a buzzer for physical actions and audible feedback.

### Software and Protocols
*   **Programming Language**: C++ on the Arduino framework.
*   **Core Libraries**: `Adafruit PN532`, `PubSubClient`, `ArduinoJson`, and `Adafruit SSD1306`.
*   **Network Protocols**:
    *   **HTTPS**: Used for request-response communication with the server's API, primarily for token authentication.
    *   **MQTT**: Employed for asynchronous communication, allowing the server to push commands (like creating a new card) to the ESP32 device instantly.

## System Workflow

### User Authentication Flow
When a user presents an NFC card or smartphone to the reader, the system automatically reads the token. This token is then encapsulated and sent via an HTTPS request to the backend server. The server validates the token and returns a result. Based on this response, the ESP32 device decides whether to activate the relay to open the door or to display an error message.

### Remote Card Management Flow
The system maintains a persistent connection to an MQTT broker, listening on a predefined topic. When an administrator wishes to grant new access, a message containing a command and a new token is published to this topic. The ESP32 device receives the message, parses its content, and switches to a card-writing mode. After the user presents a blank card and the writing process is complete, the system sends a status confirmation ("success" or "failure") back to the server.

---
*Author: TriNguyen*
*Date: June 25, 2023*
