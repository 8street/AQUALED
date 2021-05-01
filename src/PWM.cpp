#include "helper.h"
#include "main.h"
#include "PWM.h"


PWM::PWM(uint8_t gpio_pin, uint32_t frequency, uint32_t resolution):
m_gpio_pin(gpio_pin), m_frequency(frequency), m_resolution(resolution)
{
  max();
  analogWriteFreq(m_frequency);
  analogWriteRange(m_resolution);
}

PWM::~PWM()
{

}

bool PWM::stop()
{
    return set_duty_cycle(0);
}

bool PWM::max()
{
    return set_duty_cycle(m_resolution);
}

bool PWM::set_duty_cycle(int duty_cycle)
{
    m_duty_cycle = clamp(duty_cycle, 0, static_cast<int>(m_resolution));
    analogWrite(m_gpio_pin, m_duty_cycle);
    return 0;
}

bool PWM::is_active()
{
    if(m_duty_cycle){
        return true;
    }else{
        return false;
    }
}

uint32_t PWM::resolution()
{
    return m_resolution;
}