#include <LTMemory.h>
#include <LTESPmac.h>
#include <LTCredentialsConfigurationV1_0_0.h>
#include <LTwifiAPandSTA.h>
#include <LTPostAndGetRequest.h>
#include <LTJsonParser.h>
#include <LTSmartHomeDevicesConfigure.h>
#include <LTOta.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>


LTMemory EEPROMmemory;
LTESPmac deviceMac;
LTCredentialsConfiguration Config;
LTwifiAPandSTA WIFI;
LTPostAndGetRequest HttpRequest;
LTJsonParser JsonParse;
LTSmartHomeDevicesConfigure DevicesConfigure;
LTOta OtaUpdate;

int lcdAddr = 0x3F; // 63 in decimal
//int lcdAddr = 0x27; // 39 in decimal

LiquidCrystal_I2C lcd(lcdAddr, 16, 2); //addr, col, row
// C page
const char * AP_SSID = "SmartLinkedthings";
const char * AP_PASSWORD = "123456789";
const char * HUB_SSID = "Smart Home Hub App Test";
const char * HUB_PASSWORD = "123456789";

String DeviceMac;
String RoomId;
String BranchId;
String OrganizationId;
String HubId;
String HubType = "Agri";

String values[20];
char WIFI_SSID[25];
char WIFI_PASSWORD[25];
// END

// Server credentials
//MQTT
String orgId = "2uty47";
String authToken = "";
String clientName = "";
String Type = "Agri";
String evtTopic = "iot-2/evt/AgriEvents/fmt/json";
String cmdTopic = "iot-2/cmd/AppEvents/fmt/json";
String cmdTopic2 = "iot-2/cmd/DeviceEvents/fmt/json";
String cmdTopic3 = "iot-2/cmd/Restart/fmt/json";
String cmdTopic4 = "iot-2/cmd/Config/fmt/json";
String authMethod = "use-token-auth";
String server = orgId + ".messaging.internetofthings.ibmcloud.com";

void callback(char* topic, byte* Payload, unsigned int PayloadLength);
WiFiClient wifiClient;
PubSubClient mqttClient((char*)server.c_str(), 1883, callback, wifiClient);


//HTTP
String ServerHost = "linked-things-orgs.eu-gb.mybluemix.net";
String SigninUrl = "/api/v1/devices/signin";
String DevicePostServerUrl = "/api/v1/devices/bulk";
String OtaGetUrl = "/api/v1/updates?type=" + HubType + "&prerequisite=";
String InfoGetUrl = "/api/v1/devices/";
String VersionPostUrl = "/api/v1/devices/";
String OtaUrl;
String AuthenticationToken;
String ServerResponseValue;

//OTA request time
unsigned long PreviousMillis = 0;
unsigned long KeepAliveRequestTime = (unsigned long) 1000 * 60 * 30;
String CodeVersion = "0.0.1";

int TotalDevice = 0;
String deviceId[4];
String deviceType[4];
String deviceName[4];


String cmdPayload, cmdHubId, cmdDeviceId, cmdEvtType, cmdDeviceName, cmdDeviceValue;
String evtPayload;

//Devices
String DevicesType[] = {"PhIndicator"};
String DevicesName[] = {"pHIndicator"};


//Gpio's
int RESET_ESP_BUTTON_PIN = 15, POWER_LED_PIN = 2;
int LCD_SDA = 12, LCD_SLC = 14;

const byte sensorPin = A0;
const byte samplingInterval = 20, printInterval = 800, ArrayLength = 40;
int pHArray[ArrayLength];
int pHArrayIndex = 0;

