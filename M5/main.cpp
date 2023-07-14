#if 1
#include <M5Core2.h>
#include "binaryttf.h"
#include "OpenFontRender.h"
#include <WiFi.h> //追加したンゴ
#include <PubSubClient.h>//追加したンゴ

#include "Servo.h"

OpenFontRender render;

constexpr char* ssid = "SSID";//追加したンゴ
constexpr char* password = "PASSWORD";//追加したンゴ
constexpr char* mqttServer = "mqtt.eclipseprojects.io";//追加したンゴ
constexpr int mqttPort = 1883;//追加したンゴ
constexpr char* mqttUser = "";//追加したンゴ
constexpr char* mqttPassword = "";//追加したンゴ

String is_home = "0";//追加したンゴ
String itemLocation = "宅配ロッカー";//追加したンゴ

// MQTTクライアント//追加したンゴ
WiFiClient wifiClient;//追加したンゴ
PubSubClient mqttClient(wifiClient);//追加したンゴ

bool wifi_conected = false;

M5Servo servo;

//#define DEBUG

enum STATE{
	TOP,
	DELIVERY,
	SALES,
	GUEST,
	MESSAGE,
};

struct delivery_data{
	bool isContactless;
	String place;
};

int u8len(const char *str){
    int count = 0;  // 文字数のカウント用
    while (*str != '\0') {
        if ((*str & 0xC0) != 0x80) { count++; }  // 
        str++;
    }
    return count;
}

// コールバック関数: メッセージ受信時に呼び出される//追加したンゴ
void callback(char* topic, byte* payload, unsigned int length) {
	// ペイロードを文字列に変換
	String message = "";
	for (int i = 0; i < length; i++) {
		message += (char)payload[i];
	}
	
	// トピックに応じて処理を実行
	if (strcmp(topic, "isHome") == 0) {
		is_home = message;
		Serial.print("isHome: ");
		Serial.println(is_home);
	} else if (strcmp(topic, "itemLocation") == 0) {
		itemLocation = message;
		Serial.print("itemLocation: ");
		Serial.print(itemLocation);
		Serial.print(" ");
		Serial.println(u8len(itemLocation.c_str()));
	}
}

void publish(String topic, String message){
  mqttClient.publish(topic.c_str(), message.c_str());
}

void setupWiFi() { //追加したンゴ
	int c = 5;
	Serial.print("Connecting to WiFi...");
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED && c>0) {
		delay(500);
		Serial.print(".");
		c--;
	}
	if(WiFi.status() != WL_CONNECTED){
		Serial.println("WiFi connection failed");
		wifi_conected = false;
	}else{
		Serial.println("WiFi connected");
		wifi_conected = true;
	}
}

void reconnect() { //追加したンゴ
	if(!wifi_conected){
		Serial.println("MQTT connection skipped");
		return;
	}
	int c = 5;
	while (!mqttClient.connected() && c>0) {
		Serial.print("Connecting to MQTT...");
		if (mqttClient.connect("M5StickC", mqttUser, mqttPassword)) {
			Serial.println("MQTT connected");
			mqttClient.subscribe("isHome");
			mqttClient.subscribe("itemLocation");
		} 
		else {
			Serial.print("Failed, rc=");
			Serial.print(mqttClient.state());
			Serial.println(" Retrying in 3 seconds...");
			delay(3000);
			c--;
		}
	}
	if(c==0){
		Serial.println("MQTT connection failed");
		wifi_conected = false;
	}
}


//家にいるかを取得する関数
bool isHome(){
	//return rand() % 2 == 0 ? true : false;
	return is_home == "0" ? false : true;
}

//宅配データを取得する関数
delivery_data get_delivery_data(){
	delivery_data data;
	data.isContactless = !isHome();
	data.place = itemLocation;
	return data;
}

