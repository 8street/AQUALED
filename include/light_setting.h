#pragma once
#pragma once
#ifndef LIGHT_SETTING_H
#    define LIGHT_SETTING_H

#    include "PWM.h"
#    include "mytime.h"

struct Light_setting
{
    // time in seconds
    MyTime m_time;

    // brightness in percent
    int m_power_percent;

    // color temperature in K
    int m_color_temp;

    // random brightness in percent
    int m_random;

    int m_reserve = 0;

    // Light_setting(const Light_setting &s);
    // Light_setting &operator=(const Light_setting &s);
    Light_setting(const MyTime &time, int power_percent = 0, int color_temp = 6500, int random = 0);
    Light_setting(int hour = 0, int power_percent = 0, int color_temp = 6500, int random = 0);
    bool operator<(const Light_setting &a) const;
    bool operator==(const Light_setting &a) const;
    String print() const;
};

Light_setting new_random_setting(const Light_setting &setting);
bool update_random_settings(const std::vector<Light_setting> &settings, std::vector<Light_setting> &random_settings);

#endif