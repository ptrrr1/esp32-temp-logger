#include "config.h"
#include "helpers.h"

#include "time.h"

#include <WiFi.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// 96 because that's how many aattempts are possible in a single day
RTC_DATA_ATTR rtc_store_t rtc_data[96];
RTC_DATA_ATTR uint32_t failed_transmissions = 0;
RTC_DATA_ATTR uint32_t boot_count = 0;

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
		getTemperature(sensors, IN_HOUSE_SENSOR),
		getTemperature(sensors, OUT_HOUSE_SENSOR),
		getTemperature(sensors, REF_HOUSE_SENSOR)
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