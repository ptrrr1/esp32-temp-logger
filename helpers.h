#pragma once

#ifndef HELPERS_H
#define HELPERS_H

#include "time.h"
#include "config.h"

#include <stdint.h>
#include <WiFi.h>
#include <DallasTemperature.h>
#include <ESP_Google_Sheet_Client.h>

// 32 bytes in size
typedef struct {
  uint32_t boot_count;
  time_t timestamp;
  float in_temp;
  float out_temp;
  float ref_temp;
} rtc_store_t;


float getTemperature(DallasTemperature sensors, DeviceAddress deviceAddress);

bool sendReadings(rtc_store_t readings);

time_t getTime();

wl_status_t connectWiFi();

#endif
