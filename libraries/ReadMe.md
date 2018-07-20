A list of libraries needed to run PlantCam sketches (also see main sketch).
Unused functions have been removed from some libraries - those are included here

      #include <SPI.h>
      #include <SD.h>
      #include <SHT1x.h>
      #include <Wire.h>
      #include <RTClibExtended.h>
      #include <LowPower.h>
      #include <Adafruit_TCS34725.h>

LIGHT SENSOR:
https://github.com/adafruit/Adafruit_TCS34725

SOIL TEMP HUMIDITY PROBE: 
https://github.com/practicalarduino/SHT1x

RTC:
RTClibExtended 
https://github.com/FabioCuomo/FabioCuomo-DS3231/blob/master/RTClibExtended.cpp
LowPower 
https://github.com/rocketscream/Low-Power

SD CARD LOGGER:
https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/arduino-library
formatting for SD card 
https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/formatting-notes