constexpr uint16_t RGB(int r, int g, int b){
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void drawString(const String& str, int x, int y, int size, uint16_t color = TFT_BLACK){
	//M5.Lcd.setTextSize(size);
	//M5.Lcd.drawString(str, x, y, 1);

	render.setCursor(x,y);
	render.setFontSize(size);
	render.setFontColor(color);

	render.printf(str.c_str());
}

void button(const String& str, int x){
	int l = str.length() / 3;
	int _x = x - (l * 30) / 2;
	constexpr int EDGE = 2;
	constexpr int h = 200;
	constexpr uint16_t COLOR = RGB(64, 64, 64);
	//M5.Lcd.setTextColor(WHITE, COLOR);
	M5.Lcd.fillRect(_x - EDGE, h - EDGE, l * 30 + 2 * l + EDGE * 2, 30 + 2 + EDGE * 2, COLOR);
	drawString(str, x, h + 12, 24,TFT_WHITE);
}

void buttonA(const String& str){
	button(str,67);
}
void buttonB(const String& str){
	button(str,160);
}
void buttonC(const String& str){
	button(str,252);
}

void message_display();

ulong record(int* buff){
	constexpr int SAMPLING_FREQUENCY = 40000;
	constexpr uint PERIOD = 1e6 / SAMPLING_FREQUENCY;
	constexpr ulong SAMPLES = 15 * SAMPLING_FREQUENCY;
	M5.Lcd.setTextColor(BLACK,WHITE);
	delay(200);

	//memset(buff, 0, sizeof(int) * SAMPLES);
	M5.Lcd.fillRect(0, 0, 320, 16, WHITE);
	ulong c = 0;
    for (ulong i = 0; i <= SAMPLES; i++) {
		//message_display();
		M5.update();
		
		if(c==SAMPLING_FREQUENCY){
			M5.Lcd.progressBar(0, 0, 320, 16, (i*100.0/SAMPLES));
			c = 0;
		}
        //buff[i] = analogRead(36);

		if(M5.BtnB.wasPressed()){
			break;
		}
        delayMicroseconds(PERIOD);
		c++;
    }
	M5.Lcd.progressBar(0, 0, 320, 16, 100);
	return SAMPLES;
}

void upload(int* data, int size){
	M5.Lcd.setTextColor(BLACK,WHITE);
#ifdef DEBUG
	M5.Lcd.setCursor(0,10);
	M5.Lcd.print("upload");
#endif
	delay(200);
	M5.Lcd.fillRect(0, 0, 320, 16, WHITE);
    for (int i = 0; i <= 100; i++) {
		M5.Lcd.progressBar(0, 0, 320, 16, i);
        delay(10);
    }
}

void top_display(){
	M5.Lcd.fillScreen(WHITE);
	//M5.Lcd.setTextColor(BLACK);
	
	drawString("ようこそ",160,20,48);
	drawString("ご用件は？",160,72,48);
	
	buttonA("配達");
	buttonB("来客");
	buttonC("その他");
}

void delivery_display(const delivery_data& data){
	M5.Lcd.fillScreen(WHITE);
	M5.Lcd.setTextColor(BLACK,WHITE);
	if(data.isContactless){
		drawString(data.place,160,70,min((int)(60 * 5.0 / u8len(data.place.c_str())), 60));
		drawString("においてください",160,140,24);
		buttonA("伝言");
		buttonC("完了");

		servo.write(90);
	}
	else{
		drawString("受け取ります",160,70,36);
		drawString("しばらくお待ち下さい",160,110,32);
		buttonB("完了");
	}
	
}

void guest_display(){
	M5.Lcd.fillScreen(WHITE);
	M5.Lcd.setTextColor(BLACK,WHITE);
	if(isHome()){
		drawString("しばらくお待ち下さい",160,72,32);
		buttonB("完了");
	}
	else{
		drawString("只今留守です",160,72,32);

		buttonA("伝言");
		buttonC("完了");
	}
}

void sales_display(){
	M5.Lcd.fillScreen(WHITE);
	M5.Lcd.setTextColor(BLACK,WHITE);
	drawString("只今対応できません",160,72,32);
	buttonB("完了");
}

void message_display(){
	M5.Lcd.fillScreen(WHITE);
	M5.Lcd.setTextColor(BLACK,WHITE);
	drawString("メッセージをどうぞ",160,72,32);
	buttonB("完了");
}

void top_update(STATE& state){
	M5.Lcd.setTextColor(BLACK,WHITE);
#ifdef DEBUG
	M5.Lcd.setCursor(0,0);
	M5.Lcd.print("top");
#endif

	//ボタンAが押されたら
	if(M5.BtnA.wasPressed()){
		//stateをDELIVERYに変更
		state = DELIVERY;
		delivery_display(get_delivery_data());
		publish("visit", "delivery");
	}
	//ボタンBが押されたら
	if(M5.BtnB.wasPressed()){
		//stateをGUESTに変更
		state = GUEST;
		guest_display();
		publish("visit", "guest");
	}
	//ボタンCが押されたら
	if(M5.BtnC.wasPressed()){
		//stateをSALESに変更
		state = SALES;
		sales_display();
		publish("visti", "sales");
	}
}

void delivery_update(STATE& state){
	M5.Lcd.setTextColor(BLACK,WHITE);
#ifdef DEBUG
	M5.Lcd.setCursor(0,0);
	M5.Lcd.print("delivery");
#endif

	if(isHome()){
		if(M5.BtnB.wasPressed()){
			state = TOP;
			top_display();
		}
	}
	else{
		//ボタンAが押されたら
		if(M5.BtnA.wasPressed()){
			servo.write(0);
			//stateをMESSAGEに変更
			state = MESSAGE;
			message_display();
		}

		//ボタンCが押されたら
		if(M5.BtnC.wasPressed()){
			servo.write(0);
			//stateをTOPに変更
			state = TOP;
			top_display();
		}
	}
}

void guest_update(STATE& state){
	M5.Lcd.setTextColor(BLACK,WHITE);
#ifdef DEBUG
	M5.Lcd.setCursor(0,0);
	M5.Lcd.print("guest");
#endif

	if(isHome()){
		if(M5.BtnB.wasPressed()){
			state = TOP;
			top_display();
		}
	}
	else{
		//ボタンAが押されたら
		if(M5.BtnA.wasPressed()){
			//stateをMESSAGEに変更
			state = MESSAGE;
			message_display();
		}

		//ボタンCが押されたら
		if(M5.BtnC.wasPressed()){
			//stateをTOPに変更
			state = TOP;
			top_display();
		}
		
	}
}

void salse_update(STATE& state){
	M5.Lcd.setTextColor(BLACK,WHITE);
#ifdef DEBUG
	M5.Lcd.setCursor(0,0);
	M5.Lcd.print("salse");
#endif

	if(M5.BtnB.wasPressed()){
		state = TOP;
		top_display();
	}
}

void message_update(STATE& state){
	M5.Lcd.setTextColor(BLACK,WHITE);
#ifdef DEBUG
	M5.Lcd.setCursor(0,0);
	M5.Lcd.print("message");
#endif

	//M5.Lcd.setTextColor(BLACK,WHITE);
	//drawString("メッセージをどうぞ",160-9*15,50,2);
	//int buff[SAMPLES];
	ulong size = record(nullptr);
	//録音データを送信
	upload(nullptr, size);
	state = TOP;
	top_display();
}

//setup()関数はM5Stackがスタートした最初に読み込まれる
void setup(){
	Serial.begin(115200);
	// M5Stack オブジェクトの初期化
	M5.begin();

	//Power chipがgpio21, gpio22, I2Cにつながれたデバイスに接続される。
	//バッテリー動作の場合はこの関数を読んでください（バッテリーの電圧を調べるらしい）
	//M5.Power.begin();

	//M5.Lcd.setBrightness(200); //バックライトの明るさを0（消灯）～255（点灯）で制御

	M5.Lcd.fillScreen(TFT_WHITE);
	
	
	setupWiFi();
	if(wifi_conected){	
		mqttClient.setServer(mqttServer, mqttPort);
		mqttClient.setCallback(callback);
	}


	//M5.Lcd.setFreeFont(&unicode_24px);
	//M5.Lcd.setTextDatum(TC_DATUM);
	
	//Serial.println("font loaded");

	render.setSerial(Serial);
	render.showFreeTypeVersion(); // print FreeType version
	render.showCredit();		  // print FTL credit

	if (render.loadFont(binaryttf, sizeof(binaryttf)))
	{
		Serial.println("Render initialize error");
		return;
	}

	render.setDrawer(M5.Lcd);
	render.setAlignment(Align::MiddleCenter);
	delay(30);

	servo.attach(26);
	servo.write(0);

	top_display();
}

void loop() {
	if (!mqttClient.connected() && wifi_conected) {
		reconnect();
	}
	if(wifi_conected){
		mqttClient.loop();
	}

	//M5Stackのボタンの状態を更新
	M5.update();

	//STATEを保持する変数
	static STATE state = TOP;

	//stateによってswitch文で処理を分岐
	switch(state){
		case TOP:
			top_update(state);
			break;
		case DELIVERY:
			delivery_update(state);
			break;
		case SALES:
			salse_update(state);
			break;
		case GUEST:
			guest_update(state);
			break;
		case MESSAGE:
			message_update(state);
			break;
	}

	//stateを画面左上に表示
#ifdef DEBUG
	M5.Lcd.setTextColor(BLACK,WHITE);
	//M5.Lcd.setCursor(0,220);
	drawString(String(state).begin(),0,220,2);
#endif
}
#else
// ---------------------------------------------------------------
/*
	display02.ino
	
						Aug/31/2021
*/
// ---------------------------------------------------------------
#include <M5Core2.h>
#include "CUF_24px.h"

// ---------------------------------------------------------------
void setup()
{
  M5.begin();
  M5.Lcd.setFreeFont(&unicode_24px);
  M5.Lcd.setTextDatum(TC_DATUM);
}

// ---------------------------------------------------------------
void loop()
{
  M5.Lcd.fillScreen(0);
  drawString("ようこそ\nご用件は?", 160, 60, 1);
  delay(2000);
  drawString("国一番の、美人は誰じゃ", 160, 90, 1);  
  delay(4000);
  
  M5.Lcd.fillScreen(0);
  drawString("ああ、おきさま、あなたさまこそ", 160, 120, 1);
  delay(2000);
  drawString("国一番の、ごきりょうよし", 160, 160, 1);
  delay(4000);

// ---------------------------------------------------------------
}

#endif