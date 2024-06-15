

#include "time_helpers.h"
#include "main.h"

const char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

void update_NTP_time()
{
    Serial.println(F("NTP Time has been updated"));
    timeClient.update();
    Serial.print(daysOfTheWeek[timeClient.getDay()]);
    Serial.print(", ");
    Serial.println(timeClient.getFormattedTime());
}

String uptime()
{
    const unsigned long seconds = (millis() / 1000) % 60;
    const unsigned long minutes = (millis() / 1000 / 60) % 60;
    const unsigned long hours = (millis() / 1000 / 60 / 60) % 24;
    const unsigned long days = millis() / 1000 / 60 / 60 / 24;
    String ret;
    if (days)
    {
        ret += String(days) + "d:";
    }
    ret += String(hours) + "h:";
    ret += String(minutes) + "m:";
    ret += String(seconds) + "s";
    return ret;
}

bool new_day()
{
    static int old_hour;
    const int hour = timeClient.getHours();
    if (hour != old_hour)
    {
        old_hour = hour;
        if (hour == 0)
        {
            return true;
        }
    }
    return false;
}

bool new_second()
{
    static int old_sec;
    const int sec = timeClient.getSeconds();
    if (sec != old_sec)
    {
        old_sec = sec;
        return true;
    }
    return false;
}

bool new_minute()
{
    static int old_min;
    const int min = timeClient.getMinutes();
    if (min != old_min)
    {
        old_min = min;
        return true;
    }
    return false;
}

bool new_hour()
{
    static int old_hour;
    const int hour = timeClient.getHours();
    if (hour != old_hour)
    {
        old_hour = hour;
        return true;
    }
    return false;
}
