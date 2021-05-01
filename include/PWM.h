#pragma once
#ifndef PWM_H
#define PWM_H

class PWM
{
private:
    uint8_t m_gpio_pin;
    uint32_t m_frequency;
    int m_duty_cycle = 0;
    uint32_t m_resolution;

public:
    PWM(uint8_t gpio_pin, uint32_t frequency = 1000, uint32_t resolution = 1024);
    ~PWM();

    bool stop();
    bool max();
    bool set_duty_cycle(int duty_cycle);
    bool is_active();
    uint32_t resolution();
};

#endif