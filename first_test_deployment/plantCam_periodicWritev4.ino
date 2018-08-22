/*   Author: Marissa Kwon
  Date: 7/16/2018
  Description: This sketch handles sensor and dendrometer datalogging and timing
  links:
  Saving program memory with FLASH storage https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
  SHT-31 temp/humidity sensor https://www.adafruit.com/products/2857
  SHT1x (soil probe) temp/humidity sensor https://www.adafruit.com/product/1298
  TCS345 light sensor https://learn.adafruit.com/adafruit-color-sensors/overview
*/
// add libraries
//#include "pgmspace.h"
#include <Adafruit_SHT31.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClibExtended.h>
#include <LowPower.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_MAX31865.h>

// ---- EDIT HARDWARE/SETTINGS HERE ----

// set to eliminate extra steps in code
#define SLEEP 1  //0 - always powered on; 1 - RTC mode
#define DEBUG 0  //0 - in field test mode; 1 - prints to screen
#define AUTORANGE 1 //0 - set range for TCS345 light sensor; 1 - enables autorange and compensation code

// project settings
const int WakePeriod = 1; // minutes between reads
const int WritePeriod = 1; // sets # of wake/sleep cycles to go through before saving data to SD
volatile int MIN = 0; // (don't edit) placeholder for current MIN
volatile int HR = 0; // (don't edit) placeholder for current HR
byte AlarmFlag = 0; // (don't edit) checkpoint; should only be on when device is awake and collecting data

// pin constants - change these if using a different device (don't edit if hardware is set)
#define SDCheck 12 // attached to CD on SD card logger
#define WakePin 11 // attached to SQW on RTC
#define ChipSelSD 10 // attached to CS on SPI SD logger
#define ChipSelRTD 13 // attached to SPI temperature sensor 
#define VbatPin A9 // reading battery voltage

// defined sensors - true =  in use and plugged in; false = not implemented
#define SHT true //temp/hum sensor
#define MAX true //temp sensor
#define TCS true //light sensor

// ---- END EDIT HARDWARE/SETTINGS ----

#if TCS == 1
// from TCS345 sketch notes
// some magic numbers for this device from the DN40 application
#define TCS34725_R_Coef 0.136
#define TCS34725_G_Coef 1.000
#define TCS34725_B_Coef -0.444
#define TCS34725_GA 1.0
#define TCS34725_DF 310.0
#define TCS34725_CT_Coef 3810.0
#define TCS34725_CT_Offset 1391.0

#if AUTORANGE == 1
// Autorange class for TCS34725
class tcs34725 {
  public:
    tcs34725(void);

    boolean begin(void);
    void getData(void);

    boolean isAvailable, isSaturated;
    uint16_t againx, atime, atime_ms;
    uint16_t r, g, b, c;
    uint16_t ir;
    uint16_t r_comp, g_comp, b_comp, c_comp;
    uint16_t saturation, saturation75;
    float cratio, cpl, ct, lux, maxlux;
    void disable(void);
    void enable(void);

  private:
    struct tcs_agc {
      tcs34725Gain_t ag;
      tcs34725IntegrationTime_t at;
      uint16_t mincnt;
      uint16_t maxcnt;
    };
    static const tcs_agc agc_lst[];
    uint16_t agc_cur;

    void setGainTime(void);
    Adafruit_TCS34725 tcs;
};
//
// Gain/time combinations to use and the min/max limits for hysteresis
// that avoid saturation. They should be in order from dim to bright.
//
// Also set the first min count and the last max count to 0 to indicate
// the start and end of the list.
//
const tcs34725::tcs_agc tcs34725::agc_lst[] = {
  { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_700MS,     0, 20000 },
  { TCS34725_GAIN_60X, TCS34725_INTEGRATIONTIME_154MS,  4990, 63000 },
  { TCS34725_GAIN_16X, TCS34725_INTEGRATIONTIME_154MS, 16790, 63000 },
  { TCS34725_GAIN_4X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 63000 },
  { TCS34725_GAIN_1X,  TCS34725_INTEGRATIONTIME_154MS, 15740, 0 }
};
tcs34725::tcs34725() : agc_cur(0), isAvailable(0), isSaturated(0) {
}

