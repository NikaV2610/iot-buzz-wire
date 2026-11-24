import json
import sys
import time

import cv2
import numpy as np
import paho.mqtt.client as mqtt

MQTT_HOST = "broker.emqx.io"
MQTT_PORT = 1883
# Must match "PositionTopic" in Hotwire.Web appsettings.json
MQTT_TOPIC = "hotwire/iot-buzz-wire/test/position"

SEND_INTERVAL = 0.1  # 10 Nachrichten pro Sekunde

# Camera number: 0 = first camera, 2 = external Logi C270 on Andi's laptop.
# Other camera: python camera_tracker.py 1
CAMERA_INDEX = int(sys.argv[1]) if len(sys.argv) > 1 else 2

# Jump filter: a detection far away from the last good position is ignored
# (e.g. skin or something red in the background for one moment).
# It is only accepted when it stays there for several frames.
MAX_JUMP = 60          # pixels per frame
JUMP_CONFIRM = 5       # frames the new position must be stable


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

camera = cv2.VideoCapture(CAMERA_INDEX, cv2.CAP_DSHOW)  # DSHOW works better on Windows
last_send = 0.0
last_good = None       # last accepted marker position
jump_count = 0         # frames in a row that were too far away

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

        # Red marker: red is at both ends of the hue range (0-7 and 170-180),
        # so two masks are combined. Measured: red tip hue 1-5,
        # skin hue 11-23 -> hue up to 7 and saturation >= 140 ignore skin.
        mask_low = cv2.inRange(
            hsv,
            np.array([0, 140, 50]),
            np.array([7, 255, 255])
        )
        mask_high = cv2.inRange(
            hsv,
            np.array([170, 140, 50]),
            np.array([180, 255, 255])
        )
        mask = mask_low | mask_high

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

        # Jump filter (see MAX_JUMP above)
        if x is not None and last_good is not None:
            jump = ((x - last_good[0]) ** 2 + (y - last_good[1]) ** 2) ** 0.5
            if jump > MAX_JUMP:
                jump_count += 1
                if jump_count < JUMP_CONFIRM:
                    x = None   # ignore this frame
                    y = None
            else:
                jump_count = 0
        if x is not None:
            last_good = (x, y)
            jump_count = 0

        now = time.monotonic()

        # Only send when the marker is visible: the web app needs numbers
        # for x/y, "null" would be rejected.
        if x is not None and client.is_connected() and now - last_send >= SEND_INTERVAL:
            height, width = frame.shape[:2]

            payload = {
                "deviceId": "camera",
                "x": x,
                "y": y,
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