#include <algorithm>

#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#include "PWM.h"
#include "main.h"
#include "memory.h"
#include "mytime.h"
#include "network.h"
#include "password.h"
#include "text.h"
#include "time_helpers.h"
#include "web_page.h"

ADC_MODE(ADC_VCC);

// http://wikihandbk.com/wiki/ESP32:%D0%9F%D1%80%D0%B8%D0%BC%D0%B5%D1%80%D1%8B/%D0%92%D0%B5%D0%B1-%D1%81%D0%B5%D1%80%D0%B2%D0%B5%D1%80_%D0%BD%D0%B0_%D0%B1%D0%B0%D0%B7%D0%B5_ESP32:_%D1%83%D0%B4%D0%B0%D0%BB%D0%B5%D0%BD%D0%BD%D0%BE%D0%B5_%D1%83%D0%BF%D1%80%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5_%D1%81%D0%B5%D1%80%D0%B2%D0%BE%D0%BF%D1%80%D0%B8%D0%B2%D0%BE%D0%B4%D0%BE%D0%BC

const char *Default_title = "Aquarium LED driver";

String NTP_server = "pool.ntp.org";
// +3 hour time zone
long Timezone = 3;

///////////////// variables ////////////////
WiFiServer *Server_ptr = nullptr;
WiFiUDP ntpUDP;

Web_Page *Web_page_ptr = nullptr;

NTPClient timeClient(ntpUDP, "pool.ntp.org", Timezone * 60 * 60);
bool NTP_init_updated = false;

bool Direct_Control_state = false;
int Direct_Control_power = 0;
int Direct_Control_color = WHITE_COLOR_TEMP;

Light *Light_system_ptr = nullptr;
std::vector<Light_setting> Light_settings;
std::vector<Light_setting> Random_light_settings;

unsigned long Loop_time_us = 0;
unsigned long Max_loop_time_us = 0;

bool OTA_Active = false;

////////////////// setup ////////////////////

void setup(void)
{
    // GPIO:
    for (uint8_t pin = 12; pin < 16; pin++)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH); // off
    }
    for (uint8_t pin = 0; pin < 6; pin++)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH); // off
    }

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // led off

    pinMode(WIFI_PIN, INPUT_PULLUP);

    // CPU clock speed
    system_update_cpu_freq(SYS_CPU_160MHZ);

    // USB-UART
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println(F("Setup started"));
    // EEPROM
    EEPROM.begin(Eeprom_size);

    read_from_flash(Eeprom_ssid_address, Eeprom_ssid);
    read_from_flash(Eeprom_password_address, Eeprom_password);
    if (Eeprom_ssid.length() == 0)
    {
        Serial.println(F("EEPROM WiFi settings empty. Set default settings"));
        Eeprom_ssid = String(Default_ssid);
        Eeprom_password = String(Default_password);
        save_to_flash(Eeprom_ssid_address, Eeprom_ssid);
        save_to_flash(Eeprom_password_address, Eeprom_password);
    }
    Serial.println("Readed EEPROM ssid: " + Eeprom_ssid);
    // Serial.println(" password: " + Eeprom_password);

    // connect to wifi station or create AP
    wifi_init();

    Server_ptr = new WiFiServer(80);
    if (!Server_ptr)
    {
        Serial.println(F("Not enough memory for WiFiServer."));
        delay(5000);
        ESP.restart();
    }
    Server_ptr->begin(80);
    Serial.println(F("HTTP server started"));

    // NTP
    String eeprom_ntp;
    String eeprom_timezone;
    read_from_flash(Eeprom_ntp_address, eeprom_ntp);
    read_from_flash(Eeprom_timezone_address, eeprom_timezone);
    if (eeprom_ntp.length() && eeprom_timezone.length())
    {
        NTP_server = eeprom_ntp;
        Timezone = eeprom_timezone.toInt();
        NTPClient new_client(ntpUDP, NTP_server.c_str(), Timezone * 60 * 60);
        timeClient = new_client;
    }
    timeClient.begin();
    Serial.println(F("NTP client started"));
    Serial.print("NTP Server: " + eeprom_ntp);
    Serial.println(" timezone: " + eeprom_timezone + "h");

    NTP_init_updated = update_NTP_time();
    if (NTP_init_updated)
    {
        randomSeed(timeClient.getEpochTime());
    }

    // light driver settings
    Light_system_ptr = new Light;
    if (!Light_system_ptr)
    {
        Serial.println(F("Not enough memory for Light System."));
        delay(5000);
        ESP.restart();
    }
    std::vector<Light_driver> eeprom_drivers;
    load_from_flash(Eeprom_driver_address, eeprom_drivers);
    Serial.println(F("Light drivers has been readed from EEPROM."));
    if (eeprom_drivers.empty())
    {
        Serial.println(F("EEPROM Light drivers is empty. Creating first settings."));
        eeprom_drivers.clear();
        // main_240W_driver
        eeprom_drivers.push_back(Light_driver(PWM(DRIVER1_240W_PWM_PIN, 3000, 1000, 25), 200, WHITE_COLOR_TEMP, 73, 100, 5));
        // main_100W_driver
        eeprom_drivers.push_back(Light_driver(PWM(DRIVER2_100W_PWM_PIN, 3000, 1000, 25), 65, RED_COLOR_TEMP, 77, 100, 3));
        // second_240W_driver
        eeprom_drivers.push_back(Light_driver(PWM(DRIVER3_240W_PWM_PIN, 3000, 1000, 25), 200, WHITE_COLOR_TEMP, 73, 100, 5));
        // second_100W_driver
        eeprom_drivers.push_back(Light_driver(PWM(DRIVER4_100W_PWM_PIN, 3000, 1000, 25), 65, RED_COLOR_TEMP, 77, 100, 3));
        save_to_flash(Eeprom_driver_address, eeprom_drivers);
    }
    for (const Light_driver &d : eeprom_drivers)
    {
        Light_system_ptr->add_light_driver(d);
    }
    eeprom_drivers.clear();

    load_from_flash(Light_settings);
    Serial.println(F("Settings has been readed from EEPROM."));
    if (Light_settings.empty())
    {
        Serial.println(F("EEPROM is empty. Creating first settings."));
        Light_settings.clear();
        Light_settings.push_back(Light_setting(0, 0, 3000));
        Light_settings.push_back(Light_setting(8, 0, 3000));
        Light_settings.push_back(Light_setting(MyTime(8, 30), 10, 3000));
        Light_settings.push_back(Light_setting(9, 15, 4000));
        Light_settings.push_back(Light_setting(11, 60, 6500));
        Light_settings.push_back(Light_setting(12, 100, 6500));
        Light_settings.push_back(Light_setting(14, 50, 6500));
        Light_settings.push_back(Light_setting(16, 45, 4500));
        Light_settings.push_back(Light_setting(18, 30, 3500));
        Light_settings.push_back(Light_setting(20, 20, 3000));
        Light_settings.push_back(Light_setting(22, 5, 3000));
        Light_settings.push_back(Light_setting(MyTime(23, 59), 0, 3000));
        save_to_flash(Light_settings);
    }
    update_random_settings(Light_settings, Random_light_settings);

    // OTA
    ArduinoOTA.onStart([]() {
        String type;
        OTA_Active = true;
        if (ArduinoOTA.getCommand() == U_FLASH)
        {
            type = "sketch";
        }
        else
        { // U_FS
            type = "filesystem";
        }
        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("Start prog " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println(F("\nEnd"));
        OTA_Active = false;
    });
    ArduinoOTA.onProgress(
        [](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println(F("Auth Failed"));
        else if (error == OTA_BEGIN_ERROR)
            Serial.println(F("Begin Failed"));
        else if (error == OTA_CONNECT_ERROR)
            Serial.println(F("Connect Failed"));
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println(F("Receive Failed"));
        else if (error == OTA_END_ERROR)
            Serial.println(F("End Failed"));
    });

    // ArduinoOTA.setHostname("aqua500.local");
    // ArduinoOTA.setPassword("aqualed500");
    const bool use_MDNS = false;
    ArduinoOTA.begin(use_MDNS);
    Serial.println(F("OTA configured."));
    Serial.println(F("OTA hostname: ") + ArduinoOTA.getHostname());

    // read title from eeprom
    read_from_flash(Eeprom_title_address, Eeprom_title);
    if (Eeprom_title.length() == 0)
    {
        Serial.println(F("EEPROM title string is empty. Set default string"));
        Eeprom_title = String(Default_title);
        save_to_flash(Eeprom_title_address, Eeprom_title);
    }

    Web_page_ptr = new Web_Page;
    if (!Web_page_ptr)
    {
        Serial.println(F("Not enough memory for Web page."));
        delay(5000);
        ESP.restart();
    }
    // end setup
    digitalWrite(LED_PIN, LOW);
    delay(1000);
    digitalWrite(LED_PIN, HIGH);
    Serial.println(F("Setup is over."));

    // Watchdog
    ESP.wdtEnable(5000);
}

