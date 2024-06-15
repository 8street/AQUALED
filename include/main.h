#pragma once
#ifndef MAIN_H
#    define MAIN_H

#    include <Arduino.h>
#    include <NTPClient.h>
#    include <WiFiUdp.h>

#    include "Light.h"

#    define LED_PIN 2 // pin 2
#    define WIFI_PIN 5
#    define DRIVER1_240W_PWM_PIN 14
#    define DRIVER2_100W_PWM_PIN 12
#    define DRIVER3_240W_PWM_PIN 13
#    define DRIVER4_100W_PWM_PIN 15

extern NTPClient timeClient;
extern bool Direct_Control_state;
extern int Direct_Control_power;
extern int Direct_Control_color;

extern Light *Light_system_ptr;
extern std::vector<Light_setting> Light_settings;
extern std::vector<Light_setting> Random_light_settings;

extern String NTP_server;
extern long Timezone;
extern WiFiUDP ntpUDP;

extern unsigned long Loop_time_us;
extern unsigned long Max_loop_time_us;

#endif
