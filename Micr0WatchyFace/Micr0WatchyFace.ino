#include <Watchy.h> //include the Watchy library
#include "FreeMonoBold20pt7b.h" //include any fonts you want to use
#include "settings.h" //same file as the one from 7_SEG example

class MyFirstWatchFace : public Watchy{ //inherit and extend Watchy class
    public:
        MyFirstWatchFace(const watchySettings& s) : Watchy(s) {}
        void drawWatchFace(){ //override this method to customize how the watch face looks
          display.setFont(&FreeMonoBold20pt7b);
          display.setCursor(25, 110);
          if(currentTime.Hour < 10){ //use the currentTime struct to print latest time
            display.print("0");
          }
          display.print(currentTime.Hour);
          display.print(":");
          if(currentTime.Minute < 10){
            display.print("0");
          }  
          display.println(currentTime.Minute);   
        }
};

MyFirstWatchFace m(settings); //instantiate your watchface

void setup() {
  m.init(); //call init in setup
}

void loop() {
  // this should never run, Watchy deep sleeps after init();
}