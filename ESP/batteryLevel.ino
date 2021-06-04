#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define FORWARD_BUTTON 2
#define BACKWARD_BUTTON 3
#define BATTERY_LEVEL_PIN 33

const char *ssid = "tolinoRemote";
const char *password = "supersafepassword";

WiFiServer server(80);

int forward_pressed_last_loop = HIGH;
int backward_pressed_last_loop = HIGH;

WiFiClient client;

void setup() {
  pinMode(FORWARD_BUTTON, INPUT_PULLUP);
  pinMode(BACKWARD_BUTTON, INPUT_PULLUP);

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
    int backward_pressed = digitalRead(BACKWARD_BUTTON);
    if (forward_pressed == LOW && forward_pressed_last_loop == HIGH) {
      client.println("next"); 
      Serial.println("Forward pressed");
      
    }
    
    if (backward_pressed == LOW && backward_pressed_last_loop == HIGH) {
      client.print("back"); 
      Serial.println("Backward pressed");
    }
    
    forward_pressed_last_loop = forward_pressed;
    backward_pressed_last_loop = backward_pressed;

    int batteryLevel = readBatteryLevel();
    char batteryLevelString[100];
    sprintf(batteryLevelString, "%i\n", batteryLevel);
    client.print(batteryLevelString);
  }
  delay(200);
}

int readBatteryLevel() {
  return analogRead(BATTERY_LEVEL_PIN);
}
