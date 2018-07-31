/*  Soil Mositure Basic Example Based on sketch written by SparkFun Electronics
    Released under the MIT License(http://opensource.org/licenses/MIT)

    Added calibration testing for soil moisture ranging from fully
    saturated (100%) and completely dry (0%)

*/
const int sensors = 4 // # of soil moisture sensors
    
int val = 0; //value for storing moisture value
int soilPin1 = A0; // input pin for 1st soil moisture sensor
int soilPin2 = A1; // input pin for 2nd  soil moisture sensor
int soilPin1 = A2; // input pin for 3rd soil moisture sensor
int soilPin2 = A3; // input pin for 4th  soil moisture sensor

int soilPower = 6;// control pin for soil moisture base

//Rather than powering the sensor through the 3.3V or 5V pins,
//we'll use a digital pin to power the sensor. This will
//prevent corrosion of the sensor as it sits in the soil.

void setup()
{
  Serial.begin(9600);   // open serial over USB

  pinMode(soilPower, OUTPUT);//Set D7 as an OUTPUT
  digitalWrite(soilPower, LOW);//Set to LOW so no power is flowing through the sensor
}

void loop()
{ 
  //float soil1 = 0, soil2 = 0, soil3 = 0, soil4 = 0;
  int raw_soil1 = 0, raw_soil2 = 0, raw_soil3 = 0, raw_soil4 = 0;

  //ADC conversion
  digitalWrite(soilPower, HIGH);//turn D7 "On"
  delay(10);//wait 10 milliseconds
  raw_soil1 = analogRead(soilPin1);  //note: unsure if sensor readings not happening in parallel but one after the other
  raw_soil2 = analogRead(soilPin2);
  raw_soil3 = analogRead(soilPin3);
  raw_soil4 = analogRead(soilPin4);
  digitalWrite(soilPower, LOW);//turn D7 "Off"
    
  Serial.print("Soil Moisture raw = ");
  //get soil moisture value from the function below and print it - 10 bit ADC on Feather
  Serial.println(raw_soil1); // gives int based on resolution and source voltage
  Serial.println(raw_soil2); // gives int based on resolution and source voltage
  Serial.println(raw_soil3); // gives int based on resolution and source voltage
  Serial.println(raw_soil4); // gives int based on resolution and source voltage
  //add calibration factor calculations here
  
  //This 1 second timefrme is used so you can test the sensor and see it change in real-time.
  //For in-plant applications, you will want to take readings much less frequently.
  delay(1000);//take a reading every second
}


