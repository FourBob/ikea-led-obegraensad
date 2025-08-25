#pragma once

#include "constants.h"

#ifdef ENABLE_SERVER

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "webgui.h"


// Optional simple bearer token for API/WS
#ifndef API_TOKEN
#define API_TOKEN ""
#endif

extern AsyncWebServer server;
void initWebServer();

#endif
