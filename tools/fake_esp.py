"""Fake ESP: sends one complete game run over MQTT.

Use it to test the web app without the ESP32 and the wire.

    pip install paho-mqtt
    python tools/fake_esp.py              # card "12345", 3 collisions
    python tools/fake_esp.py 04A1B2C3 5   # own card UID, 5 collisions
"""
import json
import sys
import time

import paho.mqtt.client as mqtt

BROKER = "broker.emqx.io"
PORT = 1883
TOPIC = "hotwire/iot-buzz-wire/test/game"   # same as the ESP and the web app

# Event numbers (same as GameEventType in the web app)
START, STOP, COLLISION, RFID_ASSIGNED = 0, 1, 2, 4

rfid_tag = sys.argv[1] if len(sys.argv) > 1 else "12345"
collisions = int(sys.argv[2]) if len(sys.argv) > 2 else 3

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "fake-esp")
client.connect(BROKER, PORT)
client.loop_start()


def send(event_type, wait_s):
    message = json.dumps({
        "deviceId": "buzzwire-esp32c3",
        "eventType": event_type,
        "rfidTag": rfid_tag,
    })
    client.publish(TOPIC, message, qos=1).wait_for_publish()
    print("sent", message)
    time.sleep(wait_s)


send(RFID_ASSIGNED, 1)    # card scanned
send(START, 1)            # start contact
for _ in range(collisions):
    send(COLLISION, 0.8)  # wire touched
send(STOP, 1)             # stop contact

client.loop_stop()
client.disconnect()
print("Done. Check the scoreboard: http://localhost:5175")
