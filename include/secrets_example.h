#pragma once

// Copy this file to include/secrets.h and fill in your local credentials.
// NOTE: include/secrets.h is gitignored and will not be committed.

// Device hostname (shown in your router / mDNS, if enabled)
#ifndef WIFI_HOSTNAME
#define WIFI_HOSTNAME "IKEADisplay"
#endif

// ESP8266 users: define SSID/PASSWORD here (ESP32 uses WiFiManager by default)
#ifdef ESP8266
  #ifndef WIFI_SSID
  #define WIFI_SSID "YOUR_WIFI_SSID"
  #endif
  #ifndef WIFI_PASSWORD
  #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
  #endif
#endif

// OTA update credentials (optional but recommended when OTA is enabled)
#ifndef OTA_USERNAME
#define OTA_USERNAME "admin"
#endif
#ifndef OTA_PASSWORD
#define OTA_PASSWORD "change-me"
#endif

// Weather settings (used by Weather plugin)
// Provide either a city name (e.g., "Berlin") or "lat,lon" (e.g., "52.52,13.405")
#ifndef WEATHER_LOCATION
#define WEATHER_LOCATION "Berlin"
#endif

// Weather update interval in minutes
#ifndef WEATHER_UPDATE_MINUTES
#define WEATHER_UPDATE_MINUTES 30
#endif

// OpenWeatherMap settings
// Create a free API key at https://openweathermap.org/ and paste it here
#ifndef OWM_API_KEY
#define OWM_API_KEY "YOUR_OPENWEATHERMAP_API_KEY"
#endif
// Optional: units (metric, imperial, standard)
#ifndef OWM_UNITS
#define OWM_UNITS "metric"
#endif
// Optional: language for condition text (not heavily used in UI)
#ifndef OWM_LANG
#define OWM_LANG "en"
#endif
// Optional CA certificate for api.openweathermap.org (PEM). If empty, TLS verification is skipped (insecure)
#ifndef OWM_CA_CERT
#define OWM_CA_CERT ""
#endif



// Stock settings (used by Stock Chart plugin)
// Default ticker symbol in stooq format. Examples: "aapl.us" (Apple, US), "msft.us", "amzn.us".
// If you omit the suffix (e.g., "aapl"), ".us" will be assumed.
#ifndef STOCK_SYMBOL
#define STOCK_SYMBOL "aapl.us"
#endif


// Optional API token for HTTP/WS access control
#ifndef API_TOKEN
#define API_TOKEN "" // leave empty to disable authentication
#endif

// Optional CA certificate for wttr.in TLS (PEM). If empty, client will skip verification (insecure)
#ifndef WTTR_CA_CERT
#define WTTR_CA_CERT ""
#endif

