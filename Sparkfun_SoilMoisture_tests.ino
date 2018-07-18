/*  Soil Mositure Basic Example Based on sketch written by SparkFun Electronics
    Released under the MIT License(http://opensource.org/licenses/MIT)

    Added calibration testing for soil moisture ranging from fully
    saturated (100%) and completely dry (0%)

*/

int val = 0; //value for storing moisture value
int soilPin = A0;//Declare a variable for the soil moisture sensor
int soilPower = 6;//Variable for Soil moisture Power

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
  float moisture = 0;
  int raw_moisture = 0;

  //ADC conversion
  raw_moisture = readSoil();

  Serial.print("Soil Moisture raw = ");
  //get soil moisture value from the function below and print it
  Serial.println(raw_moisture); // gives int based on resolution and source voltage

  //add calibration factor calculations here
  
  //This 1 second timefrme is used so you can test the sensor and see it change in real-time.
  //For in-plant applications, you will want to take readings much less frequently.
  delay(1000);//take a reading every second
}


//This is a function used to get the soil moisture content
int readSoil()
{

  digitalWrite(soilPower, HIGH);//turn D7 "On"
  delay(10);//wait 10 milliseconds
  val = analogRead(soilPin);//Read the SIG value form sensor
  digitalWrite(soilPower, LOW);//turn D7 "Off"
  return val;//send current moisture value
}

