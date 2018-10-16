#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
 
#define FONA_RX 5
#define FONA_TX 4
#define FONA_RST 14
 
char replybuffer[255];
 
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
 
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
 
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
 
void setup() {
  while (!Serial);
  Serial.begin(9600);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));
  fonaSS.begin(9600); 
  if (! fona.begin(fonaSS)) {            
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  Serial.println(F("FONA is OK"));
 
  flushSerial();
Serial.print(F("Read #"));
uint8_t smsn = readnumber();
Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);
 
// Retrieve SMS sender address/phone number.
if (! fona.getSMSSender(smsn, replybuffer, 250)) {
   Serial.println("Failed!");
}
   Serial.print(F("FROM: ")); Serial.println(replybuffer);
 
// Retrieve SMS value.
uint16_t smslen;
if (! fona.readSMS(smsn, replybuffer, 250, &smslen)) {
  // pass in buffer and max len!
   Serial.println("Failed!");
 
}
   Serial.print(F("***** SMS #")); Serial.print(smsn); 
   Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
   Serial.println(replybuffer);
   Serial.println(F("*****"));
   

}
 
char readBlocking() {
  while (!Serial.available());
  return Serial.read();
}
 
uint16_t readnumber() {
  uint16_t x = 0;
  char c;
  while (! isdigit(c = readBlocking())) {;} 
 
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = readBlocking())) {
   Serial.print(c);
   x *= 10;
   x += c - '0';
  }
  return x;
}
void loop() {}
 
void flushSerial() {
    while (Serial.available()) 
    Serial.read();
}
 
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  if (timeout == 0) timeoutvalid = false;
  
  while (true) {
    if (buffidx > maxbuff) {
      break;
    }
 
    while(Serial.available()) {
      char c =  Serial.read();
 
      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0)   // the first 0x0A is ignored
          continue;
        
        timeout = 0;         // the second 0x0A is the end of the line
        timeoutvalid = true;
        break;
      }
      buff[buffidx] = c;
      buffidx++;
    }
    
    if (timeoutvalid && timeout == 0) {
      break;
    }
    delay(1);
  }
  buff[buffidx] = 0;  // null term
  return buffidx;


}
