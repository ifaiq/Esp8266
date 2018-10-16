#include <ESP8266WiFi.h>        // Include the Wi-Fi library
const char* ssid     = "Linkedthings709";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "F4DD396D";     // The password of the Wi-Fi network

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');

  WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" started");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP());         // Send the IP address of the ESP8266 to the computer
}

void loop() { }

