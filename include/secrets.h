// This is a defaults file, you need to add your details
#pragma once

// This defines the name of your device
#define WIFI_HOSTNAME "LostDisplay"

#ifdef ESP8266
#define WIFI_SSID "Dobendan"
#define WIFI_PASSWORD "PMQA1Ndv41HLRaKV"
#endif

// If you would like to perform OTA updates, you need to define the credentials here
#define OTA_USERNAME "admin"
#define OTA_PASSWORD "password"

// Weather location used for wttr.in queries (city name or "lat,lon")
// Example: "Berlin" or "52.52,13.405"
#ifndef WEATHER_LOCATION
#define WEATHER_LOCATION "40545"
#endif

// Weather update interval in minutes (background fetch)
#ifndef WEATHER_UPDATE_MINUTES
#define WEATHER_UPDATE_MINUTES 30
#endif
