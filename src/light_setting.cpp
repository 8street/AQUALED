
#include <EEPROM.h>

#include "Light.h"
#include "light_setting.h"
#include "helper.h"

/* Light_setting& Light_setting::operator=(const Light_setting& s)
{
    m_time = s.m_time;
    m_color_temp = s.m_color_temp;
    m_power_percent = s.m_power_percent;
    m_random = s.m_random;
    return *this;
} */

Light_setting::Light_setting(const MyTime &time, int power_percent, int color_temp, int random):
    m_time(time), m_power_percent(power_percent), m_color_temp(color_temp), m_random(random)
{
    //random_update();
}
Light_setting::Light_setting(int hour, int power_percent, int color_temp, int random):
    m_time(MyTime(hour)), m_power_percent(power_percent), m_color_temp(color_temp), m_random(random)
{
    //random_update();
}
bool Light_setting::operator<(const Light_setting& a) const
{
    return m_time < a.m_time;
}
bool Light_setting::operator==(const Light_setting& a) const
{
    return m_time == a.m_time;
}
String Light_setting::print() const
{
    return "Time: " + m_time.print() + " Power: " + String(m_power_percent) + " Color temperature: " + String(m_color_temp) + " Random: " + String(m_random);
}

bool save_to_flash(const std::vector<Light_setting> &settings)
{
    int i = 0;
    size_t count = settings.size();
    EEPROM.put(0, count);
    for(const Light_setting &s : settings){
        EEPROM.put(sizeof(size_t) + i * sizeof(Light_setting), s);
        i++;
    }
    return EEPROM.commit();
}

bool save_to_flash(size_t pos, const std::vector<Light_setting> &settings)
{
    size_t count = settings.size();
    if(pos > count - 1){
        return true;
    }
    EEPROM.put(sizeof(size_t) + pos * sizeof(Light_setting), settings[pos]);
    return EEPROM.commit();
}

bool load_from_flash(std::vector<Light_setting> &settings)
{
    settings.clear();
    size_t count = 0;
    EEPROM.get(0, count);
    if(count > 64U){
        return true;
    }
    for(size_t i = 0; i < count; i++){
        Light_setting s;
        EEPROM.get(sizeof(size_t) + i * sizeof(Light_setting), s);
        settings.push_back(s);
    }
    return settings.empty();
}

Light_setting new_random_setting(const Light_setting &setting)
{
    const int power = clamp(setting.m_power_percent + static_cast<int>(random(-(setting.m_random), setting.m_random)), 0, 100);
    const int color = clamp(setting.m_color_temp + static_cast<int>(WHITE_COLOR_TEMP * random(-(setting.m_random), setting.m_random) / 100), RED_COLOR_TEMP, WHITE_COLOR_TEMP);
    return Light_setting(setting.m_time, power, color, setting.m_random);
}

bool update_random_settings(const std::vector<Light_setting> &settings, std::vector<Light_setting> &random_settings)
{
    random_settings.clear();
    for(const Light_setting &s : settings){
      random_settings.push_back(new_random_setting(s));
    }
    return false;
}