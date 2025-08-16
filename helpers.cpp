#include "helpers.h"

float getTemperature(DallasTemperature sensors, DeviceAddress deviceAddress) {
	float tempC = sensors.getTempC(deviceAddress);

	if (tempC == DEVICE_DISCONNECTED_C) {
		Serial.println("ERROR: Could not read temperature data");
		return -127.0;
	}

	Serial.print(tempC);
	Serial.println("°C");
	return tempC;
}

bool sendReadings(rtc_store_t readings) {
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
	valueRange.set("values/[0]/[0]", readings.boot_count);
	valueRange.set("values/[1]/[0]", readings.timestamp);
	valueRange.set("values/[2]/[0]", readings.in_temp);
	valueRange.set("values/[3]/[0]", readings.out_temp);
	valueRange.set("values/[4]/[0]", readings.ref_temp);

	bool success = GSheet.values.append(
		&response,                 // returned response
		SPREADSHEET_ID,            // spreadsheet Id to append
		String(HOSTNAME) + "!A1",  // range to append
		&valueRange                // data range to append
	);

	if (success) {
		//response.toString(Serial, true);
		valueRange.clear();
	} else {
		Serial.println(GSheet.errorReason());
	}

	return success;
}

time_t getTime() {
	time_t now;
	struct tm timeinfo;

	if (!getLocalTime(&timeinfo)) {
		Serial.println("Failed to obtain time");
		return (0);
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