#pragma once
#include "storage.h"


#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#endif
#include <ArduinoJson.h>

struct WeatherData {
  int tempC = 0;
  int weatherCode = 0;
  // Moon data from wttr.in weather[0].astronomy[0]
  int moonIllum = -1;              // illumination percent (0..100), -1 if unknown
  char moonPhase[16] = {0};        // e.g., "Waxing Crescent", truncated

  // Sunrise/Sunset (minutes since midnight, -1 if unknown)
  int sunriseMinutes = -1;
  int sunsetMinutes  = -1;

  unsigned long timestamp = 0; // millis when fetched
  bool valid = false;
};

class WeatherService {
public:
  static WeatherService &getInstance();

  void begin();
  void setIntervalMinutes(uint16_t minutes);
  void maybeFetch();
  bool fetchNow();
  const WeatherData &get() const { return data_; }

private:
  WeatherService() = default;
  WeatherService(const WeatherService&) = delete;
  WeatherService& operator=(const WeatherService&) = delete;

  bool performFetch(WeatherData &out);

  unsigned long lastFetch_ = 0;
  uint32_t intervalMs_ = 30UL * 60UL * 1000UL; // default 30 min
  WeatherData data_;
};

