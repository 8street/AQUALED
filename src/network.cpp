
#include <ESP8266WiFi.h>

#include "main.h"
#include "memory.h"
#include "network.h"
#include "password.h"

bool wifi_init()
{
    bool ret_val = false;
    int attempts = 0;
    if (digitalRead(WIFI_PIN))
    {
        // Connect to existing WiFi
        Serial.print(F("Connecting"));
        WiFi.mode(WIFI_STA);
        ret_val |= WiFi.setAutoReconnect(true);
        WiFi.begin(Eeprom_ssid, Eeprom_password);

        while (WiFi.status() != WL_CONNECTED)
        {
            ESP.wdtFeed();
            delay(450);
            digitalWrite(LED_PIN, LOW);
            delay(50);
            digitalWrite(LED_PIN, HIGH);
            Serial.print(".");
            attempts++;
            if (attempts > 500)
            {
                ret_val |= WiFi.disconnect(true);
                Serial.println();
                Serial.println("WiFi cannot connect to SSID " + Eeprom_ssid + ". Switch to AP mode.");
                break;
            }
        }
        ESP.wdtFeed();
        Serial.println("");
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println(F("Connected to ") + Eeprom_ssid);
            Serial.println(F("IP address: ") + WiFi.localIP().toString());
        }
    }

    // AP mode
    if (!digitalRead(WIFI_PIN) || attempts > 500)
    {
        ESP.wdtFeed();
        WiFi.mode(WIFI_AP);
        ret_val |= WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
        ret_val |= WiFi.softAP(Default_ap_ssid, Default_ap_password, 1, 1);
        Serial.println(F("WiFi AP created. SSID: ") + String(Default_ap_ssid));
        Serial.println(F("AP password: ") + String(Default_ap_password));
        Serial.println(F("IP address: ") + WiFi.softAPIP().toString());
    }
    // 13dbm = 20 mW
    WiFi.setOutputPower(13.0f);

    Serial.println();

    return ret_val | (WiFi.status() != WL_CONNECTED);
}

bool wifi_connect_to_station()
{
    bool ret_val = false;
    const WiFiMode_t currentMode = WiFi.getMode();
    const bool ap_is_enabled = ((currentMode & WIFI_AP) != 0);
    if (ap_is_enabled)
    {
        ret_val |= WiFi.softAPdisconnect(true);
        ret_val |= WiFi.disconnect(true);
        delay(500);
        ESP.wdtFeed();
        ret_val |= wifi_init();
    }
    return ret_val;
}