#include <Arduino.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
//#include <LiquidCrystal_I2C.h>
// OLED support
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//
#include <Wire.h>
#include <TimeLib.h>
#define LED_PIN 2
// ToDo Build Serial menu for interacitons, passthrough NEMA, formatted NEMA, Drift checks of time, location, speed, abilty to configure timezone hardcoded,
//
// OLED support
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// SPI OLED display connections using specified GPIOs of ESP32
#define OLED_MOSI 23   // Master Out Slave In, corresponding to D1
#define OLED_CLK 18    // Serial Clock, corresponding to D0
#define OLED_DC 21     // Data/Command, corresponding to DC
#define OLED_CS 5      // Chip Select, corresponding to CS
#define OLED_RESET 22  // Reset, corresponding to RES

// Initialize Adafruit SSD1306 OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
// OLED support
// set up the hardware
HardwareSerial gpsSerial(1);  // GPS Module connected to UART1
TinyGPSPlus gps;              // Create a GPS object
//LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD address 0x27 for a 16 chars and 2 line display On pins 21 and 22, just try both ways to get it to work, remember 5v!!


//set up the variables
unsigned long lastSyncTime = 0;
unsigned long bootTime = 0;
int displayPage = 0;
int ledState = LOW;  // Initialize LED state variable, assuming LED is initially off
bool hasGPSFix = false;

int timezoneOffset = 0;              // Global variable to store the timezone offset
bool timezoneOffsetUpdated = false;  // Flag to check if the timezone offset needs to be updated

// Debug you say?
bool debugEnabled = false;  // Set to false to disable debug output


// Set up a variable to contain a NEMA sentance
String latestNMEA = "";  // Holds the latest NMEA sentence received from the GPS

// set up the counters
const unsigned long GPSUpdateInterval = 50;             // Time to update the internal clock from the GPS, we can do this all the time if we want
const unsigned long timeUpdateInterval = 100;           // Timeon the screen updates every 1000 milliseconds (1 second)
const unsigned long infoUpdateInterval = 3000;          // Info updates every 3000 milliseconds (3 seconds)
const unsigned long gpsDataPublishInterval = 10000;     // Set interval for publishing GPS data on the USB serial line, e.g., 5000 milliseconds (5 seconds)
const unsigned long serialDataPublishInterval = 10000;  // set interval for publishing genral diagonstics device data on USB serial line
const unsigned long timeZoneUpdateInterval = 60000;     // set interval for updating the TimeZone based off latitude and logitude.

// set up the icon locations
// Calling the function with specific positions for the satellite icon and signal bars
int satelliteIconX = 0;  // X coordinate for the satellite icon
int satelliteIconY = 0;  // Y coordinate for the satellite icon
int barsX = 105;         // X coordinate for the top-right position of the signal bars
int barsY = 14;          // Y coordinate for the top-right position of the signal bars (just below the time)
int gpsDataStartY = 25;  // Default starting position for GPS data
int mainRX_X = 110;  // X position for RX icon
int mainRX_Y = 54;   // Y position for RX icon

bool isReceivingGPSData = false;    // Flag to indicate receiving data
unsigned long lastGPSDataTime = 0;  // Last time GPS data was received
unsigned long flashInterval = 200;  // Interval for flashing "RX"
bool showRX = true;                // Whether to show "RX" or not

void setup() {
  Serial.begin(115200);                       // Start the serial connection to the computer
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);  // Start gpsSerial communication
  //lcd.init();                                 // Initialize the LCD
  //lcd.backlight();                            // Turn on the backlight on the LCD
  pinMode(2, OUTPUT);       // ESP32 Onboard LED set as an output
  bootTime = millis();      // Record the time when the system started
  setSyncProvider(getGPS);  // Set the function used to get time from GPS

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  display.display();       // Show initial display buffer contents on the screen -- the library initializes this with an Adafruit splash screen.
  delay(200);              // Pause for 2 seconds
  display.clearDisplay();  // Clear the buffer
}

