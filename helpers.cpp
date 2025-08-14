#include "helpers.h"

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