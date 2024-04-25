# ESP32_GPS_Clock
# Arduino GPS Data Display

This Arduino sketch is designed to interface with a GPS module, display GPS data on an LCD screen, and publish the GPS data over serial communication.

## Project Description

This project serves as a GPS data display system using an Arduino board (specifically tested with an ESP32), a GPS module, and an LCD screen. The system continuously reads GPS data, interprets it, and displays relevant information on the LCD screen. Additionally, it publishes detailed GPS data over the serial port for further processing or logging.

## Components Used

- **Arduino Board:** An ESP32 microcontroller board was used in testing, but the sketch should be adaptable to other Arduino-compatible boards.
- **GPS Module:** A GPS module compatible with the TinyGPS++ library is required for obtaining GPS data.
- **LCD Screen:** An I2C-compatible LCD screen is used for displaying the GPS data.
- **Cables and Connectors:** Various cables and connectors are required to connect the components together.

## Hardware Connections

The following pins on the ESP32 are used for connecting the accessories:

- **GPS Module:** Serial communication with the GPS module is established using the following pins:
  - TX pin of GPS module to RX pin (GPIO 16) of ESP32
  - RX pin of GPS module to TX pin (GPIO 17) of ESP32
  - VCC pin of GPS module to 5V
  - GND pin of GPS module to GND 
- **LCD Screen:** The I2C communication for the LCD screen is established using the following pins:
  - SDA pin of LCD to SDA pin (GPIO 21) of ESP32
  - SCL pin of LCD to SCL pin (GPIO 22) of ESP32
  - VCC pin of LCD to 5V
  - GND pin of LCD to GND 

Ensure that the pins are properly configured in the Arduino sketch.

## Setup Instructions

1. **Hardware Connections:**
   - Connect the GPS module and LCD screen to the specified pins on the ESP32 board.
   - Double-check the connections to ensure they are properly configured.

2. **Software Setup:**
   - Upload the provided Arduino sketch to the ESP32 board using the Arduino IDE or always check the baud rate if it errors...
   - Open the serial monitor to view the published GPS data.

## Functionality Overview

- **`setup()`:** Initializes serial communication, sets up GPS module communication, initializes the LCD screen, records boot time, and sets the time sync provider.
- **`loop()`:** Processes available GPS data, updates time display, rotates information display on the LCD, and publishes GPS data over serial communication.
- **Custom Functions:** Includes functions for retrieving GPS time, displaying time and date on the LCD, displaying a waiting message while waiting for GPS fix, adjusting time zone based on GPS location, and converting HDOP value to a descriptive string.

## Usage and Contributions

- **Usage:** Feel free to use, modify, or extend this project for your own purposes.
- **Contributions:** Contributions such as bug fixes, feature enhancements, or documentation improvements are welcome! Please submit pull requests or issues via GitHub.

## Limitations

### General Limitations:
- **GPS Module Compatibility:** The sketch relies on GPS modules compatible with the TinyGPS++ library. Compatibility with other GPS modules may require significant modifications to the code or library dependencies.
- **Display Limitations:** The LCD screen used in this project has inherent limitations in terms of size and resolution. Displaying large amounts of information or complex graphics may not be feasible due to space constraints.
- **GPS Data Accuracy and Reliability:** The accuracy and reliability of GPS data can vary depending on environmental factors such as signal strength, satellite visibility, atmospheric conditions, and electromagnetic interference. Users should be aware that occasional inaccuracies or fluctuations in GPS data may occur.

### Limitations of Time Zone Adjustment Function:
- **Limited to Longitude-Based Time Zone:** The time zone adjustment function implemented in the sketch calculates the local time zone based solely on the longitude obtained from the GPS module. While this approach may work in many cases, it does not account for factors such as daylight saving time or regions with irregular time zone boundaries.
- **Sensitivity to GPS Accuracy:** The accuracy of the time zone adjustment depends on the accuracy of the GPS location data, particularly the longitude value. Inaccuracies in GPS data may result in incorrect time zone adjustments.
- **Single Time Zone Support:** The current implementation supports adjusting the system time to a single time zone determined by the GPS location. It does not support dynamic switching between different time zones or handling locations that span multiple time zones.
- **No User Override:** The sketch does not provide a mechanism for users to manually override or adjust the time zone setting. Users relying on the automatic time zone adjustment should ensure that the GPS module provides accurate location data.
- **Limited Error Handling:** The time zone adjustment function lacks robust error handling mechanisms to handle edge cases or unexpected scenarios. Users should exercise caution when relying on the automatic time zone adjustment feature and consider implementing additional error checking or fallback mechanisms if necessary.


## License

This project is licensed under the [MIT License](LICENSE), which means you are free to use, modify, and distribute the code for personal or commercial purposes with appropriate attribution.
