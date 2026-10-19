import json
import time

import cv2
import numpy as np
import paho.mqtt.client as mqtt

MQTT_HOST = "broker.emqx.io"
MQTT_PORT = 1883
MQTT_TOPIC = "iot-buzz-wire/NikaV2610/camera/position"

SEND_INTERVAL = 0.1  # 10 Nachrichten pro Sekunde


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("MQTT verbunden.")
    else:
        print(f"MQTT-Verbindung abgelehnt: {reason_code}")


def on_connect_fail(client, userdata):
    print("MQTT nicht erreichbar. Neuer Versuch folgt.")


client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2
)
client.on_connect = on_connect
client.on_connect_fail = on_connect_fail
client.reconnect_delay_set(min_delay=1, max_delay=30)

camera = cv2.VideoCapture(0)
last_send = 0.0

try:
    if not camera.isOpened():
        raise RuntimeError("Kamera konnte nicht geöffnet werden.")

    # MQTT läuft im Hintergrund, damit das Kamerabild weiterläuft.
    client.connect_async(MQTT_HOST, MQTT_PORT, keepalive=60)
    client.loop_start()

    while True:
        success, frame = camera.read()

        if not success:
            print("Kein Kamerabild empfangen.")
            break

        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

        mask = cv2.inRange(
            hsv,
            np.array([100, 120, 30]),
            np.array([130, 255, 255])
        )

        kernel = np.ones((3, 3), dtype=np.uint8)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

        contours, _ = cv2.findContours(
            mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
        )

        # Für jedes Bild zunächst "nicht erkannt" annehmen.
        x = None
        y = None

        if contours:
            largest = max(contours, key=cv2.contourArea)

            if cv2.contourArea(largest) > 10:
                moments = cv2.moments(largest)

                if moments["m00"] != 0:
                    x = int(moments["m10"] / moments["m00"])
                    y = int(moments["m01"] / moments["m00"])

                    cv2.drawContours(
                        frame, [largest], -1, (0, 255, 0), 2
                    )
                    cv2.circle(frame, (x, y), 6, (0, 255, 0), -1)
                    cv2.putText(
                        frame,
                        f"X: {x}  Y: {y}",
                        (20, 35),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.8,
                        (0, 255, 0),
                        2,
                    )

        now = time.monotonic()

        if client.is_connected() and now - last_send >= SEND_INTERVAL:
            height, width = frame.shape[:2]

            payload = {
                "x": x,
                "y": y,
                "visible": x is not None,
                "width": width,
                "height": height,
            }

            result = client.publish(
                MQTT_TOPIC,
                json.dumps(payload),
                qos=0,
                retain=False,
            )

            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"MQTT zum Senden übergeben: {payload}")

            last_send = now

        cv2.imshow("Buzz Wire Kamera - Q zum Beenden", frame)
        cv2.imshow("Erkannte blaue Bereiche", mask)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

finally:
    camera.release()
    cv2.destroyAllWindows()
    client.disconnect()
    client.loop_stop()