// Notes:
//  Certificate must be added to WiFi module in order to connect to Firebase!
//  ie: myproject-12345.firebaseio.com:443
//
// Uses sensor DHT11
//
// I2C Board to UNO connections
//  GND <---> GND
//  VCC <---> 5V
//  SDA <---> A4
//  SCL <---> A5

// Include the library code
#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht.h>
#include "arduino_secrets.h"

// Please enter your sensitive data in the added file/tab arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

const char fireBaseServer[] = FIREBASE_SERVER;    // Path to the Firebase application

// Initialize the Wifi client library
int status = WL_IDLE_STATUS;
WiFiSSLClient client;

// Define the DHT11_PIN & reference voltage
dht DHT;
#define DHT11_PIN 7

// Initialize the lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);         // 20 chars by 4 lines

String jsonValue = "";                      // Initialize string used for JSON PUT value
String humidity = "0";                      // Initialize humidity variable
String temperature = "0";                   // Initialize temperature variable

int tempCalibration = -1;                   // temperature calibration value
int prevTempF = 0;                          // last temperature value sent to the database
int prevHumid = 0;                          // last humidity value sent to the database

// -------------------------------------------
// Method to print the wifi status information
// -------------------------------------------
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// -----------------------------------------------------
// Method makes a HTTP connection to the Firebase server
// -----------------------------------------------------
void httpRequest() {
  // Close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();

  //Serial.print("Trying to Connect to Firebase server: ");
  //Serial.println(fireBaseServer);

  if (client.connectSSL(fireBaseServer, 443)) {

    //Serial.println("Connected to Firebase server");

    // Populate the json value string to use for PUT
    jsonValue = "{\"humidity\": " + humidity + ", \"temperature\": " + temperature + "}";

    // Make the HTTP request to update temperature to the Firebase server
    client.println("PUT /weather.json HTTP/1.1");
    client.print("Host: ");
    client.println(fireBaseServer);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(jsonValue.length());
    client.println("Connection: close");
    client.println();
    client.print(jsonValue);
  } else {
    // if you couldn't make a connection:
    //Serial.println("Connection to Firebase server failed");
  }
}

// ------------
// Setup method
// ------------
void setup() {

  // Initialize the LCD display
  lcd.init();                           // Initialize the LCD
  lcd.backlight();                      // Turn the LCD backlight on
  lcd.clear();                          // Clear the LCD screen, position cursor to upper-left corner
  lcd.setCursor(2, 0);                  // Set the cursor to column 0, line 0
  lcd.print("IOT_WiFi_Weather");        // Print heading verbiage to the LCD
  lcd.setCursor(2, 2);                  // Set the cursor to column 0, line 2
  lcd.print("Temperature:");            // Print heading verbiage to the LCD
  lcd.setCursor(2, 3);                  // Set the cursor to column 0, line 3
  lcd.print("Humidity:");               // Print heading verbiage to the LCD

  // Initialize serial and wait for port to open (Needed for native USB port only)
  Serial.begin(9600);
  while (!Serial) {
    // waiting for serial port to connect
  }

  // Check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.2.1") {
    Serial.println("Please upgrade the firmware");
  }

  // Attempt to connect to Wifi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection
    delay(10000);
  }

  // Successfully connected, print out the status
  printWifiStatus();
}

// ------------
// Loop method
// ------------
void loop() {

  // Read the DHT11 and get the temperature and humdidity
  int chk = DHT.read11(DHT11_PIN);
  if (chk == DHTLIB_OK) {
    int readTemp = DHT.temperature;
    int readHumid = DHT.humidity;

    // Convert from Centigrade to Fahrenheit
    float temperatureF = (readTemp * 1.8) + 32.0;

    // Convert temperatureF to an integer and round up if necessary
    int intTempF = temperatureF;
    float decimalTemp = temperatureF - intTempF;
    if (decimalTemp > .49) {
      intTempF += 1;
    }

    // Adjust intTempF by the determined calibration value
    intTempF += tempCalibration;

    // Only send the temperature data if it has changed
    if (intTempF != prevTempF) {

      // Convert the temperature and humidity values to strings
      temperature = String(intTempF);
      humidity = String(readHumid);

      // Update the LCD display with the current temperature and humidity
      lcd.setCursor(15, 2);                 // Set the cursor to column 15, line 2
      lcd.print(temperature);               // Print the temperature value to the LCD
      lcd.setCursor(15, 3);                 // Set the cursor to column 15, line 3
      lcd.print(humidity);                  // Print the humidity value to the LCD

      // Connect and send the data to Firebase
      httpRequest();

      prevTempF = intTempF;
      delay(2000);                          // Delay x seconds before reading again
    }
  } else {
    // If an error connecting to DHT11 sensor, send error to Serial for debugging
    switch (chk)
    {
      case DHTLIB_OK:
        Serial.print("OK,\t");
        break;
      case DHTLIB_ERROR_CHECKSUM:
        Serial.print("Checksum error,\t");
        break;
      case DHTLIB_ERROR_TIMEOUT:
        Serial.print("Time out error,\t");
        break;
      case DHTLIB_ERROR_CONNECT:
        Serial.print("Connect error,\t");
        break;
      case DHTLIB_ERROR_ACK_L:
        Serial.print("Ack Low error,\t");
        break;
      case DHTLIB_ERROR_ACK_H:
        Serial.print("Ack High error,\t");
        break;
      default:
        Serial.print("Unknown error,\t");
        break;
    }
  }
}
