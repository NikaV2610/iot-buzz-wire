# Smart Buzz Wire – Project Documentation

Internet of Things · Team: Andi, Arian, Fabian, Nika

## 1. Idea

A classic buzz wire game, extended with IoT:

- A new player presses the **red button** and scans an **RFID card**.
- Every touch of the wire is counted as an **error**, the buzzer makes a sound.
- A **webcam** tracks the **red tip** of the handle (live position).
- Every run gets **points**: fast and clean wins.
- A **web app** shows the game live, stores every run, shows the scoreboard and replays runs on the calibrated track, including the points where the wire was touched.

## 2. Architecture

```
ESP32-C3 (game)  --- game events, RFID --->  MQTT broker  --->  Web app (Hotwire.Web)  --->  Browser
Camera (Python)  --- positions        --->   broker.emqx.io      Blazor + SQLite             live view, scoreboard, replay
```

All parts only talk via MQTT. Nobody calls anybody else directly.

| Part | Folder | Technology |
|---|---|---|
| Game firmware | `smart-buzz-wire/` | ESP32-C3 SuperMini, ESP-IDF 6.1, C |
| Camera tracker | `camera-tracker/` | Python, OpenCV, paho-mqtt |
| Web app | `Hotwire.Web/` | .NET 10, Blazor Server, Aspire, EF Core + SQLite, MQTTnet |
| Test tool | `tools/fake_esp.py` | Python: simulates a whole run without hardware |
| Wiring | `docs/Verkabelung.drawio` | draw.io |

## 3. Game flow

1. **Red button** – "new player": the ESP sends event 3 and waits for a card.
2. **Scan card** – the card number goes to the RFID topic. The web app remembers it; the next runs belong to this player. Cards are only accepted after the red button.
3. **Touch start contact** – errors = 0, event 0, the web app creates a new run. Touching start again during a run restarts it.
4. **Touch the wire** – errors + 1, event 2. The web app counts it and stores **where** it happened (last camera position).
5. **Touch stop contact** – event 1, the web app saves end time and duration. The same player can start again right away.
6. The run gets its **points** and appears on the **scoreboard**; on the **Track** page it can be replayed.

The ESP firmware is a small state machine:

```
NO_PLAYER --red button--> SCAN --card--> READY --start--> RUNNING --stop--> READY
any state (not running) --blue button--> CAL_WAIT --start--> CAL_RUN --stop--> back
```

The serial console (`idf.py monitor`) shows the game, e.g. `START - errors: 0`, `Error! errors: 2`, `FINISH - player 996C075A, errors: 2`.

Every action has its own sound: card = two short beeps, start = rising melody, wire = low buzz, stop = fanfare, buttons = click.

## 4. Scoring

```
points = 1000 - 10 x seconds - 10 x errors      (never below 0)
```

- One error costs as much as 1 second. In the test players made 20–30 errors per run, so a higher penalty (first idea: 50 per error) pushed almost every run to 0 points.
- Examples: 20 s + 25 errors = **550**, 30 s + 10 errors = **600**, 15 s + 40 errors = **450**.
- More points = better. Same points: the faster run wins.
- The points are **calculated, not stored** (`Services/ScoreCalculator.cs`). Changing the numbers there changes the ranking of all runs at once, no database change needed.
- The live view shows the points dropping while the clock runs.

## 5. Calibration (record the track)

1. Press the **blue button** – the ESP waits for the start contact (no player needed).
2. Touch **start** – the ESP sends event 5, the web app starts recording camera positions.
3. Follow the wire **slowly** to the stop contact (wire touches are not counted).
4. Touch **stop** – the web app saves the path as a new track (`wwwroot/data/calibration.json` + database) and makes it active.
5. Reload the **Track** page – the blue line is the new track. New runs belong to this track.

Pressing the blue button again before start cancels the calibration.

## 6. Hardware and wiring

The metal loop on the handle is connected to **GND**. Start, stop and wire use the internal pull-up, so a contact pulls its pin LOW.
The two button modules (blue, red) are powered with **3.3 V** and switch the pin HIGH, they use the internal pull-down.

| Component | ESP32-C3 pin |
|---|---|
| Course wire | GPIO 10 |
| Start contact | GPIO 21 |
| Stop contact | GPIO 20 |
| Cal button (blue) | GPIO 1 |
| Player button (red) | GPIO 2 |
| Buzzer (PWM, passive) | GPIO 0 |
| RC522 SCK / MISO / MOSI | GPIO 4 / 5 / 6 |
| RC522 SS | GPIO 7 (RST not connected, soft reset) |

