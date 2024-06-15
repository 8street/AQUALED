#include "mytime.h"
#include "helper.h"

MyTime::MyTime(int hour, int min, int sec)
{
    m_hour = clamp(hour, 0, 24);
    m_min = clamp(min, 0, 60);
    m_sec = clamp(sec, 0, 60);
}
MyTime::MyTime(NTPClient &ntp)
    : m_hour(ntp.getHours())
    , m_min(ntp.getMinutes())
    , m_sec(ntp.getSeconds())
{
}
int MyTime::get_hour() const
{
    return m_hour;
}
int MyTime::get_min() const
{
    return m_min;
}
int MyTime::get_sec() const
{
    return m_sec;
}
bool MyTime::set_hour(int h)
{
    m_hour = clamp(h, 0, 24);
    return 0;
}
bool MyTime::set_min(int m)
{
    m_min = clamp(m, 0, 60);
    return 0;
}
bool MyTime::set_sec(int s)
{
    m_sec = clamp(s, 0, 60);
    return 0;
}
int MyTime::get_overall_seconds() const
{
    return m_hour * 3600 + m_min * 60 + m_sec;
}
float MyTime::get_hour_float() const
{
    return m_hour + m_min / 60.0f;
}
bool MyTime::operator<(const MyTime &t) const
{
    return get_overall_seconds() < t.get_overall_seconds();
}
bool MyTime::operator==(const MyTime &t) const
{
    return m_hour == t.m_hour && m_min == t.m_min && m_sec == t.m_sec;
}
String MyTime::print() const
{
    return String(m_hour) + ":" + String(m_min) + ":" + String(m_sec);
}
