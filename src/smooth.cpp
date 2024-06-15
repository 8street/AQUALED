#include <Arduino.h>

#include "smooth.h"

Smooth::Smooth()
{
}
Smooth::Smooth(int ms_update)
    : m_ms_update(ms_update)
{
}
Smooth::~Smooth()
{
}
int Smooth::set_update_ms(int ms_update)
{
    m_ms_update = ms_update;
    return 0;
}
int Smooth::get_unsmoothed(int val)
{
    m_val = val;
    m_final_val = val;
    return m_val;
}
int Smooth::get_value()
{
    proceed();
    return m_val;
}
int Smooth::get_smoothed(int val)
{
    m_final_val = val;
    return get_value();
}
int Smooth::proceed()
{
    static int ms_passed;
    ms_passed += passed_ms();
    if (ms_passed < m_ms_update)
    {
        return 0;
    }
    if (m_val < m_final_val)
    {
        m_val += 1;
    }
    else if (m_val > m_final_val)
    {
        m_val -= 1;
    }
    ms_passed -= m_ms_update;
    return 0;
}
int Smooth::get_update_ms() const
{
    return m_ms_update;
}
int Smooth::passed_ms()
{
    static unsigned long old_millis;
    const unsigned long current_millis = millis();
    if (current_millis <= old_millis)
    {
        old_millis = current_millis;
        return 0;
    }
    const int passed = static_cast<int>(current_millis - old_millis);
    old_millis = current_millis;
    return passed;
}