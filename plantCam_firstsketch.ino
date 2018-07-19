/*   Author: Marissa Kwon
  Date: 7/17/2018
  Description: This sketch handles sensor and dendrometer datalogging and timing

  links:
  Saving program memory with FLASH storage https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
  SHT-31 temp/humidity sensor https://www.adafruit.com/products/2857
  SHT1x (soil probe) temp/humidity sensor https://www.adafruit.com/product/1298
  soil moisture sensor https://learn.sparkfun.com/tutorials/soil-moisture-sensor-hookup-guide

*/
// add libraries
#include <SPI.h>
#include <SD.h>
#include <SHT1x.h>
#include <Wire.h>
#include <RTClibExtended.h>
#include <LowPower.h>

// eliminates extra steps in code
#define SLEEP 0  //0 - always powered on; 1 - RTC mode
#define DEBUG 1  //0 - in field test mode; 1 - prints to screen

// project settings
const int WakePeriod = 5; // minutes

// pin constants
#define SDCheck 10 // attached to CD on SD card logger
#define WakePin 11 // attached to SQW on RTC
#define ChipSel 12 // attached to CS on SD card logger
#define VbatPin A9 // reading battery voltage
#define CLK 3 //const serial clock pin
#define SDA 2 //const serial data pin

#define SoilPow 6 // digital pin powering moisture probe * could replacing with hardware transistor & batt source?
#define SoilPin1 A0 // analog pin reading input for probe to attach more probes add more analog pins

// instances of sensors
SHT1x sht1x(SDA, CLK);

#if SLEEP == 1
RTC_DS3231 RTC;      //we are using the DS3231 RTC
#endif

void setup() {
#if DEBUG == 1
  Serial.begin(9600);
  while (!Serial) {}; // wait for serial port to connect. Needed for native USB port only
  delay(10);
#endif

  if (!SD.begin(ChipSel)) { // don't do anything more:
    while (1);
  }

  pinMode(SoilPow, OUTPUT); //D6 set as output for moisture probe(s)
  digitalWrite(SoilPow, LOW); //probes begin in OFF state

#if DEBUG == 1
  Serial.println(F("card initialized."));
#endif

#if SLEEP == 1
  pinMode(WakePin, INPUT);

  //Initialize communication with the clock
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));   //set RTC date and time to COMPILE time

  //clear any pending alarms
  RTC.armAlarm(1, false);
  RTC.clearAlarm(1);
  RTC.alarmInterrupt(1, false);
  RTC.armAlarm(2, false);
  RTC.clearAlarm(2);
  RTC.alarmInterrupt(2, false);

  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to WakePin for wake-up
  RTC.writeSqwPinMode(DS3231_OFF);

  //Set alarm1 every day at 18:33
  RTC.setAlarm(ALM1_MATCH_HOURS, 33, 18, 0);   //set your wake-up time here
  RTC.alarmInterrupt(1, true);
#endif


}

// ====== measure data, save it, and write to SD card ======
void loop() {
  float measuredvbat, so_temp = 0, so_hum = 0;
  int so_probe1 = 0;
  String timestamp = "--:--:--";

  // get data
  measuredvbat = analogRead(VbatPin); // reading battery voltage
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage

  //--- soil SHT temp/humidity ---
  so_temp = sht1x.readTemperatureC();
  so_hum = sht1x.readHumidity();

  //--- soil moisture probe(s) ---
  so_probe1 = readSoil();  // need to change if more probes are added
  // need to add calibration to get proper values

  // --- light intensity (LUX) ---

  // --- RGB light/lux fiter ---

  // --- dendrometer ??? ---

  // create combined string
  getWriteString(timestamp, measuredvbat, so_temp, so_hum, so_probe1);

}

// =============================================================
// Additional Functions
// =============================================================

// ===== get Moisture Content =====
int readSoil()
{
  digitalWrite(SoilPow, HIGH); // turn D6 "On"
  delay(10);//wait 10 milliseconds
  int val = analogRead(SoilPin1); // Read the SIG value form sensor
  digitalWrite(SoilPow, LOW); // turn D6 "Off"
  return val;
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
    Serial.println(F("error opening datalog.txt"));
#endif
  }
}

// ====== String function to concatenate all data ======
void getWriteString(String stamp, float vbat, float so_temp, float so_hum, int so_probe1) {
  String out, vb, so_tm, so_rh, so_pr1;
  // convert float to string
  vb = String(vbat, 1); //1 decimal place
  so_tm = String(so_temp, 4); //4 decimal places
  so_rh = String(so_hum, 4); //4 decimal places
  so_pr1 = String(so_probe1);
  out = stamp + "," + vb + "," + so_tm + "," + so_rh + "," + so_pr1;
  //log data
  logData(out);
}

