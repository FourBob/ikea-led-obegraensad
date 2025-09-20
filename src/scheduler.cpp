#include "scheduler.h"
#include "websocket.h"
#include <time.h>


PluginScheduler &PluginScheduler::getInstance()
{
  static PluginScheduler instance;
  return instance;
}

void PluginScheduler::addItem(int pluginId, unsigned long durationSeconds)
{
  ScheduleItem item = {
      .pluginId = pluginId,
      .duration = durationSeconds * 1000 // Convert to milliseconds
  };
  schedule.push_back(item);
}

void PluginScheduler::clearSchedule(bool emptyStorage)
{
  currentIndex = 0;
  isActive = false;
#ifdef ENABLE_STORAGE
  if (emptyStorage)
  {
    schedule.clear();
    scheduleDay.clear();
    scheduleNight.clear();
    storage.begin("led-wall");
    storage.putString("schedule", ""); // legacy
    storage.putString("schedule_day", "");
    storage.putString("schedule_night", "");
    storage.putInt("scheduleactive", 0);
    storage.end();
  }
#else
  if (emptyStorage)
  {
    schedule.clear();
    scheduleDay.clear();
    scheduleNight.clear();
  }
#endif
}

void PluginScheduler::start()
{
  // Ensure active schedule matches current period before starting
  rebuildActiveFromCurrentPeriod(true);
  if (!schedule.empty())
  {
    currentIndex = 0;
    lastSwitch = millis();
    isActive = true;
#ifdef ENABLE_STORAGE
    storage.begin("led-wall", false);
    storage.putInt("scheduleactive", 1);
    storage.end();
#endif
    switchToCurrentPlugin();
  }
}

void PluginScheduler::stop()
{
  isActive = false;
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", false);
  storage.putInt("scheduleactive", 0);
  storage.end();
#endif
}

void PluginScheduler::update()
{
  // Ensure active schedule matches current period; switch on boundary
  rebuildActiveFromCurrentPeriod(false);
  if (!isActive || schedule.empty())
    return;

  unsigned long currentTime = millis();
  if (currentTime - lastSwitch >= schedule[currentIndex].duration)
  {
    currentIndex = (currentIndex + 1) % schedule.size();
    lastSwitch = currentTime;
    switchToCurrentPlugin();
  }
}

void PluginScheduler::switchToCurrentPlugin()
{
  if (currentIndex < schedule.size())
  {
    pluginManager.setActivePluginById(schedule[currentIndex].pluginId);
#ifdef ENABLE_SERVER
    sendInfo();
#endif
  }
}

void PluginScheduler::init()
{
#ifdef ENABLE_STORAGE
  storage.begin("led-wall", true);
  int storedActive = storage.getInt("scheduleactive", 0);
  String sDay = storage.getString("schedule_day");
  String sNight = storage.getString("schedule_night");
  String sLegacy = storage.getString("schedule");
  dayStartMins = storage.getInt("dayStartMins", dayStartMins);
  nightStartMins = storage.getInt("nightStartMins", nightStartMins);
  storage.end();

  if (sDay.length() > 0) {
    setDayScheduleByJSONString(sDay);
  }
  if (sNight.length() > 0) {
    setNightScheduleByJSONString(sNight);
  }
  if (sDay.length() == 0 && sNight.length() == 0 && sLegacy.length() > 0) {
    // Backward compatibility: one schedule applied to both day and night
    setScheduleByJSONString(sLegacy);
  }

  isActive = (storedActive == 1);
  if (isActive) {
    rebuildActiveFromCurrentPeriod(true);
  }
#endif
}

bool PluginScheduler::setScheduleByJSONString(String scheduleJson)
{
  if (scheduleJson.length() == 0) return false;
  // Apply to both day and night for backward compatibility
  bool okDay = setDayScheduleByJSONString(scheduleJson);
  bool okNight = setNightScheduleByJSONString(scheduleJson);
#ifdef ENABLE_STORAGE
  if (okDay && okNight) {
    storage.begin("led-wall");
    storage.putString("schedule", scheduleJson); // legacy key
    storage.end();
  }
#endif
  return okDay && okNight;
}

