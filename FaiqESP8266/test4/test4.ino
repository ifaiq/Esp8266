#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid     = "Linkedthings709";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "F4DD396D";     // The password of the Wi-Fi network

HTTPClient http;
//const char* host = "http://ct.ottomatically.com/api/v1/events/phase/1";


void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println(WiFi.localIP());
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}


void loop()
{
  WiFiClient client;

  http.begin("http://ct.ottomatically.com/api/v1/events/phase/1");  //Specify request destination
  int httpCode = http.GET();                                                                  //Send the request

  if (httpCode > 0) { //Check the returning code

    String payload = http.getString();   //Get the request response payload
    Serial.println(payload);                     //Print the response payload

  }
  http.end();   //Close connection


  //  if (client.connect(host, 80))
  //  {
  //    Serial.println("connected]");
  //
  //    Serial.println("[Sending a request]");
  //    client.print(String("GET /") + " HTTP/1.1\r\n" +
  //                 "Host: " + host + "\r\n" +
  //                 "Connection: close\r\n" +
  //                 "\r\n"
  //                );
  //
  //    Serial.println("[Response:]");
  //    while (client.connected())
  //    {
  //      if (client.available())
  //      {
  //        String line = client.readStringUntil('\n');
  //        Serial.println(line);
  //      }
  //    }
  //    client.stop();
  //    Serial.println("\n[Disconnected]");
  //  }
  //  else
  //  {
  //    Serial.println("connection failed!]");
  //    client.stop();
  //  }
  delay(5000);
}
