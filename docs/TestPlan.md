# Smart Buzz Wire – Test Plan (hardware test)

Goal: check every part step by step, then the full game. Tick each box.
If a step fails, stop there and fix it before going on.

## 0. Bring along

- [ ] ESP32-C3 SuperMini + USB-C **data** cable
- [ ] RC522 + RFID cards, buzzer, jumper wires, breadboard
- [ ] Course wire, handle loop, start/stop contacts, blue + red button
- [ ] Laptop with ESP-IDF 6.1.1, .NET 10, Python (`opencv-python numpy paho-mqtt`)
- [ ] WiFi with **2.4 GHz** (phone hotspot works; the ESP32-C3 has no 5 GHz)

## 1. Test results

**23.11.2026 (without hardware)**
- Web app + fake ESP (`tools/fake_esp.py`): run, collisions, duration, scoreboard OK
- Firmware builds with ESP-IDF 6.1.1 (`idf.py build`)

**24.11.2026 (with hardware)**
- ESP: WiFi + MQTT connect, RFID card, buttons, contacts, buzzer sounds OK
- Contacts were wired to GND (not 3.3 V) – firmware adapted
- Camera: external webcam (number 2), red tip detected; skin caused outliers → stricter red + jump filter + smoothing
- Calibration with the blue button, replay with real touch points, live view OK

## 2. Web app

```
dotnet run --project Hotwire.Web/Hotwire.Web.Web
```

- [ ] http://localhost:5175 opens
- [ ] `python tools/fake_esp.py 12345 3` → new run on the scoreboard (quick check that MQTT works on this network)

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

Start, stop and wire switch to **GND** (handle loop = GND). The blue and red button modules and the RC522 on **3.3 V**, never 5 V.

| Step | Part | Pin | Expected in monitor | Sound |
|---|---|---|---|---|
| 1 | Red button | 2 | `New player: hold the card to the reader` | click |
| 2 | Card on RC522 | SPI 4/5/6/7 | `Player <UID> ready - touch the start contact` | 2 short beeps |
| 3 | Loop on start contact | 21 | `START - errors: 0` | rising melody |
| 4 | Loop on wire (3×) | 10 | `Error! errors: 1` … `3` | low buzz |
| 5 | Loop on start again | 21 | `START - errors: 0` (restart) | rising melody |
| 6 | Loop on stop contact | 20 | `FINISH - player <UID>, errors: N` | fanfare |
| 7 | Blue button | 1 | `Calibration: touch start, follow the wire, touch stop` | click |

Without a player the start contact only shows `No player - press the red button and scan a card first`.
If the card is not read: check 3.3 V/GND and SPI wires; if needed connect RC522 RST to 3.3 V.

- [ ] Card is read, UID noted: `__________`
- [ ] All 7 steps work with the right sound
- [ ] One touch = one error (not 5 in a row; lock time is 300 ms in `game.c`)

## 5. Player and full run (ESP + web app)

- [ ] Red button + card → **Players** page shows "Card … is not assigned yet", RFID field is filled in → enter a name and save
- [ ] Home page live view shows the player name
- [ ] Start → touch wire 3× → stop; live view counts errors and time
- [ ] Scoreboard shows the run with **3 errors**, a plausible time and points = 1000 − 10 × s − 10 × errors

## 6. Camera

```
python camera-tracker/camera_tracker.py
```

- [ ] Window shows the **red tip** with a green outline (no skin detected)
- [ ] Calibration: blue button → start → follow the wire slowly → stop → reload **Track** → new blue line
- [ ] Full run → **Track** → "Replay last run": red line on the blue track, red X where the wire was touched

## 7. Notes from the test

| Problem | Fix |
|---|---|
| | |
