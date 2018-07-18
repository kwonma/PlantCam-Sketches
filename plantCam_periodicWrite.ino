/* 	Author: Marissa Kwon
	Date: 7/16/2018
	Description: This sketch handles sensor and dendrometer datalogging and timing

  links:
  Saving program memory with FLASH storage https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
  SHT-31 temp/humidity sensor https://www.adafruit.com/products/2857
  SHT1x (soil probe) temp/humidity sensor https://www.adafruit.com/product/1298

*/
// add libraries
//#include "pgmspace.h"
//#include <Arduino.h>
//#include <Wire.h>
//#include "Adafruit_SHT31.h"
#include <SPI.h>
#include <SD.h>
#include <SHT1x.h>

// eliminates extra steps in code
#define SLEEP 0  //0 - always powered on; 1 - RTC mode
#define DEBUG 1  //0 - in field test mode; 1 - prints to screen

// project settings
const int WakePeriod = 5; // minutes
const int WritePeriod = 12; // every 12 wake/sleep cycles (once on the hour)

// pin constants
#define SDCheck 10 // attached to CD on SD card logger
#define WakePin 11 // attached to SQW on RTC
#define ChipSel 12 // attached to CS on SD card logger
#define VbatPin A9 // reading battery voltage
#define CLK 3 //const serial clock pin
#define SDA 2 //const serial data pin

// global variables
String data_array[WritePeriod + 1];
int count;

// instances of sensors
//Adafruit_SHT31 sht31 = Adafruit_SHT31();
SHT1x sht1x(SDA, CLK);

void setup() {
  data_array[WritePeriod] = "/0";
#if DEBUG == 1
  Serial.begin(9600);
  while (!Serial) {}; // wait for serial port to connect. Needed for native USB port only
  delay(10);
#endif

  /*
    Serial.println("SHT31 test");
    if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
     Serial.println("Couldn't find SHT31");
     while (1) delay(1);
    }
  */
  if (!SD.begin(ChipSel)) { // don't do anything more:
    while (1);
  }
#if DEBUG == 1
  Serial.println("card initialized.");
#endif


}

// ====== measure data, save it, and write to SD card ======
void loop() {
  float measuredvbat, am_temp = 0, am_hum = 0, so_temp = 0, so_hum = 0;
  String timestamp = "--:--:--", writeString = "";

  // get data
  measuredvbat = analogRead(VbatPin); // reading battery voltage
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  //--- ambient SHT temp/humidity ---
  //  am_temp = sht31.readTemperature();  // degrees C
  //  am_hum = sht31.readHumidity(); // relative as a percent

  //--- soil SHT temp/humidity ---
  so_temp = sht1x.readTemperatureC();
  so_hum = sht1x.readHumidity();

  // create combined string and store string in volatile memory
  getWriteString(timestamp, measuredvbat, am_temp, am_hum, so_temp, so_hum);

#if DEBUG == 1
  Serial.println(count);
  //Serial.println(writeString);
#endif
}

// =============================================================
// Additional Functions
// =============================================================


// ====== function that logs data ======
void logData(String * data_arr) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    for (int i = 0; i < WritePeriod; i++) {
      dataFile.println(data_arr[i]);
      delay(10);
#if DEBUG == 1
      // print to the serial port too:
      Serial.println(data_arr[i]);
#endif
    }
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
#if DEBUG == 1
    Serial.println("error opening datalog.txt");
#endif
  }
}

// ====== saving strings SD card writes less =======
bool saveWriteString(String ws) {
  // write data to array
  if (count < WritePeriod && (data_array[count] != "/0")) {
    data_array[count] = ws;
#if DEBUG == 1
    Serial.println("Saved");
#endif
    return 0;
  }
  // log data if array is full
  if ((count >= WritePeriod) || (data_array[count] == "\0")) {
#if DEBUG == 1
    Serial.println("Write");
#endif
    logData(data_array);
    return 1;
  }
}

// ====== String function to concatenate all data ======
void getWriteString(String stamp, float vbat, float am_temp, float am_hum, float so_temp, float so_hum) {
  String out, vb, am_tm, am_rh, so_tm, so_rh;
  // convert float to string
  vb = String(vbat, 1); //1 decimal place
  am_tm = String(am_temp, 4); //4 decimal places
  am_rh = String(am_hum, 4); //4 decimal places
  so_tm = String(so_temp, 4); //4 decimal places
  so_rh = String(so_hum, 4); //4 decimal places
  out = stamp + "," + vb + "," + am_tm + "," + am_rh + "," + so_tm + "," + so_rh;

  // save data and log to SD card when necessary
  if (saveWriteString(out)) {
    count = 0;
  }
  else {
    count++;
  }
  delay(500);
}

