#include <LTMemory.h>
#include <LTESPmac.h>
#include <LTCredentialsConfigurationV3_0_0.h>
#include <LTwifiAPandSTA.h>
#include <LTPostAndGetRequest.h>
#include <LTJsonParser.h>
#include <LTSmartHomeDevicesConfigure.h>
#include <LTOta.h>
#include <PubSubClient.h>


LTMemory EEPROMmemory;
LTESPmac deviceMac;
LTCredentialsConfiguration Config;
LTwifiAPandSTA WIFI;
LTPostAndGetRequest HttpRequest;
LTJsonParser JsonParse;
LTSmartHomeDevicesConfigure DevicesConfigure;
LTOta OtaUpdate;

// C page
const char * AP_SSID = "SmartLinkedthings";
const char * AP_PASSWORD = "123456789";
const char * HUB_SSID = "Smart Home Hub App Test";
const char * HUB_PASSWORD = "123456789";



const int sleepTimeS = 300;
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
String ServerResponseValue;
String AuthenticationToken;


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
String DevicesType[] = {"Moisture", "Moisture"};
String DevicesName[] = {"Moisture1", "Moisture2"};


//Gpio's
int RESET_ESP_BUTTON_PIN = 15, POWER_LED_PIN = 2;

const byte sensorPin = A0;

const byte addressA = 14;
const byte addressB = 12;
const byte addressC = 13;

float sensor1, sensor2;
float oldSensor1, oldSensor2;


unsigned long PreviousMillisPost = 0;
unsigned long PostTime = (unsigned long) 1000 * 60 * 5;

void setup() {
  Serial.begin(115200);

  pinMode(RESET_ESP_BUTTON_PIN, INPUT);
  digitalWrite(RESET_ESP_BUTTON_PIN, LOW);
  pinMode(POWER_LED_PIN, OUTPUT);
  digitalWrite(POWER_LED_PIN, HIGH);
  pinMode (addressA, OUTPUT);
  pinMode (addressB, OUTPUT);
  pinMode (addressC, OUTPUT);

  Serial.println("\n\nESP Start.");

  delay(5000);
  // Hard and Soft Reset
  EEPROMmemory.HardAndSoftReset(RESET_ESP_BUTTON_PIN);

  // read device Mac
  DeviceMac = deviceMac.mac();
  //  DeviceMac = "MoistureTesting";
  Serial.println("\n\nDevice MAC is : " + DeviceMac);

  Config.Configuration(DeviceMac, HubType, AP_SSID, AP_PASSWORD, values, 3);

  //  values[0] = "Connectify-Linkedthings";
  //  values[1] = "F4DD396D";
  //  values[2] = "LinkedThings_Fields_Field1";

  Serial.println("******** Credentials ********");
  for (int j = 0; j < 3; j++)
    Serial.println("values[" + String(j) + "] = " + values[j]);
  Serial.println("*****************************");

  values[0].toCharArray(WIFI_SSID, values[0].length() + 1);
  values[1].toCharArray(WIFI_PASSWORD, values[1].length() + 1);
  RoomId = values[2];
  OrganizationId = RoomId.substring(0, RoomId.indexOf('_'));

  WIFI.APWithSTAWithRestart(HUB_SSID, HUB_PASSWORD, 1, WIFI_SSID, WIFI_PASSWORD, POWER_LED_PIN);

  HubId = OrganizationId + "_" + DeviceMac;

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

void loop() {
  // Hard and Soft Reset
  EEPROMmemory.HardAndSoftReset(RESET_ESP_BUTTON_PIN);

  if (!mqttClient.loop())
    mqttConnect();

  //Moisture
  sensor1 = readSensor(0);
  Serial.println("\nMoisture1: " + String(sensor1) + "%");
  delay(100);
  sensor2 = readSensor(3);
  Serial.println("\nMoisture2: " + String(sensor2) + "%");
  delay(100);


  postEvt(evtTopic, HubId, "Moisture_Status", HubId + "_" + DevicesName[0], String(sensor1));
  delay(100);
  postEvt(evtTopic, HubId, "Moisture_Status", HubId + "_" + DevicesName[1], String(sensor2));
  delay(100);

  oldSensor1 = sensor1;
  oldSensor2 = sensor2;

  //  count++;
  //  EEPROMmemory.write(100, String(count) + "#");
  //  Added DeepSleep

  OtaRequest();
  delay(30000);

}

//Moisture
float readSensor (const byte which)
{
  digitalWrite (addressA, (which & 1) ? HIGH : LOW);
  digitalWrite (addressB, (which & 2) ? HIGH : LOW);
  digitalWrite (addressC, (which & 4) ? HIGH : LOW);

  return sensorReading();
}

int sensorReading() {
  int raw = analogRead(sensorPin);
  Serial.println(raw);
  int per = percentValue(raw);
  return per;

}
int percentValue(int val) {
  int percent = 0;
  percent = map(val, 0, 1023 , 0, 100);
  int mPercent = (100 - percent);
  return mPercent;
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
    evtPayload[(evtPayload.length() - 1)] = ']';

  Serial.print("\nPublish Packet: ");
  Serial.println(evtPayload);

  String check = "Not Ok";
  byte postCount = 0;
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
    postCount++;
    if (postCount > 3)
      ESP.restart();
  }
}

int counter = 0, connCount = 0;
void mqttConnect()
{
  if (!mqttClient.connected()) {
    Serial.println("mqttClient not connect");
    Serial.println("Reconnecting MQTT client to : " + server);
    digitalWrite(POWER_LED_PIN, LOW);
    counter = 0;
    while (!mqttClient.connect((char*) clientName.c_str(), (char*) authMethod.c_str(), (char*) authToken.c_str())) {
      Serial.print(".");
      counter++;
      if (counter > 10) {
        if (WiFi.status() != WL_CONNECTED)
          WIFI.APWithSTAWithoutRestart(HUB_SSID, HUB_PASSWORD, 1, WIFI_SSID, WIFI_PASSWORD);
        connCount++;
        if (connCount > 10)
          ESP.restart();
        break;
      }
    }
    initManagedDevice(cmdTopic);
    initManagedDevice(cmdTopic2);
    initManagedDevice(cmdTopic3);
    initManagedDevice(cmdTopic4);
    digitalWrite(POWER_LED_PIN, HIGH);
  }
  if (counter != 11)
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
    AuthenticationToken = loginToServer(HubId, DeviceMac);
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