//incorporates the above class and helper functions at the end of the sketch
tcs34725 rgb_sensor;
#endif

#if AUTORANGE == 0 // skips some code
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
#endif

#endif

#if MAX == 1
// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0
#endif

// instances of sensors
#if SHT == 1
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

#if MAX == 1
Adafruit_MAX31865 maxrtd = Adafruit_MAX31865(ChipSelRTD);
#endif

RTC_DS3231 RTC;  //we are using the DS3231 RTC

// global variables
String data_array[WritePeriod + 1];
int count;

void setup() {
  data_array[WritePeriod] = "/0";
#if DEBUG == 1
  Serial.begin(9600);
  while (!Serial) {}; // wait for serial port to connect. Needed for native USB port only
  delay(10);
#endif

#if SHT == 1
  Serial.println(F("test"));
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println(F("Couldn't find SHT31"));
    while (1) delay(1);
  }
#endif

#if MAX == 1
  maxrtd.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
#endif

#if TCS == 1
#if AUTORANGE == 1
  while (!rgb_sensor.begin()) {}
#endif

#if AUTORANGE == 0
  while (!tcs.begin()) {}
#endif
#endif

  if (!SD.begin(ChipSelSD)) { // don't do anything more:
    Serial.println(F("SD card error"));
    while (1);
  }

  pinMode(WakePin, INPUT_PULLUP);

  //Initialize communication with the clock
  Wire.begin();
  // RTC Timer settings here
  if (! RTC.begin()) {
#if DEBUG == 1
    Serial.println(F("Couldn't find RTC"));
#endif
    while (1);
  }
  // This may end up causing a problem in practice - what if RTC looses power in field? Shouldn't happen with coin cell batt backup
#if DEBUG == 1
  //(RTC.lostPower()) {
  Serial.println(F("RTC lost power, lets set the time!"));
  // following line sets the RTC to the date & time this sketch was compiled in DEBUG MODE
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //}
#endif

#if SLEEP == 1 //serial monitor may lag if enabled
  InitializeRTCAlarm();
#elif SLEEP == 0
  AlarmFlag = 1;
#endif

#if DEBUG == 1
  Serial.println(F("card initialized."));
#if TCS == 1
  Serial.println(F("light sensor found."));
#endif

#if SHT == 1
  Serial.println(F("SHT31D temp/humidity sensor found"));
#endif

#if MAX == 1
  Serial.println(F("PT100 temp probe found."));
#endif
#endif
}