//////////////////// loop //////////////////////////

void loop(void)
{
    const unsigned long loop_start_time_us = micros();

    // Watchdog update
    ESP.wdtFeed();

    // needs for wifi/tcp works
    yield();

    // Handle update firmware via WIFI
    ArduinoOTA.handle();

    yield();

    if (new_second())
    {
        if (ESP.getFreeContStack() < 500)
        {
            Serial.println(F("Not enough Stack memory: ") + String(ESP.getFreeContStack()));
        }
        if (ESP.getFreeHeap() < 1000)
        {
            Serial.println(F("Not enough Heap memory: ") + String(ESP.getFreeHeap()));
        }
    }

    if (new_minute() && !NTP_init_updated)
    {
        NTP_init_updated = update_NTP_time();
        randomSeed(timeClient.getEpochTime());
    }

    yield();

    if (new_hour())
    {
        wifi_connect_to_station();
        update_NTP_time();
        Max_loop_time_us = 0;
    }

    yield();

    // randomize power every day
    if (new_day())
    {
        update_random_settings(Light_settings, Random_light_settings);
    }

    std::sort(Light_settings.begin(), Light_settings.end());
    yield();
    std::sort(Random_light_settings.begin(), Random_light_settings.end());
    yield();

    // light
    if (Direct_Control_state)
    {
        // Serial.println("Direct power: " + String(Direct_Control_power) + " Direct Color: " + String(Direct_Control_color));
        Light_system_ptr->set_light(Direct_Control_power, Direct_Control_color);
        digitalWrite(LED_PIN, LOW);
    }
    else
    {
        Light_system_ptr->set_light(Random_light_settings, timeClient);
        digitalWrite(LED_PIN, HIGH);
    }
    yield();

    // WEB Server
    Web_page_ptr->show(Server_ptr);

    // needs for wifi/tcp works
    delay(50);

    if (Loop_time_us > Max_loop_time_us)
    {
        Max_loop_time_us = Loop_time_us;
    }
    const unsigned long loop_end_time_us = micros();
    if (loop_end_time_us > loop_start_time_us)
    {
        const unsigned long new_loop_time_us = loop_end_time_us - loop_start_time_us;
        Loop_time_us = (Loop_time_us + new_loop_time_us) / 2;
    }
}