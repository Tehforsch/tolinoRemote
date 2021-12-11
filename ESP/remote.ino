#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "driver/adc.h"

#define FORWARD_BUTTON 2
#define BATTERY_LEVEL_PIN 33
#define ONBOARD_LED  2

#define DOUBLE_CLICK_LENGTH 800
#define LOOP_TIMEOUT 100
#define IDLE_TIME_BEFORE_DEEP_SLEEP 300000

#define BATTERY_LEVEL_TRESHOLD 2800
#define NUM_MEASUREMENTS_REQUIRED 10

#define LOOP_COUNT_SEND_BATTERY_LEVEL 100

static unsigned long last_button_time = millis();
static bool already_sent = false;
static int num_times_voltage_too_low = 0;
static int forward_pressed_last_loop = LOW;
static int num_loop = 0;
static int battery_level = 0;

const char *ssid = "tolinoRemote";
const char *password = "supersafepassword";

WiFiServer server(80);
WiFiClient client;

void setup() {
  pinMode(FORWARD_BUTTON, INPUT_PULLDOWN);
  pinMode(ONBOARD_LED, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
  
}


int readBatteryLevel() {
  return analogRead(BATTERY_LEVEL_PIN);
}

bool batteryTooLow() {
  return battery_level < BATTERY_LEVEL_TRESHOLD;
}

void enterDeepsleep() {
  client.print("deepsleep");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, HIGH);
  // esp_sleep_enable_ext1_wakeup(GPIO_NUM_2, ESP_EXT1_WAKEUP_ANY_HIGH);
  adc_power_off(); // Not sure what this does, recommended at https://github.com/espressif/arduino-esp32/issues/1113
  esp_deep_sleep_start();
}

void loop() {  
  unsigned long current_time = millis();
  battery_level = readBatteryLevel();
  
  if (!client.connected()) {
    Serial.println("Server not reachable.");
    client.connect("192.168.4.2", 5000);
  }
  else { 
    int forward_pressed = digitalRead(FORWARD_BUTTON);
    if (forward_pressed_last_loop == LOW && forward_pressed == HIGH) {
      last_button_time = current_time;
      already_sent = false;
    }
    if (forward_pressed_last_loop == HIGH && forward_pressed == HIGH) {
      if (current_time - last_button_time > DOUBLE_CLICK_LENGTH && !already_sent) {
        client.println("back");
        Serial.println("Back pressed");
        already_sent = true;
      }
    }
    if (forward_pressed_last_loop == HIGH && forward_pressed == LOW) {
      if (!already_sent) {
        client.println("next");
        Serial.println("Forward pressed");  
      }   
    }
    forward_pressed_last_loop = forward_pressed;
    num_loop += 1;
    if (num_loop % LOOP_COUNT_SEND_BATTERY_LEVEL == 0) {
      sendBatteryLevel();
    }
  }
  if (batteryTooLow()) {
    // It takes roughly 6 years to get to an integer overflow here, so we will accept that.
    num_times_voltage_too_low += 1;
  }
  else {
    num_times_voltage_too_low = 0;
  }
  if (num_loop % 2 == 0) {
    digitalWrite(ONBOARD_LED,HIGH);
  }
  else {
    digitalWrite(ONBOARD_LED,LOW);
  }
  bool deepsleep = num_times_voltage_too_low > NUM_MEASUREMENTS_REQUIRED || (millis()-last_button_time > IDLE_TIME_BEFORE_DEEP_SLEEP);
  if(deepsleep){
    enterDeepsleep();
  }
  delay(LOOP_TIMEOUT);
}

void sendBatteryLevel() {
  char batteryLevelString[100];
  sprintf(batteryLevelString, "%i %i\n", battery_level, num_times_voltage_too_low);
  client.print(batteryLevelString);
}
