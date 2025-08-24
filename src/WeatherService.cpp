#include "WeatherService.h"
#include "secrets.h"
#include "constants.h"

WeatherService &WeatherService::getInstance() {
  static WeatherService instance;
  return instance;
}

void WeatherService::begin() {
  // Interval from secrets if provided
#ifdef WEATHER_UPDATE_MINUTES
  setIntervalMinutes(WEATHER_UPDATE_MINUTES);
#endif
#ifdef ENABLE_STORAGE
  // Load last cached weather from NVS for immediate display
  storage.begin("led-wall", true);
  int16_t temp = storage.getShort("wTemp", INT16_MIN);
  int16_t code = storage.getShort("wCode", INT16_MIN);
  uint32_t ts = storage.getUInt("wTime", 0);
  storage.end();
  if (temp != INT16_MIN && code != INT16_MIN) {
    data_.tempC = temp;
    data_.weatherCode = code;
    data_.timestamp = ts;
    data_.valid = true;
  }
#endif
}


static uint8_t retryCount = 0;
static unsigned long nextRetryAt = 0;

void WeatherService::setIntervalMinutes(uint16_t minutes) {
  if (minutes == 0) minutes = 30;
  intervalMs_ = (uint32_t)minutes * 60UL * 1000UL;
}

void WeatherService::maybeFetch() {
  unsigned long now = millis();
  if (nextRetryAt && now < nextRetryAt) return; // backoff in effect
  if (now - lastFetch_ >= intervalMs_) {
    bool ok = fetchNow();
    if (!ok) {
      // exponential backoff: 1m, 2m, 4m (cap 15m)
      retryCount = min<uint8_t>(retryCount + 1, 4);
      nextRetryAt = now + (60000UL << (retryCount - 1));
    } else {
      retryCount = 0;
      nextRetryAt = 0;
    }
  }
}

bool WeatherService::fetchNow() {
#if defined(ESP32) || defined(ESP8266)
  if (WiFi.status() != WL_CONNECTED) return false;
#endif
  WeatherData tmp;
  bool ok = performFetch(tmp);
  if (ok) {
    data_ = tmp;
    data_.valid = true;
    data_.timestamp = millis();
#ifdef ENABLE_STORAGE
    // Persist for next boot
    storage.begin("led-wall", false);
    storage.putShort("wTemp", (int16_t)data_.tempC);
    storage.putShort("wCode", (int16_t)data_.weatherCode);
    storage.putUInt("wTime", (uint32_t)data_.timestamp);
    storage.end();
#endif
  }
  lastFetch_ = millis();
  return ok;
}

bool WeatherService::performFetch(WeatherData &out) {
  String url = String("https://wttr.in/") + String(WEATHER_LOCATION) + String("?format=j1&lang=en");

#ifdef ESP32
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
#endif
#ifdef ESP8266
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, url);
#endif

  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", "esp32-display/1.0");

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  String payload = http.getString();
  StaticJsonDocument<128> filter;
  filter["current_condition"][0]["temp_C"] = true;
  filter["current_condition"][0]["weatherCode"] = true;
  DynamicJsonDocument doc(512);
  auto err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (err) {
    http.end();
    return false;
  }

  out.tempC = round(doc["current_condition"][0]["temp_C"].as<float>());
  out.weatherCode = doc["current_condition"][0]["weatherCode"].as<int>();

  http.end();
  return true;
}

