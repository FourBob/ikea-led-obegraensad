#pragma once
#include "PluginManager.h"
#include "StockService.h"

class StockChartPlugin : public Plugin {
public:
  void setup() override {}
  void loop() override;
  const char* getName() const override { return "Stock Chart"; }
};

