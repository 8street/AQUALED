#pragma once
#ifndef MYTIME_H
#define MYTIME_H

#include <Arduino.h>
#include <NTPClient.h>

class MyTime
{
private:
    int m_hour;
    int m_min;
    int m_sec;
public:
    MyTime(int hour = 0, int min = 0, int sec = 0);
    MyTime(NTPClient &ntp);
    int get_hour() const;
    int get_min() const;
    int get_sec() const;
    bool set_hour(int h);
    bool set_min(int m);
    bool set_sec(int s);
    int get_overall_seconds() const;
    bool operator<(const MyTime &t) const;
    bool operator==(const MyTime &t) const;
    String print() const;
};

#endif