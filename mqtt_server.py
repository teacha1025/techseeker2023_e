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
    # print(dict['location'] + "の天気")

    soup_tdy = s.select('.today-weather')[0]
    soup_tmr = s.select('.tomorrow-weather')[0]

    dict["today"] = forecast2dict(soup_tdy)
    # print("雨" in dict["today"]["forecasts"][0]["weather"])
    return dict["today"]
    # dict["tomorrow"] = forecast2dict(soup_tmr)

    # #JSON形式で出力
    # print(json.dumps(dict, ensure_ascii=False)["forecasts"])
    # data = json.loads(json_data)

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
    # print("=====" + data["date"] + "=====")

    # ## 取得
    weather           = soup.select('.weather-telop')[0]
    high_temp         = soup.select("[class='high-temp temp']")[0]
    high_temp_diff    = soup.select("[class='high-temp tempdiff']")[0]
    low_temp          = soup.select("[class='low-temp temp']")[0]
    low_temp_diff     = soup.select("[class='low-temp tempdiff']")[0]
    rain_probability  = soup.select('.rain-probability > td')
    wind_wave         = soup.select('.wind-wave > td')[0]

    # ## 格納
    data["forecasts"] = []
    forecast = {}
    forecast["weather"] = weather.text.strip()
    forecast["high_temp"] = high_temp.text.strip()
    forecast["high_temp_diff"] = high_temp_diff.text.strip()
    forecast["low_temp"] = low_temp.text.strip()
    forecast["low_temp_diff"] = low_temp_diff.text.strip()
    every_6h = {}
    for i in range(4):
        time_from = 0+6*i
        time_to   = 6+6*i
        itr       = '{:02}-{:02}'.format(time_from,time_to)
        every_6h[itr] = rain_probability[i].text.strip()
    forecast["rain_probability"] = every_6h
    forecast["wind_wave"] = wind_wave.text.strip()

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

###########################################################
