#include <Arduino.h>
#include "buildinfo.h"

#include <BfButton.h>
#include <SPI.h>

#ifdef ESP8266
/* Fix duplicate defs of HTTP_GET, HTTP_POST, ... in ESPAsyncWebServer.h */
#define WEBSERVER_H
#endif

#include <WiFiManager.h>

#ifdef ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#endif
#ifdef ESP32
#include <Preferences.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include "PluginManager.h"
#include "scheduler.h"

#include "plugins/BreakoutPlugin.h"
#include "plugins/CirclePlugin.h"
#include "plugins/DDPPlugin.h"
#include "plugins/DrawPlugin.h"
#include "plugins/FireworkPlugin.h"
#include "plugins/GameOfLifePlugin.h"
#include "plugins/LinesPlugin.h"
#include "plugins/PongClockPlugin.h"
#include "plugins/RainPlugin.h"
#include "plugins/SnakePlugin.h"
#include "plugins/StarsPlugin.h"
#include "plugins/TickingClockPlugin.h"
#include "plugins/ArtNet.h"
#include "plugins/TetrisDemoPlugin.h"
#include "plugins/ArcadeSpritesPlugin.h"
#include "plugins/MoonPhasePlugin.h"
#include "WeatherService.h"
#include "StockService.h"
#include "plugins/StockChartPlugin.h"
#include "plugins/SunrisePlugin.h"
#include "plugins/SunsetPlugin.h"


#ifdef ENABLE_SERVER
#include "plugins/AnimationPlugin.h"
#include "plugins/BigClockPlugin.h"
#include "plugins/ClockPlugin.h"
#include "plugins/WeatherPlugin.h"
#endif

#include "asyncwebserver.h"
#include "messages.h"
#include "ota.h"
#include "screen.h"
#include "secrets.h"
#include "websocket.h"

BfButton btn(BfButton::STANDALONE_DIGITAL, PIN_BUTTON, true, LOW);

unsigned long previousMillis = 0;
unsigned long interval = 30000;


#ifdef ESP32
bool g_apEnabled = false;
#endif

PluginManager pluginManager;
SYSTEM_STATUS currentStatus = NONE;
WiFiManager wifiManager;

unsigned long lastConnectionAttempt = 0;
const unsigned long connectionInterval = 10000;

void connectToWiFi()
{
  // if a WiFi setup AP was started, reboot is required to clear routes
  bool wifiWebServerStarted = false;
  wifiManager.setWebServerCallback([&wifiWebServerStarted]()
                                   { wifiWebServerStarted = true; });

  wifiManager.setHostname(WIFI_HOSTNAME);

#if defined(IP_ADDRESS) && defined(GWY) && defined(SUBNET) && defined(DNS1)
  auto ip = IPAddress();
  ip.fromString(IP_ADDRESS);

  auto gwy = IPAddress();
  gwy.fromString(GWY);

  auto subnet = IPAddress();
  subnet.fromString(SUBNET);

  auto dns = IPAddress();
  dns.fromString(DNS1);

  wifiManager.setSTAStaticIPConfig(ip, gwy, subnet, dns);
#endif

  wifiManager.setConnectRetries(10);
  wifiManager.setConnectTimeout(10);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.autoConnect(WIFI_MANAGER_SSID);


#ifdef ESP32
  WiFi.setSleep(false);
  Serial.println("[WIFI] PowerSave OFF");
#endif

#ifdef ESP32
  if (MDNS.begin(WIFI_HOSTNAME))
  {
    MDNS.addService("http", "tcp", 80);
    MDNS.setInstanceName(WIFI_HOSTNAME);
  }
  else
  {
    Serial.println("Could not start mDNS!");
  }
#endif

  if (wifiWebServerStarted)
  {
    // Reboot required, otherwise wifiManager server interferes with our server
    Serial.println("Done running WiFi Manager webserver - rebooting");
    ESP.restart();
  }


  // Load persisted AP flag and start if enabled
  #ifdef ESP32
  Preferences prefs;
  prefs.begin("net", true);
  g_apEnabled = prefs.getBool("ap_en", false);
  prefs.end();
  if (g_apEnabled) {
    WiFi.mode(WIFI_AP_STA);
    String apSsid = String(WIFI_HOSTNAME) + "-AP";
    bool apok = WiFi.softAP(apSsid.c_str());
    Serial.printf("[WIFI] SoftAP(%s) %s ip=%s\n", apSsid.c_str(), apok?"OK":"FAIL", WiFi.softAPIP().toString().c_str());
  }
  #endif

  lastConnectionAttempt = millis();
}

void pressHandler(BfButton *btn, BfButton::press_pattern_t pattern)
{
  switch (pattern)
  {
  case BfButton::SINGLE_PRESS:
    if (currentStatus != LOADING)
    {
      Scheduler.clearSchedule();
      pluginManager.activateNextPlugin();
    }
    break;


  case BfButton::DOUBLE_PRESS:
  #ifdef ESP32
    g_apEnabled = !g_apEnabled;
    {
      Preferences prefs;
      prefs.begin("net", false);
      prefs.putBool("ap_en", g_apEnabled);
      prefs.end();
    }
    if (g_apEnabled) {
      WiFi.mode(WIFI_AP_STA);
      String apSsid = String(WIFI_HOSTNAME) + "-AP";
      bool apok = WiFi.softAP(apSsid.c_str());
      Serial.printf("[WIFI] SoftAP(%s) %s ip=%s\n", apSsid.c_str(), apok?"OK":"FAIL", WiFi.softAPIP().toString().c_str());
    } else {
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_STA);
      Serial.println("[WIFI] SoftAP OFF");
    }
  #endif
    break;

  case BfButton::LONG_PRESS:
    if (currentStatus != LOADING)
    {
      pluginManager.activatePersistedPlugin();
    }
    break;
  }
}

