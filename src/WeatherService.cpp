#include "WeatherService.h"
#include "secrets.h"
#include "constants.h"

#include <math.h>

#ifndef WEATHER_DEBUG
#define WEATHER_DEBUG 1
#endif
#if WEATHER_DEBUG
  #define WEATHER_LOGF(...) do { Serial.printf(__VA_ARGS__); } while(0)
  #define WEATHER_LOGLN(msg) do { Serial.println(msg); } while(0)
#else
  #define WEATHER_LOGF(...)
  #define WEATHER_LOGLN(msg)
#endif


// Defaults for OpenWeatherMap macros if not provided in secrets.h
#ifndef OWM_API_KEY
#define OWM_API_KEY ""
#endif
#ifndef OWM_UNITS
#define OWM_UNITS "metric"
#endif
#ifndef OWM_LANG
#define OWM_LANG "en"
#endif
#ifndef OWM_CA_CERT
#define OWM_CA_CERT ""
#endif

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
  // If backoff is active and due, try immediately irrespective of interval
  if (nextRetryAt && now >= nextRetryAt) {
    bool ok = fetchNow();
    if (ok) { retryCount = 0; nextRetryAt = 0; }
    else {
      // increase backoff up to ~15m
      retryCount = min<uint8_t>(retryCount + 1, 4);
      nextRetryAt = now + (60000UL << (retryCount - 1));
    }
    return;
  }
  if (now - lastFetch_ >= intervalMs_) {
    bool ok = fetchNow();
    if (!ok) {
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
  } else {
    // Ensure backoff retries even when fetchNow() is called directly (e.g., at boot)
    retryCount = min<uint8_t>(retryCount + 1, 4);
    nextRetryAt = millis() + (60000UL << (retryCount - 1));
  }
  lastFetch_ = millis();
  return ok;
}