void loop() {
  static unsigned long lastGPSUpdate = 0;     // Timer for updating the device time from GPS
  static unsigned long lastSecondUpdate = 0;  // Timer for updating the time on the LCD
  //static unsigned long lastInfoUpdate = 0;         // Timer for publishing GPS info to LCD
  static unsigned long lastGPSDataPublish = 0;     // Timer for publishing GPS data
  static unsigned long lastSerialDataPublish = 0;  // Timer for publishing system data via serial
  static unsigned long lastTimezoneUpdate = 0;     // Timer for resetting the timezone offset, this is incase the device is on the move.

  debugPrint("Entering loop...");

  // Process available data from GPS
  if (millis() - lastGPSUpdate >= GPSUpdateInterval) {
    lastGPSUpdate = millis();
    if (gpsSerial.available() > 0) {
      if (!isReceivingGPSData) {
        isReceivingGPSData = true;
      }
      lastGPSDataTime = millis();  // Update the last time data was received
      debugPrint("Reading GPS data...");
      //      while (gpsSerial.available() > 0) {  ///// I think i need to change this to an If, not a while as it currently loops many times?
      while (gpsSerial.available() > 0) {
        if (gps.encode(gpsSerial.read())) {  // Read and encode GPS data
          debugPrint("GPS data encoded successfully.");
          debugPrint("GPS time valid: " + String(gps.time.isValid() ? "true" : "false"));
          debugPrint("Location valid: " + String(gps.location.isValid() ? "true" : "false"));
          debugPrint("Date valid: " + String(gps.date.isValid() ? "true" : "false"));
          debugPrint("Number of Satellites: " + String(gps.satellites.value()));
          debugPrint("hasGPSFix status: " + String(hasGPSFix ? "true" : "false"));
          debugPrint("Negated (!hasGPSFix) status: " + String(!hasGPSFix ? "true" : "false"));
          ledState = !ledState;
          digitalWrite(LED_PIN, ledState);

          if (gpsSerial.available() > 0) {
            char c = gpsSerial.read();
            if (gps.encode(c)) {
              latestNMEA += c;               // Append the read character to the latest NMEA string
              if (c == '\n') {               // If the end of the line
                Serial.println(latestNMEA);  // Print the full NMEA sentence to Serial Monitor
                latestNMEA = "";             // Optionally reset latestNMEA after printing
              }
            }
          }

          if (gps.location.isValid() && gps.time.isValid()) {  // once we have a lock on where we are and what time it is we can set the time.
            debugPrint("GPS fix acquired.");
            setTime(getGPS());  // Set system time with GPS time
            adjustTimeZone();   // Adjust system time for local timezone
            hasGPSFix = true;   // Mark that a valid GPS fix has been obtained

          } else {
            debugPrint("GPS fix not acquired.");
          }
        }
      }
    } else {
      debugPrint("No GPS data available.");
    }
  } else {
    debugPrint("Not time for a GPS update");
    //Serial.println("Not time for a GPS update");
  }

  // Update time display every second
  if (millis() - lastSecondUpdate >= timeUpdateInterval) {
    lastSecondUpdate = millis();
    debugPrint("Updating time display...");
    if (!hasGPSFix) {
      displayWaitingForGPS();  // Display waiting message if no GPS fix
      debugPrint("Updating time display... Waiting message displayed");
    } else {
      //adjustTimeZone();      // Adjust system time for local timezone
      //displayTimeAndDate(satelliteIconX, satelliteIconY, barsX, barsY);  // use the variables here.
      displayTimeAndDate(satelliteIconX, satelliteIconY, barsX, barsY, mainRX_X, mainRX_Y);
      debugPrint("Updating time display... Displaying time and Date");
    }
  }

  // Publish GPS data on a separate timer
  if (millis() - lastGPSDataPublish >= gpsDataPublishInterval) {
    lastGPSDataPublish = millis();
    //if (hasGPSFix) {     // Only publish GPS data if a GPS fix has been acquired
    publishGPSData();  // Publish GPS data to serial port
    //}
  }

  // Send data via serial on its own timer
  if (millis() - lastSerialDataPublish >= serialDataPublishInterval) {
    lastSerialDataPublish = millis();
    debugPrint("Publishing GPS data on USB serial");
    sendDataViaSerial();  // Send the current time to USB serial
  }

  // Recalculate timezone incase device is moved between timezones.
  if (millis() - lastTimezoneUpdate >= timeZoneUpdateInterval) {
    lastTimezoneUpdate = millis();
    if (gps.location.isValid() && gps.time.isValid()) {
      debugPrint("Recalculating timezone...");
      timezoneOffsetUpdated = false;  // Reset the flag to indicate timezone needs updating
      adjustTimeZone();               // Recalculate the timezone using the latest GPS data
    }
  }

  ledState = LOW;
  digitalWrite(LED_PIN, ledState);

  delay(10);  // Short delay to reduce CPU usage
}

// helper function to deploy debugs
void debugPrint(const String& message) {
  if (debugEnabled) {
    Serial.println(message);
  }
}

void sendDataViaSerial() {
  // Send the current timestamp, Hall sensor value, and internal temperature to the USB serial in a single statement
  // Serial.printf("Timestamp: %lu, Hall Sensor: %d, Internal Temp: %.2f°C\n", now(), hallRead(), temperatureRead());
}

