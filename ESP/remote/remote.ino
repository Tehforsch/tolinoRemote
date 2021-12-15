#include "driver/adc.h"
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>

#define FORWARD_BUTTON 2
#define BATTERY_LEVEL_PIN 33

#define DOUBLE_CLICK_LENGTH 800
#define LOOP_TIMEOUT 100
#define IDLE_TIME_BEFORE_DEEP_SLEEP 300000

#define BATTERY_LEVEL_TRESHOLD 2800
#define NUM_MEASUREMENTS_REQUIRED 10

#define LOOP_COUNT_SEND_BATTERY_LEVEL 100

static unsigned long last_button_time = millis();
static bool already_sent = false;
static int num_times_voltage_too_low = 0;
static bool forward_pressed_last_loop = false;
static int num_loop = 0;
static int battery_level = 0;

const char *ssid = "tolinoRemote";
const char *password = "supersafepassword";

WiFiServer server(80);
WiFiClient client;

void setup() {
  pinMode(FORWARD_BUTTON, INPUT_PULLDOWN);

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

void read_battery_level() { 
  battery_level = analogRead(BATTERY_LEVEL_PIN);
}

bool battery_too_low() {
  if (battery_level < BATTERY_LEVEL_TRESHOLD) {
    // It takes roughly 6 years to get to an integer overflow here, so we will
    // accept that.
    num_times_voltage_too_low += 1;
  } else {
    num_times_voltage_too_low = 0;
  }
  return num_times_voltage_too_low > NUM_MEASUREMENTS_REQUIRED;
}

bool timeout_expired() {
  return millis() - last_button_time > IDLE_TIME_BEFORE_DEEP_SLEEP;
}

void try_connect() {
  Serial.println("Server not reachable.");
  client.connect("192.168.4.2", 6002);
}

void enter_deepsleep() {
  client.print("deepsleep");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, HIGH);
  adc_power_off(); // Not sure what this does, recommended at
                   // https://github.com/espressif/arduino-esp32/issues/1113
  esp_deep_sleep_start();
}

void send_battery_level() {
  char batteryLevelString[100];
  sprintf(batteryLevelString, "%i %i\n", battery_level,
          num_times_voltage_too_low);
  client.print(batteryLevelString);
}

void send_commands_when_buttons_pressed(unsigned long current_time) {
  bool forward_pressed = digitalRead(FORWARD_BUTTON) == HIGH;
  if (!forward_pressed_last_loop && forward_pressed) {
    last_button_time = current_time;
    already_sent = false;
  }
  if (forward_pressed_last_loop && forward_pressed) {
    if (current_time - last_button_time > DOUBLE_CLICK_LENGTH && !already_sent) {
      client.println("prev");
      Serial.println("Back pressed");
      already_sent = true;
    }
  }
  if (forward_pressed_last_loop && !forward_pressed) {
    if (!already_sent) {
      client.println("next");
      Serial.println("Forward pressed");
    }
  }
  forward_pressed_last_loop = forward_pressed == HIGH;
}

void loop() {
  num_loop += 1;
  unsigned long current_time = millis();
  read_battery_level();

  if (!client.connected()) {
    try_connect();
  } else {
    send_commands_when_buttons_pressed(current_time);
    if (num_loop % LOOP_COUNT_SEND_BATTERY_LEVEL == 0) {
      send_battery_level();
    }
  }
  bool deepsleep = battery_too_low() || timeout_expired();
  if (deepsleep) {
    enter_deepsleep();
  }
  delay(LOOP_TIMEOUT);
}
