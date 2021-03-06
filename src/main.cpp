#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1015.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>

#define BUTTON_PIN        2
#define SETTINGS_LOCATION 0

#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     64
#define OLED_RESET        4 
#define SCREEN_ADDRESS    0x3C

struct settings {
  float mv_min;
  float mv_max;
  float calibration_factor;
  float avg_length_ms;
};

settings default_settings() {
  settings s;
  s.mv_max = 1.5;
  s.mv_min = 0.7;
  s.avg_length_ms = 1000;
  s.calibration_factor = 47.3;
  return s;
}

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_ADS1115 ads(0x48);

void display_info(String message)
{
  display.clearDisplay();
  display.setCursor(0,0);  
  display.setTextColor(WHITE,BLACK);
  display.println(message);
  Serial.println(message);
  display.display();
  delay(1000);
}


void setup() {
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    //Handle display error, beeeeeep?
    for(;;); // Don't proceed, loop forever
  }

  ads.setGain(GAIN_TWO);
  ads.begin();

  pinMode(BUTTON_PIN,INPUT_PULLUP);
  settings settings;
  if(digitalRead(BUTTON_PIN) == LOW) {
    settings = default_settings();
    EEPROM.put(SETTINGS_LOCATION,settings);
    display_info("Reset");
  }
  else {
    EEPROM.get(SETTINGS_LOCATION,settings);
  }
  
  display.clearDisplay();
  display.display();
}

void loop() {
  display.clearDisplay();
  display.setCursor(0,0);  
  display.setTextColor(WHITE,BLACK);
  display.println(abs(ads.readADC_Differential_0_1() * 0.0625F));
  display.display();
  delay(100);
}