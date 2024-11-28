#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include "MQ131.h"
#include "connexion.h"

//--------------------- Define for the dust_sensor (PM2.5) ------------------------

#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 400            //mv
#define        SYS_VOLTAGE                     5000    


//--------------------------------- Pin definition ---------------------------------
/*#define        DUST_ILED_PIN                   26 
#define        DUST_DATA_PIN                   25*/
#define        DUST_ILED_PIN                   27 
#define        DUST_DATA_PIN                   26

/*#define        MQ131_D_PIN                     0
#define        MQ131_A_PIN                     1*/

#define        MQ7_A_PIN                       32
#define        MQ7_D_PIN                       33

#define        CJMCU_6814_CO                   2
#define        CJMCU_6814_NH3                  0
#define        CJMCU_6814_NO2                  4

/*#define        CJMCU_6814_CO                   5
#define        CJMCU_6814_NH3                  18
#define        CJMCU_6814_NO2                  19*/

#define        FAN_PIN                         15 
/*#define        BUTTON_PIN                      6

#define        ledPin                          2*/




const float MAX_VOLTS = 5.0;
const float MAX_ANALOG_STEPS = 1023.0;

Adafruit_BMP280 bmp; 
unsigned bmp_status;

int fanState = 0;

/*
  Pin initialization
*/
void pinInitilization(){
  pinMode(DUST_ILED_PIN, OUTPUT);
  pinMode(MQ7_D_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  pinMode(MQ7_A_PIN, INPUT);
  pinMode(CJMCU_6814_CO, INPUT);
  pinMode(CJMCU_6814_NH3, INPUT);
  pinMode(CJMCU_6814_NO2, INPUT);

  digitalWrite(DUST_ILED_PIN, LOW);
  /*pinMode(ledPin, OUTPUT);*/
}

/*
private function
*/
int filter(int m)
{
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;
  
  if(flag_first == 0)
  {
    flag_first = 1;
    for(i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++)
    {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];
    
    i = sum / 10.0;
    return i;
  }
}

int getData(int min, int max) {
  return min + (rand() % (max - min + 1));
}

/*
  CJMCU_6814 sensor
  Get carbon monoxide data
*/
float getCO_CJMCU_6814_Data(){
  return analogRead(CJMCU_6814_CO)* (MAX_VOLTS / MAX_ANALOG_STEPS);
}

/*
  CJMCU_6814 sensor
  Get carbon monoxide data
*/
float getNH3_CJMCU_6814_Data(){
  return analogRead(CJMCU_6814_NH3)* (MAX_VOLTS / MAX_ANALOG_STEPS);
}

/*
  CJMCU_6814 sensor
  Get carbon monoxide data
*/
float getNO2_CJMCU_6814_Data(){
  return analogRead(CJMCU_6814_NO2)* (MAX_VOLTS / MAX_ANALOG_STEPS);
}

/* 
  MQ7 sensor
  Get carbon monoxide data
*/
float getCarbonMonoxideData(){
  // A) preparation
  // turn the heater fully on
  analogWrite(MQ7_A_PIN, HIGH); // HIGH = 255
  // heat for 1 min
  timer.startTimer(2000);
  // now reducing the heating power: turn the heater to approx 1,4V
  analogWrite(MQ7_A_PIN, 71.4); // 255x1400/5000
  // heat for 90 sec
  timer.startTimer(5000);
  
  // B) reading    
  // CO2 via MQ7: we need to read the sensor at 5V, but must not let it heat up. So hurry!
  analogWrite(MQ7_A_PIN, HIGH); 

  return analogRead(MQ7_A_PIN);
}

/* 
  Dust sensor ou PM2.5 
  Get fine particle data 
*/
float getFineParticleData(){

  float density, voltage;
  int   adcvalue;
  
  digitalWrite(DUST_ILED_PIN, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(DUST_DATA_PIN); 
  digitalWrite(DUST_ILED_PIN, LOW);
  
  adcvalue = filter(adcvalue);

  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11; //covert voltage (mv)

  if(voltage >= NO_DUST_VOLTAGE) //voltage to density
  {
    voltage -= NO_DUST_VOLTAGE;
    
    density = voltage * COV_RATIO;
  }
  else
    density = 0;
 
  /*Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");  */

  return density;
}

/* 
  BMP280 sensor 
  Get temperature
*/
float getTemperaturedata(){
  return bmp.readTemperature();
}


void fanManagement(float temperature){
  if (temperature > 25 && temperature <= 30){
    analogWrite(FAN_PIN, 85);
    fanState = 1;
  }
  else if (temperature > 30 && temperature <= 40){
    analogWrite(FAN_PIN, 170);
    fanState = 1;
  }
  else if (temperature > 40){
    analogWrite(FAN_PIN, 255);
    fanState = 1;
  }
  else{
    analogWrite(FAN_PIN, 0);
    fanState = 0;
  }
}