#include "Micr0Watchy.h"
#include "bit_pusab12pt7b.h"
#include "bit_pusab6pt7b.h"
#include "bit_pusab3pt7b.h" //include any fonts you want to use
#include "icons.h"
#include "settings.h" //same file as the one from 7_SEG example

WatchyRTC Micr0Watchy::RTC;
GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> Micr0Watchy::display(WatchyDisplay(DISPLAY_CS, DISPLAY_DC, DISPLAY_RES, DISPLAY_BUSY));

RTC_DATA_ATTR int guiState;
RTC_DATA_ATTR int menuIndex;
RTC_DATA_ATTR BMA423 sensor;
RTC_DATA_ATTR bool WIFI_CONFIGURED;
RTC_DATA_ATTR bool BLE_CONFIGURED;
RTC_DATA_ATTR weatherData currentWeather;
RTC_DATA_ATTR int weatherIntervalCounter = -1;
RTC_DATA_ATTR bool displayFullInit       = true;
RTC_DATA_ATTR long gmtOffset = 0;
RTC_DATA_ATTR bool alreadyInMenu         = true;
RTC_DATA_ATTR tmElements_t bootTime;

void Micr0Watchy::init(String datetime) {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause(); // get wake up reason
    Wire.begin(SDA, SCL);                         // init i2c
    RTC.init();

    // Init the display here for all cases, if unused, it will do nothing
    display.epd2.selectSPI(SPI, SPISettings(20000000, MSBFIRST, SPI_MODE0)); // Set SPI to 20Mhz (default is 4Mhz)
    display.init(0, displayFullInit, 10,
                true); // 10ms by spec, and fast pulldown reset
    display.epd2.setBusyCallback(displayBusyCallback);

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm
            RTC.read(currentTime);
            switch (guiState) {
                case WATCHFACE_STATE:
                    showWatchFace(true); // partial updates on tick
                    if (settings.vibrateOClock) {
                        if (currentTime.Minute == 0) {
                        // The RTC wakes us up once per minute
                        vibMotor(75, 4);
                        }
                    }
                    break;
                case MAIN_MENU_STATE:
                    // Return to watchface if in menu for more than one tick
                    if (alreadyInMenu) {
                        guiState = WATCHFACE_STATE;
                        showWatchFace(false);
                    } else {
                        alreadyInMenu = true;
                    }
                    break;
            }
            break;
        case ESP_SLEEP_WAKEUP_EXT1: // button Press
            //handleButtonPress();
            vibMotor(50, 2);
            RTC.read(currentTime);
            showWatchFace(true);
            break;
        default: // reset
            RTC.config(datetime);
            _bmaConfig();
            gmtOffset = settings.gmtOffset;
            RTC.read(currentTime);
            RTC.read(bootTime);
            showWatchFace(false); // full update on reset
            vibMotor(75, 4);
            break;
    }
    deepSleep();
}

void Micr0Watchy::displayBusyCallback(const void *) { 
gpio_wakeup_enable((gpio_num_t)DISPLAY_BUSY, GPIO_INTR_LOW_LEVEL);
esp_sleep_enable_gpio_wakeup();
esp_light_sleep_start();
}

void Micr0Watchy::deepSleep() {
    display.hibernate();
    if (displayFullInit) // For some reason, seems to be enabled on first boot
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    displayFullInit = false; // Notify not to init it again
    RTC.clearAlarm();        // resets the alarm flag in the RTC

    // Set GPIOs 0-39 to input to avoid power leaking out
    const uint64_t ignore = 0b11110001000000110000100111000010; // Ignore some GPIOs due to resets
    for (int i = 0; i < GPIO_NUM_MAX; i++) {
        if ((ignore >> i) & 0b1)
        continue;
        pinMode(i, INPUT);
    }
    esp_sleep_enable_ext0_wakeup((gpio_num_t)RTC_INT_PIN, 0); // enable deep sleep wake on RTC interrupt
    esp_sleep_enable_ext1_wakeup( BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); // enable deep sleep wake on button press
    esp_deep_sleep_start();
}

void Micr0Watchy::vibMotor(uint8_t intervalMs, uint8_t length) {
    pinMode(VIB_MOTOR_PIN, OUTPUT);
    bool motorOn = false;
    for (int i = 0; i < length; i++) {
        motorOn = !motorOn;
        digitalWrite(VIB_MOTOR_PIN, motorOn);
        delay(intervalMs);
    }
}
void Micr0Watchy::showWatchFace(bool partialRefresh) {
    display.setFullWindow();
    drawWatchFace();
    display.display(partialRefresh); // partial refresh
    guiState = WATCHFACE_STATE;
}

