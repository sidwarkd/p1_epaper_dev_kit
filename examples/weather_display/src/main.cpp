// Example for Particle P1 E-Paper Dev Kit
// Will update message on the display with a string sent via a Particle
// function call.
//
// Author: Kevin Sidwar, MakerCrew
//
// License: GPL because it must inherit the license from the GxEPD2 library.
//

#include <Adafruit_GFX_RK.h>

// List of fonts can be found in src directory of Adafruit GFX library
#include <FreeSans9pt7b.h>
#include <FreeMonoBold9pt7b.h>  
#include <FreeMonoBold18pt7b.h>  

#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include "weather.h"
#include "weather_icons.h"

// =========================================================================================================
// YOUR SPECIFIC OPEN WEATHER MAP DATA
// =========================================================================================================
const String city = "";//<-----ENTER YOUR CITY CODE HERE
const String cc = "";//<-------ENTER YOUR COUNTRY CODE HERE
const String apiKey = "";//<---ENTER YOUR OPEN WEATHER MAP API KEY HERE
// =========================================================================================================

enum class UpdateType { full, partial };
void writeText(char * text, int16_t x, int16_t y, const GFXfont *font);


// The pin mappings on the following line are fixed and should not be changed
GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(SS, D2, A0, A1));
TCPClient tcp;
HTTPClient httpClient(&tcp);

OpenWeatherMapClient weather_client(&httpClient, city.c_str(), apiKey.c_str(), WeatherUnits::IMPERIAL);

// The following can be used as dummy data to test changes so
// a full API call can be avoided. It should be commented out
// when using the actual Open Weather Map API
char dummy_json[] = "{\"coord\":{\"lon\":-100.00,\"lat\":35.00},\"weather\":[{\"id\":804,\"main\":\"Clouds\",\"description\":\"dummy data\",\"icon\":\"04n\"}],\"base\":\"stations\",\"main\":{\"temp\":44.47,\"feels_like\":32.86,\"temp_min\":39.99,\"temp_max\":50,\"pressure\":1014,\"humidity\":100},\"visibility\":16093,\"wind\":{\"speed\":18.34,\"deg\":310,\"gust\":21.92},\"clouds\":{\"all\":90},\"dt\":1587005671,\"sys\":{\"type\":1,\"id\":6116,\"country\":\"US\",\"sunrise\":1586954919,\"sunset\":1587002804},\"timezone\":-21600,\"id\":1234567,\"name\":\"Someplace\",\"cod\":200}";

void setup()
{
  // Turn on the display. To allow for ultra-low deep sleep power consumption
  // the display is not connected to the hardware's 3.3V supply. It is
  // connected to the P1S3 IO pin on the Particle P1 module for power.
  pinMode(P1S3, OUTPUT);
  digitalWrite(P1S3, HIGH);

  // Passing in a baud rate to display.init() will force a call to
  // Serial.begin() with the specified baud rate
  display.init(115200);

  display.setRotation(1);
  display.clearScreen();
}

