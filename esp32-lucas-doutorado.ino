#include "config.h"
#include "time.h"

#include <WiFi.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP_Google_Sheet_Client.h>
// For SD/SD_MMC mounting helper
#include <GS_SDHelper.h>

// Sensors Addresses
uint8_t IN_HOUSE_SENSOR[8]  = { 0x28, 0x10, 0xDE, 0x85, 0x00, 0x00, 0x00, 0xE1 };
uint8_t OUT_HOUSE_SENSOR[8] = { 0x28, 0xA4, 0x9D, 0x86, 0x00, 0x00, 0x00, 0x15 };
uint8_t REF_HOUSE_SENSOR[8] = { 0x28, 0x3B, 0x5E, 0x6B, 0x00, 0x00, 0x00, 0x38 };

RTC_DATA_ATTR unsigned long bootCount = 0;

OneWire oneWire(TEMP_SENSORS_BUS);
DallasTemperature sensors(&oneWire);

// Network Time Protocol
const char* ntpServer = "pool.ntp.org";

void setup(){
  Serial.begin(115200);

  // LED indicator
  pinMode(GPIO_LED_PIN, OUTPUT);
  digitalWrite(GPIO_LED_PIN, HIGH); // Indicating it's working

  // Init Temperature Sensors
  sensors.begin();
  
  // Find Sensors
  //locateDevices(); // Can be removed later

  // Define wake up source, Timer with 30min duration
  bootCount++;
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  // Init WiFi
  wl_status_t wifi_status = connectWiFi();
  configTime(0, 0, ntpServer);

  // Request temperatures
  sensors.requestTemperatures();
  float in_house = getTemperature(IN_HOUSE_SENSOR);
  float out_house = getTemperature(OUT_HOUSE_SENSOR);
  float ref_house = getTemperature(REF_HOUSE_SENSOR);
  time_t timestamp = getTime();

  if (wifi_status != WL_CONNECTED) {
    // TODO: Write memory and update variable
  } else {
    GSheet.setPrerefreshSeconds(120);
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    Serial.print("Stating Google Sheets connection");
    while (!GSheet.ready()) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected!");

    FirebaseJson response;
    FirebaseJson valueRange;

    // Define organization Column-wise
    valueRange.add("majorDimension", "COLUMNS");
    valueRange.set("values/[0]/[0]", bootCount);
    valueRange.set("values/[1]/[0]", timestamp);
    valueRange.set("values/[2]/[0]", in_house);
    valueRange.set("values/[3]/[0]", out_house);
    valueRange.set("values/[4]/[0]", ref_house);

    bool success = GSheet.values.append(
      &response, //returned response 
      SPREADSHEET_ID, // spreadsheet Id to append 
      String(HOSTNAME) + "!A1", // range to append
      &valueRange // data range to append
    );

    if (success) {
      response.toString(Serial, true);
      valueRange.clear();
    } else {
      Serial.println(GSheet.errorReason());
    }

    Serial.println();
    Serial.println(ESP.getFreeHeap());
  }

  // LED indicator
  digitalWrite(GPIO_LED_PIN, LOW);

  // Going to Sleep
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop() {
  // Not called
}

float getTemperature(DeviceAddress deviceAddress) {
  float tempC = sensors.getTempC(deviceAddress);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("ERROR: Could not read temperature data");
    return -127.0;
  }

  Serial.print(tempC);
  Serial.println("°C");
  return tempC;
}

time_t getTime() {
  time_t now;
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }

  time(&now);
  return now;
}

wl_status_t connectWiFi() {
  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWD);

  uint8_t retry_attempt = 0;
  Serial.print("Connecting to WiFi");
  // TODO: Try limited number of times
  while (WiFi.status() != WL_CONNECTED && retry_attempt <= 10) {
    delay(500);
    retry_attempt++;
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());
  }

  return WiFi.status();
}

// void locateDevices() {
//   int numberOfDevices = sensors.getDeviceCount();
//   DeviceAddress tempDeviceAddress;

//   Serial.print("Locating devices...");
//   Serial.print("Found ");
//   Serial.print(numberOfDevices, DEC);
//   Serial.println(" devices.");

//   // Loop through each device, print out address
//   for(int i = 0; i < numberOfDevices; i++){
//     // Search the wire for address
//     if(sensors.getAddress(tempDeviceAddress, i)) {
//       Serial.print("Found device ");
//       Serial.print(i, DEC);
//       Serial.print(" with address: ");
//       printAddress(tempDeviceAddress);
//       Serial.println();
//     } else {
//       Serial.print("Found ghost device at ");
//       Serial.print(i, DEC);
//       Serial.print(" but could not detect address. Check power and cabling");
//     }
//   }
// }

// void printAddress(DeviceAddress deviceAddress) {
//   for (uint8_t i = 0; i < 8; i++) {
//     Serial.print("0x");
//     if (deviceAddress[i] < 0x10) { Serial.print("0"); }
//     Serial.print(deviceAddress[i], HEX);
//     if (i < 7) { Serial.print(", "); }
//   }
//   Serial.println("");
// }