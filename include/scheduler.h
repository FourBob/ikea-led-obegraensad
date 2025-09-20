#pragma once

#include "PluginManager.h"
#include <vector>
#include <Arduino.h>

struct ScheduleItem
{
  int pluginId;
  unsigned long duration; // Duration in milliseconds
};

class PluginScheduler
{
private:
  PluginScheduler() = default;
  unsigned long lastSwitch = 0;
  size_t currentIndex = 0;
  bool lastWasDay = true;

  // Time boundaries in minutes since midnight (local time)
  int dayStartMins = 7 * 60;   // 07:00 default
  int nightStartMins = 19 * 60; // 19:00 default

public:
  static PluginScheduler &getInstance();

  PluginScheduler(const PluginScheduler &) = delete;
  PluginScheduler &operator=(const PluginScheduler &) = delete;

  bool isActive = false;

  // Active schedule used by the runtime (kept for backward compatibility)
  std::vector<ScheduleItem> schedule;
  // Separate day/night schedules (sources of truth)
  std::vector<ScheduleItem> scheduleDay;
  std::vector<ScheduleItem> scheduleNight;

  void addItem(int pluginId, unsigned long durationSeconds);
  void clearSchedule(bool emptyStorage = false);
  void start();
  void stop();
  void update();
  void init();

  // Legacy setter: applies to both day and night for backward compatibility
  bool setScheduleByJSONString(String scheduleJson);
  // New setters for day/night
  bool setDayScheduleByJSONString(String scheduleJson);
  bool setNightScheduleByJSONString(String scheduleJson);

  // Configure boundaries
  void setBoundsByMinutes(int dayStart, int nightStart);
  bool setBoundsByHHMM(String dayStart, String nightStart);
  String getDayStartHHMM() const;
  String getNightStartHHMM() const;
  bool isDayNow() const;

private:
  void switchToCurrentPlugin();
  void rebuildActiveFromCurrentPeriod(bool forceReset);
};

extern PluginScheduler &Scheduler;