bool PluginScheduler::setDayScheduleByJSONString(String scheduleJson)
{
  if (scheduleJson.length() == 0) return false;
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, scheduleJson)) return false;
  scheduleDay.clear();
  for (const auto &item : doc.as<JsonArray>()) {
    if (item.containsKey("pluginId") && item.containsKey("duration")) {
      ScheduleItem si{ item["pluginId"].as<int>(), item["duration"].as<unsigned long>() * 1000UL };
      scheduleDay.push_back(si);
    }
  }
#ifdef ENABLE_STORAGE
  storage.begin("led-wall");
  storage.putString("schedule_day", scheduleJson);
  storage.end();
#endif
  rebuildActiveFromCurrentPeriod(true);
  return true;
}

bool PluginScheduler::setNightScheduleByJSONString(String scheduleJson)
{
  if (scheduleJson.length() == 0) return false;
  DynamicJsonDocument doc(2048);
  if (deserializeJson(doc, scheduleJson)) return false;
  scheduleNight.clear();
  for (const auto &item : doc.as<JsonArray>()) {
    if (item.containsKey("pluginId") && item.containsKey("duration")) {
      ScheduleItem si{ item["pluginId"].as<int>(), item["duration"].as<unsigned long>() * 1000UL };
      scheduleNight.push_back(si);
    }
  }
#ifdef ENABLE_STORAGE
  storage.begin("led-wall");
  storage.putString("schedule_night", scheduleJson);
  storage.end();
#endif
  rebuildActiveFromCurrentPeriod(true);
  return true;
}

void PluginScheduler::setBoundsByMinutes(int dayStart, int nightStart)
{
  if (dayStart < 0 || dayStart >= 24*60) return;
  if (nightStart < 0 || nightStart >= 24*60) return;
  dayStartMins = dayStart;
  nightStartMins = nightStart;
#ifdef ENABLE_STORAGE
  storage.begin("led-wall");
  storage.putInt("dayStartMins", dayStartMins);
  storage.putInt("nightStartMins", nightStartMins);
  storage.end();
#endif
  rebuildActiveFromCurrentPeriod(true);
}

static bool parseHHMMToMinutes(const String &hhmm, int &out)
{
  int sep = hhmm.indexOf(':');
  if (sep <= 0) return false;
  int hh = hhmm.substring(0, sep).toInt();
  int mm = hhmm.substring(sep + 1).toInt();
  if (hh < 0 || hh > 23 || mm < 0 || mm > 59) return false;
  out = hh * 60 + mm;
  return true;
}

bool PluginScheduler::setBoundsByHHMM(String dayStart, String nightStart)
{
  int d, n;
  if (!parseHHMMToMinutes(dayStart, d)) return false;
  if (!parseHHMMToMinutes(nightStart, n)) return false;
  setBoundsByMinutes(d, n);
  return true;
}

String PluginScheduler::getDayStartHHMM() const
{
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", dayStartMins / 60, dayStartMins % 60);
  return String(buf);
}

String PluginScheduler::getNightStartHHMM() const
{
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", nightStartMins / 60, nightStartMins % 60);
  return String(buf);
}

bool PluginScheduler::isDayNow() const
{
  struct tm t;
  if (!getLocalTime(&t)) return true; // default to day if RTC/time not set
  int mins = t.tm_hour * 60 + t.tm_min;
  if (dayStartMins == nightStartMins) return true; // degenerate: all day
  if (dayStartMins < nightStartMins) {
    // Day within same day
    return mins >= dayStartMins && mins < nightStartMins;
  } else {
    // Day spans midnight
    return mins >= dayStartMins || mins < nightStartMins;
  }
}

void PluginScheduler::rebuildActiveFromCurrentPeriod(bool forceReset)
{
  bool wantDay = isDayNow();
  bool changed = forceReset || (wantDay != lastWasDay);
  if (changed) {
    lastWasDay = wantDay;
    schedule = wantDay ? scheduleDay : scheduleNight;
    currentIndex = 0;
    lastSwitch = millis();
    if (isActive) switchToCurrentPlugin();
  }
}

PluginScheduler &Scheduler = PluginScheduler::getInstance();