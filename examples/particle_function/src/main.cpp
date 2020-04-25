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
#include <FreeMonoBold12pt7b.h>  
#include <GxEPD2_BW.h>

enum class UpdateType { full, partial };

void updateDisplayMessage(char * message, UpdateType updateType);
int cloudFunctionHandler(String message);

// The pin mappings on the following line are fixed and should not be changed
GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(SS, D2, A0, A1));

void setup()
{
  // Turn on the display. To allow for ultra-low deep sleep power consumption
  // the display is not connected to the hardware's 3.3V supply. It is
  // connected to the P1S3 io pin on the Particle P1 module for power.
  pinMode(P1S3, OUTPUT);
  digitalWrite(P1S3, HIGH);

  // Passing in a baud rate to display.init() will force a call to
  // Serial.begin() with the specified baud rate. This allows for 
  // debug output to be displayed via a Serial connection.
  display.init(115200);

  display.clearScreen();

  char defaultMessage[] = "Particle P1\nE-Paper\nDev Kit";
  updateDisplayMessage(defaultMessage, UpdateType::full);

  Particle.function("message", cloudFunctionHandler);
}

void loop()
{
}

int cloudFunctionHandler(String message)
{
  updateDisplayMessage((char*)message.c_str(), UpdateType::partial);
}

/** brief Write text to display
 * 
 * param message char*
 * param updateType UpdateType
 * return void
 * 
 */
void updateDisplayMessage(char * message, UpdateType updateType)
{
  display.setRotation(1);
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextColor(GxEPD_BLACK);
  
  if(updateType == UpdateType::full){
    display.setFullWindow();
  }
  else{
    display.setPartialWindow(0, 0, display.width(), display.height());
  }

  display.firstPage();
  do
  {
    char *currentLine = strtok(message, "\n");
    if(currentLine != NULL) // There are newlines to process
    {
      int yOffset = FreeMonoBold12pt7b.yAdvance;
    
      while (currentLine != NULL)
      {
        int16_t tbx, tby; uint16_t tbw, tbh;
        display.getTextBounds(currentLine, 0, yOffset, &tbx, &tby, &tbw, &tbh);
        // center bounding box by transposition of origin:
        uint16_t x = ((display.width() - tbw) / 2) - tbx;
        uint16_t y = yOffset;
        display.setCursor(x, y);
        display.print(currentLine);

        yOffset += FreeMonoBold12pt7b.yAdvance;
        currentLine = strtok(NULL, "\n");
      }
    }
    else // no newlines
    {
      int16_t tbx, tby; uint16_t tbw, tbh;
      display.getTextBounds((const char*)message, 0, 0, &tbx, &tby, &tbw, &tbh);
      // center bounding box by transposition of origin:
      uint16_t x = ((display.width() - tbw) / 2) - tbx;
      uint16_t y = ((display.height() - tbh) / 2) - tby;
      display.setCursor(x, y);
      display.print(message);
    }
  }
  while (display.nextPage());
}