#include "config.h"
#include "helpers.h"

#include "time.h"

#include <WiFi.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// 96 because that's how many attempts are possible in a single day
RTC_DATA_ATTR rtc_store_t rtc_data[96];
RTC_DATA_ATTR uint32_t failed_transmissions = 0;
RTC_DATA_ATTR uint32_t boot_count = 0;

// Init Sensors
OneWire oneWire(TEMP_SENSORS_BUS);
DallasTemperature sensors(&oneWire);

// Sensors Addresses
uint8_t RED_STRAIGHT[8] = { 0x28, 0x02, 0xD1, 0xBD, 0x00, 0x00, 0x00, 0xEF };
uint8_t BLUE_STRAIGHT[8] = { 0x28, 0x9B, 0xAC, 0x51, 0x00, 0x00, 0x00, 0x12 };
uint8_t GREEN_STRAIGHT[8] = { 0x28, 0x73, 0x51, 0xC0, 0x00, 0x00, 0x00, 0xBE };

uint8_t RED_TRI[8] = { 0x28, 0x97, 0xC6, 0xBD, 0x00, 0x00, 0x00, 0x98 };
uint8_t BLUE_TRI[8] = { 0x28, 0x79, 0x2B, 0xBC, 0x00, 0x00, 0x00, 0x1B };
uint8_t GREEN_TRI[8] = { 0x28, 0x6F, 0x9B, 0x51, 0x00, 0x00, 0x00, 0x69 };

uint8_t RED_OUTSET[8] = { 0x28, 0x3B, 0x45, 0xC0, 0x00, 0x00, 0x00, 0x09 };
uint8_t BLUE_OUTSET[8] = { 0x28, 0x03, 0x40, 0xBC, 0x00, 0x00, 0x00, 0x2D };
uint8_t GREEN_OUTSET[8] = { 0x28, 0xA3, 0xE9, 0xBD, 0x00, 0x00, 0x00, 0x3E };

uint8_t RED_INSET[8] = { 0x28, 0x20, 0xDD, 0xBD, 0x00, 0x00, 0x00, 0x16 };
uint8_t BLUE_INSET[8] = { 0x28, 0x10, 0xDE, 0x85, 0x00, 0x00, 0x00, 0xE1 };//{ 0x28, 0x24, 0x54, 0xBC, 0x00, 0x00, 0x00, 0x7D };
uint8_t GREEN_INSET[8] = { 0x28, 0xA4, 0x9D, 0x86, 0x00, 0x00, 0x00, 0x15 };

uint8_t YELLOW_OUT[8] = { 0x28, 0x3B, 0x5E, 0x6B, 0x00, 0x00, 0x00, 0x38 };



void setup() {
	Serial.begin(115200);

	// LED indicator
	pinMode(GPIO_LED_PIN, OUTPUT);
	digitalWrite(GPIO_LED_PIN, HIGH);  // Indicating it's working

	// Init Temperature Sensors
	sensors.begin();

	sensors.setResolution(RED_STRAIGHT, 12);
	sensors.setResolution(BLUE_STRAIGHT, 12);
	sensors.setResolution(GREEN_STRAIGHT, 12);

	sensors.setResolution(RED_TRI, 12);
	sensors.setResolution(BLUE_TRI, 12);
	sensors.setResolution(GREEN_TRI, 12);

	sensors.setResolution(RED_OUTSET, 12);
	sensors.setResolution(BLUE_OUTSET, 12);
	sensors.setResolution(GREEN_OUTSET, 12);

	sensors.setResolution(RED_INSET, 12);
	sensors.setResolution(BLUE_INSET, 12);
	sensors.setResolution(GREEN_INSET, 12);

	sensors.setResolution(YELLOW_OUT, 12);

	// Define wake up source, Timer with TIME_TO_SLEEP duration
	boot_count++;
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

	// Init WiFi & Time
	wl_status_t wifi_status = connectWiFi();
	configTime(0, 0, ntpServer);

	// Request temperatures
	sensors.requestTemperatures();

	rtc_store_t readings = {
		boot_count,
		getTime(),
		getTemperature(sensors, RED_STRAIGHT),
		getTemperature(sensors, BLUE_STRAIGHT),
		getTemperature(sensors, GREEN_STRAIGHT),
		getTemperature(sensors, RED_TRI),
		getTemperature(sensors, BLUE_TRI),
		getTemperature(sensors, GREEN_TRI),
		getTemperature(sensors, RED_OUTSET),
		getTemperature(sensors, BLUE_OUTSET),
		getTemperature(sensors, GREEN_OUTSET),
		getTemperature(sensors, RED_INSET),
		getTemperature(sensors, BLUE_INSET),
		getTemperature(sensors, GREEN_INSET),
		getTemperature(sensors, YELLOW_OUT)
	};

	if (wifi_status == WL_CONNECTED) {
		// Try to resend readings
		uint32_t f = failed_transmissions - 1;
		while(failed_transmissions > 0) {
			Serial.print("Sending reading n# ");
			Serial.println(f - failed_transmissions);
			failed_transmissions--;
			bool success = sendReadings(rtc_data[f - failed_transmissions]);
			delay(500);
		}
		// Send current readings
		bool success = sendReadings(readings);
	} else {
		// Store failed readings in memory
		Serial.print("Saving reading n# ");
		Serial.println(failed_transmissions);
		rtc_data[failed_transmissions] = readings;
		failed_transmissions++;
		failed_transmissions = failed_transmissions % 96;
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