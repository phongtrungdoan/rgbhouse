
#include "DHT.h"          // Thu vien dht
#include <ESP8266WiFi.h>  // Thu vien ket noi wifi
#include <PubSubClient.h> // Thu vien mqtt
#include <Ticker.h>       // Thu vien timer
#include <Servo.h>        //Thu vien Servo

Servo myservo; 


#define BUTTON          0   // Den 1
#define BUTTON1         2   // Den 2
#define BUTTON2         15  // Den 3
#define FAN             16  // Quat
#define ARLAM           4   //CANH BAO
#define MOTOR           5   // Motor                             
#define FLAME           13  // Cam bien lua                              
#define DHTPIN          14  // Dht


#define DHTTYPE         DHT11                // Loai cam bien

#define MQTT_CLIENT     "RGBHouse"           // Client id. Thay doi voi moi module
#define MQTT_SERVER     "99.99.99.61"        // mqtt server
#define MQTT_PORT       1883                 // mqtt port
#define MQTT_TOPIC      "wemosd1"            // mqtt topic (Thay doi voi moi module)
#define MQTT_USER       "admin"              // mqtt user
#define MQTT_PASS       "123456"             // mqtt password

#define WIFI_SSID       "ssid"               // wifi ssid
#define WIFI_PASS       "password"           // wifi password

#define VERSION    "\n\n------------------  RGB House  -------------------"

DHT dht(DHTPIN, DHTTYPE, 11);                           

extern "C" { 
  #include "user_interface.h" 
}

bool sendStatusLight1 = false;
bool sendStatusLight2 = false;
bool sendStatusLight3 = false;
bool sendStatusFan = false;
bool sendStatusAlarm = false;
bool sendStatusMotor = false;
bool sendStatusFlame = false;
bool requestRestart = false;
bool tempReport = false;

int kUpdFreq = 1;
int kRetries = 10;

unsigned long TTasks;
unsigned long count = 0;
bool doorStatus = false;
bool FlameStatus = false;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient, MQTT_SERVER, MQTT_PORT);
Ticker btn_timer;

void callback(const MQTT::Publish& pub) {
  if (pub.payload_string() == "stat") {
  }
  else if (pub.payload_string() == "light1_off") {
    digitalWrite(BUTTON, LOW);
    sendStatusLight1 = true;
  }
  else if (pub.payload_string() == "light1_on") {
    digitalWrite(BUTTON, HIGH);
    sendStatusLight1 = true;
  }
  else if (pub.payload_string() == "light2_off") {
    digitalWrite(BUTTON1, LOW);
    sendStatusLight2 = true;
  }
  else if (pub.payload_string() == "light2_on") {
    digitalWrite(BUTTON1, HIGH);
    sendStatusLight2 = true;
  }
  else if (pub.payload_string() == "light3_off") {
    digitalWrite(BUTTON2, LOW);
    sendStatusLight3 = true;
  }
  else if (pub.payload_string() == "light3_on") {
    digitalWrite(BUTTON2, HIGH);
    sendStatusLight3 = true;
  }
  else if (pub.payload_string() == "fan_on") {
    digitalWrite(FAN, LOW);
    sendStatusFan = true;
  }
  else if (pub.payload_string() == "fan_off") {
    digitalWrite(FAN, HIGH);
    sendStatusFan = true;
  }
  else if (pub.payload_string() == "alarm_on") {
    digitalWrite(ARLAM, LOW);
    sendStatusAlarm = true;
  }
  else if (pub.payload_string() == "alarm_off") {
    digitalWrite(ARLAM, HIGH);
    sendStatusAlarm = true;
  }
  else if (pub.payload_string() == "motor_on") {
    digitalWrite(MOTOR, LOW);
    sendStatusMotor = true;
  }
  else if (pub.payload_string() == "motor_off") {
    digitalWrite(MOTOR, HIGH);
    sendStatusMotor = true;
  }
  else if (pub.payload_string() == "flame_on") {
  }
  else if (pub.payload_string() == "flame_off") {
  }
  else if (pub.payload_string() == "reset") {
    requestRestart = true;
  }
  else if (pub.payload_string() == "temp") {
    tempReport = true;
  }
}

void setup() {
  pinMode(BUTTON, OUTPUT);
  pinMode(BUTTON1, OUTPUT);
  pinMode(BUTTON2, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(ARLAM, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  digitalWrite(BUTTON, LOW);
  digitalWrite(BUTTON1, LOW);
  digitalWrite(BUTTON2, LOW);
  digitalWrite(FAN, HIGH);
  digitalWrite(ARLAM, HIGH);
  digitalWrite(MOTOR, HIGH);
  pinMode(FLAME, INPUT_PULLUP);
  
  mqttClient.set_callback(callback);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.begin(115200);
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
  getTemp();
}

void loop() { 
  mqttClient.loop();
  timedTasks();
  checkStatus();
  if (tempReport) {
    getTemp();
  }
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

void checkStatus() {
  if (sendStatusLight1 == true) {
    if(digitalRead(BUTTON) == HIGH)  
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "light1_on").set_retain().set_qos(2));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "light1_off").set_retain().set_qos(2));
   sendStatusLight1 = false;
  }
  if (sendStatusLight2 == true) {
    if(digitalRead(BUTTON1) == HIGH)
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "light2_on").set_retain().set_qos(2));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "light2_off").set_retain().set_qos(2));
   sendStatusLight2 = false;
  }
  if (sendStatusLight3 == true){  
   if(digitalRead(BUTTON2) == HIGH)
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "light3_on").set_retain().set_qos(2));
   else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "light3_off").set_retain().set_qos(2)); 
    sendStatusLight3 = false;
  }
  if (sendStatusFan == true){
    if(digitalRead(FAN) == HIGH)  
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "fan_off").set_retain().set_qos(2));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "fan_on").set_retain().set_qos(2));
    sendStatusFan = false;
  }
  if (sendStatusAlarm == true){
    if(digitalRead(ARLAM) == HIGH)
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "alarm_off").set_retain().set_qos(2));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "alarm_on").set_retain().set_qos(2));
    sendStatusAlarm = false;
    }
  if (sendStatusMotor == true){
    if(digitalRead(MOTOR) == HIGH)  
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "motor_off").set_retain().set_qos(2));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "motor_on").set_retain().set_qos(2));
    sendStatusMotor = false;
  }
  if(digitalRead(FLAME) == false)
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "flame_on").set_retain().set_qos(2));
    else
      mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/stat", "flame_off").set_retain().set_qos(2));
  
  if (requestRestart) {
    ESP.restart();
  }
}

void getTemp() {
  Serial.print("DHT read . . . . . . . . . . . . . . . . . ");
  float dhtH, dhtT;
  char message_buff[60];
  dhtH = dht.readHumidity();
  dhtT = dht.readTemperature();
  if (isnan(dhtH) || isnan(dhtT)) {
    mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/debug","\"DHT Read Error\"").set_retain().set_qos(1));
    Serial.println("ERROR");
    tempReport = false;
    return;
  }
  String pubString = "{\"Temp\": "+String(dhtT)+", "+"\"Humidity\": "+String(dhtH) + "}";
  pubString.toCharArray(message_buff, pubString.length()+1);
  mqttClient.publish(MQTT::Publish(MQTT_TOPIC"/temp", message_buff).set_retain().set_qos(1));
  Serial.println("OK");
  tempReport = false;
}

void timedTasks() {
  if ((millis() > TTasks + (kUpdFreq*1000)) || (millis() < TTasks)) { 
    TTasks = millis();
    tempReport = true;
    checkConnection();
  }
}
