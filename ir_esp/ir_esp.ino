#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* mqttServer = "mqtt.eclipseprojects.io";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // メッセージを受信したときの処理
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

constexpr int N = 24;
constexpr int CH = 2;

constexpr int threshold = 160;

bool flag[CH];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  memset(flag,false,sizeof(bool)*CH);

  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

int i=0;
void f(int id){
  static int ary[CH][N];

  ary[id-12][i] = analogRead(id);

  float a = 0;
  for(int j = 0; j < N; j++){
    a+=ary[id-12][j];
  }
  //Serial.print(ary[i]);
  //Serial.print(' ');
  //Serial.print((float)a/N);
  //Serial.print(' ');
  flag[id-12] = (float)a/N>=threshold;
  //Serial.print(flag[id-12]?1:0);
}

constexpr unsigned long TIME_THRESHOLD0=2000;
constexpr unsigned long TIME_THRESHOLD1=1500;
constexpr unsigned long TIME_THRESHOLD_MIN=20;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  static bool flag0=false,flag1=false,send_flag=false;
  static unsigned long timestamp0,timestamp1;
  unsigned long now = millis();
  //Serial.print(now);
  //Serial.print(' ');
  f(12);
  //Serial.print(' ');
  f(13);
  //Serial.print(' ');
  i++;
  i%=N;

  if(flag1){
    if((now-timestamp1)>TIME_THRESHOLD1){
      flag0=flag1=send_flag=false;
    }
  }
  else{
    //室内側だけフラグが立った
    if(!flag0&&flag[0]&&!flag[1]){
      timestamp0 = now;
      flag0=true;
    }    
    //室内側だけフラグ立ってて一定時間経過するとタイムアウト
    else if(flag0&&(now-timestamp0)>TIME_THRESHOLD0){
      flag0=false;
      flag1=true;
      timestamp1=now;
    }
    
    else if(flag0&&flag[1]&&(now-timestamp0)>TIME_THRESHOLD_MIN){
      //フラグ成立
      //ラズパイに送信
      flag1=true;
      timestamp1=now;
      //Serial.println("送信");
      const char* message = "passed message";
      client.publish("passed", message);
      send_flag=true;
    }
  }
//  Serial.print(flag0?1:0);
//  Serial.print(" ");
//  Serial.print(flag1?1:0);
//  Serial.print(" ");
//  Serial.print(send_flag?1:0);
//  Serial.print(" ");
//  Serial.print(timestamp0);
//  Serial.print(" ");
//  Serial.print(timestamp1);
//  Serial.println("");
  delay(3);
}
