#pragma once

#include "ESPAsyncWebServer.h"

void handleMessage(AsyncWebServerRequest *request);
void handleMessageRemove(AsyncWebServerRequest *request);
void handleGetInfo(AsyncWebServerRequest *request);
void handleSetPlugin(AsyncWebServerRequest *request);
void handleSetBrightness(AsyncWebServerRequest *request);
void handleGetData(AsyncWebServerRequest *request);
void handleSetSchedule(AsyncWebServerRequest *request);
void handleClearSchedule(AsyncWebServerRequest *request);
void handleStopSchedule(AsyncWebServerRequest *request);
void handleStartSchedule(AsyncWebServerRequest *request);
void handleClearStorage(AsyncWebServerRequest *request);

// New: day/night scheduling endpoints
void handleSetScheduleDay(AsyncWebServerRequest *request);
void handleSetScheduleNight(AsyncWebServerRequest *request);
void handleSetScheduleBounds(AsyncWebServerRequest *request);