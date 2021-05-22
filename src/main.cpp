
#include "helper.h"
#include "main.h"
#include "Light.h"
#include "password.h"
#include "PWM.h"

#define LED_PIN 2 // pin 2
#define WIFI_PIN 5
#define MAIN_PWM_PIN 14
#define SECOND_PWM_PIN 12
//#define WHITE_PIN 13
//#define RED_PIN 15

// http://wikihandbk.com/wiki/ESP32:%D0%9F%D1%80%D0%B8%D0%BC%D0%B5%D1%80%D1%8B/%D0%92%D0%B5%D0%B1-%D1%81%D0%B5%D1%80%D0%B2%D0%B5%D1%80_%D0%BD%D0%B0_%D0%B1%D0%B0%D0%B7%D0%B5_ESP32:_%D1%83%D0%B4%D0%B0%D0%BB%D0%B5%D0%BD%D0%BD%D0%BE%D0%B5_%D1%83%D0%BF%D1%80%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%B8%D0%B5_%D1%81%D0%B5%D1%80%D0%B2%D0%BE%D0%BF%D1%80%D0%B8%D0%B2%D0%BE%D0%B4%D0%BE%D0%BC

const char *Default_ap_ssid = "ESP8266_AP";
const char *Default_ap_password = "AP_password";

String NTP_server = "pool.ntp.org";
// +3 hour time zone
long Timezone = 3; 

//////////////// function declaration ////////////
void update_NTP_time();
String uptime();
String get_middle_string(const String &str, const String &before, const String &after);
bool save_to_flash(int address, String &str);
bool read_from_flash(int address, String &str);
bool new_day();
bool wifi_init();
bool wifi_connect_to_station();
///////////////// variables ////////////////
WiFiServer server(80);
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org", Timezone * 60 * 60);
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

String header;
bool Direct_Control_state = false;
int Direct_Control_power = 0;
int Direct_Control_color = WHITE_COLOR_TEMP;

unsigned long updated_miliseconds;

Light Light_system;
std::vector<Light_setting> Light_settings;
std::vector<Light_setting> Random_light_settings;
int timer1 = 0;

bool Wifi_settings = false;
String Eeprom_ssid;
String Eeprom_password;
// size_t - size of vector<Light_setting>
// sizeof(Light_setting) * 64 - is 64 settings max must be saved/readed
// 100 - wifi pass/ssid 
// 100 - reserve
const int Eeprom_ssid_address = sizeof(size_t) + sizeof(Light_setting) * 64;
const int Eeprom_password_address = Eeprom_ssid_address + 50;
const int Eeprom_ntp_address = Eeprom_password_address + 50;
const int Eeprom_timezone_address = Eeprom_ntp_address + 50;

const int Eeprom_size = Eeprom_timezone_address + 50;


////////////////// setup ////////////////////

