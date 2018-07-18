/*   Author: Marissa Kwon
  Date: 7/17/2018
  Description: This sketch handles sensor and dendrometer datalogging and timing

  links:
  Saving program memory with FLASH storage https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
  SHT-31 temp/humidity sensor https://www.adafruit.com/products/2857
  SHT1x (soil probe) temp/humidity sensor https://www.adafruit.com/product/1298

*/
// add libraries
#include <SPI.h>
#include <SD.h>
#include <SHT1x.h>

// eliminates extra steps in code
#define SLEEP 0  //0 - always powered on; 1 - RTC mode
#define DEBUG 0  //0 - in field test mode; 1 - prints to screen

// project settings
const int WakePeriod = 5; // minutes

// pin constants
#define SDCheck 10 // attached to CD on SD card logger
#define WakePin 11 // attached to SQW on RTC
#define ChipSel 12 // attached to CS on SD card logger
#define VbatPin A9 // reading battery voltage
#define CLK 3 //const serial clock pin
#define SDA 2 //const serial data pin

// instances of sensors
SHT1x sht1x(SDA, CLK);

void setup() {
#if DEBUG == 1
  Serial.begin(9600);
  while (!Serial) {}; // wait for serial port to connect. Needed for native USB port only
  delay(10);
  //  /*
  if (!SD.begin(ChipSel)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
  //  */
#endif


}

// ====== measure data, save it, and write to SD card ======
void loop() {
  float measuredvbat, so_temp = 0, so_hum = 0;
  String timestamp = "--:--:--", writeString = "";

  // get data
  measuredvbat = analogRead(VbatPin); // reading battery voltage
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  //--- soil SHT temp/humidity ---
  so_temp = sht1x.readTemperatureC();
  so_hum = sht1x.readHumidity();

  // create combined string
  writeString = getWriteString(timestamp, measuredvbat, so_temp, so_hum);
  logData(writeString);

#if DEBUG == 1
  Serial.println(writeString);
#endif
}

// ====== String function to concatenate all data ======
String getWriteString(String stamp, float vbat, float so_temp, float so_hum) {
  String out, vb, so_tm, so_rh;
  // convert float to string
  vb = String(vbat, 1); //1 decimal place
  so_tm = String(so_temp, 4); //4 decimal places
  so_rh = String(so_hum, 4); //4 decimal places
  out = stamp + "," + vb + "," + so_tm + "," + so_rh;
  return out;
}

// ====== function that logs data ======
void logData(String data) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(data);
    dataFile.close();
#if DEBUG == 1
    // print to the serial port too:
    Serial.println(data);
#endif
  }
  // if the file isn't open, pop up an error:
  else {
#if DEBUG == 1
    Serial.println("error opening datalog.txt");
#endif
  }
}

