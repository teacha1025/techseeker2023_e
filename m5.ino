#include <WiFi.h>
#include <PubSubClient.h>
// #include <M5StickC.h>
#include <ArduinoJson.h>
#include <M5Stack.h>

const char* ssid = "ワシ、53歳";
const char* password = "w1111111";
const char* mqttServer = "mqtt.eclipseprojects.io";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* topicPassed = "topic/passed";
const char* topicWeather = "topic/weather";
const char* messageWeatherRain = "rain";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setTextSize(4);
  M5.Speaker.setVolume(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);

  Serial.begin(115200);
  setupWiFi();
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  
  M5.update();
  if(M5.BtnB.isPressed() == 1) {
    publishMessage(topicPassed, "passed");
  }

  delay(1000);
}

void setupWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("M5StickC", mqttUser, mqttPassword)) {
      Serial.println("MQTT connected");
      mqttClient.subscribe(topicWeather);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message [");
  Serial.print(topic);
  Serial.print("]: ");
  payload[length] = '\0';  // Add null terminator to payload
  Serial.println((char*)payload);

  if (strcmp(topic, topicWeather) == 0) {
    if (strcmp((char*)payload, messageWeatherRain) == 0) {
      displayRainyMessage();
    }
  }
}

void publishMessage(const char* topic, const char* message) {
  mqttClient.publish(topic, message);
  Serial.print("Published message [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
}

void displayRainyMessage() {
    M5.Lcd.setCursor(30, 30);
    M5.Speaker.tone(440, 200);
    M5.Lcd.print("It's rainy!\n\n Umbrella!");
    M5.Lcd.fillCircle(200, 200, 40, GREEN);
    delay(1000);
    M5.Speaker.mute();
    delay(2000);
    M5.Lcd.clear();
}