/**
 * @brief Retrieves the current GPS time and converts it into a time_t format.
 *
 * This function checks if the time from the GPS module is valid. If it is,
 * the function constructs a time structure (tmElements_t) from the received
 * GPS time data and converts this structure into a time_t value. The time_t
 * value represents the number of seconds since January 1, 1970 (the Unix epoch).
 * This time format is commonly used in many computing systems for time operations.
 *
 * @return time_t The current time as a Unix epoch time if the GPS time is valid,
 *                or 0 if the GPS time is invalid.
 */
time_t getGPS() {
  // Check if the time reported by the GPS is valid
  if (gps.time.isValid()) {
    // Create a structure to hold the time elements
    tmElements_t tm;

    // Populate the time structure with time information from the GPS
    tm.Hour = gps.time.hour();      // Set the hour
    tm.Minute = gps.time.minute();  // Set the minute
    tm.Second = gps.time.second();  // Set the second
    tm.Day = gps.date.day();        // Set the day of the month
    tm.Month = gps.date.month();    // Set the month

    // Convert the year from the GPS (which is a full four-digit year)
    // to a year offset from 1970, as required by the TimeLib.h functions.
    tm.Year = CalendarYrToTm(gps.date.year());

    // Convert the tm structure to a time_t value (number of seconds since the Unix epoch)
    return makeTime(tm);
  } else {
    // If the GPS time is not valid, return 0
    return 0;
  }
}

/**
 * @brief Displays the current time and date on an LCD screen.
 *
 * This function formats the current system time and date into a human-readable string
 * and displays it on an LCD. The time is displayed in the format "HH:MM:SS" and the date
 * in the format "DD/MM", where HH is hours, MM is minutes, SS is seconds, DD is day, and
 * MM is month. This function assumes that the system time has been previously set and
 * is maintained accurately (e.g., via an NTP server, RTC, or GPS time sync).
 */

void displayTimeAndDate(int satelliteIconX, int satelliteIconY, int barsX, int barsY, int rxX, int rxY) {
    static unsigned long lastUpdate = 0;
    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate > 1000) {  // Update the display every second
        lastUpdate = currentMillis;

        display.clearDisplay();
        display.setTextSize(2);  // Larger text size for time
        display.setTextColor(SSD1306_WHITE);

        // Create time string
        char timeBuffer[20];
        sprintf(timeBuffer, "%02d:%02d:%02d", hour(), minute(), second());

        // Position and display time
        display.setCursor(0, 0);  // Position time at the top left
        display.println(timeBuffer);

        // Create date string with smaller font
        char dateBuffer[20];
        sprintf(dateBuffer, "%02d/%02d/%04d", day(), month(), year());
        display.setTextSize(1);    // Smaller text size for date
        display.setCursor(0, 16);  // Position below time
        display.println(dateBuffer);

        // Draw the signal bar at a configurable position
        displaySignalBar(barsX, barsY);

        // Update the RX display at the specified position
        //manageRXDisplay(rxX, rxY);

        // Additional GPS data display
        displayAllGPSData();

        display.display();  // Update the display with the new data
    }
}

void displaySignalBar(int xStart, int yStart) {
  static unsigned long lastToggle = 0;
  static bool showBars = true;
  unsigned long currentMillis = millis();

  float hdop = gps.hdop.hdop();
  int bars = 0;

  // Define bars based on HDOP
  if (hdop < 1.0) bars = 5;
  else if (hdop < 2.0) bars = 4;
  else if (hdop < 5.0) bars = 3;
  else if (hdop < 10.0) bars = 2;
  else if (hdop >= 10.0) bars = 1;

  if (hdop > 5.0) {
    if (currentMillis - lastToggle > 500) {
      showBars = !showBars;
      lastToggle = currentMillis;
    }
  } else {
    showBars = true;
  }

  int barWidth = 3;

  if (showBars) {
    for (int i = 0; i < bars; i++) {
      int barHeight = 2 + i * 3;
      display.fillRect(xStart + i * 6, yStart - barHeight, barWidth, barHeight, SSD1306_WHITE);
    }
  }
}

void manageRXDisplay(int rxX, int rxY) {
    unsigned long currentMillis = millis();
    
    // Toggle the RX display based on the flash interval
    if (isReceivingGPSData && (currentMillis - lastGPSDataTime > flashInterval)) {
        showRX = !showRX;  // Toggle the RX display
        lastGPSDataTime = currentMillis; // Reset timer for flashing
        Serial.print("Toggling RX to: ");
        Serial.println(showRX ? "ON" : "OFF");
    } else {
        showRX = false;  // Stop showing "RX" if no data received recently
        Serial.println("No recent GPS data, turning RX off");
    }

    // Update the display with the RX status
    display.setCursor(rxX, rxY);
    if (showRX) {
        display.print("RX");
    } else {
        display.print("  ");  // Print spaces to clear the "RX"
    }
    display.display();  // Ensure to update the display after changing content
}



