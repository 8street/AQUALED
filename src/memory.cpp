
#include <EEPROM.h>

#include "light_driver.h"
#include "memory.h"


String Eeprom_ssid;
String Eeprom_password;
String Eeprom_title;

// size_t - size of vector<Light_setting>
// sizeof(Light_setting) * 64 - is 64 settings max must be saved/readed
const int Eeprom_ssid_address = sizeof(size_t) + sizeof(Light_setting) * 64;
const int Eeprom_password_address = Eeprom_ssid_address + 50;
const int Eeprom_ntp_address = Eeprom_password_address + 50;
const int Eeprom_timezone_address = Eeprom_ntp_address + 50;
const int Eeprom_title_address = Eeprom_timezone_address + 50;
const int Eeprom_driver_address = Eeprom_title_address + 50;

const int Eeprom_size = Eeprom_driver_address + sizeof(size_t) + sizeof(Light_driver) * 10;


bool save_to_flash(const std::vector<Light_setting> &settings)
{
    int i = 0;
    size_t count = settings.size();
    if (count == 0)
    {
        return true;
    }
    EEPROM.put(0, count);
    for (const Light_setting &s : settings)
    {
        EEPROM.put(sizeof(size_t) + i * sizeof(Light_setting), s);
        i++;
    }
    return !EEPROM.commit();
}

bool save_to_flash(size_t pos, const std::vector<Light_setting> &settings)
{
    size_t count = settings.size();
    if (pos > count - 1)
    {
        return true;
    }
    EEPROM.put(sizeof(size_t) + pos * sizeof(Light_setting), settings[pos]);
    return !EEPROM.commit();
}

bool load_from_flash(std::vector<Light_setting> &settings)
{
    size_t count = 0;
    EEPROM.get(0, count);
    if (count == 0 || count > 64U)
    {
        return true;
    }
    settings.clear();
    for (size_t i = 0; i < count; i++)
    {
        Light_setting s;
        EEPROM.get(sizeof(size_t) + i * sizeof(Light_setting), s);
        settings.push_back(s);
    }
    return settings.empty();
}

bool save_to_flash(int address, String &str)
{
    int len = static_cast<int>(str.length());
    if (len > 50 - static_cast<int>(sizeof(len)))
    {
        str = str.substring(0, 50 - sizeof(len));
        len = 50 - sizeof(len);
        Serial.println("String to exeed max lenght to save in flash.");
        Serial.println("Saving: " + str);
    }
    EEPROM.put(address, len);
    for (int i = 0; i < len; i++)
    {
        char symbol = str.charAt(i);
        EEPROM.put(address + sizeof(len) + sizeof(symbol) * i, symbol);
    }
    return !EEPROM.commit();
}

bool read_from_flash(int address, String &str)
{
    str.clear();
    int len = 0;
    EEPROM.get(address, len);
    if (len > 50 - static_cast<int>(sizeof(len)) || len < 0)
    {
        return true;
    }
    for (int i = 0; i < len; i++)
    {
        char symbol = ' ';
        EEPROM.get(address + sizeof(len) + sizeof(symbol) * i, symbol);
        str += String(symbol);
    }
    return str.isEmpty();
}

bool save_to_flash(int address, uint8_t *data, size_t size)
{
    if (size == 0 || !data)
    {
        return true;
    }
    for (size_t i = 0; i < size; i++)
    {
        EEPROM.write(address + i, data[i]);
    }
    return !EEPROM.commit();
}

bool read_from_flash(int address, uint8_t *data, size_t size)
{
    if (size == 0 || !data)
    {
        return true;
    }
    for (size_t i = 0; i < size; i++)
    {
        uint8_t symbol = 0;
        EEPROM.get(address + i, symbol);
        data[i] = symbol;
    }
    return false;
}

bool save_to_flash(int address, const std::vector<Light_driver> &drivers)
{
    size_t count = drivers.size();
    if (!count || count > 10U)
    {
        return true;
    }
    EEPROM.put(address, count);
    int i = 0;
    for (const Light_driver &d : drivers)
    {
        // sizeof(size_t) is count in memory
        EEPROM.put(address + sizeof(size_t) + i * sizeof(Light_driver), d);
        i++;
    }
    return !EEPROM.commit();
}

bool load_from_flash(int address, std::vector<Light_driver> &drivers)
{
    size_t count = 0;
    EEPROM.get(address, count);
    if (count == 0 || count > 10U)
    {
        return true;
    }
    drivers.clear();
    for (size_t i = 0; i < count; i++)
    {
        Light_driver d;
        // sizeof(size_t) is count in memory
        EEPROM.get(address + sizeof(size_t) + i * sizeof(Light_driver), d);
        drivers.push_back(d);
    }
    return drivers.empty();
}