// ====== measure data, save it, and write to SD card ======
void loop() {
  float measuredvbat, am_temp = 0, am_hum = 0, max_temp = 0;
  uint16_t r, g, b, c, colorTemp, lux, IR = 0, CPL = 0, max_lux = 0, sat = 0;
  String timestamp = "--:--:--", out;
#if SLEEP == 1
  // enable interrupt for PCINT7...
  pciSetup(11);
  // Power down SD??? Soil moisture (use transistor?),  TSL module.

#if TCS == 1
#if AUTORANGE == 1
  rgb_sensor.disable();
#endif
#endif

  // Wake up when wake up pin is low.
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  // <----  Wait in sleep here until pin interrupt wakes sensor up
  // Power on devices
#if TCS == 1
#if AUTORANGE == 1
  rgb_sensor.enable();
#endif
#endif

  PCICR = 0x00;         // Disable PCINT interrupt
  clearAlarmFunction(); // Clear RTC Alarm
#endif

  delay(50);
  if (AlarmFlag == true) { //cycles the led to indicate that we are no more in sleep mode
    // get RTC timestamp string
    DateTime now = RTC.now();
    uint8_t mo = now.month();
    uint8_t d = now.day();
    uint8_t h = now.hour();
    uint8_t mm = now.minute();

    String RTC_monthString = String(mo, DEC);
    String RTC_dayString = String(d, DEC);
    String RTC_hrString = String(h, DEC);
    String RTC_minString = String(mm, DEC);
    timestamp = RTC_hrString + ":" + RTC_minString + "_" + RTC_monthString + "/" + RTC_dayString;

    // get data
    measuredvbat = analogRead(VbatPin); // reading battery voltage
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage

#if SHT == 1
    //--- ambient SHT temp/humidity ---
    am_temp = sht31.readTemperature();  // degrees C
    am_hum = sht31.readHumidity(); // relative as a percent
#endif

#if MAX == 1
    //--- RTD temperature ---
    max_temp = maxrtd.temperature(RNOMINAL, RREF);
#endif

#if TCS == 1
    // --- RGB light/lux compensated ---
#if AUTORANGE == 1
    rgb_sensor.getData();
    r = rgb_sensor.r_comp;
    g = rgb_sensor.g_comp;
    b = rgb_sensor.b_comp;
    c = rgb_sensor.c_comp;
    CPL = rgb_sensor.cpl;  // stands for circular polarized luminescence
    max_lux = rgb_sensor.maxlux;
    IR = rgb_sensor.ir;
    colorTemp = rgb_sensor.ct;  // color temperature
    sat = rgb_sensor.saturation;
    lux = rgb_sensor.lux;
#endif

    // --- RGB light/lux raw data from sensor---
#if AUTORANGE == 0
    tcs.getRawData(&r, &g, &b, &c);
    colorTemp = tcs.calculateColorTemperature(r, g, b);
    lux = tcs.calculateLux(r, g, b);
#endif
#endif
    // --- dendrometer ??? ---

    // create combined string and store string in volatile memory
    String vb = String(measuredvbat, 1); //1 decimal place
    String am_tm = String(am_temp, 4); //4 decimal places
    String am_rh = String(am_hum, 4); //4 decimal places
    String max_tm = String(max_temp, 4); // 4 decimal places
    String r_str = String(r);
    String g_str = String(g);
    String b_str = String(b);
    String c_str = String(c);
    String ct_str = String(colorTemp);
    String lux_str = String(lux);
    String sat_str = String(sat);
    String ml_str = String(max_lux);
    String CPL_str = String(CPL);
    String IR_str = String(IR);

    //combine strings for storage
    String out = timestamp + "," + vb + "," + max_tm + "," + am_tm + "," + am_rh + "," + r_str + "," + g_str + "," + b_str + "," + c_str + "," + ct_str + "," + lux_str + "," + sat_str + "," + IR_str + "," + CPL_str + "," + ml_str  ;

    // save data and log to SD card when necessary
    if (saveWriteString(out)) {
      count = 0;
    }
    else {
      count++;
    }
#if DEBUG == 1
    //Serial.println(count);
    //Serial.println(writeString);
    // delay(500);
#endif
    delay(500);
#if SLEEP == 1
    setAlarmFunction();
#endif
  } //AlarmFlag == true
}

// =============================================================
// Additional Functions
// =============================================================
#if TCS == 1
#if AUTORANGE == 1
// --- initialize the TCS345 light sensor ---
boolean tcs34725::begin(void) {
  tcs = Adafruit_TCS34725(agc_lst[agc_cur].at, agc_lst[agc_cur].ag);
  if ((isAvailable = tcs.begin()))
    setGainTime();
  return (isAvailable);
}

// ==== enable and disable TCS ====
void tcs34725::enable(void) {
  tcs.enable();
}

void tcs34725::disable(void) {
  tcs.disable();
}

// ==== Set the gain and integration time ====
void tcs34725::setGainTime(void) {
  tcs.setGain(agc_lst[agc_cur].ag);
  tcs.setIntegrationTime(agc_lst[agc_cur].at);
  atime = int(agc_lst[agc_cur].at);
  atime_ms = ((256 - atime) * 2.4);
  switch (agc_lst[agc_cur].ag) {
    case TCS34725_GAIN_1X:
      againx = 1;
      break;
    case TCS34725_GAIN_4X:
      againx = 4;
      break;
    case TCS34725_GAIN_16X:
      againx = 16;
      break;
    case TCS34725_GAIN_60X:
      againx = 60;
      break;
  }
}

