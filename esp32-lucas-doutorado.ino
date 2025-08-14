#include "config.h"
#include "helpers.h"

#include "time.h"

#include <WiFi.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP_Google_Sheet_Client.h>

RTC_DATA_ATTR uint32_t bootCount = 0;

// Init Sensors
OneWire oneWire(TEMP_SENSORS_BUS);
DallasTemperature sensors(&oneWire);

// Sensors Addresses
uint8_t IN_HOUSE_SENSOR[8] = { 0x28, 0x10, 0xDE, 0x85, 0x00, 0x00, 0x00, 0xE1 };
uint8_t OUT_HOUSE_SENSOR[8] = { 0x28, 0xA4, 0x9D, 0x86, 0x00, 0x00, 0x00, 0x15 };
uint8_t REF_HOUSE_SENSOR[8] = { 0x28, 0x3B, 0x5E, 0x6B, 0x00, 0x00, 0x00, 0x38 };

void setup() {
	Serial.begin(115200);

	// LED indicator
	pinMode(GPIO_LED_PIN, OUTPUT);
	digitalWrite(GPIO_LED_PIN, HIGH);  // Indicating it's working

	// Init Temperature Sensors
	sensors.begin();

	// Define wake up source, Timer with TIME_TO_SLEEP duration
	bootCount++;
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

	// Init WiFi & Time
	wl_status_t wifi_status = connectWiFi();
	configTime(0, 0, ntpServer);

	// Request temperatures
	sensors.requestTemperatures();
	float in_house = getTemperature(IN_HOUSE_SENSOR);
	float out_house = getTemperature(OUT_HOUSE_SENSOR);
	float ref_house = getTemperature(REF_HOUSE_SENSOR);
	time_t timestamp = getTime();

	if (wifi_status == WL_CONNECTED) {
		GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

		Serial.print("Starting Google Sheets connection");
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
		  &response,                 // returned response
		  SPREADSHEET_ID,            // spreadsheet Id to append
		  String(HOSTNAME) + "!A1",  // range to append
		  &valueRange                // data range to append
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