#include "PluginManager.h"
#include "scheduler.h"
#include "plugins/AnimationPlugin.h"

#ifdef ENABLE_SERVER

#include <set>

AsyncWebSocket ws("/ws");
static const char* kApiToken = API_TOKEN;
static std::set<uint32_t> wsAuthed; // client ids with auth

void sendInfo()
{
  static unsigned long lastSent = 0;
  unsigned long now = millis();
  // Throttle broadcast a bit more to avoid WS queue overflow
  if (now - lastSent < 100) return;

  // If token is required but no client is authenticated yet, skip
  if (kApiToken && strlen(kApiToken) > 0) {
    if (wsAuthed.empty()) return;
  } else {
    // No token: if there are no clients, or not writable, skip
    if (ws.count() == 0) return;
    if (!ws.availableForWriteAll()) return;
  }

  lastSent = now;

  DynamicJsonDocument jsonDocument(8192);
  if (currentStatus == NONE)
  {
    for (int j = 0; j < ROWS * COLS; j++)
    {
      jsonDocument["data"][j] = Screen.getRenderBuffer()[j];
    }
  }

  jsonDocument["status"] = currentStatus;
  if (pluginManager.getActivePlugin()) {
    jsonDocument["plugin"] = pluginManager.getActivePlugin()->getId();
  } else {
    jsonDocument["plugin"] = -1;
  }
  jsonDocument["event"] = "info";
  jsonDocument["rotation"] = Screen.currentRotation;
  jsonDocument["brightness"] = Screen.getCurrentBrightness();
  jsonDocument["scheduleActive"] = Scheduler.isActive;

  // Active schedule (compat)
  JsonArray scheduleArray = jsonDocument.createNestedArray("schedule");
  for (const auto &item : Scheduler.schedule) {
    JsonObject o = scheduleArray.createNestedObject();
    o["pluginId"] = item.pluginId;
    o["duration"] = item.duration / 1000; // seconds
  }

  // Day schedule
  JsonArray scheduleDay = jsonDocument.createNestedArray("scheduleDay");
  for (const auto &item : Scheduler.scheduleDay) {
    JsonObject o = scheduleDay.createNestedObject();
    o["pluginId"] = item.pluginId;
    o["duration"] = item.duration / 1000; // seconds
  }
  // Night schedule
  JsonArray scheduleNight = jsonDocument.createNestedArray("scheduleNight");
  for (const auto &item : Scheduler.scheduleNight) {
    JsonObject o = scheduleNight.createNestedObject();
    o["pluginId"] = item.pluginId;
    o["duration"] = item.duration / 1000; // seconds
  }
  // Bounds and current period
  jsonDocument["dayStart"] = Scheduler.getDayStartHHMM();
  jsonDocument["nightStart"] = Scheduler.getNightStartHHMM();
  jsonDocument["currentPeriod"] = Scheduler.isDayNow() ? "day" : "night";

  // Build metadata
#ifdef BUILD_TIME_STR
  jsonDocument["buildTime"] = BUILD_TIME_STR;
#endif
#ifdef APP_VERSION_STR
  jsonDocument["version"] = APP_VERSION_STR;
#endif

  JsonArray plugins = jsonDocument.createNestedArray("plugins");
  std::vector<Plugin *> &allPlugins = pluginManager.getAllPlugins();
  for (Plugin *plugin : allPlugins)
  {
    JsonObject object = plugins.createNestedObject();
    object["id"] = plugin->getId();
    object["name"] = plugin->getName();
  }
  String output;
  serializeJson(jsonDocument, output);

  if (kApiToken && strlen(kApiToken) > 0) {
    // Send only to authorized clients that are writable
    bool anySent = false;
    for (auto id : wsAuthed) {
      if (ws.availableForWrite(id)) {
        ws.text(id, output);
        anySent = true;
      }
    }
    (void)anySent; // suppress unused warning
  } else {
    // Double-check writability before broadcast
    if (ws.availableForWriteAll()) {
      ws.textAll(output);
    }
  }

  jsonDocument.clear();
}

