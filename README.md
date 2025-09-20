# IKEA OBEGRÄNSAD Hack/Mod

Turn your OBEGRÄNSAD LED Wall Lamp into a live drawing canvas

## Differences from the original (Änderungen gegenüber dem Original)

This repository is a fork of the original project by ph1p:
- Original repository: https://github.com/ph1p/ikea-led-obegraensad

What this fork adds/changes:
- New plugins
  - Arcade Sprites: 5 Space‑Invaders‑inspired sprites; exactly 2 are visible at a time; slow right‑to‑left fly‑by; spawns are always fully visible (no top/bottom clipping) and avoid starting overlaps; randomized start bands/types for variety
  - Tetris (Demo): simple heuristic AI chooses placements (prefers low height and few holes); line clears with blink and collapse; automatic restart on overflow (blocks reach hidden top rows); optional manual control via WebSocket (rotate/left/right/softDrop/hardDrop); randomized first “next piece” at boot
  - Sunrise: Sonnenaufgang mit Icon oben mittig und Uhrzeit unten mittig im PongClock‑Stil (kleine Ziffern, ohne Doppelpunkt); Zeitformat 12/24h über `TIME_FORMAT_24H` in `include/secrets.h`; erzwungene Neuzeichnung, damit beim Reaktivieren keine Platzhalter erscheinen
  - Sunset: Sonnenuntergang mit Icon oben mittig und Uhrzeit unten mittig im PongClock‑Stil; nutzt Astronomie‑Daten aus dem WeatherService
  - Stock Chart: zeigt die letzten bis zu 16 Schlusskurse als Mini-Chart (Raster, rechtsbündig); Daten via stooq.com CSV; Ticker per `STOCK_SYMBOL` in `include/secrets.h` konfigurierbar (Standard: `aapl.us`)


- Stability/quality improvements
  - Tetris: fixed preview flicker at the top‑right; corrected full‑row clear and collapse
  - Randomized starts in both plugins so they don’t always start with the same piece/sprites
  - Tetris: L‑Tetromino in Rotation 0 korrigiert (3er‑Basis mit rechtem „Fuß“)
  - Tetris: AI führt aktive Verschiebungen/Rotationen aus (inkl. einfachen Wall‑Kicks) und nutzt verbesserte Heuristik (LinesCleared belohnt, Holes stark bestraft, Bumpiness berücksichtigt)
  - Tetris: `yield()` im Plugin‑Loop, damit Web‑Interface/WiFi responsiv bleiben (verhindert mögliche Freezes)

- Developer experience
  - Added include/secrets_example.h and updated README to simplify local setup
  - `TIME_FORMAT_24H` in `include/secrets.h` zur Steuerung des 12/24‑Stunden‑Formats für Zeit‑Plugins (PongClock, Sunrise, Sunset)


If you want the original feature set without these additions, use the upstream repository linked above.


> **⚠ Disclaimer**: Use this code and instructions at your own risk! Improper use may damage the device.
> **Contribute**: Have suggestions or improvements? Feel free to submit a PR or open an issue. 😊

