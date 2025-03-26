
#include <ESP8266WiFi.h>

#include "helper.h"
#include "light_setting.h"
#include "main.h"
#include "memory.h"
#include "text.h"
#include "time_helpers.h"
#include "web_page.h"

Web_Page::Web_Page()
{
    m_client_ptr = new WiFiClient;
    if (!m_client_ptr)
    {
        Serial.println(F("ERROR: does not create new web page client."));
        delay(5000);
        ESP.restart();
    }
}

Web_Page::~Web_Page()
{
    if (m_client_ptr)
    {
        m_client_ptr->stopAll();
        delete m_client_ptr;
        m_client_ptr = nullptr;
    }
}

int Web_Page::show(WiFiServer *server_ptr)
{
    *m_client_ptr = server_ptr->accept();
    if (!*m_client_ptr || m_client_ptr->localPort() != 80 || !m_client_ptr->connected())
    {
        m_client_ptr->stopAll();
        return -1;
    }

    m_client_ptr->disableKeepAlive();
    m_client_ptr->setTimeout(500);
    Serial.println(F("New client")); // print a message out in the serial port
    Serial.println(F("Time: ") + timeClient.getFormattedTime());
    Serial.println(F("Local IP: ") + m_client_ptr->localIP().toString());
    Serial.println(F("Local port: ") + String(m_client_ptr->localPort()));
    Serial.println(F("Remote IP: ") + m_client_ptr->remoteIP().toString());
    Serial.println(F("Remote port: ") + String(m_client_ptr->remotePort()));

    int ret_val = 0;
    ret_val |= requests();
    yield();
    // Display the HTML web page
    ret_val |= web_head();
    yield();
    ret_val |= web_body();
    yield();
    // Close the connection
    // do not m_client->flush(): it is for output only, see below
    // The client will actually be *flushed* then disconnected
    // when the function returns and 'client' object is destroyed (out-of-scope)
    // flush = ensure written data are received by the other side

    Serial.println(F("Disconnecting from client."));
    Serial.println();

    return ret_val;
}