void onWsEvent(
    AsyncWebSocket *server,
    AsyncWebSocketClient *client,
    AwsEventType type,
    void *arg,
    uint8_t *data,
    size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    // Simple token check via query string: ws://host/ws?token=...
    if (kApiToken && strlen(kApiToken) > 0) {
      // AsyncWebSocket unfortunately doesn't expose URL here; require an initial auth message instead
      // Client must send a JSON: {"event":"auth","token":"..."} as the first message
    }
    sendInfo();
  }

  if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
      // Auth gate: require first text frame to be auth when token is set
      if (kApiToken && strlen(kApiToken) > 0 && wsAuthed.find(client->id()) == wsAuthed.end()) {
        // Peek minimal JSON: allow only auth as first message
      }

    if (info->final && info->index == 0 && info->len == len)
    {
      if (info->opcode == WS_BINARY && currentStatus == WSBINARY && info->len == 256)
      {
        if (kApiToken && strlen(kApiToken) > 0 && wsAuthed.find(client->id()) == wsAuthed.end()) return;
        Screen.setRenderBuffer(data, true);
      }
      else if (info->opcode == WS_TEXT)
      {
        DynamicJsonDocument wsRequest(1024);
        DeserializationError error = deserializeJson(wsRequest, (const char *)data, len);

        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }
        // If token is set, require first message to be {"event":"auth","token":"..."}
        if (kApiToken && strlen(kApiToken) > 0 && wsAuthed.find(client->id()) == wsAuthed.end()) {
          const char* ev = wsRequest["event"] | "";
          const char* tok = wsRequest["token"] | "";
          if (strcmp(ev, "auth") != 0 || String(tok) != String(kApiToken)) {
            client->close(1008, "Unauthorized");
            return;
          }
          wsAuthed.insert(client->id());
          return; // don't process further for auth frame
        }

        pluginManager.getActivePlugin()->websocketHook(wsRequest);

        const char *event = wsRequest["event"];

        if (!strcmp(event, "plugin"))
        {
          int pluginId = wsRequest["plugin"];

          Scheduler.clearSchedule();
          pluginManager.setActivePluginById(pluginId);
          sendInfo();
        }
        else if (!strcmp(event, "persist-plugin"))
        {
          pluginManager.persistActivePlugin();
        }
        else if (!strcmp(event, "rotate"))
        {
          bool isRight = (bool)!strcmp(wsRequest["direction"], "right");
          Screen.setCurrentRotation((Screen.currentRotation + (isRight ? 1 : 3)) % 4, true);
        }
        else if (!strcmp(event, "info"))
        {
          sendInfo();
        }
        else if (!strcmp(event, "brightness"))
        {
          uint8_t brightness = wsRequest["brightness"].as<uint8_t>();
          Screen.setBrightness(brightness, true);
        }
        else if (!strcmp(event, "get-animation"))
        {
          // Minimal fetch: return current frames from Animation plugin as 32-byte arrays
          Plugin* p = pluginManager.getActivePlugin();
          if (p && strcmp(p->getName(), "Animation") == 0) {
            AnimationPlugin* ap = static_cast<AnimationPlugin*>(p);
            const auto &frames = ap->getFrames();
            DynamicJsonDocument resp(4096);
            resp["event"] = "animation-frames";
            resp["screens"] = (int)frames.size();
            JsonArray dataArr = resp.createNestedArray("data");
            for (const auto &f : frames) {
              JsonArray row = dataArr.createNestedArray();
              for (int k = 0; k < (int)f.size(); ++k) {
                row.add(f[k]);
              }
            }
            String out;
            serializeJson(resp, out);
            if (ws.availableForWrite(client->id())) {
              ws.text(client->id(), out);
            }
          } else {
            // Not animation plugin: return empty response
            DynamicJsonDocument resp(256);
            resp["event"] = "animation-frames";
            resp["screens"] = 0;
            resp.createNestedArray("data");
            String out;
            serializeJson(resp, out);
            if (ws.availableForWrite(client->id())) {
              ws.text(client->id(), out);
            }
          }
        }
      }
    }
  }
}

void initWebsocketServer(AsyncWebServer &server)
{
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);
}

void cleanUpClients()
{
  ws.cleanupClients();
}

#endif