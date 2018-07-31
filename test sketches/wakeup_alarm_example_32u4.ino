#include <Wire.h>
#include <RTClibExtended.h>
#include <LowPower.h>

#define wakePin 11    //use interrupt 0 (pin 2) and run function wakeUp when pin 2 gets LOW
#define ledPin 13    //use arduino on-board led for indicating sleep or wakeup status
volatile int MIN = 0;
volatile int HR = 0;
const int WakePeriod = 1;

RTC_DS3231 RTC;      //we are using the DS3231 RTC

byte AlarmFlag = 0;
byte ledStatus = 1;

//-------------------------------------------------

void setup() {
  Serial.begin(9600);
  while (! Serial) {}
  //switch-on the on-board led for 1 second for indicating that the sketch is ok and running
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  if (ledStatus == 0) {
    ledStatus = 1;
    digitalWrite(ledPin, HIGH);
  }
  else {
    ledStatus = 0;
    digitalWrite(ledPin, LOW);
  }

  delay(50);
  Serial.println("test");
  //Set pin D2 as INPUT for accepting the interrupt signal from DS3231

  pinMode(wakePin, INPUT_PULLUP);

  delay(1000);

  //Initialize communication with the clock
  InitializeRTC();

}

//------------------------------------------------------------

void loop() {
  // enable interrupt for PCINT7...
  pciSetup(11);
  // Enter power down state with SD??? Soil moisture (use transistor?),  TSL module disabled.
  // Wake up when wake up pin is low.
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  // <----  Wait in sleep here until pin interrupt wakes sensor up
  PCICR = 0x00;         // Disable PCINT interrupt
  clearAlarmFunction(); // Clear RTC Alarm

  Serial.println("wake");

  if ( AlarmFlag == true) { //cycles the led to indicate that we are no more in sleep mode
    if (ledStatus == 0) {
      ledStatus = 1;
      digitalWrite(ledPin, HIGH);
    }
    else {
      ledStatus = 0;
      digitalWrite(ledPin, LOW);
    }

    delay(500);
    AlarmFlag = false; // resets the flag for next wake
    setAlarmFunction();
  }
}

void InitializeRTC()
{ Wire.begin();
  // RTC Timer settings here
  if (! RTC.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  // This may end up causing a problem in practice - what if RTC looses power in field? Shouldn't happen with coin cell batt backup
  if (RTC.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //clear any pending alarms
  clearAlarmFunction();

  // Querry Time and print
  DateTime now = RTC.now();
  Serial.print("RTC Time is: ");
  Serial.print(now.hour(), DEC); Serial.print(':'); Serial.print(now.minute(), DEC); Serial.print(':'); Serial.print(now.second(), DEC); Serial.println();
  //Set SQW pin to OFF (in my case it was set by default to 1Hz)
  //The output of the DS3231 INT pin is connected to this pin
  //It must be connected to arduino Interrupt pin for wake-up
  RTC.writeSqwPinMode(DS3231_OFF);

  //Set alarm1
  setAlarmFunction();
}

// --- clears any alarms ---
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

void setAlarmFunction() {
  DateTime now = RTC.now(); // Check the current time
  // Calculate new time
  MIN = (now.minute() + WakePeriod) % 60; // wrap-around using modulo every 60 sec
  HR  = (now.hour() + ((now.minute() + WakePeriod) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs

  Serial.print("Resetting Alarm 1 for: "); Serial.print(HR); Serial.print(":"); Serial.println(MIN);

  //Set alarm1 every 1 min
  RTC.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0);   //set your wake-up time here
  RTC.alarmInterrupt(1, true);
}
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
