# Smart Buzz Wire – Test Plan (hardware test)

Goal: check every part step by step, then the full game. Tick each box.
If a step fails, stop there and fix it before going on.

## 0. Bring along

- [ ] ESP32-C3 SuperMini + USB-C **data** cable
- [ ] RC522 + RFID cards, buzzer, jumper wires, breadboard
- [ ] Course wire, handle loop, start/stop contacts, calibration button
- [ ] Laptop with ESP-IDF 6.1.1, .NET 10, Python (`opencv-python numpy paho-mqtt`)
- [ ] WiFi with **2.4 GHz** (phone hotspot works; the ESP32-C3 has no 5 GHz)

## 1. Already tested without hardware (23.11.2026)

- Web app + fake ESP (`tools/fake_esp.py`): run, collisions, duration, scoreboard OK
- Unknown card → fallback player, collision/stop without start → ignored
- Firmware builds with ESP-IDF 6.1.1 (`idf.py build`)

## 2. Web app

```
dotnet run --project Hotwire.Web/Hotwire.Web.Web
```

- [ ] http://localhost:5175 opens
- [ ] `python tools/fake_esp.py` → new run on the scoreboard (quick check that MQTT works on this network)

## 3. Flash the ESP

1. Enter WiFi in `smart-buzz-wire/main/include/config.h` (`WIFI_SSID`, `WIFI_PASS`).
   **Never commit the password** – undo it after the test with `git checkout smart-buzz-wire/main/include/config.h`.
2. In the "ESP-IDF PowerShell":
   ```
   cd smart-buzz-wire
   idf.py build
   idf.py -p COMx flash monitor
   ```
   COM port: Device Manager → "Ports (COM & LPT)". If flashing fails: hold **BOOT**, press **RESET**, release BOOT.

- [ ] Monitor shows `WiFi connected, got IP`
- [ ] Monitor shows `MQTT connected`

## 4. Wiring and single parts

All contacts switch to **GND** (handle loop = GND). RC522 on **3.3 V**, not 5 V.

| Part | Pin | Test | Expected in monitor |
|---|---|---|---|
| RC522 SCK/MISO/MOSI/SS/RST | 4 / 5 / 6 / 7 / 10 | hold a card | `Player card: <UID>`, short beep |
| Start contact | 1 | touch with loop | `Run started` |
| Course wire | 0 | touch with loop | `Collision`, low beep |
| Stop contact | 3 | touch with loop | `Run finished`, long beep |
| Calibration button | 20 | press | `Calibration button` |
| Buzzer | 21 | (all of the above) | beeps |

Order matters: start only works after a card, wire and stop only during a run.

- [ ] Card is read, UID noted: `__________`
- [ ] Start / collision / stop / button all work
- [ ] One touch = one collision (not 5 in a row; lock time is 300 ms in `game.c`)

## 5. Player and full run (ESP + web app)

- [ ] **Players** page: create a player with the noted UID
- [ ] Card → start → touch wire 3× → stop
- [ ] Scoreboard shows the run with **3 collisions** and a plausible time

## 6. Camera

```
python camera-tracker/camera_tracker.py
```

- [ ] Window shows the blue marker detected (light can be a problem → adjust HSV values)
- [ ] Full run with camera running → **Track** page shows the path as replay

## 7. Notes from the test

| Problem | Fix |
|---|---|
| | |
