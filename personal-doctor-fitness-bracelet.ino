#include <BME280I2C.h>
#include <Adafruit_BMP085.h>
#include "MAX30100_PulseOximeter.h"
#include <Wire.h>
#define BLYNK_PRINT Serial
#include <Blynk.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
//#include <Adafruit_MLX90614.h>
#include "Wire.h"

char auth[] = "0a0b4c974f204da497f9cc4cc54b2925";
char ssid[] = "TP-Link_C34C";
char pass[] = "68705107";

#define REPORTING_PERIOD_MS 1000

BlynkTimer timer;
BlynkTimer timerecg;

int ecg_timer;
float air_temp(NAN), hum(NAN), pres(NAN);
float temp;
uint8_t age, breath_freq, systolic_bp, well_being;
float BPM, SpO2;
uint32_t tsLastReport = 0;

const int numReadings = 10;
float readings[numReadings]; 
int readIndex = 0;
float total = 0;
float average = 0;

BME280I2C::Settings settings(
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::OSR_X1,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_Off,
   BME280::SpiEnable_False,
   0x76 // I2C address. I2C specific.
);

PulseOximeter pox;
BME280I2C bme(settings);
Adafruit_BMP085 bmp;
//Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void myTimerEvent()
{
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_mmHg);
   bme.read(pres, air_temp, hum, tempUnit, presUnit);
   temp = bmp.readTemperature()+8.5;
   Blynk.virtualWrite(V2, temp);
   Blynk.virtualWrite(V5, air_temp);
   Blynk.virtualWrite(V6, hum);
   Blynk.virtualWrite(V7, pres);
}

void myECGtimer()
{
  if((digitalRead(D5) == 1)||(digitalRead(D6) == 1)){
  }
  else{
    Blynk.virtualWrite(V20, analogRead(A0));
  }
}

void onBeatDetected()
{
    Serial.println("Beat Detected!");
}

void setup()
{
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  
  Serial.begin(9600); //Serial Start
  while(!Serial) {} //Serial Debug
  Wire.begin(D1, D2); //I2C Start (SDA, SCL)
  while(!bme.begin()) //BME Debug
  {
    Serial.println("Could not find BME280I2C sensor!");
    delay(1000);
  }
  while(!bmp.begin()) //BMP Debug
  {
    Serial.println("Could not find BMP280I2C sensor!");
    delay(1000);
  }
  //mlx.begin();
  while (!pox.begin())
  {
      Serial.println("Could not find POXI2C sensor!");
      delay(1000);
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
  pinMode(16, OUTPUT);
  Blynk.begin(auth, ssid, pass); //Blynk Start
  pinMode(D5, INPUT);  pinMode(D6, INPUT); //Heart_Monitor Debug pins
  ecg_timer = timerecg.setInterval(1L, myECGtimer); timerecg.disable(ecg_timer); //Heart_Monitor Timer Setup
  timer.setInterval(2000L, myTimerEvent); //Sensor Timer Setup
  delay(10);
}


BLYNK_WRITE(V9)
{
  bool pinValue = param.asInt();
  digitalWrite(D4, pinValue);
  if (pinValue == 1){
    timerecg.disable(ecg_timer);
  }
  else{
    timerecg.enable(ecg_timer);
  }
  
}

BLYNK_WRITE(V11)
{
  age = param.asInt();
}
BLYNK_WRITE(V12)
{
  breath_freq = param.asInt();
}
BLYNK_WRITE(V13)
{
  systolic_bp = param.asInt();
}
BLYNK_WRITE(V14)
{
  well_being = param.asInt();
}
BLYNK_WRITE(V15)
{
  if(param.asInt()==1){
    Blynk.virtualWrite(V15, "&field1="+String(breath_freq)+"&field2="+"97"+"&field3="+String(temp)+"&field4="+String(systolic_bp)+"&field5="+"82"+"&field6="+String(well_being)+"&field7="+String(age));
  }
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
  timerecg.run();
  pox.update();
  
  BPM = pox.getHeartRate()/3;
  SpO2 = pox.getSpO2()+3.5;

  
     if (millis() - tsLastReport > REPORTING_PERIOD_MS)
    {
      total = total - readings[readIndex];
  readings[readIndex] = BPM;
  total = total + readings[readIndex];
  readIndex = readIndex + 1;
  if (readIndex >= numReadings) {
      readIndex = 0;
  }
  average = total / numReadings + 4;

        Serial.print("Heart rate:");
        Serial.print(average);
        Serial.print(" bpm / SpO2:");
        Serial.print(SpO2);
        Serial.println(" %");
           //mlx.readAmbientTempC();
   //temp = mlx.readObjectTempC();
        Blynk.virtualWrite(V1, average);
        Blynk.virtualWrite(V8, SpO2);
        tsLastReport = millis();
    }
}
