#include <Arduino.h>
#include <Watchy.h> //include the Watchy library
#include "bit_pusab12pt7b.h"
#include "bit_pusab6pt7b.h"
#include "bit_pusab3pt7b.h" //include any fonts you want to use
#include "icons.h"
#include "settings.h" //same file as the one from 7_SEG example

class MyFirstWatchFace : public Watchy{ //inherit and extend Watchy class
    public:
        MyFirstWatchFace(const watchySettings& s) : Watchy(s) {}
        void drawWatchFace(){ //override this method to customize how the watch face looks
            int xOffset = 10;

            display.fillScreen(DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);
            display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
            display.setFont(&bit_pusab12pt7b);
            
            // Time
            display.setCursor(10, xOffset+28);
            xOffset += 28;
            int hour;

            if(currentTime.Hour > 12) { 
                hour = currentTime.Hour - 12; 
            } else if(currentTime.Hour == 0) {
                hour = 12;
            } else {
                hour = currentTime.Hour;
            }

            display.print(hour);
            display.print(":");

            if(currentTime.Minute < 10){
                display.print("0");
            }  
            
            display.print(currentTime.Minute); 
            display.print(currentTime.Hour < 12 || currentTime.Hour == 24 ? " AM" : " PM"); 
            
            // Date
            display.setFont(&bit_pusab6pt7b);
            display.setCursor(10, xOffset+(8+14));
            xOffset += 8+14;

            String day = dayStr(currentTime.Wday);
            String date = day.substring(0, 3);
            date += " " + String(currentTime.Month) + "/";
            date += String(currentTime.Day) + " ";
            date += monthShortStr(currentTime.Month);
            
            date.toUpperCase();

            display.print(date);

            // Battery

            auto batteryIcon = bm_battery0;
            
            float VBAT = getBatteryVoltage();
            if(VBAT > 4.02){
                batteryIcon = bm_battery4;
            }
            else if(VBAT > 3.87){
                batteryIcon = bm_battery3;
            }
            else if(VBAT > 3.8){
                batteryIcon = bm_battery2;
            }
            else if(VBAT > 3.73){
                batteryIcon = bm_battery1;
            }

            int yOffset = 10;

            display.drawBitmap(yOffset, xOffset+10, batteryIcon, 80, 30, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
            xOffset += 10+30;
            yOffset += 80;
            
            // Wifi

            display.drawBitmap(yOffset+10, xOffset-30, WIFI_CONFIGURED ? bm_wifi3 : bm_wifi0, 31, 30, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
            yOffset += 31+10;

            // TODO: Alarm?

            // Steps

            display.setCursor(10, xOffset+(8+14));
            xOffset += 8+14;

            if (currentTime.Hour == 0 && currentTime.Minute == 0){
                sensor.resetStepCounter();
            }
            uint32_t stepCount = sensor.getCounter();
            display.print(commafy(stepCount) + " Steps");
            
            Serial.begin(115200);

            // Pull data from server
            JSONVar responseObject;

            if (connectWiFi()) {
                HTTPClient http;
                http.setConnectTimeout(3000); // 3 second max timeout
                String serverUrl = "https://"+ String(BACKEND_IP) +":18724/";
                http.begin(serverUrl.c_str());
                int httpResponseCode = http.GET();
                if (httpResponseCode == 200) {
                    
                    String payload             = http.getString();
                    responseObject     = JSON.parse(payload);
                }
                Serial.println(http.getString());
                http.end();
            }
            // turn off radios
            WiFi.mode(WIFI_OFF);
            btStop();

            // TODO: Spotify

            display.setCursor(10, xOffset+(10+30));
            xOffset += 10+30;
            
            display.drawBitmap(yOffset+10, xOffset-30, responseObject['isPlaying'] ? bm_pause : bm_play, 30, 30, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);

            display.setCursor(10+30+10, xOffset);

            String trackName = responseObject["name"];
            trackName.remove(0,1);
            trackName.remove(trackName.length()-2,1);

            display.print(trackName);

            // TODO: Weather

        }

        bool connectWiFi() {
        if (WL_CONNECT_FAILED ==
            WiFi.begin(WIFI_SSID, WIFI_PASS)) { // WiFi not setup, you can also use hard coded credentials
                            // with WiFi.begin(SSID,PASS);
            WIFI_CONFIGURED = false;
        } else {
            if (WL_CONNECTED ==
                WiFi.waitForConnectResult()) { // attempt to connect for 10s
            WIFI_CONFIGURED = true;
            } else { // connection failed, time out
            WIFI_CONFIGURED = false;
            // turn off radios
            WiFi.mode(WIFI_OFF);
            btStop();
            }
        }
        return WIFI_CONFIGURED;
        }
    private:
        String commafy(uint32_t num) {
            String numStr = String(num);
            String result = "";
            int len = numStr.length();
            int insertCount = (len - 1) / 3;
            for (int i = 0; i < len; i++) {
                if (i > 0 && (len - i) % 3 == insertCount) {
                    result += ",";
                }
                result += numStr[i];
            }
            return result;
        }
};

MyFirstWatchFace m(settings); //instantiate your watchface

void setup() {
  m.init(); //call init in setup
}

void loop() {
  // this should never run, Watchy deep sleeps after init();
}