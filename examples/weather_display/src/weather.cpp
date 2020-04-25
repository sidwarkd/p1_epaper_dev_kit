#include "weather.h"

OpenWeatherMapClient::OpenWeatherMapClient(HTTPClient *http_client, const char *city_id, const char *app_id, WeatherUnits units) :
                                           _http_client(http_client), 
                                           _units(units)
{
    strcpy(_city_id, city_id);
    strcpy(_app_id, app_id);
    generate_request_url(_url);
    memset(_response, 0, sizeof(_response));
}

OpenWeatherMapClient::~OpenWeatherMapClient()
{
    memset(_city_id, 0, sizeof(_city_id));
    memset(_app_id, 0, sizeof(_app_id));
    memset(_url, 0, sizeof(_url));
    memset(_response, 0, sizeof(_response));
}

void OpenWeatherMapClient::generate_request_url(char *url)
{
    if (_units == WeatherUnits::IMPERIAL)
    {
        sprintf(url, OWM_URL_TEMPLATE, _city_id, _app_id, "imperial");
    }
    else
    {
        sprintf(url, OWM_URL_TEMPLATE, _city_id, _app_id, "si");
    }
}

OpenWeatherMapData* OpenWeatherMapClient::get_current_weather()
{
    auto response = _http_client->get(_url, _response, sizeof(_response), 10000);
    if(response != HTTP_OK)
    {
        return nullptr;
    }
    _weather_data.set_json(_response);
    return &_weather_data;
}

void OpenWeatherMapData::set_json(char* json)
{
    strcpy(_raw_json, json);
    DeserializationError err = deserializeJson(_doc, _raw_json);
    // Parse succeeded?
    if (err) {
        Serial.print(F("ARDUINOJSON ERROR deserializeJson() returned "));
        Serial.println(err.c_str());
        return;
    }
}

const char* OpenWeatherMapData::get_description()
{
    return _doc["weather"][0]["description"];
}

double OpenWeatherMapData::get_temperature()
{
    return _doc["main"]["temp"];
}

double OpenWeatherMapData::get_humidity()
{
    return _doc["main"]["humidity"];
}

double OpenWeatherMapData::get_feels_like()
{
    return _doc["main"]["feels_like"];
}

double OpenWeatherMapData::get_wind_speed()
{
    return _doc["wind"]["speed"];
}

int OpenWeatherMapData::get_wind_direction()
{
    return _doc["wind"]["deg"];
}

WeatherImage OpenWeatherMapData::get_weather_icon()
{
    int weather_code = _doc["weather"][0]["id"];
    int weather_group = weather_code / 100;

    switch(weather_group)
    {
        case 2:
            return WeatherImage::THUNDERSTORM;
        
        case 3:
        case 5:
            return WeatherImage::RAIN;

        case 6:
            return WeatherImage::SNOW;

        case 7:
            return WeatherImage::CLOUDY_DAY;

        case 8:
            if(weather_code == 800) // It's clear
            {
                if(_is_daytime())
                {
                    return WeatherImage::CLEAR_DAY;
                }
                else
                {
                    return WeatherImage::CLEAR_NIGHT;
                }
            }
            else // It's cloudy
            {
                if(_is_daytime())
                {
                    if(weather_code < 803)
                    {
                        return WeatherImage::PARTLY_SUNNY;
                    }
                    else
                    {
                        return WeatherImage::CLOUDY_DAY;
                    }
                }   
                else
                {
                    return WeatherImage::CLOUDY_NIGHT;
                }
            }
            
        default:
            return WeatherImage::CLEAR_DAY;
    }
}

int OpenWeatherMapData::get_cloud_cover()
{
    return _doc["clouds"]["all"];
}

bool OpenWeatherMapData::_is_daytime()
{
    // This is a clever hack to not have to deal with time calculations
    // to determine if it is night or day. The API icon will be
    // postfixed with a 'd' for daytime and 'n' for night time. By
    // looking at the character we tightly couple to the API icon
    // names but it allows us to greatly simplify this function.
    const char *image_icon_code = _doc["weather"][0]["icon"];
    return *(image_icon_code + 2) == 'd';
}