#include "plugins/WeatherPlugin.h"

// https://github.com/chubin/wttr.in/blob/master/share/translations/en.txt
#ifdef ESP8266
WiFiClient wiFiClient;
#endif

void WeatherPlugin::setup()
{
    // Show loading indicator once; actual data fetched by WeatherService
    Screen.clear();
    currentStatus = LOADING;
    Screen.setPixel(4, 7, 1);
    Screen.setPixel(5, 7, 1);
    Screen.setPixel(7, 7, 1);
    Screen.setPixel(8, 7, 1);
    Screen.setPixel(10, 7, 1);
    Screen.setPixel(11, 7, 1);
    currentStatus = NONE;
}

void WeatherPlugin::loop()
{
    // No background networking here; WeatherService manages updates.
    // We just render based on cached data.
    update();
}

void WeatherPlugin::update()
{
    const WeatherData &d = WeatherService::getInstance().get();

    int weatherIcon = 0;
    int iconY = 1;
    int tempY = 10;

    if (d.valid)
    {
        // OpenWeatherMap condition ID mapping
        int id = d.weatherCode;
        int cat = id / 100; // 2xx thunder, 3xx drizzle, 5xx rain, 6xx snow, 7xx atmosphere, 8xx clouds
        if (id == 800) {
            weatherIcon = 2; // clear
            iconY = 1;
            tempY = 9;
        } else if (id >= 801 && id <= 803) {
            weatherIcon = 3; // partly cloudy
            iconY = 2;
        } else if (id == 804) {
            weatherIcon = 0; // cloudy
            iconY = 2;
            tempY = 9;
        } else if (cat == 2) {
            weatherIcon = 1; // thunderstorm
        } else if (cat == 3 || cat == 5) {
            weatherIcon = 4; // drizzle/rain
        } else if (cat == 6) {
            weatherIcon = 4; // snow → use rain icon (no snow icon)
        } else if (cat == 7) {
            weatherIcon = 0; // fog/haze/mist → map to cloudy
            iconY = 2;
        }

        if (weatherIcon < 0 || weatherIcon > 4)
        {
            weatherIcon = 0;
        }

        Screen.beginUpdate();
        Screen.clear();
        Screen.drawWeather(0, iconY, weatherIcon, 100);

        int temperature = d.tempC;
        if (temperature >= 10)
        {
            Screen.drawCharacter(9, tempY, Screen.readBytes(degreeSymbol), 4, 50);
            Screen.drawNumbers(1, tempY, {(temperature - temperature % 10) / 10, temperature % 10});
        }
        else if (temperature <= -10)
        {
            Screen.drawCharacter(0, tempY, Screen.readBytes(minusSymbol), 4);
            Screen.drawCharacter(11, tempY, Screen.readBytes(degreeSymbol), 4, 50);
            temperature *= -1;
            Screen.drawNumbers(3, tempY, {(temperature - temperature % 10) / 10, temperature % 10});
        }
        else if (temperature >= 0)
        {
            Screen.drawCharacter(7, tempY, Screen.readBytes(degreeSymbol), 4, 50);
            Screen.drawNumbers(4, tempY, {temperature});
        }
        else
        {
            Screen.drawCharacter(0, tempY, Screen.readBytes(minusSymbol), 4);
            Screen.drawCharacter(9, tempY, Screen.readBytes(degreeSymbol), 4, 50);
            Screen.drawNumbers(3, tempY, {-temperature});
        }
        Screen.endUpdate();
    }
    else
    {
        // show loading indicator if no data yet
        Screen.beginUpdate();
        Screen.clear();
        Screen.drawCharacter(3, 7, Screen.readBytes(degreeSymbol), 4, 20);
        Screen.endUpdate();
    }
}

const char *WeatherPlugin::getName() const
{
    return "Weather";
}