// ===== Retrieve data from the sensor and do the calculations ====
void tcs34725::getData(void) {
  // read the sensor and autorange if necessary
  tcs.getRawData(&r, &g, &b, &c);
  while (1) {
    if (agc_lst[agc_cur].maxcnt && c > agc_lst[agc_cur].maxcnt)
      agc_cur++;
    else if (agc_lst[agc_cur].mincnt && c < agc_lst[agc_cur].mincnt)
      agc_cur--;
    else break;

    setGainTime();
    delay((256 - atime) * 2.4 * 2); // shock absorber
    tcs.getRawData(&r, &g, &b, &c);
    break;
  }

  // DN40 calculations
  ir = (r + g + b > c) ? (r + g + b - c) / 2 : 0;
  r_comp = r - ir;
  g_comp = g - ir;
  b_comp = b - ir;
  c_comp = c - ir;
  cratio = float(ir) / float(c);

  saturation = ((256 - atime) > 63) ? 65535 : 1024 * (256 - atime);
  saturation75 = (atime_ms < 150) ? (saturation - saturation / 4) : saturation;
  isSaturated = (atime_ms < 150 && c > saturation75) ? 1 : 0;
  cpl = (atime_ms * againx) / (TCS34725_GA * TCS34725_DF);
  maxlux = 65535 / (cpl * 3);

  lux = (TCS34725_R_Coef * float(r_comp) + TCS34725_G_Coef * float(g_comp) + TCS34725_B_Coef * float(b_comp)) / cpl;
  ct = TCS34725_CT_Coef * float(b_comp) / float(r_comp) + TCS34725_CT_Offset;
}
#endif
#endif

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
    Serial.println(F("error opening datalog.txt"));
#endif
  }
}

// ====== saving strings SD card writes less =======
bool saveWriteString(String ws) {
  // write data to array
  if (count < WritePeriod && (data_array[count + 1] != "/0")) {
    data_array[count] = ws;
#if DEBUG == 1
    Serial.println(F("Saved"));
#endif
    return 0;
  }
  // log data if array is full
  if ((count >= WritePeriod - 1) || (data_array[count + 1] == "\0")) {
    data_array[count] = ws;
#if DEBUG == 1
    Serial.println(F("Write"));
#endif
    logData(data_array);
    return 1;
  }
}

// ======= RTC subroutines =====
void InitializeRTCAlarm() {
  //clear any pending alarms
  clearAlarmFunction();

  // Querry Time and print
  DateTime now = RTC.now();
  Serial.print(F("RTC Time is: "));
  Serial.print(now.hour(), DEC); Serial.print(F(":")); Serial.print(now.minute(), DEC); Serial.print(F(":")); Serial.print(now.second(), DEC); Serial.println();
  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to arduino Interrupt pin for wake-up
  RTC.writeSqwPinMode(DS3231_OFF);

  //Set alarm1
  setAlarmFunction();
}

// clears any alarms
void clearAlarmFunction()
{
  //clear any pending alarms
  RTC.armAlarm(1, false);
  RTC.clearAlarm(1);
  RTC.alarmInterrupt(1, false);
  RTC.armAlarm(2, false);
  RTC.clearAlarm(2);
  RTC.alarmInterrupt(2, false);
}

// sets external alarms for RTC
void setAlarmFunction() {
  DateTime now = RTC.now(); // Check the current time
  // Calculate new time
  MIN = (now.minute() + WakePeriod) % 60; // wrap-around using modulo every 60 sec
  HR  = (now.hour() + ((now.minute() + WakePeriod) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs

  Serial.print(F("Resetting Alarm 1 for: ")); Serial.print(HR); Serial.print(":"); Serial.println(MIN);

  //Set alarm1 every 1 min
  RTC.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0);   //set your wake-up time here
  RTC.alarmInterrupt(1, true);
  AlarmFlag = false; // resets the flag for next wake
}

// special interrupt handler for 32u4
void pciSetup(byte pin)
{
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

// Use one Routine to handle each group
ISR (PCINT0_vect) // handle pin change interrupt for D8 to D13 here
{
  if (digitalRead(11) == LOW)
    AlarmFlag = true;
}

