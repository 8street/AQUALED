#pragma once
#ifndef SMOOTH_H
#    define SMOOTH_H

class Smooth
{
public:
    Smooth();
    Smooth(int ms_update);
    ~Smooth();

    int set_update_ms(int ms_update);
    int get_unsmoothed(int val);
    int get_smoothed(int val);
    int proceed();
    int get_update_ms() const;

private:
    int passed_ms();
    int get_value();
    int m_ms_update = 0;
    int m_val = 0;
    int m_final_val = 0;
};

#endif