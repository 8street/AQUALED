#pragma once
#ifndef LIGHT_H
#    define LIGHT_H

#    include "PWM.h"
#    include "light_driver.h"
#    include "light_setting.h"
#    include <vector>

#    define RED_COLOR_TEMP 3000
#    define WHITE_COLOR_TEMP 6500
#    define AVERAGE_COLOR_TEMP 5400

class Light
{
public:
    Light();
    ~Light();

    bool add_light_driver(const Light_driver &driver);
    bool set_light(int in_power_percent, int in_color_temp);
    bool set_light(const std::vector<Light_setting> &settings, NTPClient &time_client);
    bool set_light(const Light_setting &setting);
    const Light_setting &get_previous_point(const std::vector<Light_setting> &settings, NTPClient &time_client) const;
    const Light_setting &get_next_point(const std::vector<Light_setting> &settings, NTPClient &time_client) const;
    Light_setting get_current_point(const std::vector<Light_setting> &settings, NTPClient &time_client) const;
    int get_summary_power() const;
    int get_max_power(int color_temp) const;
    int get_max_power_in_percent(int color_temp) const;
    bool set_max_power(int color_temp);
    int get_driver_current(int driver_index) const;
    int get_driver_current(const Light_setting &setting, int driver_index) const;
    int get_driver_power_w(int driver_index) const;
    int get_driver_power_w(const Light_setting &setting, int driver_index) const;
    int get_driver_max_power_w(int driver_index) const;
    int get_drivers_count() const;
    int get_summary_power_w(const Light_setting &setting) const;
    int get_day_power_wh(const std::vector<Light_setting> &settings) const;
    Light_driver &get_driver(int driver_index);
    const Light_driver &get_driver(int driver_index) const;
    std::vector<Light_driver> &get_drivers();

private:
    std::vector<Light_driver> m_drivers;

    int check_driver_num(int driver_index) const;
};

#endif
