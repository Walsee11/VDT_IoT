#include "WiFi.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define COLD_WHITE 18
#define WARM_WHITE 19
#define FADESPEED 20     // make this higher to slow down
#define ID_Device "123666888"

#define ssid "Phong 4"
#define password "44QppQ44"

const char* mqtt_server = "mqttvcloud.innoway.vn";
const char* mqttUser = "PMD";
const char* mqttPassword = "bR86Cw1tAFwY9kvgwxN7LkuJrpyEkv7O";
WiFiClient espClient;
PubSubClient client(espClient);

int control_white = 0;

void taskBlinkWhite(void *pvParameters) {
  for (;;) {
    if (control_white == 0) {
      analogWrite(WARM_WHITE, 256);
      vTaskDelay(pdMS_TO_TICKS(500)); // Thay thế delay(1000) bằng vTaskDelay
      analogWrite(WARM_WHITE, 0);
      vTaskDelay(pdMS_TO_TICKS(500)); // Thay thế delay(1000) bằng vTaskDelay
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "message/control/white") {
    Serial.print("Changing output to ");
     if (messageTemp.indexOf("on") != -1){
      control_white = 1;
      Serial.println("on white");
      analogWrite(WARM_WHITE, 256);
      send_message_mqtt("white","on");
    }
    else if(messageTemp.indexOf("off") != -1){
      control_white = 1;
      Serial.println("off while");
      analogWrite(WARM_WHITE, 0);
      send_message_mqtt("white","off");
    }
    else if(messageTemp.indexOf("blink") != -1){
      control_white = 0;
      Serial.println("blink white");
      send_message_mqtt("white","blink");
    }
    else if(messageTemp.indexOf("dim") != -1){
      int colonIndex = messageTemp.indexOf(':');
      String dim = messageTemp.substring(colonIndex);
      Serial.println("brightness");
      Serial.println(dim);
      analogWrite(WARM_WHITE, dim.toInt());
      String sent = "brightness-"+dim;
      send_message_mqtt("white", sent);
    }
  }
  
  Serial.println("Exit Call back");
}


void setup() {
  Serial.begin(115200);
  pinMode(COLD_WHITE, OUTPUT);
  analogWrite(COLD_WHITE, 256);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  Serial.println("\nConnecting to the Wifi network");

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
    Serial.print(".");
  }

  Serial.println("WiFi Connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  pinMode(WARM_WHITE, OUTPUT);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  xTaskCreate(taskBlinkWhite, "BlinkWhite", 1000, NULL, 1, NULL);
} 

void send_message_mqtt(String type, String status){
    StaticJsonDocument<100> jsonDocument;
    jsonDocument["id"] = ID_Device;
    jsonDocument["status"] = status;
    jsonDocument["type"] = type;
    char buffer[100];
    serializeJson(jsonDocument, buffer);
    client.publish("message/status/rgb", buffer);
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      analogWrite(COLD_WHITE, 0);
      Serial.println("connected to broker");
      client.subscribe("message/control/white");
      send_message_mqtt("white","on");
    } else {
      control_white = 1;
      analogWrite(WARM_WHITE, 0);
      analogWrite(COLD_WHITE, 256);
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(FADESPEED));
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}
