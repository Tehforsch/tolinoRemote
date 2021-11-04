#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define FORWARD_BUTTON 2

#define DOUBLE_CLICK_LENGTH 800

static unsigned long last_button_time = millis();
static bool already_sent = false;
static int forward_pressed_last_loop = LOW;

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

void loop() {  
  if (!client.connected()) {
    Serial.println("Server not reachable.");
    client.connect("192.168.4.2", 5000);
  }
  else { 
    int forward_pressed = digitalRead(FORWARD_BUTTON);
    unsigned long current_time = millis();
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
  }
  delay(100);
  bool deepsleep = millis()-last_button_time > 300000;
  if(deepsleep){
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, HIGH);
    esp_deep_sleep_start();
  }
}
