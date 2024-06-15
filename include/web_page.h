#pragma once
#ifndef WEB_PAGE_H
#    define WEB_PAGE_H

#    include <WiFiClient.h>

class Web_Page
{
public:
    Web_Page();
    ~Web_Page();
    int show(WiFiServer *server_ptr);

private:
    int requests();
    int web_head();
    int web_body();
    int main_web();
    int settings_web();
    WiFiClient *m_client_ptr = nullptr;
    bool m_wifi_settings = false;
    bool m_settings_not_saved = false;
    bool m_drivers_not_saved = false;
};

#endif
