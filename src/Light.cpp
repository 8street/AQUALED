
#include "Light.h"
#include "helper.h"
#include <EEPROM.h>

Light::Light()
{
}
Light::~Light()
{
}
bool Light::add_light_driver(const Light_driver &driver)
{
    m_drivers.push_back(driver);
    return 0;
}
bool Light::set_light(const std::vector<Light_setting> &settings, NTPClient &time_client)
{
    return set_light(get_current_point(settings, time_client));
}
bool Light::set_light(const Light_setting &setting)
{
    return set_light(setting.m_power_percent, setting.m_color_temp);
}
const Light_setting &Light::get_previous_point(const std::vector<Light_setting> &settings, NTPClient &time_client) const
{
    const int current_time = time_client.getHours() * 3600 + time_client.getMinutes() * 60 + time_client.getSeconds();

    int i = 0;
    int min_index = 0;
    int min = INT32_MAX;
    for (const Light_setting &s : settings)
    {
        const int sub_time = s.m_time.get_overall_seconds() - current_time;
        if (abs(sub_time) < min && sub_time <= 0)
        {
            min = abs(sub_time);
            min_index = i;
        }
        i++;
    }
    return settings.at(min_index);
}
const Light_setting &Light::get_next_point(const std::vector<Light_setting> &settings, NTPClient &time_client) const
{
    const int current_time = time_client.getHours() * 3600 + time_client.getMinutes() * 60 + time_client.getSeconds();

    int i = 0;
    int min_index = 0;
    int min = INT32_MAX;
    for (const Light_setting &s : settings)
    {
        const int sub_time = s.m_time.get_overall_seconds() - current_time;
        if (abs(sub_time) < min && sub_time > 0)
        {
            min = abs(sub_time);
            min_index = i;
        }
        i++;
    }
    return settings.at(min_index);
}
Light_setting Light::get_current_point(const std::vector<Light_setting> &settings, NTPClient &time_client) const
{
    const int current_time = time_client.getHours() * 3600 + time_client.getMinutes() * 60 + time_client.getSeconds();
    // found 2 nearest time poitns
    Light_setting previous = get_previous_point(settings, time_client);
    Light_setting next = get_next_point(settings, time_client);

    if (next.m_time < previous.m_time)
    {
        std::swap(next, previous);
    }
    if (next.m_time == previous.m_time)
    {
        return Light_setting();
    }

    const int power_percent = (next.m_power_percent - previous.m_power_percent)
            * (current_time - previous.m_time.get_overall_seconds())
            / (next.m_time.get_overall_seconds() - previous.m_time.get_overall_seconds())
        + previous.m_power_percent;
    const int color_temp = (next.m_color_temp - previous.m_color_temp) * (current_time - previous.m_time.get_overall_seconds())
            / (next.m_time.get_overall_seconds() - previous.m_time.get_overall_seconds())
        + previous.m_color_temp;

    return Light_setting(MyTime(time_client), power_percent, color_temp, 0);
}
bool Light::set_light(int in_power_percent, int in_color_temp)
{
    bool ret_val = false;
    for (Light_driver &driver : m_drivers)
    {
        ret_val |= driver.set_current(in_power_percent, in_color_temp);
    }
    return ret_val;
}
int Light::get_summary_power() const
{
    int power = 0;
    for (const Light_driver &driver : m_drivers)
    {
        power += driver.get_max_power_w();
    }
    return power;
}
int Light::get_max_power(int color_temp) const
{
    int red_power = 0;
    int white_power = 0;
    color_temp = clamp(color_temp, RED_COLOR_TEMP, WHITE_COLOR_TEMP);

    for (const Light_driver &driver : m_drivers)
    {
        if (driver.get_color_temp() == WHITE_COLOR_TEMP)
        {
            white_power += driver.get_max_power_w();
        }
        else
        {
            red_power += driver.get_max_power_w();
        }
    }

    const int summary_power = get_summary_power();
    // average color temp is maximum poit of red + white led power
    if (color_temp < AVERAGE_COLOR_TEMP)
    {
        return (summary_power - red_power) * (color_temp - RED_COLOR_TEMP) / (AVERAGE_COLOR_TEMP - RED_COLOR_TEMP) + red_power;
    }
    else
    {
        return (white_power - summary_power) * (color_temp - AVERAGE_COLOR_TEMP) / (WHITE_COLOR_TEMP - AVERAGE_COLOR_TEMP)
            + summary_power;
    }
}
int Light::get_max_power_in_percent(int color_temp) const
{
    return get_max_power(color_temp) * 100 / get_summary_power();
}
bool Light::set_max_power(int color_temp)
{
    return set_light(get_max_power_in_percent(color_temp), color_temp);
}
int Light::get_driver_current(int driver_index) const
{
    driver_index = check_driver_num(driver_index);
    return m_drivers.at(driver_index).get_current();
}
int Light::get_driver_current(const Light_setting &setting, int driver_index) const
{
    driver_index = check_driver_num(driver_index);
    return m_drivers.at(driver_index).get_power(setting.m_power_percent, setting.m_color_temp);
}
int Light::get_driver_power_w(int driver_index) const
{
    driver_index = check_driver_num(driver_index);
    return m_drivers.at(driver_index).get_current_power_w();
}
int Light::get_driver_power_w(const Light_setting &setting, int driver_index) const
{
    driver_index = check_driver_num(driver_index);
    return m_drivers.at(driver_index).get_power_w(setting.m_power_percent, setting.m_color_temp);
}
int Light::get_driver_max_power_w(int driver_index) const
{
    driver_index = check_driver_num(driver_index);
    return m_drivers.at(driver_index).get_max_power_w();
}
int Light::get_drivers_count() const
{
    return static_cast<int>(m_drivers.size());
}
int Light::get_summary_power_w(const Light_setting &setting) const
{
    int power = 0;
    for (const Light_driver &driver : m_drivers)
    {
        power += driver.get_power_w(setting.m_power_percent, setting.m_color_temp);
    }
    return power;
}
int Light::get_day_power_wh(const std::vector<Light_setting> &settings) const
{
    if (settings.size() < 2 || m_drivers.empty())
    {
        return 0;
    }
    size_t set_num = 0;
    float wh = 0.0f;
    for (set_num = 0; set_num < settings.size() - 1; set_num++)
    {
        const float delta_hour = settings.at(set_num + 1).m_time.get_hour_float()
            - settings.at(set_num).m_time.get_hour_float();
        const int cur_summ_p = get_summary_power_w(settings.at(set_num));
        const int delta_w = get_summary_power_w(settings.at(set_num + 1)) - cur_summ_p;
        wh += delta_hour * (delta_w / 2 + cur_summ_p);
    }
    return lroundf(wh);
}
int Light::check_driver_num(int driver_index) const
{
    if (driver_index < 0)
    {
        driver_index = 0;
        Serial.println(F("Driver index < 0."));
    }
    if (driver_index > static_cast<int>(m_drivers.size()) - 1)
    {
        driver_index = static_cast<int>(m_drivers.size()) - 1;
        Serial.println(F("Driver index > max index."));
    }
    return driver_index;
}
Light_driver &Light::get_driver(int driver_index)
{
    driver_index = check_driver_num(driver_index);
    return m_drivers.at(driver_index);
}
const Light_driver &Light::get_driver(int driver_index) const
{
    driver_index = check_driver_num(driver_index);
    return m_drivers.at(driver_index);
}
std::vector<Light_driver> &Light::get_drivers()
{
    return m_drivers;
}