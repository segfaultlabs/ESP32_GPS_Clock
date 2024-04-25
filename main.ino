#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <TimeLib.h>
#define LED_PIN 2
//set up the variables 
unsigned long lastSyncTime = 0;
unsigned long lastInfoUpdate = 0;
unsigned long bootTime = 0;
int displayPage = 0;
int ledState = LOW;  // Initialize LED state variable, assuming LED is initially off
bool hasGPSFix = false;
// set up the counters 
const unsigned long timeUpdateInterval = 1000;  // Time updates every 1000 milliseconds (1 second)
const unsigned long infoUpdateInterval = 2000;  // Info updates every 3000 milliseconds (3 seconds)
const unsigned long gpsDataPublishInterval = 5000;  // Set interval for publishing GPS data on the USB serial line, e.g., 5000 milliseconds (5 seconds)
// set up the hardware 
HardwareSerial gpsSerial(1);  // GPS Module connected to UART1
TinyGPSPlus gps;  // Create a GPS object
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD address 0x27 for a 16 chars and 2 line display On pins 21 and 22, just try both ways to get it to work, remember 5v!! 

void setup() {
  Serial.begin(115200);  // Start the serial connection to the computer
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // Start gpsSerial communication
  lcd.init();  // Initialize the LCD
  lcd.backlight();  // Turn on the backlight on the LCD
  pinMode(2, OUTPUT);  // ESP32 Onboard LED set as an output
  bootTime = millis();  // Record the time when the system started
  setSyncProvider(getGPS);  // Set the function used to get time from GPS
}

