import paho.mqtt.client as mqtt
import re
import requests
from bs4 import BeautifulSoup
import json

def main(url):
    # bs4でパース
    s = soup(url)

    dict = {}

    # 予測地点
    l_pattern = r"(.+)の今日明日の天気"
    l_src = s.title.text
    dict['location'] = re.findall(l_pattern, l_src)[0]
    print(dict['location'] + "の天気")
    soup_tdy = s.select('.today-weather')[0]

    dict["today"] = forecast2dict(soup_tdy)
    return dict["today"]

def soup(url):
    r = requests.get(url)
    html = r.text.encode(r.encoding)
    return BeautifulSoup(html, 'lxml')

def forecast2dict(soup):
    data = {}

    # 日付処理
    d_pattern = r"(\d+)月(\d+)日\(([土日月火水木金])+\)"
    d_src = soup.select('.left-style')
    date = re.findall(d_pattern, d_src[0].text)[0]
    data["date"] = "%s-%s(%s)" % (date[0], date[1], date[2])
    print("=====" + data["date"] + "=====")

    # ## 取得
    weather           = soup.select('.weather-telop')[0]
    # ## 格納
    data["forecasts"] = []
    forecast = {}
    forecast["weather"] = weather.text.strip()
    data["forecasts"].append(forecast)

    print("天気              ： " + forecast["weather"])

    return data

################################################################################
################################################################################
# MQTTブローカーの接続情報
broker_address = "mqtt.eclipseprojects.io"
broker_port = 1883
mqtt_topic_is_rain = "topic/passed"
mqtt_topic_weather = "topic/weather"

# MQTTクライアントの作成
client = mqtt.Client()

# on_connectイベントハンドラ
def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker.")
    client.subscribe(mqtt_topic_is_rain)

# on_messageイベントハンドラ
def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8")
    print("Received message:", payload)

    if msg.topic == mqtt_topic_is_rain and payload == "passed":
        # user_input = input("Enter a message: ")　試験的にキーボード入力で検証した
        URL = 'https://tenki.jp/forecast/6/30/6200/27127/'
        # main(URL) returns ["雨 in 天気予報の情報"]
        if main(URL):
            client.publish(mqtt_topic_weather, "rain")
            print("Published [topic/weather] rain")

# MQTTブローカーへの接続とイベントハンドラの設定
client.on_connect = on_connect
client.on_message = on_message
client.connect(broker_address, broker_port)

# メッセージの待機と処理
client.loop_forever()