void loop()
{
  // Line 66 to 78 should be uncommented when using the full
  // Open Weather Map API. Defaults to using dummy data below
  // auto weather_data = weather_client.get_current_weather();

  // // If there was an issue fetching the data go to sleep and try again in a minute
  // if(! weather_data)
  // {
  //   digitalWrite(P1S3, LOW);
  //   pinMode(P1S3, INPUT);

  //   SystemSleepConfiguration sleep_config;
  //   sleep_config.mode(SystemSleepMode::HIBERNATE)
  //               .duration(1min);
  //   System.sleep(sleep_config);
  // }

  // Use dummy data by default
  OpenWeatherMapData *weather_data = new OpenWeatherMapData();
  weather_data->set_json(dummy_json);

  // Uncomment the following lines to see the API data in serial output

  // Serial.printlnf("Description: %s", weather_data.get_description());
  // Serial.printlnf("Temperature: %f", weather_data.get_temperature());
  // Serial.printlnf("Humidity: %f", weather_data.get_humidity());
  // Serial.printlnf("Feels Like: %f", weather_data.get_feels_like());
  // Serial.printlnf("Wind Speed: %f", weather_data.get_wind_speed());
  // Serial.printlnf("Wind Direction: %d", weather_data.get_wind_direction());
  // Serial.printlnf("Image Code: %d", weather_data.get_image_code());

  uint16_t y_position = 105;
  char data_line[20];
  
  sprintf(data_line, "%3.1fF", weather_data->get_temperature());
  writeText(data_line, 80, 35, &FreeMonoBold18pt7b);
  writeText((char*)weather_data->get_description(), 75, 65, &FreeSans9pt7b);

  sprintf(data_line, "Humidity: %2.0f%%", weather_data->get_humidity());
  writeText(data_line, 5, y_position, &FreeMonoBold9pt7b);
  y_position += FreeMonoBold9pt7b.yAdvance + 5;

  sprintf(data_line, "Wind: %3.1f mph", weather_data->get_wind_speed());
  writeText(data_line, 5, y_position, &FreeMonoBold9pt7b);
  y_position += FreeMonoBold9pt7b.yAdvance + 5;

  sprintf(data_line, "Feels Like: %2.0f °F", weather_data->get_feels_like());
  writeText(data_line, 5, y_position, &FreeMonoBold9pt7b);
   y_position += FreeMonoBold9pt7b.yAdvance + 5;

  sprintf(data_line, "Clouds: %d%%", weather_data->get_cloud_cover());
  writeText(data_line, 5, y_position, &FreeMonoBold9pt7b);

  switch(weather_data->get_weather_icon())
  {
    case WeatherImage::CLEAR_DAY:
      display.writeImage(sunny_icon, 136, 8, 64, 64, false, false, true);
      break;
    case WeatherImage::CLEAR_NIGHT:
      display.writeImage(moon_icon, 136, 8, 64, 64, false, false, true);
      break;
    case WeatherImage::CLOUDY_DAY:
      display.writeImage(cloud_icon, 136, 8, 64, 64, false, false, true);
      break;
    case WeatherImage::CLOUDY_NIGHT:
      display.writeImage(clouds_night_icon, 136, 8, 64, 64, false, false, true);
      break;
    case WeatherImage::PARTLY_SUNNY:
      display.writeImage(partly_sunny_icon, 136, 8, 64, 64, false, false, true);
      break;
    case WeatherImage::RAIN:
      display.writeImage(rain_icon, 136, 8, 64, 64, false, false, true);
      break;
    case WeatherImage::SNOW:
      display.writeImage(snow_icon, 136, 8, 64, 64, false, false, true);
      break;
    case WeatherImage::THUNDERSTORM:
      display.writeImage(thunderstorm_icon, 136, 8, 64, 64, false, false, true);
      break;
  }

  display.refresh(8, 8, 64, 64);

  // Stay connected to the Particle Cloud while waiting for next update

  //delay(300000); // Wait 5 minutes for next update

  // Battery efficient deep sleep. If you need to stay connected to
  // the Particle Cloud between updates comment out the rest of 
  // loop and use the delay() call above instead

  // Turn off the display and float power pin to prevent leakage current
  digitalWrite(P1S3, LOW);
  pinMode(P1S3, INPUT);

  SystemSleepConfiguration sleep_config;
  sleep_config.mode(SystemSleepMode::HIBERNATE)
              .duration(5min);
  System.sleep(sleep_config);
}

/** brief Write text to display
 * 
 * param message char*
 * param updateType UpdateType
 * return void
 * 
 */
void writeText(char * text, int16_t x, int16_t y, const GFXfont *font)
{
  display.setFont(font);
  display.setTextColor(GxEPD_BLACK);

  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds((const char*)text, x, y, &tbx, &tby, &tbw, &tbh);

  // Uncommenting the following lines can help you understand where
  // your text will be drawn on the display
  // Serial.printlnf("x: %d", x);
  // Serial.printlnf("tbx: %d", tbx);
  // Serial.printlnf("tby: %d", tby);
  // Serial.printlnf("tbh: %d", tbh);
  // Serial.printlnf("tbw: %d", tbw);

  display.setPartialWindow(tbx, tby, tbw, tbh);

  display.firstPage();
  do
  {
  
    // The following lines of code are useful for centering text on the display.
    // Left here for informational purposes.

    // int16_t tbx, tby; uint16_t tbw, tbh;
    // display.getTextBounds((const char*)text, 0, 0, &tbx, &tby, &tbw, &tbh);
    // // center bounding box by transposition of origin:
    // uint16_t x = ((display.width() - tbw) / 2) - tbx;
    // uint16_t y = ((display.height() - tbh) / 2) - tby;

    display.setCursor(x, y);
    display.print(text);
  }
  while (display.nextPage());
}