void loop() {
  static unsigned long lastSecondUpdate = 0;
  static unsigned long lastInfoUpdate = 0;
  static unsigned long lastGPSDataPublish = 0;  // Timer for publishing GPS data
  // Process available data from GPS
  if (gpsSerial.available() > 0) {
    while (gpsSerial.available() > 0) {
      if (gps.encode(gpsSerial.read())) {  // Read and encode GPS data
        if (!hasGPSFix && gps.time.isValid()) {
          setTime(getGPS());        // Set system time with GPS time
          adjustTimeZone();         // Adjust system time for local timezone
          hasGPSFix = true;         // Mark that a valid GPS fix has been obtained
        } // this is my hack to flash the LED when there is data on the line, without blocking, it feels wrong tho?  
        if (ledState == HIGH) {       // Check state 
        digitalWrite(LED_PIN, LOW);  // If LED is on, turn it off
        ledState = LOW;              // Update LED state variable
        } else {
        digitalWrite(LED_PIN, HIGH); // If LED is off, turn it on
        ledState = HIGH;             // Update LED state variable
        }
        // digitalWrite(LED_PIN, HIGH); // Turn on LED
        // delay(5);                   // Blocking delay to keep LED on for 10 milliseconds, blocking is bad but so is my code.
        // digitalWrite(LED_PIN, LOW);  // Turn off LED
      }
    }
  }
  // Update time display every second
  if (millis() - lastSecondUpdate >= timeUpdateInterval) {
    lastSecondUpdate = millis();
    if (!hasGPSFix) {
      displayWaitingForGPS();       // Display waiting message if no GPS fix
    } else {
      displayTimeAndDate();         // Display current time and date
    }
  }
  // Rotate info display every 2 seconds
  if (millis() - lastInfoUpdate >= infoUpdateInterval) {
    lastInfoUpdate = millis();
    if (hasGPSFix) {  // Only update info if a GPS fix has been acquired
      rotateInfoDisplay();          // Rotate through additional GPS data on display
    }
  }
  // Publish GPS data on a separate timer
  if (millis() - lastGPSDataPublish >= gpsDataPublishInterval) {
    lastGPSDataPublish = millis();
    if (hasGPSFix) {  // Only publish GPS data if a GPS fix has been acquired
      publishGPSData();             // Publish GPS data to serial port
    }
  }
  delay(10);  // Short delay to reduce CPU usage
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
    tm.Hour = gps.time.hour();         // Set the hour
    tm.Minute = gps.time.minute();     // Set the minute
    tm.Second = gps.time.second();     // Set the second
    tm.Day = gps.date.day();           // Set the day of the month
    tm.Month = gps.date.month();       // Set the month

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
void displayTimeAndDate() {
  char buffer[20];  // Buffer to hold the formatted time and date strings.
  // Format the current time as "HH:MM:SS" and store it in the buffer.
  // Spaces are added after the seconds to ensure any previous data displayed is fully cleared.
  sprintf(buffer, "%02d:%02d:%02d    ", hour(), minute(), second());
  // Set the cursor to the beginning of the first line of the LCD.
  lcd.setCursor(0, 0);
  // Print the formatted time to the LCD.
  lcd.print(buffer);
  // Format the current date as "DD/MM" and store it in the buffer.
  // Again, spaces are added to clear any residual characters from previous displays.
  sprintf(buffer, "%02d/%02d    ", day(), month() % 100);
  // Move the cursor to position 11 on the first line to start printing the date.
  lcd.setCursor(11, 0);
  // Print the formatted date to the LCD.
  lcd.print(buffer);
}

/**
 * @brief Displays a waiting message on the LCD while the system is waiting for a GPS fix.
 *
 * This function is intended to provide visual feedback on the LCD indicating that the
 * system is actively trying to acquire a GPS signal. It also shows the system uptime
 * in seconds, updating every second. This function uses a static variable to keep track
 * of the last update time, ensuring the display is refreshed at one-second intervals.
 */
void displayWaitingForGPS() {
  static unsigned long lastUpdate = 0;  // Holds the last time the display was updated
  // Check if more than 1000 milliseconds (1 second) have passed since the last update
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();  // Update the last update time to the current time
    // Set the cursor to the beginning of the first line of the LCD
    lcd.setCursor(0, 0);
    // Print the waiting message on the first line of the LCD
    // Additional spaces at the end ensure any previous content is cleared
    lcd.print("Waiting for GPS!!   "); 
    // Set the cursor to the beginning of the second line of the LCD
    lcd.setCursor(0, 1);
    // Print "Up Time: " followed by the system uptime in seconds
    // The uptime is calculated as the current time minus the boot time, divided by 1000 to convert milliseconds to seconds
    lcd.print("Up Time Secconds: ");
    lcd.print((millis() - bootTime) / 1000);
  }
  lcd.blink();  // Enable blinking of the LCD text to make the wait more noticeable
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
  // Check if the GPS location data is valid
  if (gps.location.isValid()) {  // Calculate the time zone offset based on longitude (1 hour per 15 degrees)
    int timezone = round(gps.location.lng() / 15);  // Calculate the local hour by adding the time zone offset to the GPS hour
    int localHour = (gps.time.hour() + timezone + 24) % 24;     // The result is adjusted to ensure it wraps around correctly within 24 hours
    // Set the system time using the adjusted local hour, along with minutes, seconds,
    // day, month, and year obtained from the GPS data
    setTime(localHour, gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
  }
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
    if (hdop < 10.0) return "Fair";
    return "BAD!";
}

void rotateInfoDisplay() {
    lcd.setCursor(0, 1);  // Set cursor to the beginning of the second line
    String info;
    float hdopValue;  // Declare hdopValue outside the switch statement
    // Select the information to display based on the current page
    switch (displayPage++ % 7) { // changed from 6 to 7 due to chaning view options
        case 0: info = "ALT: " + String(gps.altitude.meters()) + "m"; break;
        case 1: info = "Satalites: " + String(gps.satellites.value()); break;
        case 2: hdopValue = gps.hdop.hdop(); info = "HDOP: " + String(hdopValue, 1); break;             // Use hdopValue here
        case 3: hdopValue = gps.hdop.hdop(); info = "Signal: " + hdopDescription(hdopValue); break;
        case 4: info = "LAT:" + String(gps.location.lat(), 6); break;
        case 5: info = "LONG:" + String(gps.location.lng(), 6); break;
        case 6: info = "SPEED:" + String(gps.speed.kmph()) + "km/h"; break;
    }
    // Ensure the info string fits in the LCD's width and clears extra characters
    int spacesNeeded = 16 - info.length(); // Assumes a 16-character wide display
    if (spacesNeeded > 0) {
        info += String("                ").substring(0, spacesNeeded);
    }
    lcd.print(info);  // Print the information followed by enough spaces to clear the rest of the line
}

// push data to serial, 
// TODO: Check the NEMA standard to see if there is any other intreseting data i could be publishing to serial 
// nb: note the use of some println vs print's this is key for formatting. 
void publishGPSData() {
  Serial.println("Detailed GPS Data:");
  Serial.print("Latitude: "); Serial.println(gps.location.lat(), 6);
  Serial.print("Longitude: "); Serial.println(gps.location.lng(), 6);
  Serial.print("Altitude: "); Serial.println(gps.altitude.meters());
  Serial.print("Satellites: "); Serial.println(gps.satellites.value());
  Serial.print("HDOP: "); Serial.println(gps.hdop.hdop());
  Serial.print("Date: "); Serial.print(month()); Serial.print("/"); Serial.print(day());
  Serial.print("/"); Serial.println(year() % 100);
  Serial.print("Time: "); Serial.print(hour()); Serial.print(":"); Serial.print(minute());
  Serial.print(":"); Serial.println(second());
  Serial.print("Speed: "); Serial.println(gps.speed.kmph());
  Serial.println("---------------------");
}