void setup(void){

  // GPIO:
  pinMode(LED_PIN, OUTPUT);
  pinMode(MAIN_PWM_PIN, OUTPUT);
  pinMode(SECOND_PWM_PIN, OUTPUT);
  pinMode(WIFI_PIN, INPUT_PULLUP);
    
  digitalWrite(LED_PIN, HIGH); // led off
  digitalWrite(MAIN_PWM_PIN, HIGH); // off
  digitalWrite(SECOND_PWM_PIN, HIGH); // off

  delay(200);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Setup started");
  // EEPROM
  EEPROM.begin(Eeprom_size);

  read_from_flash(Eeprom_ssid_address, Eeprom_ssid);
  read_from_flash(Eeprom_password_address, Eeprom_password);
  if(Eeprom_ssid.length() == 0){
    Serial.println("EEPROM WiFi settings empty. Set default settings");
    Eeprom_ssid = String(Default_ssid);
    Eeprom_password = String(Default_password);
    save_to_flash(Eeprom_ssid_address, Eeprom_ssid);
    save_to_flash(Eeprom_password_address, Eeprom_password);
  }
  Serial.print("Readed EEPROM ssid: " + Eeprom_ssid);
  Serial.println(" password: " + Eeprom_password);

  // connect to wifi station or create AP
  wifi_init();

  server.begin();
  Serial.println("HTTP server started");

  // NTP
  String eeprom_ntp;
  String eeprom_timezone;
  read_from_flash(Eeprom_ntp_address, eeprom_ntp);
  read_from_flash(Eeprom_timezone_address, eeprom_timezone);
  if(eeprom_ntp.length() && eeprom_timezone.length()){
    NTP_server = eeprom_ntp;
    Timezone = eeprom_timezone.toInt();
    NTPClient new_client(ntpUDP, NTP_server.c_str(), Timezone * 60 * 60);
    timeClient = new_client;
  }
  timeClient.begin();
  Serial.println("NTP client started");
  Serial.print("NTP Server: " + eeprom_ntp);
  Serial.println(" timezone: " + eeprom_timezone + "h");
  update_NTP_time();

  randomSeed(timeClient.getEpochTime());

  // light settings
  Light_driver main_driver(PWM(MAIN_PWM_PIN, 5000, 1000), 28, WHITE_COLOR_TEMP, 20);
  Light_driver second_driver(PWM(SECOND_PWM_PIN, 5000, 1000), 12, RED_COLOR_TEMP, 30);

  Light_system.add_light_driver(main_driver);
  Light_system.add_light_driver(second_driver);

  load_from_flash(Light_settings);
  Serial.println("Settings has been readed from EEPROM.");

  if(Light_settings.empty()){
    Serial.println("EEPROM is empty. Creating first settings.");
    Light_settings.clear();
    Light_settings.push_back(Light_setting(0, 0, 3000));
    Light_settings.push_back(Light_setting(8, 0, 3000));
    Light_settings.push_back(Light_setting(MyTime(8, 30), 10, 3000));
    Light_settings.push_back(Light_setting(9, 15, 4000));
    Light_settings.push_back(Light_setting(11, 60, 6500));
    Light_settings.push_back(Light_setting(12, 100, 6500));
    Light_settings.push_back(Light_setting(14, 50, 6500));
    Light_settings.push_back(Light_setting(16, 45, 4500));
    Light_settings.push_back(Light_setting(18, 30, 3500));
    Light_settings.push_back(Light_setting(20, 20, 3000));
    Light_settings.push_back(Light_setting(22, 5, 3000));
    Light_settings.push_back(Light_setting(23, 0, 3000));
    save_to_flash(Light_settings);
  }
  update_random_settings(Light_settings, Random_light_settings);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA configured.");

  // end setup
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Setup is over.");
}

//////////////////// loop //////////////////////////