int Web_Page::requests()
{
    // Read the first line of the request
    String req = m_client_ptr->readStringUntil('\r');
    Serial.println(F("Request: ") + req);

    // turns the GPIOs on and off
    if (req.indexOf("/control/on") >= 0)
    {
        Serial.println(F("Direct Control on"));
        Direct_Control_state = true;
        Light_setting curr_s = Light_system_ptr->get_current_point(Random_light_settings, timeClient);
        Direct_Control_power = curr_s.m_power_percent;
        Direct_Control_color = curr_s.m_color_temp;
    }
    else if (req.indexOf("/control/off") >= 0)
    {
        Serial.println(F("Direct Control off"));
        Direct_Control_state = false;
        Direct_Control_power = 0;
    }
    else if (req.indexOf("/add_set") >= 0)
    {
        String add_index = get_middle_string(req, "add_set", "&");
        Serial.println("Add index: " + add_index);
        if (add_index.toInt() < 64)
        {
            MyTime next_added_time(
                Light_settings[add_index.toInt()].m_time.get_hour(), Light_settings[add_index.toInt()].m_time.get_min() + 1);
            Light_settings.push_back(Light_setting(next_added_time));
            std::sort(Light_settings.begin(), Light_settings.end());
            update_random_settings(Light_settings, Random_light_settings);
            m_settings_not_saved = true;
        }
        else
        {
            Serial.println(F("Settings limit exceeded!"));
        }
    }
    else if (req.indexOf("/del_set") >= 0)
    {
        String del_index = get_middle_string(req, "del_set", "&");
        if (Light_settings.size() > 0)
        {
            Serial.println("Del index: " + del_index);
            Light_settings.erase(Light_settings.begin() + del_index.toInt());
            std::sort(Light_settings.begin(), Light_settings.end());
            update_random_settings(Light_settings, Random_light_settings);
            m_settings_not_saved = true;
        }
        else
        {
            Serial.println(F("Cannot delete last element."));
        }
    }
    else if (req.indexOf("/setting") >= 0)
    {
        String save_index = get_middle_string(req, "setting", "?");
        String hour = get_middle_string(req, "hour=", "&endh");
        String minute = get_middle_string(req, "minute=", "&endm");
        String power = get_middle_string(req, "power=", "&endp");
        String color = get_middle_string(req, "color=", "&endc");
        String random = get_middle_string(req, "random=", " HTTP/1.1");
        Serial.print("Save index: " + save_index);
        Serial.print(" hour: " + hour);
        Serial.print(" minute: " + minute);
        Serial.print(" power: " + power);
        Serial.print(" color: " + color);
        Serial.println(" random: " + random);
        Light_settings.at(save_index.toInt()).m_time.set_hour(hour.toInt());
        Light_settings.at(save_index.toInt()).m_time.set_min(minute.toInt());
        Light_settings.at(save_index.toInt()).m_color_temp = color.toInt();
        Light_settings.at(save_index.toInt()).m_power_percent = clamp(static_cast<int>(power.toInt()), 0, 100);
        Light_settings.at(save_index.toInt()).m_random = random.toInt();
        std::sort(Light_settings.begin(), Light_settings.end());
        update_random_settings(Light_settings, Random_light_settings);
        m_settings_not_saved = true;
    }
    else if (req.indexOf("/control?") >= 0)
    {
        String power = get_middle_string(req, "power=", "&endp");
        String color = get_middle_string(req, "color=", " HTTP/1.1");
        Serial.print("Direct power: " + power);
        Serial.println(" Direct color: " + color);
        Direct_Control_color = color.toInt();
        Direct_Control_power = power.toInt();
    }
    else if (req.indexOf("/save_set") >= 0)
    {
        Serial.println(F("Save settings to FLASH."));
        const int led_state = digitalRead(LED_PIN);
        digitalWrite(LED_PIN, LOW);
        delay(200);
        save_to_flash(Light_settings);
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        ESP.wdtFeed();
        delay(200);
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, led_state);
        m_settings_not_saved = false;
    }
    else if (req.indexOf("/wifi_settings") >= 0)
    {
        if (m_wifi_settings)
        {
            m_wifi_settings = false;
        }
        else
        {
            m_wifi_settings = true;
        }
    }
    else if (req.indexOf("/wifi?") >= 0)
    {
        String ssid = get_middle_string(req, "ssid=", "&ends");
        String password = get_middle_string(req, "pa=", " HTTP/1.1");
        ssid = change_browser_code_to_symbol(ssid);
        password = change_browser_code_to_symbol(password);
        Serial.println("New SSID: " + ssid + " New Password: " + password);
        save_to_flash(Eeprom_ssid_address, ssid);
        save_to_flash(Eeprom_password_address, password);
        Serial.println(F("New WiFi settings saved."));
        Serial.println(F("Rebooting..."));
        ESP.restart();
    }
    else if (req.indexOf("/ntp?") >= 0)
    {
        String ntp = get_middle_string(req, "server=", "&ends");
        String timezone = get_middle_string(req, "tz=", " HTTP/1.1");
        ntp = change_browser_code_to_symbol(ntp);
        Serial.println("New NTP server: " + ntp + " Timezone: " + timezone);
        save_to_flash(Eeprom_ntp_address, ntp);
        save_to_flash(Eeprom_timezone_address, timezone);
        NTP_server = ntp;
        Timezone = timezone.toInt();
        NTPClient new_client(ntpUDP, NTP_server.c_str(), Timezone * 60 * 60);
        timeClient.end();
        timeClient = new_client;
        timeClient.begin();
        Serial.println(F("New NTP settings saved."));
        update_NTP_time();
    }
    else if (req.indexOf("/reboot") >= 0)
    {
        ESP.restart();
    }
    else if (req.indexOf("/title?") >= 0)
    {
        Eeprom_title = get_middle_string(req, "str=", " HTTP/1.1");
        Eeprom_title = change_plus_to_space(Eeprom_title);
        Eeprom_title = change_browser_code_to_symbol(Eeprom_title);
        Serial.println("New title: " + Eeprom_title);
        save_to_flash(Eeprom_title_address, Eeprom_title);
        Serial.println(F("New title string saved."));
    }
    else if (req.indexOf("/driver") >= 0)
    {
        String save_index = get_middle_string(req, "driver", "?");
        String pin = get_middle_string(req, "pin=", "&endp");
        String frequency = get_middle_string(req, "frequency=", "&endf");
        String resolution = get_middle_string(req, "resolution=", "&endr");
        String smooth = get_middle_string(req, "smooth=", "&ends");
        String max_power = get_middle_string(req, "max_power=", "&endm");
        String color_temp = get_middle_string(req, "color_temp=", "&endc");
        String led_off = get_middle_string(req, "led_off=", "&endl1");
        String led_max = get_middle_string(req, "led_max=", "&endl2");
        String nonlin = get_middle_string(req, "nonlin=", " HTTP/1.1");
        Serial.print("Driver index: " + save_index);
        Serial.print(" pin: " + pin);
        Serial.print(" frequency: " + frequency);
        Serial.print(" resolution: " + resolution);
        Serial.print(" smooth: " + smooth);
        Serial.print(" max_power: " + max_power);
        Serial.print(" color_temp: " + color_temp);
        Serial.print(" led_off: " + led_off);
        Serial.print(" led_max: " + led_max);
        Serial.println(" nonlin: " + nonlin);
        std::vector<Light_driver> &drivers = Light_system_ptr->get_drivers();
        drivers.at(save_index.toInt())
            .reinit(
                PWM(pin.toInt(), frequency.toInt(), resolution.toInt(), smooth.toInt()), max_power.toInt(), color_temp.toInt(),
                led_off.toInt(), led_max.toInt(), nonlin.toInt());
        m_drivers_not_saved = true;
    }
    else if (req.indexOf("/add_drv") >= 0)
    {
        String add_index = get_middle_string(req, "add_drv", "&");
        Serial.println("Add index: " + add_index);
        const int i = add_index.toInt();
        if (i < 10)
        {
            std::vector<Light_driver> &drivers = Light_system_ptr->get_drivers();
            drivers.insert(drivers.cbegin() + i, Light_driver(PWM(), 0, RED_COLOR_TEMP, 0, 100, 0));
            m_drivers_not_saved = true;
        }
        else
        {
            Serial.println(F("Driver limit exceeded!"));
        }
    }
    else if (req.indexOf("/del_drv") >= 0)
    {
        String del_index = get_middle_string(req, "del_drv", "&");
        Serial.println("Del index: " + del_index);
        const int i = del_index.toInt();
        const int drivers_count = Light_system_ptr->get_drivers_count();
        if (drivers_count > 0 && i < drivers_count && i > 0)
        {
            std::vector<Light_driver> &drivers = Light_system_ptr->get_drivers();
            drivers.erase(drivers.cbegin() + i);
            m_drivers_not_saved = true;
        }
        else
        {
            Serial.println(F("Cannot delete driver."));
        }
    }
    else if (req.indexOf("/save_drv") >= 0)
    {
        Serial.println(F("Save drivers to FLASH."));
        digitalWrite(LED_PIN, LOW);
        delay(200);
        save_to_flash(Eeprom_driver_address, Light_system_ptr->get_drivers());
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        ESP.wdtFeed();
        delay(200);
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        m_drivers_not_saved = false;
        ESP.restart();
    }
    else
    {
        Serial.println(F("Default request"));
    }

    if (m_client_ptr->available())
    {
        // byte by byte is not very efficient
        m_client_ptr->read();
    }
    return 0;
}

