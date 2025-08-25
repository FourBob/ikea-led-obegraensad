#include "StockService.h"
#include "secrets.h"
#ifdef ESP32
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#endif
#include <ArduinoJson.h>

#ifndef STOCK_SYMBOL
#define STOCK_SYMBOL "aapl.us"
#endif

void StockService::begin(){ lastFetch_=0; retryCount=0; nextRetryAt=0; }

void StockService::setIntervalMinutes(uint16_t minutes){ if(minutes==0) minutes=60; intervalMs_=(uint32_t)minutes*60UL*1000UL; }

String StockService::buildUrl() const{
  // Use stooq free CSV API for simplicity: https://stooq.com/q/d/l/?s=aapl.us&i=d
  String s = STOCK_SYMBOL; s.toLowerCase();
  if (s.indexOf('.') < 0) s += ".us"; // default to US ticker if no suffix given
  return String("https://stooq.com/q/d/l/?s=") + s + "&i=d";
}

bool StockService::performFetch(StockData &out){
#ifdef ESP32
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http; http.setTimeout(8000);
  auto url = buildUrl();
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != 200) { http.end(); return false; }
  String csv = http.getString();
  http.end();
  // CSV header: Date,Open,High,Low,Close,Volume
  // Parse forward and keep last 16 closes
  float ring[16]; int ringCount=0;
  int start = 0;
  while (start < csv.length()){
    int end = csv.indexOf('\n', start);
    if (end < 0) end = csv.length();
    String line = csv.substring(start, end);
    start = end + 1;
    if (line.length() == 0) continue;
    if (line.startsWith("Date")) continue; // header
    // strip CR
    if (line.endsWith("\r")) line.remove(line.length()-1);

    // columns: Date,Open,High,Low,Close,Volume
    int c1 = line.indexOf(','); if (c1<0) continue;
    int c2 = line.indexOf(',', c1+1); if (c2<0) continue;
    int c3 = line.indexOf(',', c2+1); if (c3<0) continue;
    int c4 = line.indexOf(',', c3+1); if (c4<0) continue;
    int c5 = line.indexOf(',', c4+1); // may be -1
    String closeStr = (c5>=0) ? line.substring(c4+1, c5) : line.substring(c4+1);
    float val = closeStr.toFloat();
    if (val <= 0) continue;
    if (ringCount < 16) ring[ringCount++] = val;
    else { for (int i=0;i<15;i++) ring[i]=ring[i+1]; ring[15]=val; }
  }
  if (ringCount == 0) return false;
  for (int i=0;i<ringCount; ++i) out.close[i] = ring[i];
  out.count = ringCount;
  out.timestamp = millis();
  out.valid = true;
  return true;
#else
  return false;
#endif
}

bool StockService::fetchNow(){
  StockData tmp{};
  bool ok = performFetch(tmp);
  if (ok){ data_ = tmp; lastFetch_=millis(); retryCount=0; nextRetryAt=0; }
  return ok;
}

void StockService::maybeFetch(){
  unsigned long now = millis();
  if (nextRetryAt && now >= nextRetryAt){ if(fetchNow()) { retryCount=0; nextRetryAt=0; } else { retryCount = min<uint8_t>(retryCount+1,4); nextRetryAt = now + (60000UL << (retryCount-1)); } return; }
  if (now - lastFetch_ >= intervalMs_) {
    if(!fetchNow()){ retryCount = min<uint8_t>(retryCount+1,4); nextRetryAt = now + (60000UL << (retryCount-1)); }
  }
}