void baseSetup()
{
  Serial.begin(115200);

  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);

#if !defined(ESP32) && !defined(ESP8266)
  Screen.setup();
#endif

// server
#ifdef ENABLE_SERVER
  connectToWiFi();

  // set time server
  configTzTime(TZ_INFO, NTP_SERVER);

  initOTA(server);
  initWebsocketServer(server);
  initWebServer();
#endif
  pluginManager.addPlugin(new DrawPlugin());
  pluginManager.addPlugin(new BreakoutPlugin());
  pluginManager.addPlugin(new SnakePlugin());
  pluginManager.addPlugin(new GameOfLifePlugin());
  pluginManager.addPlugin(new StarsPlugin());
  pluginManager.addPlugin(new LinesPlugin());
  pluginManager.addPlugin(new CirclePlugin());
  pluginManager.addPlugin(new RainPlugin());
  pluginManager.addPlugin(new FireworkPlugin());
  pluginManager.addPlugin(new TetrisDemoPlugin());
  pluginManager.addPlugin(new ArcadeSpritesPlugin());
  pluginManager.addPlugin(new MoonPhasePlugin());
  pluginManager.addPlugin(new SunrisePlugin());
  pluginManager.addPlugin(new SunsetPlugin());
  pluginManager.addPlugin(new StockChartPlugin());

#ifdef ENABLE_SERVER
  pluginManager.addPlugin(new BigClockPlugin());
  pluginManager.addPlugin(new ClockPlugin());
  pluginManager.addPlugin(new PongClockPlugin());
  pluginManager.addPlugin(new TickingClockPlugin());
  pluginManager.addPlugin(new WeatherPlugin());
  pluginManager.addPlugin(new AnimationPlugin());
  pluginManager.addPlugin(new DDPPlugin());
  pluginManager.addPlugin(new ArtNetPlugin());
#endif

  pluginManager.init();
  Scheduler.init();

  StockService::getInstance().begin();
  StockService::getInstance().fetchNow();

  WeatherService::getInstance().begin();
  WeatherService::getInstance().fetchNow();

  btn.onPress(pressHandler).onDoublePress(pressHandler).onPressFor(pressHandler, 1000);
}

static unsigned long lastWeatherTick = 0;


#include "WeatherService.h"


#ifdef ESP32
TaskHandle_t screenDrawingTaskHandle = NULL;

void screenDrawingTask(void *parameter)
{
  Screen.setup();
  for (;;)
  {
    pluginManager.runActivePlugin();
    vTaskDelay(10);
  }
}

void setup()
{
  baseSetup();
  xTaskCreatePinnedToCore(screenDrawingTask,
                          "screenDrawingTask",
                          10000,
                          NULL,
                          1,
                          &screenDrawingTaskHandle,
                          0);
}
#endif
#ifdef ESP8266
void screenDrawingTask()
{
  Screen.setup();
  pluginManager.runActivePlugin();
  yield();
}

void setup()
{
  baseSetup();
  Scheduler.start();
}
#endif

void loop()
{
  static uint8_t taskCounter = 0;
  static unsigned long lastHeartbeat = 0;

  btn.read();

#ifdef ENABLE_SERVER
  ElegantOTA.loop();
#endif

#if !defined(ESP32) && !defined(ESP8266)
  pluginManager.runActivePlugin();
#endif

  if (currentStatus == NONE)
  {
    Scheduler.update();

    if ((taskCounter % 4) == 0)
    {
      Messages.scrollMessageEveryMinute();
    }

    // Background stock fetch
    if ((taskCounter % 64) == 0) { // ~1/16th der Weather-Frequenz
      StockService::getInstance().maybeFetch();
    }
  }

  // Background weather fetch (independent of active plugin)
  if ((taskCounter % 8) == 0)
  {
    WeatherService::getInstance().maybeFetch();
  }

  if ((taskCounter % 16) == 0)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      connectToWiFi();
    }
  }

  // Heartbeat: alle 5s Status ins Log
  if (millis() - lastHeartbeat >= 5000)
  {
    wl_status_t st = WiFi.status();
    IPAddress ip = WiFi.localIP();
    IPAddress gw = WiFi.gatewayIP();
    long rssi = WiFi.RSSI();
    uint32_t freeH = ESP.getFreeHeap();
    uint32_t minH  = ESP.getMinFreeHeap();
    uint32_t maxBlk= ESP.getMaxAllocHeap();
    Serial.printf("[HB] up=%lus status=%d ip=%s gw=%s rssi=%lddBm heap(free=%lu,min=%lu,maxBlk=%lu)\n",
                  millis() / 1000,
                  (int)st,
                  ip.toString().c_str(),
                  gw.toString().c_str(),
                  rssi,
                  (unsigned long)freeH,
                  (unsigned long)minH,
                  (unsigned long)maxBlk);
    lastHeartbeat = millis();
  }

  taskCounter++;
  if (taskCounter > 16)
  {
    taskCounter = 0;
  }

#ifdef ENABLE_SERVER
  cleanUpClients();
#endif
  delay(1);
}