int Web_Page::web_head()
{
    m_client_ptr->println(F("HTTP/1.1 200 OK"));
    m_client_ptr->println(F("Content-type:text/html"));
    m_client_ptr->println(F("Connection: close"));
    m_client_ptr->println();
    m_client_ptr->println(F("<!DOCTYPE html><html>"));
    m_client_ptr->println(F("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"));
    m_client_ptr->println(F("<title>Aquarium</title>"));
    m_client_ptr->println(F("<link rel=\"icon\" href=\"data:,\">"));
    // CSS to style the on/off buttons
    // Feel free to change the background-color and font-size attributes to fit your preferences
    m_client_ptr->println(F("<style>html {background-color: #27292f; color: #DCDCDC; font-family: Helvetica; display: "
                            "inline-block; margin: 0px auto; text-align: center;}"));
    m_client_ptr->println(
        F(".button {background-color: #195B6A; border: none; color: white; padding: 16px 40px; text-decoration: "
          "none; font-size: 30px; margin: 2px; cursor: pointer;}"));
    m_client_ptr->println(F(".button2 {background-color: #77878A;}"));
    m_client_ptr->println(
        F(".button3 {background-color: #195B6A; border: none; color: white; padding: 6px 40px; text-decoration: "
          "none; font-size: 16px; margin: 2px; cursor: pointer;}"));
    m_client_ptr->println(F(".input {background-color: #b6b6b6; color: #220e0e; font-size: 20px;}"));
    m_client_ptr->println(F(".center {margin-left: auto; margin-right: auto;}</style>"));
    // script
    m_client_ptr->println(F("<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>"));
    m_client_ptr->println(F("<script type=\"text/javascript\">"));
    // show range value
    m_client_ptr->println(F("function showVal(newVal){"));
    m_client_ptr->println(F(" document.getElementById(\"value\").textContent = newVal;};"));
    // chart
    m_client_ptr->println(F("google.charts.load('current', {'packages':['corechart']});"));
    m_client_ptr->println(F("google.charts.setOnLoadCallback(drawChart);"));
    // chart scale
    m_client_ptr->println(F("var max_scale;"));
    m_client_ptr->println(F("function setScale(newScale){"));
    m_client_ptr->println(F("max_scale = newScale;"));
    m_client_ptr->println(F("drawChart();};"));

    m_client_ptr->println(F("function drawChart() {"));
    m_client_ptr->println(F("var data = google.visualization.arrayToDataTable(["));
    // ['Time', 'Summary W', 'Rnd W', 'Color temp K', 'Driver0 xW, %', 'Driver1 yW, %', etc.],
    m_client_ptr->print(F("['Time', 'Summary, W', 'Rnd, W', 'Color temp, 10K'"));
    for (int i = 0; i < Light_system_ptr->get_drivers_count(); i++)
    {
        m_client_ptr->print(", 'Driver" + String(i) + " " + String(Light_system_ptr->get_driver_max_power_w(i)) + "W, %'");
    }
    m_client_ptr->println(" ],");
    for (size_t set_num = 0; set_num < Light_settings.size(); set_num++)
    {
        m_client_ptr->print("[  " + String(Light_settings.at(set_num).m_time.get_hour_float()));               // Time
        m_client_ptr->print(", " + String(Light_system_ptr->get_summary_power_w(Light_settings.at(set_num)))); // Summary, W
        m_client_ptr->print(", " + String(Light_system_ptr->get_summary_power_w(Random_light_settings.at(set_num)))); // Rnd W
        m_client_ptr->print(", " + String(Light_settings.at(set_num).m_color_temp / 10)); // Color temp, 10K

        for (int drv_num = 0; drv_num < Light_system_ptr->get_drivers_count(); drv_num++)
        {
            m_client_ptr->print(
                ", " + String(Light_system_ptr->get_driver_current(Light_settings.at(set_num), drv_num))); // Drivern
                                                                                                           // xW%
        }

        if (set_num != Light_settings.size() - 1)
        {
            m_client_ptr->println(F(" ],"));
        }
        else
        {
            m_client_ptr->println(F(" ]"));
        }
    }
    m_client_ptr->println(F("]);"));

    // chart options
    m_client_ptr->println(F("var myOptions = {"));
    m_client_ptr->println(F("backgroundColor: '#aab8bb',"));
    m_client_ptr->println(F("vAxis:{"));
    m_client_ptr->println(F("viewWindowMode:'explicit',"));
    m_client_ptr->println(F("viewWindow: {"));
    m_client_ptr->println(F("max:max_scale,"));
    m_client_ptr->println(F("min:0}}"));
    m_client_ptr->println(F("};"));

    m_client_ptr->println(F("var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));"));
    m_client_ptr->println(F("chart.draw(data, myOptions);}"));

    m_client_ptr->println(F("function show_password(){"));
    m_client_ptr->println(F("var x = document.getElementById(\"passwd\");"));
    m_client_ptr->println(F("if (x.type===\"password\") {x.type=\"text\";} else {x.type=\"password\";}}"));

    m_client_ptr->println(F("</script>"));
    m_client_ptr->println(F("</head>"));
    return 0;
}
int Web_Page::web_body()
{
    m_client_ptr->println(F("<body>"));
    if (!m_wifi_settings)
    {
        main_web();
    }
    else
    {
        settings_web();
    }
    m_client_ptr->println(F("</body></html>"));
    m_client_ptr->println();
    return 0;
}
int Web_Page::main_web()
{
    // Title
    m_client_ptr->println("<h1>" + Eeprom_title + "</h1>");

    // Time
    m_client_ptr->println("<p>Current time: " + timeClient.getFormattedTime() + "</p>");
    m_client_ptr->println("<p>Uptime: " + uptime() + "</p>");

    // Update button
    m_client_ptr->println(F("<p><a href=\"/\"><button class=\"button\">Update</button></a></p>"));

    // Direct Control button
    if (Direct_Control_state)
    {
        m_client_ptr->println(F("<p><a href=\"/control/off\"><button class=\"button\">Direct Control</button></a></p>"));
    }
    else
    {
        m_client_ptr->println(F("<p><a href=\"/control/on\"><button class=\"button button2\">Direct Control</button></a></p>"));
    }

    // Points
    if (!Direct_Control_state)
    {
        m_client_ptr->println(
            "<p>Current point: " + Light_system_ptr->get_current_point(Random_light_settings, timeClient).print() + " </p>");
        m_client_ptr->println(
            "<p>Next point: " + Light_system_ptr->get_next_point(Random_light_settings, timeClient).print() + " </p>");
    }

    // Direct input forms
    if (Direct_Control_state)
    {
        m_client_ptr->println(F("<p><form action=\"/control\">"));
        m_client_ptr->println(
            "<input type=\"range\" class=\"input\" name=\"power\" min=\"0\" max=\"100\" value=\"" + String(Direct_Control_power)
            + "\" oninput=\"showVal(this.value)\" onchange=\"showVal(this.value)\">");
        m_client_ptr->println(
            "<input type=\"range\" class=\"input\" name=\"endp_color\" min=\"3000\" max=\"6500\" value=\""
            + String(Direct_Control_color) + "\" oninput=\"showVal(this.value)\" onchange=\"showVal(this.value)\">");
        m_client_ptr->println("<input type=\"submit\" class=\"button3\" value=\"Send\"></form></p>");
        m_client_ptr->println(
            "<p>Current power: " + String(Direct_Control_power) + "% Current color:  " + String(Direct_Control_color)
            + "K</p>");
    }

    // Print current and watts for each driver
    for (int i = 0; i < Light_system_ptr->get_drivers_count(); i++)
    {
        m_client_ptr->println(
            "<p>Driver " + String(i) + " current: " + String(Light_system_ptr->get_driver_current(i))
            + "% power: " + String(Light_system_ptr->get_driver_power_w(i))
            + "W max power: " + String(Light_system_ptr->get_driver_max_power_w(i)) + "W</p>");
    }
    // summary power
    m_client_ptr->print(
        "<p>Current power: "
        + String(Light_system_ptr->get_summary_power_w(Light_system_ptr->get_current_point(Random_light_settings, timeClient)))
        + "W");
    const int day_power_wh = Light_system_ptr->get_day_power_wh(Light_settings);
    m_client_ptr->print(" day power: " + String(day_power_wh) + "W*h");
    m_client_ptr->println(" month power: " + String(day_power_wh * 30LL / 1000LL) + "kW*h</p>");

    // Diagramms
    m_client_ptr->println(F("<h2>Charts</h2>"));
    m_client_ptr->println(F("<div id=\"curve_chart\" class=\"center\" style=\"width: 1600px; height: 700px\"></div>"));
    m_client_ptr->println(
        F("<p>Y scale: <input type=\"range\" class=\"input\" id=\"scale\" min=\"10\" max=\"1000\" value=\"1000\" "
          "oninput=\"setScale(this.value)\" onchange=\"setScale(this.value)\"></p>"));

    // Settings
    m_client_ptr->println(F("<h2>Settings</h2>"));
    // Sliders Value
    m_client_ptr->println(F("<p>Current input value: <output id=\"value\"></output></p>"));
    // Table's cell's names
    m_client_ptr->println(F("<table class=\"center\" width=\"930\"><tr><th width=\"80\">Hour:</th>"));
    m_client_ptr->println(F("<th width=\"80\">Minute:</th><th width=\"140\">Power:</th><th width=\"150\">Color:</th>"));
    m_client_ptr->println(F("<th width=\"80\">Random:</th><th></th></tr></table>"));

    // Print settings
    m_client_ptr->println(F("<table class=\"center\">"));
    int i = 0;
    for (const Light_setting &s : Light_settings)
    {
        m_client_ptr->println("<tr><th><form action=\"/setting" + String(i) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"hour\" placeholder=\"Hour\" min=\"0\" max=\"23\" value=\""
            + String(s.m_time.get_hour()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endh_minute\" placeholder=\"Minute\" min=\"0\" max=\"59\" value=\""
            + String(s.m_time.get_min()) + "\">");
        m_client_ptr->println(
            "<input type=\"range\" class=\"input\" name=\"endm_power\" min=\"0\" max=\"100\" value=\""
            + String(s.m_power_percent) + "\" oninput=\"showVal(this.value)\" onchange=\"showVal(this.value)\">");
        m_client_ptr->println(String(Light_system_ptr->get_summary_power_w(s)) + "W");
        m_client_ptr->println(
            "<input type=\"range\" class=\"input\" name=\"endp_color\" min=\"3000\" max=\"6500\" value=\""
            + String(s.m_color_temp) + "\" oninput=\"showVal(this.value)\" onchange=\"showVal(this.value)\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endc_random\" placeholder=\"In %\" min=\"0\" max=\"100\" value=\""
            + String(s.m_random) + "\">");
        m_client_ptr->println(F("<input type=\"submit\" class=\"button3\" value=\"Send\"></form></th>"));
        m_client_ptr->println("<th><a href=\"/add_set" + String(i) + "&\"><button class=\"button3\">Add</button></a>");
        m_client_ptr->println("<a href=\"/del_set" + String(i) + "&\"><button class=\"button3\">Delete</button></a></th></tr>");
        i++;
    }
    m_client_ptr->println(F("</table>"));
    if (m_settings_not_saved)
    {
        m_client_ptr->println(F("<p style=\"color:red;\">! Settings not saved !</p>"));
    }
    m_client_ptr->println(F("<p><a href=\"/save_set\"><button class=\"button\">Save to FLASH</button></a></p>"));

    m_client_ptr->println(F("<p><a href=\"/wifi_settings\"><button class=\"button button2\">Settings</button></a></p>"));
    return 0;
}

int Web_Page::settings_web()
{
    // Title setting
    m_client_ptr->println(F("<p><form action=\"/title\">"));
    m_client_ptr->println("<input type=\"text\" class=\"input\" name=\"str\" maxlength=\"45\" value=\"" + Eeprom_title + "\">");
    m_client_ptr->println(F("<input type=\"submit\" class=\"button3\" value=\"Save\"></form></p>"));
    // Time
    m_client_ptr->println("<p>Current time: " + timeClient.getFormattedTime() + "</p>");
    m_client_ptr->println("<p>Uptime: " + uptime() + "</p>");
    // Update button
    m_client_ptr->println(F("<p><a href=\"/\"><button class=\"button\">Update</button></a></p>"));
    m_client_ptr->println(F("<p><a href=\"/reboot\"><button class=\"button\">Reboot</button></a></p>"));
    // Cycle time
    m_client_ptr->println("<p>Average cycle time: " + String(Loop_time_us) + " us</p>");
    m_client_ptr->println("<p>Max cycle time per hour: " + String(Max_loop_time_us) + " us</p>");
    // Memory size
    m_client_ptr->println("<p>Free Stack: " + String(ESP.getFreeContStack()) + "</p>");
    m_client_ptr->println("<p>Free Heap: " + String(ESP.getFreeHeap()) + "</p>");
    m_client_ptr->println("<p>Program Free Space: " + String(ESP.getFreeSketchSpace()) + "</p>");
    m_client_ptr->println(
        "<p>Program Size: " + String(ESP.getSketchSize()) + " ("
        + String(ESP.getSketchSize() * 100LL / (ESP.getFreeSketchSpace() + ESP.getSketchSize())) + " %)</p>");
    m_client_ptr->println("<p>Flash Size: " + String(ESP.getFlashChipSize()) + "</p>");
    // Speed
    m_client_ptr->println("<p>CPU Speed: " + String(ESP.getCpuFreqMHz()) + " MHz</p>");
    m_client_ptr->println("<p>Flash Speed: " + String(ESP.getFlashChipSpeed() / 1000000) + " MHz</p>");
    // Chip voltage (ADC is off and ADC_MODE(ADC_VCC); there)
    m_client_ptr->println("<p>Chip Voltage: " + String(ESP.getVcc()) + " mV</p>");
    m_client_ptr->println("<p>WiFi Signal: " + String(WiFi.RSSI()) + " dBm</p><br>");

    // Light Drivers settings
    m_client_ptr->println(F("<h3>Drivers settings</h3>"));
    m_client_ptr->println(F("<table class=\"center\" width=\"1380\"><tr><th width=\"100\">PWM Pin:</th>"));
    m_client_ptr->println(F("<th width=\"100\">Frequency:</th><th width=\"120\">Resolution:</th><th "
                            "width=\"100\">Smooth:</th><th width=\"140\">Max power:</th>"));
    m_client_ptr->println(F("<th width=\"100\">Color temp:</th><th width=\"100\">Led off:</th><th width=\"100\">Led "
                            "max:</th><th width=\"100\">Nonlinearity:</th><th></th></tr></table>"));
    m_client_ptr->println(F("<table class=\"center\">"));
    for (int i = 0; i < Light_system_ptr->get_drivers_count(); i++)
    {
        // PWM(DRIVER1_240W_PWM_PIN, 3000, 1000), 200, WHITE_COLOR_TEMP, 73, 100)
        m_client_ptr->println("<tr><th><form action=\"/driver" + String(i) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"pin\" placeholder=\"Pin number\" min=\"0\" max=\"16\" value=\""
            + String(Light_system_ptr->get_driver(i).get_pwm().get_pin()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endp_frequency\" placeholder=\"Frequency, Hz\" min=\"0\" "
            "max=\"10000\" "
            "value=\""
            + String(Light_system_ptr->get_driver(i).get_pwm().get_frequency()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endf_resolution\" placeholder=\"Resolution\" min=\"0\" "
            "max=\"10000\" "
            "value=\""
            + String(Light_system_ptr->get_driver(i).get_pwm().get_resolution()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endr_smooth\" placeholder=\"Smooth, ms\" min=\"0\" max=\"1000\" "
            "value=\""
            + String(Light_system_ptr->get_driver(i).get_pwm().get_smooth_ms()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"ends_max_power\" placeholder=\"Max power, W\" min=\"0\" "
            "max=\"10000\" "
            "value=\""
            + String(Light_system_ptr->get_driver(i).get_max_power_w()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endm_color_temp\" placeholder=\"Color temperature, K\" min=\"0\" "
            "max=\"20000\" value=\""
            + String(Light_system_ptr->get_driver(i).get_color_temp()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endc_led_off\" placeholder=\"Led off current, %\" min=\"0\" "
            "max=\"100\" "
            "value=\""
            + String(Light_system_ptr->get_driver(i).get_setting_led_off()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endl1_led_max\" placeholder=\"Led max current, %\" min=\"0\" "
            "max=\"100\" "
            "value=\""
            + String(Light_system_ptr->get_driver(i).get_setting_led_max()) + "\">");
        m_client_ptr->println(
            "<input type=\"number\" class=\"input\" name=\"endl2_nonlin\" placeholder=\"Nonlinearity, %\" min=\"0\" "
            "max=\"1000\" "
            "value=\""
            + String(Light_system_ptr->get_driver(i).get_nonlinearity()) + "\">");
        m_client_ptr->println(F("<input type=\"submit\" class=\"button3\" value=\"Send\"></form></th>"));
        m_client_ptr->println("<th><a href=\"/add_drv" + String(i) + "&\"><button class=\"button3\">Add</button></a>");
        m_client_ptr->println("<a href=\"/del_drv" + String(i) + "&\"><button class=\"button3\">Delete</button></a></th></tr>");
    }
    m_client_ptr->println(F("</table>"));
    if (m_drivers_not_saved)
    {
        m_client_ptr->println(F("<p style=\"color:red;\">! Drivers not saved !</p>"));
    }
    m_client_ptr->println(F("<p><a href=\"/save_drv\"><button class=\"button\">Save and Reboot</button></a></p>"));

    // WiFi settings
    m_client_ptr->println(F("<h3>WIFI settings</h3>"));
    m_client_ptr->println(F("<p><form action=\"/wifi\"><table class=\"center\"><tr><th>SSID:</th>"));
    m_client_ptr->println(
        "<th><input type=\"text\" class=\"input\" name=\"ssid\" placeholder=\"SSID\" value=\"" + Eeprom_ssid
        + "\"></th></tr><tr><th>Password:</th>");
    m_client_ptr->println(
        F("<th><input type=\"password\" class=\"input\" name=\"ends_pa\" id=\"passwd\" autocomplete=\"off\" value=\"\"></th>"));
    m_client_ptr->println(F("<th><input type=\"checkbox\" onclick=\"show_password()\"> Show</th></tr></table>"));
    m_client_ptr->println(F("<input type=\"submit\" class=\"button\" value=\"Save and Connect\"></form></p>"));
    m_client_ptr->println(F("<br><br>"));

    // NTP settings
    m_client_ptr->println(F("<h3>NTP settings</h3>"));
    m_client_ptr->println(F("<p><form action=\"/ntp\"><table class=\"center\"><tr><th>Ntp server:</th>"));
    m_client_ptr->println(
        "<th><input type=\"text\" class=\"input\" name=\"server\" placeholder=\"Ntp server\" value=\"" + NTP_server
        + "\"></th></tr><tr><th>Time zone:</th>");
    m_client_ptr->println(
        "<th><input type=\"number\" class=\"input\" name=\"ends_tz\" placeholder=\"Hours\" min=\"-24\" max=\"24\" value=\""
        + String(Timezone) + "\"></th></tr></table>");
    m_client_ptr->println(F("<input type=\"submit\" class=\"button\" value=\"Save Time Settings\"></form></p>"));
    m_client_ptr->println(F("<br><br>"));
    m_client_ptr->println(F("<p><a href=\"/wifi_settings\"><button class=\"button\">Close Settings</button></a></p>"));
    return 0;
}