GPIO 8 and 9 are boot pins and stay free. GPIO 2 is also a boot pin; the ESP boots fine with the red button module on it. Logs go over the USB port (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`), so GPIO 20/21 (RX/TX) can be used.

## 7. MQTT interface

Broker: `broker.emqx.io:1883` (public, no login – for testing only). Three topics, as planned on the whiteboard (GameData, Position, RFID):

| Topic | From | Payload |
|---|---|---|
| `hotwire/iot-buzz-wire/test/game` | ESP | `{"deviceId":"buzzwire-esp32c3","eventType":0}` |
| `hotwire/iot-buzz-wire/test/rfid` | ESP | `{"deviceId":"buzzwire-esp32c3","number":"996C075A"}` |
| `hotwire/iot-buzz-wire/test/position` | Camera | `{"deviceId":"camera","x":412,"y":233,"width":640,"height":480}` |

### Game events

| eventType | Meaning | Sent when | Web app action |
|---|---|---|---|
| 0 | Start | start contact | new `GameRun` for the remembered card |
| 1 | Stop | stop contact | set `EndTime`, `Duration`, `Completed` (or save calibration) |
| 2 | Collision | wire touched | `Collisions + 1`, store a `CollisionPoint` (time, x, y) |
| 3 | PlayerRegistration ("newPlayer") | red button | forget the old card, wait for a new one |
| 5 | CalibrationSelected | start during calibration | start recording the track |

- One message = one event. The whiteboard had five `bool` fields; a single number is simpler and means the same.
- `eventType` must be a **number**, it is read into the C# enum `GameEventType`.
- No timestamp: the web app uses the time when the message arrives.
- The card number is sent once on the RFID topic. The web app keeps it in memory (`LiveGameState.CurrentRfid`); after a restart of the web app the card has to be scanned again.

### Positions

About 10 messages per second, only when the red tip is visible. Stored only while a run is active (or recorded during a calibration).

## 8. Camera tracking

- OpenCV converts each frame to HSV and keeps only **strong red** (hue 0–7 and 170–180, saturation ≥ 140). Measured: red tip hue 1–5, skin hue 11–23, so skin is ignored.
- The largest red blob gives x/y.
- **Jump filter:** a detection more than 60 px away from the last good position is ignored, unless it stays there for 5 frames.
- Camera number as argument: `python camera_tracker.py 2` (default 2 = external webcam on the test laptop).

The web app cleans every path again before showing or saving it (`Services/TrackSmoothing.cs`): jump filter (120 px), median over 7 points (removes outliers), moving average over 5 points (smooths jitter). The display is mirrored left/right, because the camera looks at the course from the front.

## 9. Web app

| Page | Content |
|---|---|
| **Home** | Live view: player, status, errors, running time, live points, camera status (updates by itself) |
| **Scoreboard** | Ranking by points (podium + list), updates after every run |
| **Track** | Calibrated track (blue), live dot (green) while playing, replay of the last or best run by points (red) with a red X where the wire was touched |
| **Players** | Shows the last scanned card; an unknown card is filled into the form, so a new player only needs a name |

The top bar shows the live status on every page. New database tables are created automatically on start (EF Core migrations, e.g. `CollisionPoints`).

## 10. How to run

**Firmware**

1. Enter WiFi name and password (2.4 GHz) in `smart-buzz-wire/main/include/config.h` – never commit them.
2. In an ESP-IDF terminal:
   ```
   cd smart-buzz-wire
   idf.py set-target esp32c3
   idf.py -p COMx build flash monitor
   ```

**Camera tracker**

```
pip install opencv-python numpy paho-mqtt
python camera-tracker/camera_tracker.py
```
Press `q` in the camera window to quit.

**Web app**

```
dotnet run --project Hotwire.Web/Hotwire.Web.Web
```
Then open http://localhost:5175.

**Without hardware:** `python tools/fake_esp.py <card> <errors>` sends a complete run (red button, card, start, errors, stop).

## 11. Changes compared to the handout

- **LED strip for the best run removed.** The best run is shown as a replay in the web app instead.
- **ESP32 → ESP32-C3 SuperMini.** Pins and SPI host were adapted.
- The camera tracks the **red tip** of the handle instead of a bright ball.
- **Red button** for a new player, **blue button** for the calibration.
- **Points** instead of time only (time and errors both count).

## 12. Known limitations

- **Run time is measured on the server** (arrival time of Start and Stop). With a normal WiFi this is accurate enough; after a connection drop it can be wrong.
- The **public broker** can be read and written by anyone. For real use: own broker (e.g. Mosquitto) with login and TLS.
- The remembered card is lost when the web app restarts.
- A restarted run (start touched again) stays in the database as not completed; the scoreboard only shows completed runs.
- Deleting a player also deletes all of his runs.
- Bootstrap is loaded from a CDN, so the web app needs internet (it needs it for the broker anyway).
