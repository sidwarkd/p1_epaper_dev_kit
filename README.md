# Particle P1 E-Paper Dev Kit
Documentation and example firmware for using the Particle P1 E-paper Dev Kit.

All examples are found in the **examples** folder. They include:
  - **Particle Function** - Update the display by calling a Particle function.
  - **Simple Clock** - Highlights the partial update capabilities of the display by showing the time update every second.
  - **Weather Display** - An example of getting data from a web API and displaying it.

## Features
  - Ultra-low deep sleep current for months of battery operation
  - Ready to connect to full Particle ecosystem out of the box
  - Supports Over-The-Air (OTA) Updates out of the box
  - 200x200 pixel 1.54" Display with fast partial refresh
  - Onboard LiPo charger 
  - Standard Particle Setup/Rest buttons and onboard RGB LED
  - Schematic and BOM available on request 
  - PCB design fits perfectly into included enclosure which can be 3D printed if desired.

## Display Power
The display is powered by the P1S3 pin on the P1 Module. Connecting the display directly to the 3.3V rail would create a constant current draw even with the display is put into hibernate mode. By connecting the display's power rail to an IO pin on the P1 module we can get 0 power draw from the display when not in use. Since the display is e-paper it can hold it's display while drawing **zero power**.

To turn the display on in firmware simple do:

```c
pinMode(P1S3, OUTPUT);
digitalWrite(P1S3, HIGH);
```

To turn the display off:

```c
digitalWrite(P1S3, LOW);
```

To achieve a zero power consumption state on the display it's necessary to disconnect it from the P1 by putting the pin into a high impedance state.

```c
pinMode(P1S3, INPUT)
```

## Display Libraries
All example projects make use of the popular GxEPD2 e-paper library. Code and documentation are available in the project's Github repository located at [https://github.com/ZinggJM/GxEPD2](https://github.com/ZinggJM/GxEPD2).

The GxEPD2 library inherits from the Adafruit GFX library. As such, display objects in the firmware code can make use of the entire feature set of that library. More information about the library can be found at the following links:

  - [Adafruit Graphics Library Overview](https://learn.adafruit.com/adafruit-gfx-graphics-library?view=all)
  - [Adafruit GFX Github](https://github.com/adafruit/Adafruit-GFX-Library)

## Power Considerations
The onboard PMIC is specifically tuned to charge a 1S LiPo battery while in the sealed enclosure. The dev kit will charge an attached battery while connected to a power source via the micro USB port. Because there are no air vents it intentionally has a minimal charge curve to prevent overheating. The following describes the characteristics of the charge circuit.

  - Pre-charge Current: 18mA
  - Fast-charge Current: 180mA
  - Top-off Current: 10mA max

The dev kit also makes use of the Dynamic Charge Timer functionality of the BQ24074 which prevents the battery from charging for too long. The max charge time is set with a resistor on the board and is configured to allow a max charge time of 9.5 hours. If the connected battery is not fully charged after 9.5 hours the charge circuit is disabled until a power reset event.

> **WARNING:** If you choose to use a battery other than the included battery it is your responsibility to ensure the charging profile described above will not damage your battery.

### Deep Sleep Current
With the display completely disconnected (see Display Power section above) the **deep sleep current of the dev kit is 52uA**. This is low enough to allow for months of operation on a single charge. 52uA of current equates to 1.248mAh of battery life per day. One month of sleep current is 39mAh of battery life.

## Enclosure
The STL files in the **enclosure** folder can be used to 3D print an enclosure perfectly matched to the custom PCB design. If you do not have access to a 3D printer an enclosure can be purchased with the dev kit.