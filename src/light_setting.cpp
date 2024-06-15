
#include "light_setting.h"
#include "Light.h"
#include "helper.h"

/* Light_setting& Light_setting::operator=(const Light_setting& s)
{
    m_time = s.m_time;
    m_color_temp = s.m_color_temp;
    m_power_percent = s.m_power_percent;
    m_random = s.m_random;
    return *this;
} */

Light_setting::Light_setting(const MyTime &time, int power_percent, int color_temp, int random)
    : m_time(time)
    , m_power_percent(power_percent)
    , m_color_temp(color_temp)
    , m_random(random)
{
    // random_update();
}
Light_setting::Light_setting(int hour, int power_percent, int color_temp, int random)
    : m_time(MyTime(hour))
    , m_power_percent(power_percent)
    , m_color_temp(color_temp)
    , m_random(random)
{
    // random_update();
}
bool Light_setting::operator<(const Light_setting &a) const
{
    return m_time < a.m_time;
}
bool Light_setting::operator==(const Light_setting &a) const
{
    return m_time == a.m_time;
}
String Light_setting::print() const
{
    return "Time: " + m_time.print() + " Power: " + String(m_power_percent) + " Color temperature: " + String(m_color_temp)
        + " Random: " + String(m_random);
}

Light_setting new_random_setting(const Light_setting &setting)
{
    const int power = clamp(setting.m_power_percent + static_cast<int>(random(-(setting.m_random), setting.m_random)), 0, 100);
    // const int color = clamp(setting.m_color_temp + static_cast<int>(WHITE_COLOR_TEMP * random(-(setting.m_random),
    // setting.m_random) / 100), RED_COLOR_TEMP, WHITE_COLOR_TEMP);
    const int color = setting.m_color_temp;
    return Light_setting(setting.m_time, power, color, setting.m_random);
}

bool update_random_settings(const std::vector<Light_setting> &settings, std::vector<Light_setting> &random_settings)
{
    random_settings.clear();
    for (const Light_setting &s : settings)
    {
        random_settings.push_back(new_random_setting(s));
    }
    return false;
}