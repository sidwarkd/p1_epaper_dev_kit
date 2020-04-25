# GxEPD2_PP
## Particle Display Library for SPI E-Paper Displays

- With full Graphics and Text support using Adafruit_GFX  (Adafruit_GFX_RK Version 1.3.5)

- For SPI e-paper displays from Dalian Good Display 
- and SPI e-paper boards from Waveshare

### important note :
- the display panels are for 3.3V supply and 3.3V data lines
- never connect data lines directly to 5V data pins.
- the actual Waveshare display boards now have level converters and series regulator, safe for 5V

### Paged Drawing, Picture Loop
 - This library uses paged drawing to limit RAM use and cope with missing single pixel update support
 - buffer size can be selected in the application by template parameter page_height, see GxEPD2_Example
 - Paged drawing is implemented as picture loop, like in U8G2 (Oliver Kraus)
 - see https://github.com/olikraus/u8glib/wiki/tpictureloop
 - Paged drawing is also available using drawPaged() and drawCallback(), like in GxEPD
- ` // GxEPD style paged drawing; drawCallback() is called as many times as needed `
- ` void drawPaged(void (*drawCallback)(const void*), const void* pv) `
- paged drawing is done using Adafruit_GFX methods inside picture loop or drawCallback

### Full Screen Buffer Support
 - full screen buffer is selected by setting template parameter page_height to display height
 - drawing to full screen buffer is done using Adafruit_GFX methods without picture loop or drawCallback
 - and then calling method display()

### Low Level Bitmap Drawing Support
 - bitmap drawing support to the controller memory and screen is available:
 - either through the template class instance methods that forward calls to the base display class
 - or directy using an instance of a base display class and calling its methods directly

### Supporting Particle Community Forum Topic:

- https://community.particle.io/t/gxepd2-pp-particle-display-library-for-spi-e-paper-displays/46305

### Supporting Arduino Forum Topics:

- Waveshare e-paper displays with SPI: http://forum.arduino.cc/index.php?topic=487007.0
- Good Dispay ePaper for Arduino : https://forum.arduino.cc/index.php?topic=436411.0

### Note on documentation
- GxEPD2 uses Adafruit_GFX for Graphics and Text support, which is well documented there
- GxEPD2 uses meaningful method names, and has some comments in the header files
- consult the header files GxEPD2_BW.h, GxEPD2_3C.h and GxEPD2_GFX.h
- for the concept of paged drawing and picture loop see: 
- https://github.com/olikraus/u8glib/wiki/tpictureloop

### Supported SPI e-paper panels from Good Display:
- GDEP015OC1     1.54" b/w
- GDEW0154Z04    1.54" b/w/r 200x200
- GDE0213B1      2.13" b/w
- GDEH0213B72    2.13" b/w, replacement for GDE0213B1
- GDEH0213B73    2.13" b/w, new replacement for GDE0213B1, GDEH0213B72
- GDEW0213I5F    2.13" b/w flexible
- GDEW0213Z16    2.13" b/w/r
- GDEH029A1      2.9" b/w
- GDEW029T5      2.9" b/w
- GDEW029Z10     2.9" b/w/r
- GDEW026T0      2.6" b/w
- GDEW027C44     2.7" b/w/r
- GDEW027W3      2.7" b/w
- GDEW0371W7     3.7" b/w
- GDEW042T2      4.2" b/w
- GDEW042Z15     4.2" b/w/r
- GDEW0583T7     5.83" b/w
- GDEW075T8      7.5" b/w
- GDEW075Z09     7.5" b/w/r
- GDEW075Z08     7.5" b/w/r 800x480
#### Supported SPI e-paper panels & boards from Waveshare: compare with Good Display, same panel
#### other supported panels
- ED060SCT        6" grey levels, on Waveshare e-Paper IT8951 Driver HAT

### Version 1.1.9
- last version for GxEPD2_PP
- corresponds to GxEPD2 Version 1.2.0
- please use GxEPD2 for new projects
#### Version 1.1.0+
- added GxEPD2_PP_WiFi_Example, for bitmap download from web
- only download from http: works, would need help for https:
- only on GitHub, to avoid the need to increase the version for publish
- this is preliminary!
#### Version 1.1.0
- corresponds to GxEPD2 Version 1.1.6
- added support for GDEH0213B72 2.13" b/w, replacement for GDE0213B1
#### Version 1.0.9
- added support for GDEW029T5
- fixed (added) clipping for partial window
- fixed (added) powerOff() after full update (partial update keeps power on)
- added hibernate() for minimum power use by displays that support it
#### Version 1.0.8
- retired, wrong dependency in GxEPD2_PP_Example
#### Version 1.0.7
- fix drawImage(...) overloaded methods signature matching abiguity
- preliminary version
#### Version 1.0.6
- preliminary version
- based on GxEPD2 Version 1.1.0
