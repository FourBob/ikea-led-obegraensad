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
        int weatherCode = d.weatherCode;
        if (std::find(thunderCodes.begin(), thunderCodes.end(), weatherCode) != thunderCodes.end())
        {
            weatherIcon = 1; // thunderstorm
        }
        else if (std::find(rainCodes.begin(), rainCodes.end(), weatherCode) != rainCodes.end())
        {
            weatherIcon = 4; // rain
        }
        else if (std::find(snowCodes.begin(), snowCodes.end(), weatherCode) != snowCodes.end())
        {
            weatherIcon = 4; // map snow to rain icon (no snow icon available)
        }
        else if (std::find(fogCodes.begin(), fogCodes.end(), weatherCode) != fogCodes.end())
        {
            weatherIcon = 0; // map fog to cloudy
            iconY = 2;
        }
        else if (std::find(clearCodes.begin(), clearCodes.end(), weatherCode) != clearCodes.end())
        {
            weatherIcon = 2; // clear
            iconY = 1;
            tempY = 9;
        }
        else if (std::find(cloudyCodes.begin(), cloudyCodes.end(), weatherCode) != cloudyCodes.end())
        {
            weatherIcon = 0; // cloudy
            iconY = 2;
            tempY = 9;
        }
        else if (std::find(partyCloudyCodes.begin(), partyCloudyCodes.end(), weatherCode) != partyCloudyCodes.end())
        {
            weatherIcon = 3; // partly cloudy
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
