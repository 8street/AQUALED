#pragma once
#ifndef TIME_HELPERS_H
#    define TIME_HELPERS_H

#    include <Arduino.h>

void update_NTP_time();
String uptime();
bool new_day();
bool new_hour();
bool new_minute();
bool new_second();

#endif