void loop(void){
  static unsigned long loop_time;
  const unsigned long loop_start_time = millis();

  // Handle update firmware via WIFI
  ArduinoOTA.handle();

  // every hour
  if(millis() - updated_miliseconds >= 60 * 60 * 1000){
    wifi_connect_to_station();
    update_NTP_time();

    updated_miliseconds = millis();
  }

  // randomize power every day
  if(new_day()){
    update_random_settings(Light_settings, Random_light_settings);
  }

  std::sort(Light_settings.begin(), Light_settings.end());
  std::sort(Random_light_settings.begin(), Random_light_settings.end());

  // light
  if(Direct_Control_state){
    Light_system.set_light(Direct_Control_power, Direct_Control_color);
    digitalWrite(LED_PIN, LOW);
  }else{
    Light_system.set_light(Random_light_settings, timeClient);
    digitalWrite(LED_PIN, HIGH);
  }

  // WEB Server
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /control/on") >= 0) {
              Serial.println("Direct Control on");
              Direct_Control_state = true;
              Light_setting curr_s = Light_system.get_current_point(Random_light_settings, timeClient);
              Direct_Control_power = curr_s.m_power_percent;
              Direct_Control_color = curr_s.m_color_temp;
            } else if (header.indexOf("GET /control/off") >= 0) {
              Serial.println("Direct Control off");
              Direct_Control_state = false;
              Direct_Control_power = 0;
            } else if (header.indexOf("GET /add") >= 0) {
              String add_index = get_middle_string(header, "add", "&");
              Serial.println("Add index: " + add_index);
              if(add_index.toInt() < 64){
                MyTime next_added_time(Light_settings[add_index.toInt()].m_time.get_hour(), Light_settings[add_index.toInt()].m_time.get_min() + 1);
                Light_settings.push_back(Light_setting(next_added_time));
                std::sort(Light_settings.begin(), Light_settings.end());
                update_random_settings(Light_settings, Random_light_settings);
              }else{
                Serial.println("Settings limit exceeded!");
              }
            } else if (header.indexOf("GET /del") >= 0) {
              String del_index = get_middle_string(header, "del", "&");
              if(Light_settings.size() > 0){
                Serial.println("Del index: " + del_index);
                Light_settings.erase( Light_settings.begin() + del_index.toInt() );
                std::sort(Light_settings.begin(), Light_settings.end());
                update_random_settings(Light_settings, Random_light_settings);
              }else{
                Serial.println("Cannot delete last element.");
              }
            } else if (header.indexOf("GET /setting") >= 0 ) {
              String save_index = get_middle_string(header, "setting", "?");
              String hour = get_middle_string(header, "hour=", "&endh");
              String minute = get_middle_string(header, "minute=", "&endm");
              String power = get_middle_string(header, "power=", "&endp");
              String color = get_middle_string(header, "color=", "&endc");
              String random = get_middle_string(header, "random=", " HTTP/1.1");
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
              update_random_settings(Light_settings, Random_light_settings);
            }else if (header.indexOf("GET /control?") >= 0 ) {
              String power = get_middle_string(header, "power=", "&endp");
              String color = get_middle_string(header, "color=", " HTTP/1.1");
              Serial.print("Direct power: " + power);
              Serial.println(" Direct color: " + color);
              Direct_Control_color = color.toInt();
              Direct_Control_power = power.toInt();
            }else if (header.indexOf("GET /save") >= 0 ) {
              Serial.println("Save settings to FLASH.");
              const int led_state = digitalRead(LED_PIN);
              digitalWrite(LED_PIN, LOW);
              delay(200);
              save_to_flash(Light_settings);
              digitalWrite(LED_PIN, HIGH);
              delay(200);
              digitalWrite(LED_PIN, LOW);
              delay(200);
              digitalWrite(LED_PIN, HIGH);
              delay(200);
              digitalWrite(LED_PIN, led_state);
            }else if (header.indexOf("GET /wifi_settings") >= 0 ) {
              if(Wifi_settings){
                Wifi_settings = false;
              }else{
                Wifi_settings = true;
              }
            }else if (header.indexOf("GET /wifi?") >= 0 ) {
              String ssid = get_middle_string(header, "ssid=", "&ends");
              String password = get_middle_string(header, "pa=", " HTTP/1.1");
              Serial.println("New SSID: " + ssid + " New Password: " + password);
              save_to_flash(Eeprom_ssid_address, ssid);
              save_to_flash(Eeprom_password_address, password);
              Serial.println("New WiFi settings saved.");
              Serial.println("Rebooting...");
              ESP.restart();
            }else if (header.indexOf("GET /ntp?") >= 0 ) {
              String ntp = get_middle_string(header, "server=", "&ends");
              String timezone = get_middle_string(header, "tz=", " HTTP/1.1");
              Serial.println("New NTP server: " + ntp + " Timezone: " + timezone);
              save_to_flash(Eeprom_ntp_address, ntp);
              save_to_flash(Eeprom_timezone_address, timezone);
              NTP_server = ntp;
              Timezone = timezone.toInt();
              NTPClient new_client(ntpUDP, NTP_server.c_str(), Timezone * 60 * 60);
              timeClient.end();
              timeClient = new_client;
              timeClient.begin();
              Serial.println("New NTP settings saved.");
              update_NTP_time();
            }else if (header.indexOf("GET /reboot") >= 0 ) {
              ESP.restart();
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html {background-color: #27292f; color: #DCDCDC; font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button {background-color: #195B6A; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}");
            client.println(".button3 {background-color: #195B6A; border: none; color: white; padding: 6px 40px; text-decoration: none; font-size: 16px; margin: 2px; cursor: pointer;}");
            client.println(".input {background-color: #b6b6b6; color: #220e0e; font-size: 20px;}");
            client.println(".center {margin-left: auto; margin-right: auto;}</style>");
            // script
            //client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
            client.println("</head>");
            
            // Web Page Heading
            client.println("<body><h1>Aquarium LED driver</h1>");

            // Time
            client.println("<p>Current time: " + timeClient.getFormattedTime() + "</p>");
            client.println("<p>Uptime: " + uptime() + "</p>");

            // Update button
            client.println("<p><a href=\"/\"><button class=\"button\">Update</button></a></p>");

            if(!Wifi_settings){
              // Direct Control button
              if (Direct_Control_state) {
                client.println("<p><a href=\"/control/off\"><button class=\"button\">Direct Control</button></a></p>");
              } else {
                client.println("<p><a href=\"/control/on\"><button class=\"button button2\">Direct Control</button></a></p>");
              } 

              // Points
              if (!Direct_Control_state) {
                client.println("<p>Current point: "+ Light_system.get_current_point(Random_light_settings, timeClient).print() + " </p>");
                client.println("<p>Next point: "+ Light_system.get_next_point(Random_light_settings, timeClient).print() + " </p>");
              }

              // Direct input forms
              if (Direct_Control_state) {
                client.println("<p><form action=\"/control\">");
                client.println("<input type=\"range\" class=\"input\" name=\"power\" min=\"0\" max=\"100\" value=\"" + String(Direct_Control_power) + "\">");
                client.println("<input type=\"range\" class=\"input\" name=\"endp_color\" min=\"3000\" max=\"6500\" value=\"" + String(Direct_Control_color) + "\">");
                client.println("<input type=\"submit\" class=\"button3\" value=\"Send\"></form></p>");
                client.println("<p>Current power: " + String(Direct_Control_power) + "% Current color:  " + String(Direct_Control_color) + "K</p>");
              }

              // Print current in drivers
              for(int i = 0; i < Light_system.get_drivers_count(); i++){
                client.println("<p>Driver " + String(i) + " current: " + String(Light_system.get_driver_current(i)) + "%</p>");
              }
              // summary power
              client.println("<p>Summary power: " + String(Light_system.get_summary_power_w(Light_system.get_current_point(Random_light_settings, timeClient))) + "W</p>");

              // Settings
              client.println("<h2>Settings</h2>");
              // Table's cell's names
              client.println("<table class=\"center\" width=\"930\"><tr><th width=\"80\">Hour:</th>");
              client.println("<th width=\"80\">Minute:</th><th width=\"140\">Power:</th><th width=\"150\">Color:</th>");
              client.println("<th width=\"80\">Random:</th><th></th></tr></table>");

              // Print settings
              client.println("<table class=\"center\">");
              int i = 0;
              for(const Light_setting s : Light_settings)
              {
                client.println("<tr><th><form action=\"/setting" + String(i) + "\">");
                client.println("<input type=\"number\" class=\"input\" name=\"hour\" placeholder=\"Hour\" min=\"0\" max=\"23\" value=\"" + String(s.m_time.get_hour()) + "\">");
                client.println("<input type=\"number\" class=\"input\" name=\"endh_minute\" placeholder=\"Minute\" min=\"0\" max=\"59\" value=\"" + String(s.m_time.get_min()) + "\">");
                client.println("<input type=\"range\" class=\"input\" name=\"endm_power\" min=\"0\" max=\"100\" value=\"" + String(s.m_power_percent) + "\">");
                client.println(String(Light_system.get_summary_power_w(s))+ "W");
                client.println("<input type=\"range\" class=\"input\" name=\"endp_color\" min=\"3000\" max=\"6500\" value=\"" + String(s.m_color_temp) + "\">");
                client.println("<input type=\"number\" class=\"input\" name=\"endc_random\" placeholder=\"In %\" min=\"0\" max=\"100\" value=\"" + String(s.m_random) + "\">");
                client.println("<input type=\"submit\" class=\"button3\" value=\"Send\"></form></th>");
                client.println("<th><a href=\"/add" + String(i) + "&\"><button class=\"button3\">Add</button></a>");
                client.println("<a href=\"/del" + String(i) + "&\"><button class=\"button3\">Delete</button></a></th></tr>");
                i++;
              }
              client.println("</table>");

              client.println("<p><a href=\"/save\"><button class=\"button\">Save to FLASH</button></a></p>");
              
              client.println("<p><a href=\"/wifi_settings\"><button class=\"button button2\">Settings</button></a></p>");
            }else{
              client.println("<p><a href=\"/reboot\"><button class=\"button\">Reboot</button></a></p>");
              // Memory size
              client.println("<p>Free Heap: " + String(ESP.getFreeHeap()) + "</p>");  
              client.println("<p>Average cycle time: " + String(loop_time) + " ms</p><br>");  
              // WiFi settings
              client.println("<p><form action=\"/wifi\"><table class=\"center\"><tr><th>SSID:</th>");
              client.println("<th><input type=\"text\" class=\"input\" name=\"ssid\" placeholder=\"SSID\" value=\"" + Eeprom_ssid + "\"></th></tr><tr><th>Password:</th>");
              client.println("<th><input type=\"text\" class=\"input\" name=\"ends_pa\" value=\"\"></th></tr></table>");
              client.println("<input type=\"submit\" class=\"button\" value=\"Save and connect\"></form></p>");
              client.println("<br><br>");
              client.println("<p><form action=\"/ntp\"><table class=\"center\"><tr><th>Ntp server:</th>");
              client.println("<th><input type=\"text\" class=\"input\" name=\"server\" placeholder=\"Ntp server\" value=\"" + NTP_server + "\"></th></tr><tr><th>Time zone:</th>");
              client.println("<th><input type=\"number\" class=\"input\" name=\"ends_tz\" placeholder=\"Hours\" min=\"-24\" max=\"24\" value=\"" + String(Timezone) +  "\"></th></tr></table>");
              client.println("<input type=\"submit\" class=\"button\" value=\"Save time settings\"></form></p>");
              client.println("<br><br>");
              client.println("<p><a href=\"/wifi_settings\"><button class=\"button\">Close Settings</button></a></p>");
            }

            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    client.flush();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  // calculate average loop time
  const unsigned long loop_end_time = millis();
  if(loop_end_time > loop_start_time){
    // loop time = (old_loop_time + new_loop_time) / 2
    loop_time = (loop_time + loop_end_time - loop_start_time) / 2;
  }
}

void update_NTP_time()
{
  Serial.println("NTP Time has been updated");
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
  if(days){
    ret += String(days) + "d:";
  }
  ret += String(hours) + "h:";
  ret += String(minutes) + "m:";
  ret += String(seconds) + "s";
  return ret;
}

String get_middle_string(const String &str, const String &before, const String &after)
{
  const int pos1 = str.indexOf(before) + before.length();
  const int pos2 = str.indexOf(after);
  return str.substring(pos1, pos2);
}

bool save_to_flash(int address, String &str)
{
  int len = static_cast<int>(str.length());
  if(len > 50 - static_cast<int>(sizeof(len))){
    str = str.substring(0, 50 - sizeof(len));
    len = 50 - sizeof(len);
    Serial.println("String to exeed max lenght to save in flash.");
    Serial.println("Saving: " + str);
    
  }
  EEPROM.put(address, len);
  for(int i = 0; i < len; i++){
    char symbol = str.charAt(i);
    EEPROM.put(address + sizeof(len) + sizeof(symbol) * i, symbol);
  }
  return EEPROM.commit();
}

bool read_from_flash(int address, String &str)
{
  str.clear();
  int len = 0;
  EEPROM.get(address, len);
  if(len > 50 - static_cast<int>(sizeof(len)) || len < 0)
  {
    return true;
  }
  for(int i = 0; i < len; i++){
    char symbol = ' ';
    EEPROM.get(address + sizeof(len) + sizeof(symbol) * i, symbol);
    str += String(symbol);
  }
  return str.isEmpty();
}

bool new_day()
{
  static int old_hour;
  const int hour = timeClient.getHours();
  if(hour != old_hour){
    old_hour = hour;
    if(hour == 0){
      return true;
    }
  }
  return false;
}

bool wifi_init()
{
  bool ret_val = false;
  int attempts = 0;
  if(digitalRead(WIFI_PIN))
  {
    // Connect to existing WiFi
    Serial.print("Connecting");
    WiFi.begin(Eeprom_ssid, Eeprom_password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(450);
      digitalWrite(LED_PIN, LOW);
      delay(50);
      digitalWrite(LED_PIN, HIGH);
      Serial.print(".");
      attempts++;
      if(attempts > 50){
        ret_val |= WiFi.disconnect(true);
        Serial.println("");
        Serial.println("WiFi cannot connect to SSID " + Eeprom_ssid + ". Switch to AP mode.");
        break;
      }
    }

    Serial.println("");
    if(WiFi.status() == WL_CONNECTED){
      ret_val |= WiFi.setAutoReconnect(true);
      Serial.println("Connected to " + Eeprom_ssid);
      Serial.println("IP address: " + WiFi.localIP().toString());
    }
  }

  // AP mode
  if(!digitalRead(WIFI_PIN) || attempts > 50){
    ret_val |= WiFi.softAPConfig(IPAddress(192, 168, 0, 1), IPAddress(192, 168, 0, 1), IPAddress(255, 255, 255, 0));
    ret_val |= WiFi.softAP(Default_ap_ssid, Default_ap_password, 1, 1); 
    Serial.println("WiFi AP created. SSID: " + String(Default_ap_ssid));
    Serial.println("AP password: " + String(Default_ap_password));
    Serial.println("IP address: " + WiFi.softAPIP().toString());
  }

  Serial.println("");

  return ret_val;
}

bool wifi_connect_to_station()
{
  bool ret_val = false;
  WiFiMode_t currentMode = WiFi.getMode();
  bool ap_is_enabled = ((currentMode & WIFI_AP) != 0);
  if(ap_is_enabled){
    ret_val |= WiFi.softAPdisconnect(true);
    delay(5);

    ret_val |= wifi_init();
  }
  return ret_val;
}