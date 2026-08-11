# ESP32 Environmental Control System

## Overview
This repository contains firmware for an ESP32-based environmental testing chamber[cite: 7]. The system actively monitors temperature and humidity utilizing a DHT22 sensor and regulates environmental conditions by toggling external heater and humidifier relays[cite: 6]. It includes a local TFT display, a 4x4 matrix keypad for parameter configuration, and UDP-based Wi-Fi telemetry[cite: 6].

## Hardware Specifications and Pin Mapping

### Microcontroller
*   ESP32 Development Board[cite: 7]

### Sensor
*   **Model:** DHT22[cite: 6]
*   **Data Pin:** GPIO 15[cite: 6]

### Relays
*   **Heater Control:** GPIO 17 (Active HIGH)[cite: 6]
*   **Humidifier Control:** GPIO 16 (Active HIGH)[cite: 6]

### 4x4 Matrix Keypad
*   **Row Pins (1-4):** GPIO 27, 14, 13, 4[cite: 6]
*   **Column Pins (1-4):** GPIO 32, 33, 25, 26[cite: 6]

### TFT SPI Display
*   **Driver:** ILI9488[cite: 7]
*   **MOSI:** GPIO 19[cite: 7]
*   **SCLK:** GPIO 18[cite: 7]
*   **CS:** GPIO 23[cite: 7]
*   **DC:** GPIO 21[cite: 7]
*   **RST:** GPIO 22[cite: 7]
*   **MISO:** Not connected (-1)[cite: 7]

## Software Dependencies
This project requires PlatformIO. The following libraries are defined in the `platformio.ini` environment:
*   `adafruit/DHT sensor library` (v1.4.6)[cite: 7]
*   `adafruit/Adafruit Unified Sensor` (v1.1.14)[cite: 7]
*   `chris--a/Keypad` (v3.1.1)[cite: 7]
*   `bodmer/TFT_eSPI` (v2.5.43)[cite: 7]

*Note: The `TFT_eSPI` configuration is handled via build flags in `platformio.ini`. Modifying `User_Setup.h` is not required[cite: 7].*

## Operation and Controls

### Startup Sequence
1.  System initializes serial communication at 115200 baud[cite: 6].
2.  System attempts Wi-Fi connection using hardcoded credentials[cite: 6].
3.  If connection fails after 10 seconds, the system bypasses telemetry and enters offline operational mode[cite: 6].
4.  Initial target setpoints default to 50.0°C and 95.0% RH[cite: 6].

### Keypad Interface
*   **`A`**: Initiate target temperature entry[cite: 6].
*   **`B`**: Initiate target humidity entry[cite: 6].
*   **`C`**: Insert decimal point[cite: 6].
*   **`*`**: Backspace/Delete last character, or cancel current input mode[cite: 6].
*   **`#`**: Confirm numerical entry and save to the selected setpoint[cite: 6].
*   **`0`-`9`**: Numerical input (limited to 5 characters)[cite: 6].

### Control Logic
*   The DHT22 is polled every 2000 milliseconds[cite: 6].
*   If the current temperature is below the target temperature, the heater relay is energized[cite: 6].
*   If the current humidity is below the target humidity, the humidifier relay is energized[cite: 6].

### Fail-Safe Mechanism
*   If the DHT22 fails to return valid data (returns `NAN`), both the heater and humidifier relays are immediately forced LOW (disabled) to prevent runaway thermal or moisture events[cite: 6].
*   The TFT display will output "Error" in place of the numerical readings[cite: 6].

## Network Telemetry
When successfully connected to a Wi-Fi network, the ESP32 operates as a UDP client.
*   **Destination IP:** Hardcoded in `targetIP`[cite: 6].
*   **Destination Port:** 8080[cite: 6].
*   **Payload Format:** Comma-separated string containing `<Temperature_C>,<Humidity_RH>`[cite: 6].
*   **Transmission Condition:** UDP packets are only dispatched when valid, non-NAN sensor readings are acquired[cite: 6].