void setup() {
  Serial.begin(115200);

  pinMode(RESET_ESP_BUTTON_PIN, INPUT);
  digitalWrite(RESET_ESP_BUTTON_PIN, LOW);
  pinMode(POWER_LED_PIN, OUTPUT);
  digitalWrite(POWER_LED_PIN, HIGH);

  Serial.println("\n\nESP Start.");
  // LCD initialize
  LcdInitialize();

  delay(5000);
  // Hard and Soft Reset
  EEPROMmemory.HardAndSoftReset(RESET_ESP_BUTTON_PIN);

  // read device Mac
  DeviceMac = deviceMac.mac();
  //  DeviceMac = "MoistureTesting";
  Serial.println("\n\nDevice MAC is : " + DeviceMac);
  lcdPrint(2, "Config Mode", 1, "Enter Settings");

  Config.ConfigurationWithoutToken(DeviceMac, HubType, AP_SSID, AP_PASSWORD, values, 3);

  //  values[0] = "Linkedthings709";
  //  values[1] = "F4DD396D";
  //  values[2] = "LinkedThings_HeadOffice_Agri";

  Serial.println("******** Credentials ********");
  for (int j = 0; j < 3; j++)
  {
    Serial.println("values[" + String(j) + "] = " + values[j]);
  }
  Serial.println("*****************************");

  values[0].toCharArray(WIFI_SSID, values[0].length() + 1);
  values[1].toCharArray(WIFI_PASSWORD, values[1].length() + 1);
  RoomId = values[2];
  OrganizationId = RoomId.substring(0, RoomId.indexOf('_'));

  WIFI.APWithSTAWithRestart(HUB_SSID, HUB_PASSWORD, 1, WIFI_SSID, WIFI_PASSWORD, POWER_LED_PIN);

  HubId = OrganizationId + "_" + DeviceMac;

  lcdPrint(3, "Connecting", 1, "Scanning WiFi");
  delay(1500);
  String wifiRange = "0";
  while (wifiRange == "0") {
    wifiRange = WIFI.SCAN(WIFI_SSID);
    if (wifiRange != "0") {
      lcdPrint(0, String(WIFI_SSID), 3, "(" + wifiRange + ") dBm");
      delay(1500);
      lcdPrint(1, "Connecting to ", 0, String(WIFI_SSID));
      WIFI.APWithSTAWithoutRestart(HUB_SSID, HUB_PASSWORD, 1, WIFI_SSID, WIFI_PASSWORD);
      lcdPrint(3, "Connecting", 1, "WiFi Connected");
    } else {
      lcdPrint(0, String(WIFI_SSID), 3, "Not Found");
      delay(3000);
      lcdPrint(1, "Enter New WiFi", 0, "OR Restart Device");
      delay(3000);
    }
  }

  authToken = DeviceMac;
  clientName += "d:";
  clientName += orgId + ":";
  clientName += Type + ":";
  clientName += HubId;

  AuthenticationToken = loginToServer(HubId, DeviceMac);
  deviceInitialize();
  mqttConnect();
  VersionPost();
}
float phValue;

void loop() {
  // Hard and Soft Reset
  EEPROMmemory.HardAndSoftReset(RESET_ESP_BUTTON_PIN);

  if (!mqttClient.loop())
    mqttConnect();

  phValue = PhRead();
  //  if (phValue != "") {
  Serial.println("PH value is:" + String(phValue));
  lcdPrint(0, "pH value is:" + String(phValue), 0, "");

  postEvt(evtTopic, HubId, "PhIndicator_Status", HubId + "_" + DevicesName[0], String(phValue));
  //  }

  OtaRequest();
  delay(3000);

}
int buffer[10], temp;
float pHValue, Voltage;

