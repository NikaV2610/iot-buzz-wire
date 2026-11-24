# Smart Buzz Wire – Project Documentation

Internet of Things · Team: Andi, Arian, Fabian, Nika

## 1. Idea

A classic buzz wire game, extended with IoT:

- Players log in with an **RFID card**.
- Every touch of the wire is counted as a **collision**, the buzzer beeps.
- A **webcam** tracks a blue marker on the handle (live position).
- A **web app** stores every run and shows the scoreboard and a replay of the best run.

## 2. Architecture

```
ESP32-C3 (game)  --- game events --->  MQTT broker  --->  Web app (Hotwire.Web)  --->  Browser
Camera (Python)  --- positions  --->   broker.emqx.io      Blazor + SQLite             scoreboard, replay
```

All parts only talk via MQTT. Nobody calls anybody else directly.

| Part | Folder | Technology |
|---|---|---|
| Game firmware | `smart-buzz-wire/` | ESP32-C3 SuperMini, ESP-IDF, C |
| Camera tracker | `camera-tracker/` | Python, OpenCV, paho-mqtt |
| Web app | `Hotwire.Web/` | .NET 10, Blazor Server, Aspire, EF Core + SQLite, MQTTnet |
| Wiring | `docs/Verkabelung.drawio` | draw.io |

## 3. Game flow

1. **Scan card** – the ESP sends `RfidAssigned` with the card UID.
2. **Touch start contact** – `Start`, the web app creates a new run.
3. **Touch the wire** – `Collision`, the web app adds one collision.
4. **Touch stop contact** – `Stop`, the web app saves end time and duration.
5. The run appears on the **scoreboard**; the best run can be replayed on the **Track** page.

The ESP firmware is a state machine with three states: `IDLE` (wait for card) → `READY` (player known) → `RUNNING` (count collisions) → back to `IDLE`.

## 4. Hardware and wiring

The metal loop on the handle is connected to **GND**. All inputs use the internal pull-up, so a contact pulls its pin LOW.

| Component | ESP32-C3 pin |
|---|---|
| Course wire | GPIO 10 |
| Start contact | GPIO 21 |
| Stop contact | GPIO 20 |
| Calibration button | GPIO 1 |
| Buzzer (PWM) | GPIO 0 |
| RC522 SCK / MISO / MOSI | GPIO 4 / 5 / 6 |
| RC522 SS / RST | GPIO 7 / 2 |

GPIO 8 and 9 are boot pins and stay free. GPIO 2 is also a boot pin, but the RC522 RST input does not pull it LOW, so it is safe. Logs go over the USB port (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`), so GPIO 20/21 (RX/TX) can be used.

## 5. MQTT interface

Broker: `broker.emqx.io:1883` (public, no login – for testing only).

### Game events (ESP → web app)

Topic: `hotwire/iot-buzz-wire/test/game`, QoS 1

```json
{"deviceId":"buzzwire-esp32c3","eventType":0,"rfidTag":"04A1B2C3"}
```

| eventType | Meaning | Sent when | Web app action |
|---|---|---|---|
| 0 | Start | start contact | new `GameRun` (player by RFID tag) |
| 1 | Stop | stop contact | set `EndTime`, `Duration`, `Completed` |
| 2 | Collision | wire touched | `Collisions + 1` |
| 4 | RfidAssigned | card scanned | shown as last event only |
| 5 | CalibrationSelected | calibration button | not used yet |

- `eventType` must be a **number**, it is read into the C# enum `GameEventType`.
- No timestamp: the web app uses the time when the message arrives.

### Positions (camera → web app)

Topic: `hotwire/iot-buzz-wire/test/position`, QoS 0, about 10 messages per second

```json
{"deviceId":"camera","x":412,"y":233,"width":1280,"height":720}
```

Only sent when the marker is visible. Positions are stored only while a run is active.

## 6. How to run

**Firmware**

1. Enter WiFi name and password in `smart-buzz-wire/main/include/config.h`.
2. In an ESP-IDF terminal:
   ```
   cd smart-buzz-wire
   idf.py set-target esp32c3
   idf.py build flash monitor
   ```

**Camera tracker**

```
pip install opencv-python numpy paho-mqtt
python camera-tracker/camera_tracker.py
```
Press `q` to quit.

**Web app**

```
dotnet run --project Hotwire.Web/Hotwire.Web.Web
```
Then open http://localhost:5175. Players and their RFID cards are created on the **Players** page.

## 7. Changes compared to the handout

- **LED strip for the best run removed.** The best-run comparison is shown as a replay in the web app instead.
- **ESP32 → ESP32-C3 SuperMini.** Pins and SPI host were adapted.
- The camera tracks a **blue marker** instead of a bright ball.

## 8. Known limitations and open points

- The **calibration button** sends event 5, but the web app does not use it yet. The track calibration comes from `wwwroot/data/calibration.json`.
- **Run time is measured on the server** (arrival time of Start and Stop). With a normal WiFi this is accurate enough; after a connection drop it can be wrong.
- The **public broker** can be read and written by anyone. For real use: own broker (e.g. Mosquitto) with login and TLS.
- End-to-end test with all three parts at the same time is still open.
