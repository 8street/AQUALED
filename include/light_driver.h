#pragma once
#ifndef LIGHT_DRIVER_H
#    define LIGHT_DRIVER_H

#    include "PWM.h"

class Light_driver
{
public:
    Light_driver();
    Light_driver(const PWM &pwm, int power, int color_temp, int led_off, int led_max, int nonlinearity);
    ~Light_driver();

    int reinit(const PWM &pwm, int power, int color_temp, int led_off, int led_max, int nonlinearity);
    int off();
    int set_current(int percent);
    int get_max_power_w() const;
    int get_color_temp() const;
    int get_current() const;
    float linearization(int power);
    int set_current(int percent, int color_temp);
    int get_power(int power, int color) const;
    int get_power_w(int power, int color) const;
    int get_current_power_w() const;
    PWM &get_pwm();
    const PWM &get_pwm() const;
    int get_setting_led_off() const;
    int get_setting_led_max() const;
    int set_nonlinearity(int nonlin);
    int get_nonlinearity() const;

private:
    int init_linearity_coeffs();
    PWM m_pwm;

    // power in W
    int m_max_power_w = 0;

    // color temp in K
    int m_color_temp = 0;

    // current in percent
    int m_current = 0;

    int m_current_led_off = 0;
    int m_current_led_max = 100;

    int m_nonlinearity = 1000;
    float m_dx = 0.0f;
    float m_c = 100.0f;
};

#endif