float PhRead() {
  unsigned long avgValue;
  unsigned long samplingTime = millis();
  unsigned long printTime = millis();

  int a = analogRead(sensorPin);
  float percent = 0;
  percent = fmap(a, 0, 1024, 0, 14);
  //   if (percent >= 1)
  //  {
  //    percent -= 1;
  //  }
  Serial.println(percent);
  Serial.println(analogRead(sensorPin));

  return percent;
}
float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
double avergearray(int* arr, int number) {
  int Max, Min;
  double avg;
  long amount = 0;
  if (number <= 0) {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if (number < 5) { //less than 5, calculated directly statistics
    for (byte i = 0; i < number; i++)
      amount += arr[i];

    avg = amount / number;
    return avg;
  } else {
    if (arr[0] < arr[1]) {
      Min = arr[0];
      Max = arr[1];
    }
    else {
      Min = arr[1];
      Max = arr[0];
    }
    for (byte i = 2; i < number; i++) {
      if (arr[i] < Min) {
        amount += Min;      //arr<min
        Min = arr[i];
      } else {
        if (arr[i] > Max) {
          amount += Max;  //arr>max
          Max = arr[i];
        } else
          amount += arr[i]; //min<=arr<=max
      }
    }
    avg = (double)amount / (number - 2);
  }
  return avg;
}

void LcdInitialize() {
  lcd.begin(16, 2); // fix
  lcd.init(LCD_SDA, LCD_SLC); // sda_pin, slc_pin
  lcd.backlight();

  lcdPrint(3, "PH Level", 3, "Indicator");
  delay(3000);
}

void lcdPrint(int p1, String msg1, int p2, String msg2) {
  lcd.clear();
  lcd.setCursor(p1, 0);
  lcd.print(msg1);
  lcd.setCursor(p2, 1);
  lcd.print(msg2);
}
String loginToServer(String user, String pass)
{
  String token, PostPacket, ServerResponseValue;
  PostPacket = "{\"_id\":\"" + user + "\", \"authToken\":\"" + pass + "\"}";
  ServerResponseValue = HttpRequest.Post((char*) ServerHost.c_str(), SigninUrl, PostPacket);
  ServerResponseValue = JsonParse.removeGarbage(ServerResponseValue);
  token = JsonParse.extractValue(ServerResponseValue, "token");
  return token;
}

void deviceInitialize()
{
  int totalDevices = (sizeof(DevicesType) / sizeof(String));
  Serial.println("\nTotal Devices is: " + String(totalDevices));
  for (int i = 0; i < totalDevices; i++)
    Serial.println("Device" + String(i + 1) + " Type is: " + DevicesType[i] + " and Name is: " + DevicesName[i]);

  //EEPROMmemory.write(328, "1#");
  if (EEPROMmemory.read(328, 329) != "1") {
    CreateDevice(totalDevices, DeviceMac, DevicesType, DevicesName);
    EEPROMmemory.write(328, "1#");
  }
}

void VersionPost()
{
  Serial.println("\nGet Request for Version Post");
  ServerResponseValue = HttpRequest.GetWithToken((char*) ServerHost.c_str(), InfoGetUrl + HubId, AuthenticationToken);
  if (ServerResponseValue.startsWith("HTTP/1.1 401")) {
    AuthenticationToken = loginToServer(HubId, DeviceMac);
    ServerResponseValue = HttpRequest.GetWithToken((char*) ServerHost.c_str(), InfoGetUrl + HubId, AuthenticationToken);
  }
  ServerResponseValue = JsonParse.removeGarbage(ServerResponseValue);
  Serial.println("\nAfter Remove garbage Response is : " + ServerResponseValue);
  String GetVersion = JsonParse.extractValue(ServerResponseValue, "version");

  Serial.println("\nGet Version Value : " + GetVersion);
  Serial.println("Current Version Value : " + CodeVersion);

  String packet = "{\"version\":\"" + CodeVersion + "\"}";
  if (!GetVersion.startsWith("Error"))
  {
    if (GetVersion != CodeVersion)
    {
      Serial.println("\nPost Request for Version Post");
      ServerResponseValue = HttpRequest.Put((char*) ServerHost.c_str(), VersionPostUrl + HubId, packet, AuthenticationToken);
      if (ServerResponseValue.startsWith("HTTP/1.1 401")) {
        AuthenticationToken = loginToServer(HubId, DeviceMac);
        ServerResponseValue = HttpRequest.Put((char*) ServerHost.c_str(), VersionPostUrl + HubId, packet, AuthenticationToken);
      }
    }
  }
}
void CreateDevice(int TotalDevice, String DeviceId, String DeviceType[], String DeviceName[])
{
  evtPayload = "[";
  for (int i = 0; i < TotalDevice; i++)
  {
    if (DeviceType[i] != NULL)
    {
      Serial.println("Module" + String(i + 1) + " Type is: " + DeviceType[i] + " , Name is: " + DeviceName[i]);
      evtPayload += "{";
      evtPayload += "\"_id\": \"" + DeviceId + "_" + DeviceName[i] + "\", ";
      evtPayload += "\"levelId\": \""  + RoomId + "\", ";
      evtPayload += "\"hubId\": \"" + HubId + "\", ";
      evtPayload += "\"name\": \"" + DeviceName[i] + "\", ";
      evtPayload += "\"type\": \"" + DeviceType[i] + "\"";
      evtPayload += "}";
      evtPayload += ",";
    }
  }
  if (evtPayload[(evtPayload.length() - 1)] == ',')
  {
    evtPayload[(evtPayload.length() - 1)] = ']';
  }
  Serial.print("\nPublish Packet: ");
  Serial.println(evtPayload);

  String check = "Not Ok";
  while (check != "Ok")
  {
    // Posting Create device request on Server just one time
    Serial.println("\nPosting Request for Create device");
    ServerResponseValue = HttpRequest.Post((char*) ServerHost.c_str(), DevicePostServerUrl, evtPayload, AuthenticationToken);

    if (ServerResponseValue.startsWith("HTTP/1.1 200") || ServerResponseValue.startsWith("HTTP/1.1 201")) {
      evtPayload = "";
      Serial.println("Device Created Sucessfully!");
      check = "Ok";
      TotalDevice = 0;
    } else if (ServerResponseValue.startsWith("HTTP/1.1 409")) {
      evtPayload = "";
      Serial.println("Device Created Sucessfully!");
      check = "Ok";
      TotalDevice = 0;
    }
    else if (ServerResponseValue.startsWith("HTTP/1.1 401"))
      AuthenticationToken = loginToServer(HubId, DeviceMac);
  }
}
int counter = 0, connCount = 0;
void mqttConnect()
{
  if (!mqttClient.connected()) {
    lcdPrint(2, "Internet Not", 3, "Available");
    Serial.println("mqttClient not connect");
    Serial.println("Reconnecting MQTT client to : " + server);
    digitalWrite(POWER_LED_PIN, LOW);
    int counter = 0;
    while (!mqttClient.connect((char*) clientName.c_str(), (char*) authMethod.c_str(), (char*) authToken.c_str())) {
      Serial.print(".");
      counter++;
      if (counter > 1) {
        if (WiFi.status() != WL_CONNECTED)
          lcdPrint(0, String(WIFI_SSID), 3, "Not Found");

        WIFI.APWithSTAWithoutRestart(HUB_SSID, HUB_PASSWORD, 1, WIFI_SSID, WIFI_PASSWORD);
        connCount++;
        if (connCount > 1)
          //          ESP.restart();
          break;
      }
    }
    initManagedDevice(cmdTopic);
    initManagedDevice(cmdTopic2);
    initManagedDevice(cmdTopic3);
    initManagedDevice(cmdTopic4);

    lcdPrint(0, "pH value is:" + String(phValue), 0, "");

    digitalWrite(POWER_LED_PIN, HIGH);
  }
  if (counter != 2)
    connCount = 0;
}

void initManagedDevice(String Topic)
{
  if (mqttClient.subscribe((char*) Topic.c_str()))
    Serial.println("subscribe to " + Topic + " OK");
  else {
    Serial.println("subscribe to " + Topic + " FAILED");
  }
}

void postEvt(String deviceTopic, String hubId, String evtType, String deviceId, String deviceValue)
{
  mqttConnect();
  evtPayload = "{";
  evtPayload += "\"hubId\": \"" + hubId + "\", ";
  evtPayload += "\"deviceId\": \"" + deviceId + "\", ";
  evtPayload += "\"type\": \"" + evtType + "\", ";
  evtPayload += "\"value\": " + deviceValue + "";
  evtPayload += "}";

  Serial.print("\nTopic: "); Serial.println(deviceTopic);
  Serial.print("Publish Packet: ");
  Serial.println(evtPayload);

  if (mqttClient.publish((char*) deviceTopic.c_str(), (char*) evtPayload.c_str())) {
    Serial.println("Publish ok " + deviceId + " " + deviceValue);
    digitalWrite(POWER_LED_PIN, LOW);
    delay(250);
    digitalWrite(POWER_LED_PIN, HIGH);
  } else
    Serial.println("Publish failed " + deviceId + " " + deviceValue);
}

void callback(char* topic, byte * Payload, unsigned int PayloadLength)
{
  cmdPayload = "";
  Serial.print("\ncallback invoked for topic: "); Serial.println(topic);
  for (int i = 0; i < PayloadLength; i++) {
    cmdPayload += (char)Payload[i];
  }

  if (cmdPayload.length() > 0)
    Serial.println("\n\nCMD receive is : " + cmdPayload);

  cmdHubId = JsonParse.extractValue(cmdPayload, "hubId");

  if (HubId == cmdHubId) {
    Serial.println("\nHub Id Matched CMD Accept");

    if (JsonParse.extractValue(cmdPayload, "ota") == "true") {
      OtaUrl = JsonParse.extractValue(cmdPayload, "url");
      Serial.println("OTA url is : " + OtaUrl);
      Serial.println("OTA Begin...");
      OtaUpdate.Update(OtaUrl);
    }


    cmdDeviceId = JsonParse.extractValue(cmdPayload, "deviceId");
    cmdEvtType = JsonParse.extractValue(cmdPayload, "type");
    String temp[5];
    Config.split(cmdDeviceId, 5, temp);
    cmdDeviceName = temp[4];
    Serial.println("\ncmdDeviceName is : " + cmdDeviceName);
    cmdDeviceValue = JsonParse.extractValue(cmdPayload, "value");
    Serial.println("\ncmdDeviceValue is : " + cmdDeviceValue + "\n");
  } else
    cmdPayload = "";
}

//Check ota in every 30 minutes
void OtaRequest()
{
  unsigned long CurrentMillis = millis();
  if (CurrentMillis - PreviousMillis > KeepAliveRequestTime)
  {
    PreviousMillis = CurrentMillis;
    // Get OTA request in every 30 minutes
    Serial.println("\nGet Request for OTA");
    ServerResponseValue = HttpRequest.GetWithToken((char*) ServerHost.c_str(), OtaGetUrl + CodeVersion, AuthenticationToken);
    if (ServerResponseValue.startsWith("HTTP/1.1 401")) {
      AuthenticationToken = loginToServer(HubId, DeviceMac);
      ServerResponseValue = HttpRequest.GetWithToken((char*) ServerHost.c_str(), OtaGetUrl + CodeVersion, AuthenticationToken);
    }
    ServerResponseValue = JsonParse.removeGarbage(ServerResponseValue);
    Serial.println("\nAfter Remove garbage Response is : " + ServerResponseValue);

    String VersionValue = JsonParse.extractValue(ServerResponseValue, "version");
    String scanDeviceId = JsonParse.extractValue(ServerResponseValue, "deviceId");
    OtaUrl = JsonParse.extractValue(ServerResponseValue, "url");
    String tag = JsonParse.extractValue(ServerResponseValue, "tag");

    if (scanDeviceId == HubId) {
      Serial.println("\nNew Version Value : " + VersionValue);
      Serial.println("Current Version Value : " + CodeVersion);
      if (!VersionValue.startsWith("Error")) {
        if (VersionValue != CodeVersion) {
          Serial.println("OTA url is : " + OtaUrl);
          Serial.println("OTA Begin...");
          OtaUpdate.Update(OtaUrl);
        } else
          Serial.println("Code Updated");
      }
    }
    else if (tag == HubId) {
      Serial.println("\nNew Version Value : " + VersionValue);
      Serial.println("Current Version Value : " + CodeVersion);
      if (!VersionValue.startsWith("Error")) {
        if (VersionValue != CodeVersion) {
          Serial.println("OTA url is : " + OtaUrl);
          Serial.println("OTA Begin...");
          OtaUpdate.Update(OtaUrl);
        } else
          Serial.println("Code Updated");
      }
    }
  }
}
