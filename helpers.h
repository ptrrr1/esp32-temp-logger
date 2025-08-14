#pragma once

#ifndef HELPERS_H
#define HELPERS_H

#include "time.h"
#include "config.h"

#include <WiFi.h>
#include <stdint.h>

time_t getTime();

wl_status_t connectWiFi();

#endif