bool WeatherService::performFetch(WeatherData &out) {
  // OpenWeather One Call 3.0
  String apiKey = String(OWM_API_KEY);
  if (apiKey.length() == 0) {
    Serial.println("[WEATHER] Missing OWM_API_KEY");
    return false;
  }

  // Resolve coordinates from WEATHER_LOCATION
  String loc = String(WEATHER_LOCATION);
  loc.trim();
  String latStr, lonStr;
  WEATHER_LOGF("[WEATHER] location='%s'\n", loc.c_str());

  int comma = loc.indexOf(',');
  if (comma > 0) {
    latStr = loc.substring(0, comma); latStr.trim();
    lonStr = loc.substring(comma + 1); lonStr.trim();
  } else {
    // Geocoding: city -> lat/lon
    String q = loc; q.replace(" ", "%20");
    String geourl = String("https://api.openweathermap.org/geo/1.0/direct?q=") + q + String("&limit=1&appid=") + apiKey;
#ifdef ESP32
    WiFiClientSecure gclient;
    #ifdef OWM_CA_CERT
    if (strlen(OWM_CA_CERT) > 0) gclient.setCACert(OWM_CA_CERT); else gclient.setInsecure();
    #else
    gclient.setInsecure();
    #endif
    HTTPClient ghttp; ghttp.begin(gclient, geourl);
#endif
#ifdef ESP8266
    WiFiClientSecure gclient; gclient.setInsecure();
    HTTPClient ghttp; ghttp.begin(gclient, geourl);
#endif
    ghttp.setTimeout(10000);
    ghttp.addHeader("Accept", "application/json");
    int gcode = ghttp.GET();
      WEATHER_LOGF("[WEATHER] geocode GET...\n");

    if (gcode != HTTP_CODE_OK) { ghttp.end(); return false; }
      WEATHER_LOGF("[WEATHER] geocode HTTP %d for '%s'\n", gcode, loc.c_str());

    String gpayload = ghttp.getString();
    StaticJsonDocument<128> gfilter; gfilter[0]["lat"] = true; gfilter[0]["lon"] = true;
    DynamicJsonDocument gdoc(512);
    auto gerr = deserializeJson(gdoc, gpayload, DeserializationOption::Filter(gfilter));
    if (gerr) { ghttp.end(); return false; }
    if (!gdoc[0]["lat"].isNull() && !gdoc[0]["lon"].isNull()) {
      latStr = String(gdoc[0]["lat"].as<float>(), 6);
      lonStr = String(gdoc[0]["lon"].as<float>(), 6);
    }
      WEATHER_LOGF("[WEATHER] geocode result lat=%s lon=%s\n", latStr.c_str(), lonStr.c_str());

    ghttp.end();
    if (latStr.length() == 0 || lonStr.length() == 0) return false;
  }
  WEATHER_LOGF("[WEATHER] coords lat=%s lon=%s (from '%s')\n", latStr.c_str(), lonStr.c_str(), loc.c_str());


  String units = "metric";
#ifdef OWM_UNITS
  units = String(OWM_UNITS);
#endif
  String lang = "en";
#ifdef OWM_LANG
  lang = String(OWM_LANG);
#endif
  WEATHER_LOGF("[WEATHER] units=%s lang=%s\n", units.c_str(), lang.c_str());


  // Try One Call 3.0, fallback to 2.5 if not available on this API key
  String url3 = String("https://api.openweathermap.org/data/3.0/onecall?lat=") + latStr + "&lon=" + lonStr + "&exclude=minutely,hourly,alerts&units=" + units + "&lang=" + lang + "&appid=" + apiKey;
  WEATHER_LOGF("[WEATHER] OneCall 3.0 request lat=%s lon=%s units=%s lang=%s\n", latStr.c_str(), lonStr.c_str(), units.c_str(), lang.c_str());


#ifdef ESP32
  WiFiClientSecure client;
  #ifdef OWM_CA_CERT
  if (strlen(OWM_CA_CERT) > 0) client.setCACert(OWM_CA_CERT); else client.setInsecure();
  #else
  client.setInsecure();
  #endif
  HTTPClient http; http.begin(client, url3);
#endif
#ifdef ESP8266
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; http.begin(client, url3);
#endif

  http.setTimeout(10000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", "esp32-display/1.0");

  int code = http.GET();
  WEATHER_LOGF("[WEATHER] OneCall 3.0 HTTP %d\n", code);

  if (code != HTTP_CODE_OK) {
    Serial.printf("[WEATHER] OneCall 3.0 HTTP %d, trying 2.5...\n", code);
    http.end();

    // Fallback to One Call 2.5
#ifdef ESP32
    WiFiClientSecure client2;
    #ifdef OWM_CA_CERT
    if (strlen(OWM_CA_CERT) > 0) client2.setCACert(OWM_CA_CERT); else client2.setInsecure();
    #else
    client2.setInsecure();
    #endif
    HTTPClient http2;
    String url25 = String("https://api.openweathermap.org/data/2.5/onecall?lat=") + latStr + "&lon=" + lonStr + "&exclude=minutely,hourly,alerts&units=" + units + "&lang=" + lang + "&appid=" + apiKey;
    http2.begin(client2, url25);
#endif
#ifdef ESP8266
    WiFiClientSecure client2; client2.setInsecure();
    HTTPClient http2;
    String url25 = String("https://api.openweathermap.org/data/2.5/onecall?lat=") + latStr + "&lon=" + lonStr + "&exclude=minutely,hourly,alerts&units=" + units + "&lang=" + lang + "&appid=" + apiKey;
    http2.begin(client2, url25);
#endif
    WEATHER_LOGF("[WEATHER] OneCall 2.5 request lat=%s lon=%s units=%s lang=%s\n", latStr.c_str(), lonStr.c_str(), units.c_str(), lang.c_str());

    http2.setTimeout(10000);
    http2.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http2.addHeader("Accept", "application/json");
    http2.addHeader("User-Agent", "esp32-display/1.0");
    code = http2.GET();
    WEATHER_LOGF("[WEATHER] OneCall 2.5 HTTP %d\n", code);

    if (code != HTTP_CODE_OK) { Serial.printf("[WEATHER] OneCall 2.5 HTTP %d\n", code); http2.end(); return false; }
    String payload = http2.getString();
    WEATHER_LOGF("[WEATHER] OneCall 2.5 payload %u bytes\n", (unsigned)payload.length());

    StaticJsonDocument<192> filter; // same filter works for 2.5
    filter["timezone_offset"] = true;
    filter["current"]["temp"] = true;
    filter["current"]["sunrise"] = true;
    filter["current"]["sunset"] = true;
    filter["current"]["weather"][0]["id"] = true;
    filter["daily"][0]["moon_phase"] = true;
    DynamicJsonDocument doc(3072);
    auto err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    if (err) { WEATHER_LOGF("[WEATHER] OneCall 2.5 JSON error: %s\n", err.c_str()); http2.end(); return false; }

    long tz = doc["timezone_offset"].as<long>();
    out.tempC = round(doc["current"]["temp"].as<float>());
    out.weatherCode = doc["current"]["weather"][0]["id"].as<int>();
    long sr = doc["current"]["sunrise"].as<long>();
    long ss = doc["current"]["sunset"].as<long>();
    if (sr > 0) { long localSr = sr + tz; int minutes = (int)((localSr % 86400L) / 60L); if (minutes < 0) minutes += 1440; out.sunriseMinutes = minutes; }
    if (ss > 0) { long localSs = ss + tz; int minutes = (int)((localSs % 86400L) / 60L); if (minutes < 0) minutes += 1440; out.sunsetMinutes = minutes; }
    WEATHER_LOGF("[WEATHER] tz=%ld srMin=%d ssMin=%d\n", tz, out.sunriseMinutes, out.sunsetMinutes);

    float phase = doc["daily"][0]["moon_phase"].as<float>();
    if (phase >= 0.0f) {
      float illum = (1.0f - cosf(2.0f * (float)M_PI * phase)) * 50.0f;
      out.moonIllum = (int)roundf(illum);
    }
    WEATHER_LOGF("[WEATHER] moon phase=%.3f illum=%d%% (2.5)\n", phase, out.moonIllum);

    http2.end();
    Serial.println("[WEATHER] ok OneCall 2.5");
    return true;
  }

  String payload = http.getString();
  WEATHER_LOGF("[WEATHER] OneCall 3.0 payload %u bytes\n", (unsigned)payload.length());

  // Filter only needed fields from One Call
  StaticJsonDocument<192> filter;
  filter["timezone_offset"] = true;
  filter["current"]["temp"] = true;
  filter["current"]["sunrise"] = true;
  filter["current"]["sunset"] = true;
  filter["current"]["weather"][0]["id"] = true;
  filter["daily"][0]["moon_phase"] = true;
  DynamicJsonDocument doc(3072);
  auto err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
  if (err) { WEATHER_LOGF("[WEATHER] OneCall 3.0 JSON error: %s\n", err.c_str()); http.end(); return false; }

  long tz = doc["timezone_offset"].as<long>();
  out.tempC = round(doc["current"]["temp"].as<float>());
  out.weatherCode = doc["current"]["weather"][0]["id"].as<int>();

  long sr = doc["current"]["sunrise"].as<long>();
  long ss = doc["current"]["sunset"].as<long>();
  if (sr > 0) { long localSr = sr + tz; int minutes = (int)((localSr % 86400L) / 60L); if (minutes < 0) minutes += 1440; out.sunriseMinutes = minutes; }
  if (ss > 0) { long localSs = ss + tz; int minutes = (int)((localSs % 86400L) / 60L); if (minutes < 0) minutes += 1440; out.sunsetMinutes = minutes; }
  WEATHER_LOGF("[WEATHER] tz=%ld srMin=%d ssMin=%d\n", tz, out.sunriseMinutes, out.sunsetMinutes);


  // Moon phase (0=new, 0.25=first quarter, 0.5=full, 0.75=last quarter)
  float phase = doc["daily"][0]["moon_phase"].as<float>();
  if (phase >= 0.0f) {
    // Approximate illumination percent
    float illum = (1.0f - cosf(2.0f * (float)M_PI * phase)) * 50.0f; // 0..100
    int illPct = (int)roundf(illum);
    out.moonIllum = illPct;
    const char* label = "";
    if (phase < 0.0625f || phase > 0.9375f) label = "New";
    else if (phase < 0.1875f) label = "Wax Cres";
    else if (phase < 0.3125f) label = "First Q";
    else if (phase < 0.4375f) label = "Wax Gib";
    else if (phase < 0.5625f) label = "Full";
    else if (phase < 0.6875f) label = "Wan Gib";
    else if (phase < 0.8125f) label = "Last Q";
    else label = "Wan Cres";
    strncpy(out.moonPhase, label, sizeof(out.moonPhase)-1);
    out.moonPhase[sizeof(out.moonPhase)-1] = '\0';
  }
  WEATHER_LOGF("[WEATHER] moon phase=%.3f illum=%d%%\n", phase, out.moonIllum);


  Serial.printf("[WEATHER] ok OneCall tempC=%d id=%d sr=%d ss=%d moon=%d%%\n", out.tempC, out.weatherCode, out.sunriseMinutes, out.sunsetMinutes, out.moonIllum);
  http.end();
  return true;
}

