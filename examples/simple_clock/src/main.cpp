// Example for Particle P1 E-Paper Dev Kit
// Illustrates partial refresh capabilities with a simple clock
//
// Author: Kevin Sidwar, MakerCrew
//
// License: GPL because it must inherit the license from the GxEPD2 library.
//

#include <Adafruit_GFX_RK.h>
// List of fonts can be found in src directory of Adafruit GFX library
#include <FreeSansOblique24pt7b.h>  
#include <GxEPD2_BW.h>

void updateClock();
String current_time = "";

// The pin mappings on the following line are fixed and should not be changed
GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(SS, D2, A0, A1));

void setup()
{
  // Turn on the display. To allow for ultra-low deep sleep power consumption
  // the display is not connected to the hardware's 3.3V supply. It is
  // connected to the P1S3 io pin on the Particle P1 module for power.
  pinMode(P1S3, OUTPUT);
  digitalWrite(P1S3, HIGH);

  // By default the clock displays UTC time. If you want to see local
  // time simply uncomment the line below and set your timezone
  // offset value.
  Time.zone(-6);

  // Passing in a baud rate to display.init() will force a call to
  // Serial.begin() with the specified baud rate. This allows for
  // debug output over serial from the GxEPD2 library
  display.init(115200);

  display.clearScreen();

  // The following lines can be used to change the rotation and font
  // of the display. A list of fonts is found in the folder
  // lib/Adafruit_GFX_RK/src
  display.setRotation(1);
  display.setFont(&FreeSansOblique24pt7b);
  display.setTextColor(GxEPD_BLACK);
}

void loop()
{
  updateClock();
}

void updateClock()
{
  String new_time = Time.format(Time.now(), "%T");
  if (new_time == current_time) return;

  current_time = new_time;

  // The partial window is static to improve performance. If you
  // move the location of the time display you will need to 
  // change this partial window bounding box.
  display.setPartialWindow(0, 72, 199, 40);

  display.firstPage();
  do
  {
    display.setCursor(0, 111);
    display.print(current_time.c_str());
  }
  while (display.nextPage());
}