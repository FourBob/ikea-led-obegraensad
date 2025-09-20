#include "asyncwebserver.h"
#include "messages.h"
#include "webhandler.h"
#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#ifdef ENABLE_SERVER

AsyncWebServer server(80);

void initWebServer()
{
  Serial.println("[HTTP] initWebServer()");

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Accept, Content-Type, Authorization");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Credentials", "true");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  // Simple bearer token guard: if API_TOKEN is set, verify Authorization header
  auto authGuard = [](AsyncWebServerRequest *request) -> bool {
    #if defined(API_TOKEN)
    if (strlen(API_TOKEN) > 0) {
      if (!request->hasHeader("Authorization")) return false;
      const AsyncWebHeader* h = request->getHeader("Authorization");
      String expect = String("Bearer ") + API_TOKEN;
      if (!h || h->value() != expect) return false;
    }
    #endif
    return true;
  };

  // Health endpoint for quick checks
  server.on("/healthz", HTTP_GET, [](AsyncWebServerRequest *req){ req->send(200, "text/plain", "ok"); });

  // Ping endpoint to measure raw latency
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *req){
    uint32_t t0 = micros();
    IPAddress rip = req && req->client() ? req->client()->remoteIP() : IPAddress();
    req->send(200, "text/plain", "pong");
    uint32_t dt = micros() - t0;
    Serial.printf("[HTTP] /ping served to %s in %uus\n", rip.toString().c_str(), (unsigned)dt);
  });

  // Simple diagnostics (no JSON dep) with timing
  server.on("/diag", HTTP_GET, [](AsyncWebServerRequest *req){
    uint32_t t0 = micros();
    IPAddress rip = req && req->client() ? req->client()->remoteIP() : IPAddress();
    String body = String("ip=") + WiFi.localIP().toString() +
                  ", gw=" + WiFi.gatewayIP().toString() +
                  ", rssi=" + String(WiFi.RSSI()) +
                  ", heap_free=" + String(ESP.getFreeHeap());
    req->send(200, "text/plain", body);
    uint32_t dt = micros() - t0;
    Serial.printf("[HTTP] /diag served to %s in %uus\n", rip.toString().c_str(), (unsigned)dt);
  });

  server.on("/", HTTP_GET, startGui);
  server.onNotFound([&](AsyncWebServerRequest *request)
                    {
                      String url = request->url();
                      String method = (request->method() == HTTP_GET ? "GET" :
                                       request->method() == HTTP_POST ? "POST" :
                                       request->method() == HTTP_PUT ? "PUT" :
                                       request->method() == HTTP_PATCH ? "PATCH" :
                                       request->method() == HTTP_DELETE ? "DELETE" : "OTHER");
                      IPAddress rip;
                      if (request && request->client()) rip = request->client()->remoteIP();
                      Serial.printf("[HTTP] %s %s -> 404 from %s\n", method.c_str(), url.c_str(), rip.toString().c_str());
                      request->send(404, "text/plain", "Page not found!");
                    });

  // Route to handle  http://your-server/message?text=Hello&repeat=3&id=42&delay=30&graph=1,2,3,4&miny=0&maxy=15
  server.on("/api/message", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleMessage(req); });
  server.on("/api/removemessage", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleMessageRemove(req); });

  server.on("/api/info", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleGetInfo(req); });

  // Handle API request to set an active plugin by ID
  server.on("/api/plugin", HTTP_PATCH, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleSetPlugin(req); });

  // Handle API request to set the brightness (0..255);
  server.on("/api/brightness", HTTP_PATCH, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleSetBrightness(req); });
  server.on("/api/data", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleGetData(req); });

  // Scheduler
  server.on("/api/schedule", HTTP_POST, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleSetSchedule(req); });
  server.on("/api/schedule/day", HTTP_POST, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleSetScheduleDay(req); });
  server.on("/api/schedule/night", HTTP_POST, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleSetScheduleNight(req); });
  server.on("/api/schedule/bounds", HTTP_POST, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleSetScheduleBounds(req); });
  server.on("/api/schedule/clear", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleClearSchedule(req); });
  server.on("/api/schedule/stop", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleStopSchedule(req); });
  server.on("/api/schedule/start", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleStartSchedule(req); });

  server.on("/api/storage/clear", HTTP_GET, [=](AsyncWebServerRequest *req){ if(!authGuard(req)) { req->send(401, "text/plain", "Unauthorized"); return;} handleClearStorage(req); });

  server.begin();
  Serial.println("[HTTP] server.begin() on :80");
}

#endif
