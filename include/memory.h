#pragma once
#ifndef MEMORY_H
#    define MEMORY_H

#    include <vector>

#    include "light_driver.h"
#    include "light_setting.h"

extern String Eeprom_title;
extern String Eeprom_password;
extern String Eeprom_ssid;

extern const int Eeprom_ssid_address;
extern const int Eeprom_password_address;
extern const int Eeprom_ntp_address;
extern const int Eeprom_timezone_address;
extern const int Eeprom_title_address;
extern const int Eeprom_driver_address;

extern const int Eeprom_size;

bool save_to_flash(const std::vector<Light_setting> &settings);
bool save_to_flash(size_t pos, const std::vector<Light_setting> &settings);
bool load_from_flash(std::vector<Light_setting> &settings);
bool save_to_flash(int address, String &str);
bool read_from_flash(int address, String &str);
bool save_to_flash(int address, uint8_t *data, size_t size);
bool read_from_flash(int address, uint8_t *data, size_t size);
bool save_to_flash(int address, const std::vector<Light_driver> &drivers);
bool load_from_flash(int address, std::vector<Light_driver> &drivers);

#endif