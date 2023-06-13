import paho.mqtt.client as mqtt

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
        user_input = input("Enter a message: ")
        if user_input == "rain":
            client.publish(mqtt_topic_weather, "rain")
            print("Published [topic/weather] rain")

# MQTTブローカーへの接続とイベントハンドラの設定
client.on_connect = on_connect
client.on_message = on_message
client.connect(broker_address, broker_port)

# メッセージの待機と処理
client.loop_forever()
