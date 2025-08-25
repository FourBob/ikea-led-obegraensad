#pragma once
#include <Arduino.h>
#include <vector>

struct StockData {
  float close[16];
  uint8_t count = 0; // number of valid points in close[]
  unsigned long timestamp = 0; // millis when fetched
  bool valid = false;
};

class StockService {
public:
  static StockService& getInstance(){ static StockService s; return s; }

  void begin();
  void setIntervalMinutes(uint16_t minutes);
  void maybeFetch();
  bool fetchNow();
  const StockData& get() const { return data_; }

private:
  bool performFetch(StockData &out);
  String buildUrl() const;

  unsigned long lastFetch_ = 0;
  uint32_t intervalMs_ = 60UL*60UL*1000UL; // default 60 min
  uint8_t retryCount = 0;
  unsigned long nextRetryAt = 0;

  StockData data_{};
};

