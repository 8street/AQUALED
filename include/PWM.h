#pragma once
#ifndef PWM_H
#    define PWM_H

#    include <cstdint>

#    include "smooth.h"

class PWM
{
public:
    PWM();
    PWM(uint8_t gpio_pin, uint32_t frequency, uint32_t resolution, int smooth_ms);
    PWM(const PWM &p);
    ~PWM();

    PWM &operator=(const PWM &p);

    bool reinit(uint8_t gpio_pin, uint32_t frequency, uint32_t resolution, int smooth_ms, uint32_t pwm_min, uint32_t pwm_max);
    bool stop();
    bool max();
    bool set_duty_cycle_fast(int duty_cycle);
    bool set_duty_cycle_smooth(int duty_cycle);
    bool is_active();
    bool set_min_max(int min, int max);
    uint32_t get_resolution() const;
    uint8_t get_pin() const;
    uint32_t get_frequency() const;
    int get_smooth_ms() const;

private:
    uint8_t m_gpio_pin = 0;
    uint32_t m_frequency = 1000;
    int m_duty_cycle = 1000;
    uint32_t m_resolution = 1024;
    Smooth m_smooth;
    uint32_t m_pwm_max = 1024;
    uint32_t m_pwm_min = 0;
    void destroy();
};

#endif