void Micr0Watchy::drawWatchFace(){ 
    int yOffset = 10;

    Serial.begin(115200);

    display.fillScreen(DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);
    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.setFont(&bit_pusab12pt7b);
    
    // Time
    display.setCursor(10, yOffset+28);
    yOffset += 28;
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
    display.setCursor(10, yOffset+(10+14));
    yOffset += 10+14;

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

    int xOffset = 10;

    display.drawBitmap(xOffset, yOffset+10, batteryIcon, 80, 30, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    yOffset += 10+30;
    xOffset += 80;
    
    // Wifi

    display.drawBitmap(xOffset+10, yOffset-(30+3), WIFI_CONFIGURED ? bm_wifi3 : bm_wifi0, 31, 30, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    xOffset += 31+10;

    // TODO: Alarm?

    // Steps

    display.setCursor(10, yOffset+(10+14));
    yOffset += 10+14;

    if (currentTime.Hour == 0 && currentTime.Minute == 0){
        sensor.resetStepCounter();
    }
    uint32_t stepCount = sensor.getCounter();
    display.print(commafy(stepCount) + " Steps");

    // Pull data from server
    JSONVar responseObject;

    if (connectWiFi()) {
        HTTPClient http;
        http.setConnectTimeout(3000); // 3 second max timeout
        String serverUrl = "http://"+ String(BACKEND_IP) +":18724/";
        http.begin(serverUrl.c_str());
        int httpResponseCode = http.GET();
        if (httpResponseCode == 200) {
            String payload     = http.getString();
            Serial.println(payload);
            responseObject     = JSON.parse(payload);
        }
        http.end();
        //syncNTP();
    }
    // turn off radios
    WiFi.mode(WIFI_OFF);
    btStop();

    String trackName = responseObject["name"];
    String artistsName = responseObject["artists"];
    bool isPlaying = responseObject["isPlaying"];

    // Spotify
    yOffset += 10;
    
    // drawBar
    display.setFont(&bit_pusab12pt7b);
    for(int x = 0; x < 25; x++){
        for(int y = 0; y < 2; y++){
            display.setCursor((x*8), yOffset+28+(y*4));
            display.print("|");
        }
    }

    display.setTextColor(!DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.setFont(&bit_pusab6pt7b);

    display.drawBitmap(10, yOffset, isPlaying ? bm_pause : bm_play, 30, 30, !DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);

    display.setCursor(10+30+10, yOffset+15);
    
    if(trackName.length() > 10) { trackName.remove(8); trackName+="..."; } // if track name too long, trim it.
    
    display.print(trackName);

    display.setCursor(10+30+10, yOffset+15+7+3);
    display.setFont(&bit_pusab3pt7b);

    if(artistsName.length() > 21) { artistsName.remove(19); artistsName+="..."; } // if artist(s) name too long, trim it.
    
    display.print(artistsName);

    yOffset += 30;

    // Weather

    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);

    // int temperature = RTC.temperature();
    int temperature = responseObject["temperature"];
    String condition = responseObject["status"];;

    display.setCursor(10, yOffset+(10+14));
    yOffset += 10+14;

    display.setFont(&bit_pusab6pt7b);

    display.print(String(temperature) + "C " + condition);


}

bool Micr0Watchy::connectWiFi() {
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

float Micr0Watchy::getBatteryVoltage() {
    if (RTC.rtcType == DS3231) {
        return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f * 2.0f; // Battery voltage goes through a 1/2 divider.
    } else {
        return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f * 2.0f; 
    }
}

bool Micr0Watchy::syncNTP() { // NTP sync - call after connecting to WiFi and remember to turn it back off
  return syncNTP(gmtOffset, settings.ntpServer.c_str());
}

bool Micr0Watchy::syncNTP(long gmt) {
  return syncNTP(gmt, settings.ntpServer.c_str());
}

bool Micr0Watchy::syncNTP(long gmt, String ntpServer) {
  // NTP sync - call after connecting to
  // WiFi and remember to turn it back off
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, ntpServer.c_str(), gmt);
  timeClient.begin();
  if (!timeClient.forceUpdate()) {
    return false; // NTP sync failed
  }
  tmElements_t tm;
  breakTime((time_t)timeClient.getEpochTime(), tm);
  RTC.set(tm);
  return true;
}

uint16_t Micr0Watchy::_readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)address, (uint8_t)len);
    uint8_t i = 0;
    while (Wire.available()) {
        data[i++] = Wire.read();
    }
    return 0;
}

