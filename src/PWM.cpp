#include <Arduino.h>

#include "PWM.h"
#include "helper.h"

PWM::PWM()
{
}

PWM::PWM(uint8_t gpio_pin, uint32_t frequency, uint32_t resolution, int smooth_ms)
{
    reinit(gpio_pin, frequency, resolution, smooth_ms, 0, resolution);
}

PWM::~PWM()
{
    destroy();
}

PWM::PWM(const PWM &p)
{
    reinit(p.m_gpio_pin, p.m_frequency, p.m_resolution, p.get_smooth_ms(), p.m_pwm_min, p.m_pwm_max);
}

PWM &PWM::operator=(const PWM &p)
{
    reinit(p.m_gpio_pin, p.m_frequency, p.m_resolution, p.get_smooth_ms(), p.m_pwm_min, p.m_pwm_max);
    return *this;
}

void PWM::destroy()
{
    stop();
    m_gpio_pin = 0;
}

bool PWM::reinit(uint8_t gpio_pin, uint32_t frequency, uint32_t resolution, int smooth_ms, uint32_t pwm_min, uint32_t pwm_max)
{
    destroy();
    m_gpio_pin = clamp(gpio_pin, uint8_t{ 0 }, uint8_t{ 16 });
    pinMode(m_gpio_pin, OUTPUT);
    digitalWrite(m_gpio_pin, HIGH); // off
    m_smooth.set_update_ms(smooth_ms);
    m_frequency = frequency;
    m_resolution = resolution;
    m_pwm_min = clamp(pwm_min, 0u, m_resolution);
    m_pwm_max = clamp(pwm_max, 0u, m_resolution);
    analogWriteFreq(m_frequency);
    analogWriteRange(m_resolution);
    max(); // off
    return false;
}

bool PWM::stop()
{
    m_duty_cycle = 0;
    m_smooth.get_unsmoothed(m_pwm_max);
    analogWrite(m_gpio_pin, 0);
    digitalWrite(m_gpio_pin, HIGH); // off
    return 0;
}

bool PWM::max()
{
    m_smooth.get_unsmoothed(m_pwm_max);
    analogWrite(m_gpio_pin, m_resolution);
    return 0;
}

bool PWM::set_duty_cycle_fast(int duty_cycle)
{
    // Serial.println("Duty cycle: " + String(m_duty_cycle));
    m_duty_cycle = clamp(duty_cycle, static_cast<int>(m_pwm_min), static_cast<int>(m_pwm_max));
    // Serial.println("Analog_write: " + String(m_smooth.get_unsmoothed(m_duty_cycle)));
    analogWrite(m_gpio_pin, m_smooth.get_unsmoothed(m_duty_cycle));
    yield();
    return 0;
}
bool PWM::set_duty_cycle_smooth(int duty_cycle)
{
    m_duty_cycle = clamp(duty_cycle, static_cast<int>(m_pwm_min), static_cast<int>(m_pwm_max));
    analogWrite(m_gpio_pin, m_smooth.get_smoothed(m_duty_cycle));
    yield();
    return 0;
}
bool PWM::is_active()
{
    if (m_duty_cycle)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool PWM::set_min_max(int min, int max)
{
    if (min > max)
    {
        std::swap(min, max);
    }
    m_pwm_min = min;
    m_pwm_max = max;
    return false;
}

uint32_t PWM::get_resolution() const
{
    return m_resolution;
}

uint8_t PWM::get_pin() const
{
    return m_gpio_pin;
}

uint32_t PWM::get_frequency() const
{
    return m_frequency;
}
int PWM::get_smooth_ms() const
{
    return m_smooth.get_update_ms();
}