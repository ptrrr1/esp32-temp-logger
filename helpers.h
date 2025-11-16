#pragma once

#ifndef HELPERS_H
#define HELPERS_H

#include "time.h"
#include "config.h"

#include <stdint.h>
#include <WiFi.h>
#include <DallasTemperature.h>
#include <ESP_Google_Sheet_Client.h>

typedef struct {
  uint32_t boot_count;
  time_t timestamp;
  float red_straight;
  float blue_straight;
  float green_straight;
  float red_tri;
  float blue_tri;
  float green_tri;
  float red_outset;
  float blue_outset;
  float green_outset;
  float red_inset;
  float blue_inset;
  float green_inset;
  float yellow_out;
} rtc_store_t;


float getTemperature(DallasTemperature sensors, DeviceAddress deviceAddress);

bool sendReadings(rtc_store_t readings);

time_t getTime();

wl_status_t connectWiFi();

#endif
