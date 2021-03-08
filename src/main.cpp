#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1015.h>
#include <EEPROM.h>
#include <RunningAverage.h>
#include <SPI.h>
#include <Wire.h>

#define BUTTON_PIN        9
#define SETTINGS_LOCATION 0

#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     64
#define OLED_RESET        4 
#define SCREEN_ADDRESS    0x3C

#define SCREEN_SECTION_HEADER 0
#define SCREEN_SECTION_NORMAL 16


struct input {
  float o2_mv;
  byte button;
};

struct settings {
  float mv_min;
  float mv_max;
  float calibration_factor;
  float avg_length_ms;
  float sample_delay_ms;
};

settings default_settings() {
  settings s;
  s.mv_max = 15.0;
  s.mv_min = 7.0;
  s.avg_length_ms = 1000;
  s.calibration_factor = 0.021;
  s.sample_delay_ms = 100;
  return s;
}

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_ADS1115 ads(0x48);

input read_input() {
  input i;
  i.o2_mv = abs(ads.readADC_Differential_0_1() * 0.0625F);
  i.button = digitalRead(BUTTON_PIN);
  return i;
}

void display_main(String message){
  display.setCursor(0,SCREEN_SECTION_NORMAL);
  display.setTextColor(WHITE,BLACK);
  display.setTextSize(4);
  display.print(message);
  Serial.println(message);
}

void display_info(String message) {
  display.clearDisplay();
  display_main(message);
  display.display();
  delay(1000);
}

input prev_input;
settings system_settings;
RunningAverage RA(10); //TODO: Base on settings
  
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
  prev_input = read_input();
  if(prev_input.button == LOW) {
    system_settings = default_settings();
    EEPROM.put(SETTINGS_LOCATION,system_settings);
    display_info("RESET");
  }
  else {
    EEPROM.get(SETTINGS_LOCATION,system_settings);
  }
  RA.clear();
  display.clearDisplay();
  display.display();
}

String prev_message = "";

void loop() {
  input cur_input = read_input();
  RA.addValue(cur_input.o2_mv);
  float mv = RA.getAverage();

  float o2_percent = mv * system_settings.calibration_factor * 100;
  String message = "";
  
  if(cur_input.button == LOW && cur_input.button != prev_input.button) {
    system_settings.calibration_factor = 0.209/mv;
    EEPROM.put(SETTINGS_LOCATION,system_settings);
    message = "=CAL=";
  }

  else if(cur_input.o2_mv > system_settings.mv_max || cur_input.o2_mv < system_settings.mv_min) message = "CELL!";
  else if (o2_percent < 0.0 || o2_percent > 99.9) message = "CAL!";
  else message = String(o2_percent,1) + "%";
  if(message != prev_message) {
    display.clearDisplay();
    display_main(message);
    display.display();
  }

  prev_input = cur_input;
  prev_message = message;
  if(cur_input.button == LOW) while(read_input().button == LOW) {} //Wait for button release
  delay(system_settings.sample_delay_ms);
}