// void manageRXDisplay(int rxX, int rxY) {
//     unsigned long currentMillis = millis();
    
//     // Check if data was received in the last second
//     if (isReceivingGPSData && (currentMillis - lastGPSDataTime < 1000)) {
//         // Toggle the RX display based on the flash interval
//         if (currentMillis - lastGPSDataTime > flashInterval) {
//             showRX = !showRX;  // Toggle the RX display
//             lastGPSDataTime = currentMillis; // Reset timer for flashing
//             Serial.print("Toggling RX to: ");
//             Serial.println(showRX ? "ON" : "OFF");
//         }
//     } else {
//         isReceivingGPSData = false;
//         showRX = false;  // Stop showing "RX" if no data received recently
//         Serial.println("No recent GPS data, turning RX off");
//     }

//     // Update the display with the RX status
//     display.setCursor(rxX, rxY);
//     if (showRX) {
//         display.print("RX");
//     } else {
//         display.print("  ");  // Print spaces to clear the "RX"
//     }
// }


// void manageRXDisplay(int rxX, int rxY) {
//     if (isReceivingGPSData && (millis() - lastGPSDataTime < 1000)) {
//         if (millis() - lastGPSDataTime > flashInterval) {
//             showRX = !showRX;  // Toggle the RX display
//             lastGPSDataTime = millis(); // Reset timer for flashing
//         }
//     } else {
//         isReceivingGPSData = false;
//         showRX = false;  // Stop showing "RX" if no data received recently
//     }

//     if (showRX) {
//         display.setCursor(rxX, rxY);  // Use the provided position for "RX"
//         display.print("RX");
//     } else {
//         display.setCursor(rxX, rxY);  // Ensure to clear the previous "RX"
//         display.print("  ");  // Print spaces to clear the "RX"
//     }
// }

/**
 * @brief Displays a waiting message on the LCD while the system is waiting for a GPS fix.
 *
 * This function is intended to provide visual feedback on the LCD indicating that the
 * system is actively trying to acquire a GPS signal. It also shows the system uptime
 * in seconds, updating every second. This function uses a static variable to keep track
 * of the last update time, ensuring the display is refreshed at one-second intervals.
 */

void displayWaitingForGPS() {
  static int animationFrame = 0;  // This will keep track of the animation frame

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Display a static message
  display.println("Waiting for GPS!!");
  // TODO: fix this, need to move it to a set positino as its floating it displayis under the up time
  // Simple animation - rotating slash
  char animationChars[4] = { '|', '/', '-', '\\' };
  display.print("Loading ");
  display.print(animationChars[animationFrame]);
  animationFrame = (animationFrame + 1) % 4;  // Update frame

  // Display up time
  display.setCursor(0, 10);
  display.print("Up Time: ");
  display.print((millis() - bootTime) / 1000);

  // Display the latest NMEA sentence
  display.setCursor(0, 20);
  display.print("NMEA: ");
  display.print(latestNMEA);

  display.display();
  delay(250);  // Slow down the animation a bit
}

/**
 * @brief Adjusts the system time based on the local time zone calculated from GPS coordinates.
 * 
 * This function calculates the local time zone offset based on the longitude obtained from the GPS location.
 * It then adjusts the current system time by adding the time zone offset to the UTC time obtained from the GPS.
 * The adjusted time is set using the setTime function provided by the TimeLib.h library.
 * 
 * Note: This function assumes that the GPS location and time data are valid.
 */

void adjustTimeZone() {
  if (gps.location.isValid() && gps.time.isValid()) {
    if (!timezoneOffsetUpdated) {
      timezoneOffset = calculateTimeZoneOffset(gps.location.lat(), gps.location.lng());
      timezoneOffsetUpdated = true;  // Mark the timezone as updated
    }
    // Get the current UTC time from GPS data
    time_t utc = getGPS();
    // Convert UTC to local time based on the stored timezone offset
    time_t localTime = utc + timezoneOffset * SECS_PER_HOUR;  // SECS_PER_HOUR is 3600
    // Apply the local time
    setTime(localTime);
    // Optionally, print the adjusted time for verification
    Serial.print("Adjusted Time: ");
    printTime(localTime);
  } else {
    Serial.println("GPS data not valid, cannot adjust timezone.");
    timezoneOffsetUpdated = false;  // Reset the update flag if GPS data is not valid
  }
}