uint16_t Micr0Watchy::_writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(data, len);
    return (0 != Wire.endTransmission());
}

void Micr0Watchy::_bmaConfig() {

    if (sensor.begin(_readRegister, _writeRegister, delay) == false) {
        // fail to init BMA
        return;
    }

    // Accel parameter structure
    Acfg cfg;
    /*!
        Output data rate in Hz, Optional parameters:
            - BMA4_OUTPUT_DATA_RATE_0_78HZ
            - BMA4_OUTPUT_DATA_RATE_1_56HZ
            - BMA4_OUTPUT_DATA_RATE_3_12HZ
            - BMA4_OUTPUT_DATA_RATE_6_25HZ
            - BMA4_OUTPUT_DATA_RATE_12_5HZ
            - BMA4_OUTPUT_DATA_RATE_25HZ
            - BMA4_OUTPUT_DATA_RATE_50HZ
            - BMA4_OUTPUT_DATA_RATE_100HZ
            - BMA4_OUTPUT_DATA_RATE_200HZ
            - BMA4_OUTPUT_DATA_RATE_400HZ
            - BMA4_OUTPUT_DATA_RATE_800HZ
            - BMA4_OUTPUT_DATA_RATE_1600HZ
    */
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    /*!
        G-range, Optional parameters:
            - BMA4_ACCEL_RANGE_2G
            - BMA4_ACCEL_RANGE_4G
            - BMA4_ACCEL_RANGE_8G
            - BMA4_ACCEL_RANGE_16G
    */
    cfg.range = BMA4_ACCEL_RANGE_2G;
    /*!
        Bandwidth parameter, determines filter configuration, Optional parameters:
            - BMA4_ACCEL_OSR4_AVG1
            - BMA4_ACCEL_OSR2_AVG2
            - BMA4_ACCEL_NORMAL_AVG4
            - BMA4_ACCEL_CIC_AVG8
            - BMA4_ACCEL_RES_AVG16
            - BMA4_ACCEL_RES_AVG32
            - BMA4_ACCEL_RES_AVG64
            - BMA4_ACCEL_RES_AVG128
    */
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

    /*! Filter performance mode , Optional parameters:
        - BMA4_CIC_AVG_MODE
        - BMA4_CONTINUOUS_MODE
    */
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    // Configure the BMA423 accelerometer
    sensor.setAccelConfig(cfg);

    // Enable BMA423 accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    sensor.enableAccel();

    struct bma4_int_pin_config config;
    config.edge_ctrl = BMA4_LEVEL_TRIGGER;
    config.lvl       = BMA4_ACTIVE_HIGH;
    config.od        = BMA4_PUSH_PULL;
    config.output_en = BMA4_OUTPUT_ENABLE;
    config.input_en  = BMA4_INPUT_DISABLE;
    // The correct trigger interrupt needs to be configured as needed
    sensor.setINTPinConfig(config, BMA4_INTR1_MAP);

    struct bma423_axes_remap remap_data;
    remap_data.x_axis      = 1;
    remap_data.x_axis_sign = 0xFF;
    remap_data.y_axis      = 0;
    remap_data.y_axis_sign = 0xFF;
    remap_data.z_axis      = 2;
    remap_data.z_axis_sign = 0xFF;
    // Need to raise the wrist function, need to set the correct axis
    sensor.setRemapAxes(&remap_data);

    // Enable BMA423 isStepCounter feature
    sensor.enableFeature(BMA423_STEP_CNTR, true);
    // Enable BMA423 isTilt feature
    sensor.enableFeature(BMA423_TILT, true);
    // Enable BMA423 isDoubleClick feature
    sensor.enableFeature(BMA423_WAKEUP, true);

    // Reset steps
    sensor.resetStepCounter();

    // Turn on feature interrupt
    sensor.enableStepCountInterrupt();
    sensor.enableTiltInterrupt();
    // It corresponds to isDoubleClick interrupt
    sensor.enableWakeupInterrupt();
}

String Micr0Watchy::commafy(uint32_t num) {
    String numStr = String(num);
    String result = "";
    int len = numStr.length();
    int insertCount = (len - 1) / 3;
    for (int i = len-1; i <= 0; i++) {
        if (i > 0 && (len - i) % 3 == insertCount) {
            result += ",";
        }
        result += numStr[i];
    }
    return result;
}

Micr0Watchy m(settings); //instantiate your watchface

void setup() {
  m.init(); //call init in setup
}

void loop() {
  // this should never run, Watchy deep sleeps after init();
}