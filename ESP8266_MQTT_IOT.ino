#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "ArduinoJson.h"
#include <NTPClient.h>
#include <WiFiUdp.h>  
#include "DHT.h"
#define DHTTYPE DHT11
const char *ssid = "Dat";   
const char *password = "123456789"; 
const int DHTPin = 5;
DHT dht(DHTPin, DHTTYPE);
#define senas 16
#define led 4
#define led2 0
#define mqtt_server "broker.mqttdashboard.com" // Thông tin về MQTT Broker
unsigned long lastMsg = 0;
const uint16_t mqtt_port = 1883; //Port TCP của MQTT broker
const int timezone = 7*3600; //timezone vietnam
const int senasout=0;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "vn.pool.ntp.org", timezone, 60000);
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
void setup() {
  pinMode(led, OUTPUT);
  pinMode(senas, INPUT);
  pinMode(led2, OUTPUT);
  dht.begin();
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  // gọi hàm callback để thực hiện các chức năng publish/subcribe
  client.setCallback(callback);
  // gọi hàm reconnect() để thực hiện kết nối lại với server khi bị mất kết nối
  reconnect();
    timeClient.begin();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // kết nối đến mạng Wifi
  WiFi.begin(ssid, password);
  // in ra dấu . nếu chưa kết nối được đến mạng Wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // in ra thông báo đã kết nối và địa chỉ IP của ESP8266
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(char* topic, byte* payload, unsigned int length) {
  //in ra tên của topic và nội dung nhận được từ kênh MQTT lens đã publish
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  // kiểm tra nếu dữ liệu nhận được từ topic ESP8266/LED_GPIO2/status là chuỗi "on"
  // sẽ bậtled GPIO2, nếu là chuỗi "off" sẽ tắt led GPIO2
  if ((char)payload[0] == 'O' && (char)payload[1] == 'N') //on
    digitalWrite(led, HIGH);
  else if ((char)payload[0] == 'O' && (char)payload[1] == 'F' && (char)payload[2] == 'F') //off
    digitalWrite(led, LOW);
  Serial.println();
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("ESP8266/LED/status");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(500);
    }
  }
}
void loop() {
  // kiểm tra nếu ESP8266 chưa kết nối được thì sẽ thực hiện kết nối lại
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if(digitalRead(senas)==0){
    digitalWrite(led2,LOW);
    }
    else{
    digitalWrite(led2,HIGH);
      }
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  String currentMonthName = months[currentMonth-1];
  int currentYear = ptm->tm_year+1900;
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
     char jsonStr[60];
     char jsonStr2[60];
     DynamicJsonDocument doc(1024);
     DynamicJsonDocument doc2(1024);
     JsonObject obj=doc.as<JsonObject>();
     JsonObject obj2=doc2.as<JsonObject>();
     float h = dht.readHumidity();
     float t = dht.readTemperature();
     static char temperatureTemp[7];
     static char humidityTemp[7];
     dtostrf(t, 4, 2, temperatureTemp);
     dtostrf(h, 4, 2, humidityTemp);
     doc["D"] = currentDate;
     doc["T"] = timeClient.getFormattedTime();
     doc["Tem"] = t;
     doc["Hum"]=h;
     serializeJson(doc,jsonStr);
     doc2["Tem"] = t;
     doc2["Hum"]=h;
     serializeJson(doc2,jsonStr2);
     unsigned long now = millis();
     if (now - lastMsg > 10000) {
      lastMsg = now;
      client.publish("ESP8266/home/TempHumd", jsonStr);
      Serial.print(jsonStr);
      Serial.println();
      client.publish("Son1239", jsonStr2);
      Serial.print(jsonStr2);
      Serial.println();
  }
}
