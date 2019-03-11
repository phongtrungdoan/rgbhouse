#include <SPI.h>               
#include <RFID.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

Servo myservo; 
#define RST_PIN         5
#define SS_PIN          15


RFID rfid(SS_PIN, RST_PIN);


#define MQTT_CLIENT     "RGBHouse1"           
#define MQTT_SERVER     "99.99.99.61"    
#define MQTT_PORT       1883                         
#define MQTT_TOPIC      "nodemcu"          
#define MQTT_USER       "admin"                        
#define MQTT_PASS       "123456"                             

#define WIFI_SSID       "ssid"                          
#define WIFI_PASS       "pass"                           

#define VERSION    "\n\n------------------  RGB House  -------------------"

extern "C" { 
  #include "user_interface.h" 
}

bool requestRestart = false;

int kUpdFreq = 1;
int kRetries = 10;

unsigned long TTasks;
unsigned long count = 0;

int serNum[5];
int cards[][5] = {{147,88,73,36,166}};
#define GAS           16
#define SERVO         2
bool access = false;
bool sendStatusServo = false;
bool doorStatus = false;

unsigned char i, j;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient, MQTT_SERVER, MQTT_PORT);

void callback(const MQTT::Publish& pub) {
  if (pub.payload_string() == "stat") {
  }
  else if (pub.payload_string() == "rfid_on") {
  }
  else if (pub.payload_string() == "rfid_off") {
  }
  else if (pub.payload_string() == "gas_on") {
  }
  else if (pub.payload_s
  
  tring() == "gas_off") {
  }
    else if (pub.payload_string() == "servo_on") {
    myservo.write(95);
    doorStatus = true;
    sendStatusServo = true;
  }
  else if (pub.payload_string() == "servo_off") {
    myservo.write(0);
    doorStatus = false;
    sendStatusServo = true;
  }
  else if (pub.payload_string() == "reset") {
    requestRestart = true;
  }
}

void setup() {


  pinMode(GAS, INPUT_PULLUP);
  pinMode(SERVO, OUTPUT);
  myservo.attach(SERVO);
  mqttClient.set_callback(callback);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.begin(115200);
  SPI.begin();      
  rfid.init();   
  Serial.println(VERSION);
  Serial.print("\nESP ChipID: ");
  Serial.print(ESP.getChipId(), HEX);
  Serial.print("\nConnecting to "); Serial.print(WIFI_SSID); Serial.print(" Wifi"); 
  while ((WiFi.status() != WL_CONNECTED) && kRetries --) {
    delay(500);
    Serial.print(" .");
  }
  if (WiFi.status() == WL_CONNECTED) {  
    Serial.println(" DONE");
    Serial.print("IP Address is: "); Serial.println(WiFi.localIP());
    Serial.print("Connecting to ");Serial.print(MQTT_SERVER);Serial.print(" Broker . .");
    delay(500);
    while (!mqttClient.connect(MQTT::Connect(MQTT_CLIENT).set_keepalive(90).set_auth(MQTT_USER, MQTT_PASS)) && kRetries --) {
      Serial.print(" .");
      delay(1000);
    }
    if(mqttClient.connected()) {
      Serial.println(" DONE");
      Serial.println("\n----------------------------  Logs  ----------------------------");
      Serial.println();
      mqttClient.subscribe(MQTT_TOPIC);
    }
    else {
      Serial.println(" FAILED!");
      Serial.println("\n----------------------------------------------------------------");
      Serial.println();
    }
  }
  else {
    Serial.println(" WiFi FAILED!");
    Serial.println("\n----------------------------------------------------------------");
    Serial.println();
  }
}

void loop() { 
  mqttClient.loop();
  timedTasks();
  checkStatus();
  CheckRFID();
}


void checkConnection() {
  if (WiFi.status() == WL_CONNECTED)  {
    if (mqttClient.connected()) {
      Serial.println("mqtt broker connection . . . . . . . . . . OK");
    } 
    else {
      Serial.println("mqtt broker connection . . . . . . . . . . LOST");
      requestRestart = true;
    }
  }
  else { 
    Serial.println("WiFi connection . . . . . . . . . . LOST");
    requestRestart = true;
  }
}

void CheckRFID()
{ 
  int serNum[][5] = {{0,0,0,0,0}};
  if(rfid.isCard()){
        if(rfid.readCardSerial()){
            Serial.print(rfid.serNum[0]);
            Serial.print(" ");
            Serial.print(rfid.serNum[1]);
            Serial.print(" ");
            Serial.print(rfid.serNum[2]);
            Serial.print(" ");
            Serial.print(rfid.serNum[3]);
            Serial.print(" ");
            Serial.print(rfid.serNum[4]);
            Serial.println("");
            
            for(int x = 0; x < sizeof(cards); x++){
              for(int i = 0; i < sizeof(rfid.serNum); i++ ){
                  if(rfid.serNum[i] != cards[x][i]) {
                      access = false;
                      break;
                  } else {
                      access = true;
                  }
              }
              if(access) break;
            }
        }
       if(access){
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "rfid_on").set_retain().set_qos(1));
          delay(3000);
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "rfid_off").set_retain().set_qos(1));  
      } else {
          mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "rfid_off").set_retain().set_qos(1));     
       }        
    }      
    rfid.halt();
}

void checkStatus() {
  if(digitalRead(GAS) == false)
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "gas_on").set_retain().set_qos(1));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "gas_off").set_retain().set_qos(1));
   if (sendStatusServo == true){
    if(doorStatus == true)  
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "servo_on").set_retain().set_qos(1));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "servo_off").set_retain().set_qos(1));
    sendStatusServo = false;
  }
  if (requestRestart) {
    ESP.restart();
  }
}

void timedTasks() {
  if ((millis() > TTasks + (kUpdFreq*60000)) || (millis() < TTasks)) { 
    TTasks = millis();
    checkConnection();
  }
}