int calculateTimeZoneOffset(float latitude, float longitude) {
  Serial.print("Calculating timezone for coordinates: Latitude = ");
  Serial.print(latitude, 6);  // 6 decimal places for precision
  Serial.print(", Longitude = ");
  Serial.println(longitude, 6);

  // Implement specific checks for known exceptions
  // Example: India
  if (longitude > 68.0f && longitude < 97.0f && latitude > 8.0f && latitude < 37.0f) {
    Serial.println("Location is in India, applying UTC +5.5");
    return 5.5;  // UTC + 5:30
  }
  // Example: Newfoundland
  if (longitude > -59.0f && longitude < -52.0f && latitude > 46.0f && latitude < 61.0f) {
    Serial.println("Location is in Newfoundland, applying UTC -3.5");
    return -3.5;  // UTC - 3:30
  }
  // Explicit handling for Korean Peninsula
  if ((longitude >= 124.0f && longitude <= 131.0f) && (latitude >= 33.0f && latitude <= 43.0f)) {
    Serial.println("Location is on the Korean Peninsula, applying UTC +9");
    return 9;  // UTC +9 for both South Korea and North Korea
  }
  // Default calculation
  int calculatedTimeZone = round(longitude / 15.0);
  Serial.print("Location does not match specific exceptions, calculated timezone using default method: UTC ");
  Serial.println(calculatedTimeZone);
  return calculatedTimeZone;
}

// Helper function to print time_t in human-readable form
void printTime(time_t t) {
  char buffer[30];  // Buffer to hold the formatted time string
  sprintf(buffer, "%02d:%02d:%02d %02d/%02d/%04d\n",
          hour(t), minute(t), second(t), day(t), month(t), year(t));
  Serial.print(buffer);
}

/**
 * @brief Horizontal Dilution of Precision (HDOP) is a measure of the satellite geometry affecting GPS accuracy,
 * specifically in the horizontal plane.
 * Mathematically, HDOP is calculated as the ratio of the positional uncertainty in the horizontal plane to the theoretical minimum 
 * uncertainty achievable with the same number of satellites in optimal positions. 
 * A lower HDOP value indicates better geometric satellite distribution and, consequently, better position accuracy.
 * 
 * In practical terms, a typical HDOP value ranges from 1 to 10, with lower values indicating higher accuracy. 
 * A HDOP value below 1.0 is considered excellent, while values above 5.0 may result in significant position errors. 
 * HDOP values significantly above 10.0 indicate poor satellite geometry and may render the GPS position fix unreliable.
 */
String hdopDescription(float hdop) {
  if (hdop < 2.0) return "Perfect";
  if (hdop < 5.0) return "Good";
  if (hdop < 7.0) return "OK";
  if (hdop < 10.0) return "Poor";
  return "BAD!";
}

void displayAllGPSData() {
  display.setTextSize(1);               // Set text size for more data on the screen
  display.setTextColor(SSD1306_WHITE);  // Set text color

  int yPos = gpsDataStartY;  // Use the global variable for the start position
  int lineSpacing = 9;       // Space between lines

  // Display each piece of information on a new line
  display.setCursor(0, yPos);
  display.println("Altitude: " + String(gps.altitude.meters()) + "m");

  display.setCursor(0, yPos + lineSpacing * 1);
  display.println("Satalites: " + String(gps.satellites.value()));

  display.setCursor(0, yPos + lineSpacing * 2);
  display.println("Lat: " + String(gps.location.lat(), 6));

  display.setCursor(0, yPos + lineSpacing * 3);
  display.println("Lon: " + String(gps.location.lng(), 6));

  display.setCursor(0, yPos + lineSpacing * 4);
  display.println("UTC Offset: " + String(timezoneOffset));

  display.display();
}


// push data to serial,
void publishGPSData() {
  Serial.println("Detailed GPS Data:");
  Serial.print("Latitude: ");
  Serial.println(gps.location.lat(), 6);
  Serial.print("Longitude: ");
  Serial.println(gps.location.lng(), 6);
  Serial.print("Altitude: ");
  Serial.println(gps.altitude.meters());
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());
  Serial.print("HDOP: ");
  Serial.println(gps.hdop.hdop());
  Serial.print("Date: ");
  Serial.print(month());
  Serial.print("/");
  Serial.print(day());
  Serial.print("/");
  Serial.println(year() % 100);
  Serial.print("Time: ");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.println(second());
  Serial.print("Speed: ");
  Serial.println(gps.speed.kmph());
  Serial.println("---------------------");
}
