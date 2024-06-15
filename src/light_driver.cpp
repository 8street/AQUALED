
#include "light_driver.h"
#include "Light.h"
#include "helper.h"

///////////////////// Light_driver ///////////////////////////
Light_driver::Light_driver()
{
}
Light_driver::Light_driver(const PWM &pwm, int power, int color_temp, int led_off, int led_max, int nonlinearity)
{
    reinit(pwm, power, color_temp, led_off, led_max, nonlinearity);
}
Light_driver::~Light_driver()
{
}
int Light_driver::reinit(const PWM &pwm, int power, int color_temp, int led_off, int led_max, int nonlinearity)
{
    m_pwm = pwm;
    const int max = m_pwm.get_resolution() - m_pwm.get_resolution() * led_off / 100;
    const int min = m_pwm.get_resolution() - m_pwm.get_resolution() * led_max / 100;
    m_pwm.set_min_max(min, max);
    m_max_power_w = power;
    m_color_temp = color_temp;
    m_current_led_off = led_off;
    m_current_led_max = led_max;
    return set_nonlinearity(nonlinearity);
}
int Light_driver::off()
{
    m_current = 0;
    return m_pwm.set_duty_cycle_fast(m_current_led_off);
}
int Light_driver::set_current(int percent)
{
    m_current = clamp(percent, 0, 100);
    // const float pwm_power = 100.0f - linearization(m_current);
    // Serial.println("In percent: " + String(m_current));
    int pwm_power = 100 - linearization(m_current);
    pwm_power = clamp(pwm_power, 0, 100);
    // Serial.println("Linear power: " + String(pwm_power));
    // return m_pwm.set_duty_cycle_smooth(lroundf(m_pwm.get_resolution() * pwm_power / 100.0f));
    return m_pwm.set_duty_cycle_smooth(m_pwm.get_resolution() * pwm_power / 100);
}
int Light_driver::get_max_power_w() const
{
    return m_max_power_w;
}
int Light_driver::get_color_temp() const
{
    return m_color_temp;
}
int Light_driver::get_current() const
{
    return m_current;
}
float Light_driver::linearization(int power)
{
    /*if (power == 0)
    {
        return 0.0f;
    }
    if (power == 100)
    {
        return 100.0f;
    };

    //if(m_nonlinearity == 1000)
    //{
        // y = k*x + c
        // y = k*x + led_off
        //return ;
    //}

    if(-power == lroundf(m_dx))
    {
        return m_c;
    }
    // y = -1/(x + dx) + c
    return -m_nonlinearity / (power + m_dx) + m_c;*/
    if (power <= 0)
    {
        return 0;
    }
    if (power >= 100)
    {
        return 100;
    }
    return power * (m_current_led_max - m_current_led_off) / (100 - 0) + m_current_led_off;
}
int Light_driver::set_current(int percent, int color_temp)
{
    return set_current(get_power(percent, color_temp));
}
int Light_driver::get_power(int power, int color) const
{
    color = clamp(color, RED_COLOR_TEMP, WHITE_COLOR_TEMP);
    power = clamp(power, 0, 100);

    int max_white = 0;
    int max_red = 0;
    if (color <= AVERAGE_COLOR_TEMP)
    {
        max_red = 100;
        max_white = 100 * (color - RED_COLOR_TEMP) / (AVERAGE_COLOR_TEMP - RED_COLOR_TEMP);
    }
    else
    {
        max_white = 100;
        max_red = 100 * (WHITE_COLOR_TEMP - color) / (WHITE_COLOR_TEMP - AVERAGE_COLOR_TEMP);
    }

    if (m_color_temp == WHITE_COLOR_TEMP)
    {
        return max_white * power / 100;
    }
    else if (m_color_temp == RED_COLOR_TEMP)
    {
        return max_red * power / 100;
    }
    Serial.println(F("Invalid Light Driver color temp: ") + String(m_color_temp));
    return 0;
}
int Light_driver::get_power_w(int power, int color) const
{
    const int power_percent = get_power(power, color);
    return power_percent * m_max_power_w / 100;
}
int Light_driver::get_current_power_w() const
{
    const int driver_power_w = get_max_power_w();
    const int driver_current = get_current();
    return driver_power_w * driver_current / 100;
}
PWM &Light_driver::get_pwm()
{
    return m_pwm;
}
const PWM &Light_driver::get_pwm() const
{
    return m_pwm;
}
int Light_driver::get_setting_led_off() const
{
    return m_current_led_off;
}
int Light_driver::get_setting_led_max() const
{
    return m_current_led_max;
}
int Light_driver::init_linearity_coeffs()
{
    int ret_val = 0;
    const float c = -static_cast<float>(100 * m_nonlinearity) / (m_current_led_max - m_current_led_off);
    const float Discriminant = 10000.0f - 4.0f * c;
    m_dx = (-100.0f + sqrtf(Discriminant)) / 2.0f;
    if (m_dx < 0.0f)
    {
        m_dx = (-100.0f - sqrtf(Discriminant)) / 2.0f;
    }
    if (m_dx < 0.0f)
    {
        ret_val = -1;
        Serial.println(F("ERR! Bad linearity coefficients!"));
    }
    m_c = m_current_led_off + m_nonlinearity / m_dx;
    return ret_val;
}
int Light_driver::set_nonlinearity(int nonlin)
{
    nonlin = clamp(nonlin, 1, 1000);
    m_nonlinearity = lroundf(1000.0f / nonlin);
    return init_linearity_coeffs();
}
int Light_driver::get_nonlinearity() const
{
    return lroundf(1000.0f / m_nonlinearity);
}