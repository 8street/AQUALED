#pragma once
#ifndef LIGHT_DRIVER_H
#define LIGHT_DRIVER_H

#include "PWM.h"

class Light_driver
{
private:
    PWM m_pwm;
    
    // power in W
    int m_power = 0;

    // color temp in K
    int m_color_temp = 0;

    // current in percent
    int m_current = 0;

    int m_current_led_off = 0;
    int m_current_led_max = 100;

public:
    Light_driver(const PWM &pwm, int power = 0, int color_temp = 0, int led_off = 0, int led_max = 100);
    ~Light_driver();

    bool off();
    bool set_current(int percent);
    int get_power() const;
    int get_color_temp() const;
    int get_current() const;
    int linearization(int power);
    bool set_current(int percent, int color_temp);
    int get_power(int power, int color) const;
    int get_power_w(int power, int color) const;
};


#endif