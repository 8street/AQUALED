
#include "helper.h"
#include "light_driver.h"
#include "Light.h"

///////////////////// Light_driver ///////////////////////////

Light_driver::Light_driver(const PWM &pwm, int power, int color_temp, int led_off, int led_max):
    m_pwm(pwm), m_power(power), m_color_temp(color_temp), m_current_led_off(led_off), m_current_led_max(led_max)
{

}
Light_driver::~Light_driver()
{

}
bool Light_driver::off()
{
    m_current = 0;
    return m_pwm.stop();
}
bool Light_driver::set_current(int percent)
{
    m_current = clamp(percent, 0, 100);
    const int pwm_power = 100 - linearization(m_current);
    return m_pwm.set_duty_cycle(m_pwm.resolution() * pwm_power / 100);
}
int Light_driver::get_power() const
{
    return m_power;
}
int Light_driver::get_color_temp() const
{
    return m_color_temp;
}
int Light_driver::get_current() const
{
    return m_current;
}
int Light_driver::linearization(int power)
{
    if(power == 0){
        return 0;
    }
    if(power == 100){
        return 100;
    }
    return power * (m_current_led_max - m_current_led_off) / (100 - 0) + m_current_led_off;
}
bool Light_driver::set_current(int percent, int color_temp)
{
    return set_current(get_power(percent, color_temp));
}
int Light_driver::get_power(int power, int color) const
{
    color = clamp(color, RED_COLOR_TEMP, WHITE_COLOR_TEMP);
    power = clamp(power, 0, 100);

    int max_white = 0;
    int max_red = 0;
    if(color <= AVERAGE_COLOR_TEMP){
        max_red = 100;
        max_white = 100 * (color - RED_COLOR_TEMP) / (AVERAGE_COLOR_TEMP - RED_COLOR_TEMP);
    }else{
        max_white = 100;
        max_red = 100 * (WHITE_COLOR_TEMP - color) / (WHITE_COLOR_TEMP - AVERAGE_COLOR_TEMP);
    }

    // rounding 99 to 100 and 1 to 0
    // max_red = rounding(max_red, 100, 1);
    // max_red = rounding(max_red, 0, 1);
    // max_white = rounding(max_white, 100, 1);
    // max_white = rounding(max_white, 0, 1);

    if(m_color_temp == WHITE_COLOR_TEMP){
        return max_white * power / 100;
    }else if(m_color_temp == RED_COLOR_TEMP){
        return max_red * power / 100;
    }
    Serial.println("Invalid Light Driver color temp: " + String(m_color_temp));
    return 0;
}
int Light_driver::get_power_w(int power, int color) const
{
    const int power_percent = get_power(power, color);
    return power_percent * m_power / 100;
}