#ifndef MAKERCREW_WEATHER_H
#define MAKERCREW_WEATHER_H

#include <HTTPClient.h>
#include <Arduino.h> // Must come before ArduinoJson or you will get compiler errors
#include <ArduinoJson.h>

enum class WeatherUnits {SI, IMPERIAL};

enum class WeatherImage {CLEAR_DAY, CLEAR_NIGHT, CLOUDY_DAY, CLOUDY_NIGHT, PARTLY_SUNNY, RAIN, SNOW, THUNDERSTORM};

const char OWM_URL_TEMPLATE[] = "http://api.openweathermap.org/data/2.5/weather?id=%s&appid=%s&units=%s";

class OpenWeatherMapClient;

class OpenWeatherMapData
{
  public:
    const char* get_description();
    double get_temperature();
    double get_humidity();
    double get_feels_like();
    double get_wind_speed();
    int get_cloud_cover();
    int get_wind_direction();
    WeatherImage get_weather_icon();
    void set_json(char* json);

  private:
    char _raw_json[512];
    StaticJsonDocument<1000> _doc;
    bool _is_daytime();
};

class OpenWeatherMapClient
{
  public:
    OpenWeatherMapClient(HTTPClient *http_client, const char *city_id, const char *app_id, WeatherUnits units);
    ~OpenWeatherMapClient();
    OpenWeatherMapData* get_current_weather(void);

  private:
    HTTPClient *_http_client;
    char _city_id[16];
    char _app_id[36];
    WeatherUnits _units;
    OpenWeatherMapData _weather_data;
    char _url[128];
    char _response[512];

    void generate_request_url(char *url);
};

#endif // MAKERCREW_WEATHER_H