![ezgif-3-2019fca7a4](https://user-images.githubusercontent.com/15351728/200184222-a590575d-983d-4ab8-a322-c6bcf433d364.gif)

<details>
  <summary><h1>Features</h1></summary>

- Persist your drawing
- Rotate image
- Live Drawing
- OTA Update
- Wifi Control
- Web-GUI
- Load an image
- Switch plugin by pressing the button
- Schedule Plugins to switch after "n" seconds
- Plugins
  - Draw
  - Game of life
  - Breakout
  - Snake
  - Stars
  - Lines
  - Circle
  - Clock
  - Big Clock
  - Weather
  - Stock Chart

  - Sunrise
  - Sunset

  - Rain
  - Animation with the "Animation Creator in Web UI"
  - Firework
  - DDP
  - Pong Clock
  - Arcade Sprites (Space Invaders fly-by)
  - Tetris (Demo) with simple AI
  - Moon Phase (uses WeatherService, aspect-corrected)

</details>

# Control the board

https://github.com/user-attachments/assets/ddf91be1-2c95-4adc-b178-05b0781683cc

You can control the lamp with a supplied web GUI.
You can get the IP via serial output or you can search it in your router settings.


## Netzwerk & Diagnose (neu)

- WiFi Power Save ist deaktiviert (schnellere Reaktionszeit des Web-Servers)
- Diagnose-Endpunkte:
  - `http://<ip>/healthz` → kurzer Healthcheck ("ok")
  - `http://<ip>/diag` → einfache Textdiagnose (ip, gw, rssi, heap)
  - `http://<ip>/ping` → Latenztest; die bediente Zeit wird im Seriell-Log protokolliert
- SoftAP-Fallback (ohne Router):
  - Per Doppel‑Klick auf den Taster kann ein Access Point ein-/ausgeschaltet werden
  - SSID: `<WIFI_HOSTNAME>-AP` (z. B. `LostDisplay-AP`), Standard ohne Passwort
  - Der Status wird persistent gespeichert (NVS) und beim Booten wiederhergestellt
  - Doppel‑Klick erneut → AP wieder aus; Normalbetrieb ausschließlich im Heimnetz

# How to

First of all. This software was written for the ESP32 Dev Board, but it should work with any other Arduino board as well. You just need to remove the WiFi, OTA and web server related code.

The ESP32 I used:

<img src="https://user-images.githubusercontent.com/15351728/200148521-86d0f9e6-2c41-4707-b2d9-8aa24a0e440e.jpg" width="60%" />

Verified to work with TTGO LoRa32 V2.1 (T3_V1.6.1).
Note: On esp8266 per pixel brightness only works when storage and global brightness (analogWrite) are disabled.

## Open the lamp

I'm sorry to say this, but you'll have to pry open the back of your Lamp, as IKEA didn't install regular screws here. I lifted the back with a screwdriver between the screws and pried it open with a second object, but you can also drill out the rivets to avoid breaking the backpanel.

## The panels

<img src="https://user-images.githubusercontent.com/15351728/200183585-39c1668d-665b-4c12-bcbb-387aec1d3874.JPG" width="60%" />

After you open the back, you will see 4 identical plates. These are each equipped with 64 Leds in 4 fields. We are only interested in the lowest one. Here you will find 6 connectors at the bottom edge, to which we connect our board.
Above is a microcontroller. You have to remove it, because it contains the standard programs.

<img src="https://user-images.githubusercontent.com/86414213/205998862-e9962695-1328-49ea-b546-be592cbad3c2.jpg" width="90%" />

### ESP32 Setup with VS Code and PlatformIO

1. **Prerequisites**

   - Install **[Visual Studio Code](https://code.visualstudio.com/)**.
   - Install the **PlatformIO IDE** extension from the VS Code Extensions Marketplace.

2. **Clone the Project**

   - Download the project from GitHub and open it in VS Code. PlatformIO will automatically load dependencies.

```bash
git clone git@github.com:ph1p/ikea-led-obegraensad.git
cd ikea-led-obegraensad
code .
```

3. **Connect ESP32**

   - Connect your ESP32 via USB.
   - Check the COM port in the **PlatformIO Devices** tab.

4. **Prepare the Project**

   - Perform a `PlatformIO: Clean` (Recycle bin icon at the bottom right).
   - Copy `include/secrets_example.h` to `include/secrets.h` and fill in your local credentials (WiFi/OTA/Weather).
   - Set variables inside `include/constants.h`.

5. **Build the Project**

   - Click the `PlatformIO Build` icon (bottom right corner).

### Moon Phase (new)

- Shows a large moon disc with the illuminated portion bright and the shadow side dimmed
- Uses WeatherService (wttr.in) to fetch moon_phase and moon_illumination
- Aspect-corrected rendering so the moon appears circular on non-square panels (config via DISPLAY_ASPECT_YX)
- Falls back to a loading indicator until data is available

   - Check the log for missing libraries.

### Arcade Sprites: Aspect-corrected rendering

- Sprites are vertically compressed during drawing to compensate for non-square pixel ratio
- Toggle via ARCADE_SPRITES_ASPECT_CORRECT (1=on, default) and set DISPLAY_ASPECT_YX accordingly
- Keeps sprites fully visible and reduces “gestreckt” look

     - Use the **Libraries** icon in PlatformIO to install required libraries.
   - Repeat `Clean` and `Build` until the build succeeds.

6. **Upload to ESP32**
   - Click `PlatformIO Upload` (bottom right) to upload the firmware to the ESP32.

### New Plugins

- Arcade Sprites
  - Shows 2 classic-inspired invader sprites at a time (from a set of 5), slowly flying from right to left
  - Spawns are always fully visible (no clipping top/bottom) and avoid starting overlaps
  - Start bands (top/bottom) are randomized at boot so the initial scene varies
- Tetris (Demo)
  - Simple AI picks placements to avoid holes and keep stack low; line clears with blink and collapse
  - Game restarts automatically when blocks reach the hidden top rows
  - Manual demo control via WebSocket events (optional): rotate/left/right/softDrop/hardDrop
  - First “next piece” is randomized at boot for variety

- Sunrise
  - Icon oben mittig; Uhrzeit unten mittig im PongClock‑Stil (kleine Ziffern, ohne Doppelpunkt)
  - Datenquelle: WeatherService (wttr.in Astronomie)
  - 12/24h umschaltbar über `TIME_FORMAT_24H` in `include/secrets.h`
- Sunset
  - Icon oben mittig; Uhrzeit unten mittig im PongClock‑Stil
  - Datenquelle: WeatherService (wttr.in Astronomie)
- Stock Chart
  - Zeigt die letzten bis zu 16 Schlusskurse (tägliche Daten) als Mini‑Chart auf 16×16, mit feinem Raster und rechtsbündiger Verlaufslinie
  - Datenquelle: stooq.com (kostenlose CSV‑API)
  - Konfiguration: Ticker über `STOCK_SYMBOL` in `include/secrets.h` (Standard: `aapl.us`; wenn keine Suffixe angegeben, wird `.us` ergänzt)
  - Aktualisierung: Standardmäßig stündlich im Hintergrund; Darstellung wird mit ~2 Hz aktualisiert

  Beispiel `include/secrets.h`:

  ```c
  #define STOCK_SYMBOL "msft.us"
  ```

  - Erzwungene Neuzeichnung, damit beim erneuten Aktivieren keine „— — —“ Platzhalter bleiben


### Configuring WiFi with WiFi manager

_Note:_ The WiFi manager only works on ESP32. For ESP8266, `WIFI_SSID` and `WIFI_PASSWORD` need to be provided in `secrets.h`.

This project uses [tzapu's WiFiManager](https://github.com/tzapu/WiFiManager). After booting up, the device will try
to connect to known access points. If no known access point is available, the device will create a network called
`Ikea Display Setup WiFi`. Connect to this network on any device. A captive portal will pop up and will take you
through the configuration process. After a successful connection, the device will reboot and is ready to go.

The name of the created network can be changed by modifying `WIFI_MANAGER_SSID` in `include/constants.h`.

### PINS

Connect them like this and remember to set them in `include/constants.h` according to your board.

|       LCD        | ESP32  | TTGO LoRa32 | NodeMCUv2 | Lolin D32 (Pro) |
| :--------------: | :----: | :---------: | :-------: | :-------------: |
|       GND        |  GND   |     GND     |    GND    |       GND       |
|       VCC        |   5V   |     5V      |    VIN    |       USB       |
| EN (PIN_ENABLE)  | GPIO26 |    IO22     | GPIO16 D0 |     GPIO26      |
|  IN (PIN_DATA)   | GPIO27 |    IO23     | GPIO13 D7 |     GPIO27      |
| CLK (PIN_CLOCK)  | GPIO14 |    IO02     | GPIO14 D5 |     GPIO14      |
| CLA (PIN_LATCH)  | GPIO12 |    IO15     | GPIO0 D3  |     GPIO12      |
|  BUTTON one end  | GPIO16 |    IO21     | GPIO2 D4  |     GPIO25      |
| BUTTON other end |  GND   |     GND     |    GND    |       GND       |

<img src="https://user-images.githubusercontent.com/86414213/205999001-6213fc4f-be2f-4305-a17a-44fdc9349069.jpg" width="60%" />

### Alternate Button Wiring

Thanks to [RBEGamer](https://github.com/RBEGamer) who is showing in this [issue](https://github.com/ph1p/ikea-led-obegraensad/issues/79) how to use the original button wiring. With this solution you won't need the "BUTTON one end" and "BUTTON other end" soldering from the table above.
# HTTP API Endpoints

## Get Information

Get current values and the (fixed) metadata, like number of rows and columns and a list of available plugins.

```
GET http://your-server/api/info
```

### Example `curl` Command:

```bash
curl http://your-server/api/info
```

### Response

```json
{
  "rows": 16,
  "cols": 16,
  "status": "active",
  "plugin": 3,
  "rotation": 90,
  "brightness": 255,
  "scheduleActive": true,
  "schedule": [
    {
      "pluginId": 2,
      "duration": 60
    },
    {
      "pluginId": 4,
      "duration": 120
    }
  ],
  "plugins": [
    {"id": 1, "name": "Plugin One"},
    {"id": 2, "name": "Plugin Two"},
    {"id": 3, "name": "Plugin Three"}
  ]
}
```

---

## Set Active Plugin by ID

To set an active plugin by ID, make an HTTP PATCH request to the following endpoint, passing the parameter as a query string:

```
PATCH http://your-server/api/plugin
```

#### Example `curl` Command:

```bash
curl -X PATCH "http://your-server/api/plugin?id=7"
```

### Parameters

- `id` (required): The ID of the plugin to set as active.

### Response

- **Success:**

```json
{
  "status": "success",
  "message": "Plugin set successfully"
}
```

- **Error (Plugin not found):**

```json
{
  "error": true,
  "errormessage": "Could not set plugin to id 7"
}
```

---

## Set Brightness

To set the brightness of the LED display, make an HTTP PATCH request to the following endpoint, passing the parameter as a query string:

```
PATCH http://your-server/api/brightness
```

#### Example `curl` Command:

```bash
curl -X PATCH "http://your-server/api/brightness?value=100"
```

### Parameters

- `value` (required): The brightness value (0..255).

### Response

- **Success:**

```json
{
  "status": "success",
  "message": "Brightness set successfully"
}
```

- **Error (Invalid Brightness Value):**

```json
{
  "error": true,
  "errormessage": "Invalid brightness value: 300 - must be between 0 and 255."
}
```

---

## Get Current Display Data

To get the current displayed data as a byte-array, each byte representing the brightness value. Be aware that the global brightness value gets applied AFTER these values.

```
GET http://your-server/api/data
```

#### Example `curl` Command:

```bash
curl http://your-server/api/data
```

### Response (Raw Byte-Array Example)

```json
[255, 255, 255, 0, 128, 255, 255, 0, ...]
```

---

## Use HTTP API in Home Assistant

An example configuration for an automation to set the brightness based on the sun's position. Dims the display when the sun is setting.

- Add the following code to your `configuration.yaml`:
  ```yaml
  rest_command:
    obegraensad_brightness_high:
      url: "http://your-server/api/brightness/"
      method: PATCH
      content_type: "application/x-www-form-urlencoded"
      payload: "value=100"
    obegraensad_brightness_low:
      url: "http://your-server/api/brightness/"
      method: PATCH
      content_type: "application/x-www-form-urlencoded"
      payload: "value=1"
  ```
- Go to *Settings* --> *Automations* and create a new automation.
- Select *Edit in YAML* and add the following content:
  ```yaml
  alias: Obegraensad low bightness
  description: ""
  triggers:
    - trigger: sun
      event: sunset
      offset: 0
  conditions: []
  actions:
    - action: rest_command.obegraensad_brightness_low
      data: {}
  mode: single
  ```
- To set the brightness back to bright, create e.g. another automation or a condition in which `rest_command.obegraensad_brightness_high` is called.

---

# Plugin Scheduler

It is possible to switch between plugins automatically.
You can define your schedule in the Web UI or just send an API call.

### Set Schedule

To define a schedule for switching between plugins automatically, make a POST request with your schedule data:

```bash
curl -X POST http://your-server/api/schedule -d 'schedule=[{"pluginId":10,"duration":2},{"pluginId":8,"duration":5}]'
```

#### Example Response

```json
{
  "status": "success",
  "message": "Schedule updated"
}
```

### Clear Schedule

To clear the existing schedule, make a GET request:

```bash
curl http://your-server/api/schedule/clear
```

#### Example Response

```json
{
  "status": "success",
  "message": "Schedule cleared"
}
```

### Start Schedule

To start the current schedule, make a GET request:

```bash
curl http://your-server/api/schedule/start
```

#### Example Response

```json
{
  "status": "success",
  "message": "Schedule started"
}
```

### Stop Schedule

To stop the current schedule, make a GET request:

```bash
curl http://your-server/api/schedule/stop
```

#### Example Response

```json
{
  "status": "success",
  "message": "Schedule stopped"
}
```

---


---

# WebSocket API (Realtime)

- Endpoint: `ws://<device-ip>/ws`
- Auth (optional): Wenn `API_TOKEN` gesetzt ist, muss als erste Nachricht gesendet werden:
  ```json
  {"event":"auth","token":"<API_TOKEN>"}
  ```
  Andernfalls wird die Verbindung mit `Unauthorized` geschlossen.

## Beispiele: Tetris (Demo) manuell steuern

Nach erfolgreichem Verbindungsaufbau (und ggf. Auth) kann die Demo über JSON‑Events gesteuert werden.

- Rotieren:
  ```json
  {"event":"tetris","action":"rotate"}
  ```
- Nach links/rechts schieben:
  ```json
  {"event":"tetris","action":"left"}
  {"event":"tetris","action":"right"}
  ```
- Soft/Hard‑Drop:
  ```json
  {"event":"tetris","action":"softDrop"}
  {"event":"tetris","action":"hardDrop"}
  ```

### Schnelltest mit websocat

```bash
# Optional: websocat installieren (macOS)
brew install websocat

# Verbinden (ohne Token)
websocat ws://<device-ip>/ws

# Verbinden (mit Token)
websocat -H "Sec-WebSocket-Protocol: json" ws://<device-ip>/ws
# Danach zuerst senden:
{"event":"auth","token":"<API_TOKEN>"}
# Und z. B. rotieren:
{"event":"tetris","action":"rotate"}
```

Hinweis: Die Web‑GUI nutzt die gleiche WebSocket‑Schnittstelle. Per HTTP‑API lassen sich weiterhin Plugins wechseln, Helligkeit setzen usw. (siehe Abschnitte oben).

## Get Display Data

To retrieve the current display data as a byte-array, each byte representing the brightness value. The global brightness is applied after these values.

```
GET http://your-server/api/data
```

#### Example `curl` Command:

```bash
curl http://your-server/api/data
```

### Response (Raw Byte-Array Example)

```json
[255, 255, 255, 0, 128, 255, 255, 0, ...]
```

---

## Message Display

To display a message on the LED display, users can make an HTTP GET request to the following endpoint:

```
GET http://your-server/api/message
```

### Parameters

- `text` (optional): The text message to be displayed on the LED display.
- `graph` (optional): A comma-separated list of integers representing a graph (0-15).
- `miny` (optional): Scaling for the lower end of the graph, defaults to 0.
- `maxy` (optional): Scaling for the upper end of the graph, defaults to 15.
- `repeat` (optional): Number of times the message should be repeated. Default is 1. Set to `-1` for infinite.
- `id` (optional): A unique identifier for the message.
- `delay` (optional): Delay in ms between every scroll movement. Default is 50ms.

#### Example `curl` Command:

```bash
curl "http://your-server/api/message?text=Hello&graph=8,5,2,1,0,0,1,4,7,10,13,14,15,15,14,11&repeat=3&id=1&delay=60"
```

### Response

```json
{
  "status": "success",
  "message": "Message received"
}
```

---

## Message Removal

To remove a message from the display, users can make an HTTP GET request to the following endpoint:

```
GET http://your-server/api/removemessage
```

### Parameters

- `id` (required): The unique identifier of the message to be removed.

#### Example `curl` Command:

```bash
curl "http://your-server/api/removemessage?id=1"
```

### Response

```json
{
  "status": "success",
  "message": "Message removed"
}
```

---

## Clear Storage

To clear the data storage:

```
GET http://your-server/api/clearstorage
```

#### Example `curl` Command:

```bash
curl http://your-server/api/clearstorage
```

### Response

```json
{
  "status": "success",
  "message": "Storage cleared"
}
```

# Development

- `src` contains the arduino code.

  - Run it with platform io
  - You can uncomment the OTA lines in `platformio.ini` if you want. Replace the IP with your device IP.

- `frontend` contains the web code.

  - First run `pnpm install`
  - Set your device IP inside the `.env` file
  - Start the server with `pnpm dev`
  - Build it with `pnpm build`. This command creates the `webgui.cpp` for you.

- Build frontend using `Docker`
  - From the root of the repo, run `docker compose run node`

# Plugin Development

1. Start by creating a new C++ file for your plugin. For example, let's call it plugins/MyPlugin.(cpp/h).

**plugins/MyPlugin.h**

```cpp
#pragma once

#include "PluginManager.h"

class MyPlugin : public Plugin {
public:
    MyPlugin();
    ~MyPlugin() override;

    void setup() override;
    void loop() override;
    const char* getName() const override;

    void teardown() override; // optional
    void websocketHook(DynamicJsonDocument &request) override; // optional
};
```

**plugins/MyPlugin.cpp**

```cpp
#include "plugins/MyPlugin.h"

MyPlugin::MyPlugin() {
    // Constructor logic, if needed
}

void MyPlugin::setup() {
    // Setup logic for your plugin
}

void MyPlugin::loop() {
    // Loop logic for your plugin
}

const char* MyPlugin::getName() const {
    return "MyPlugin"; // name in GUI
}

void MyPlugin::teardown() {
  // code if plugin gets deactivated
}

void MyPlugin::websocketHook(DynamicJsonDocument &request) {
  // handle websocket requests
}
```

2. Add your plugin to the `main.cpp`.

## Web-UI nicht erreichbar / sehr langsam

- Prüfe, dass dein Gerät und das ESP im selben Netz sind (kein Gast-WLAN/Client‑Isolation)
- Teste die Diagnose:
  - `http://<ip>/healthz` (sollte "ok" liefern)
  - `http://<ip>/diag`
  - `http://<ip>/ping` (Latenz im Seriell‑Log)
- Falls der Router Peer‑to‑Peer blockiert: SoftAP‑Fallback nutzen
  - Taster doppelt drücken → mit `SSID: <WIFI_HOSTNAME>-AP` verbinden → `http://192.168.4.1/`
  - Doppel‑Klick erneut → AP wieder aus
- Browser‑Hinweise: HTTP (nicht HTTPS), ggf. harter Reload/Cache leeren, anderer Browser probieren


```cpp
#include "plugins/MyPlugin.h"

pluginManager.addPlugin(new MyPlugin());
```

# Troubleshooting

## Flickering panel

- Check all soldering points, especially VCC
- Check if the board gets enough power
