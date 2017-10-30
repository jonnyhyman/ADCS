/*
    This file is part of ADCS.

    ADCS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ADCS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ADCS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;

void bonjour(){
  Serial.begin(250000);
  Serial.setTimeout(1);
  while (!Serial){} // wait for serial port to open
  delay(20);Serial.println("-->Micro4");delay(100); // wait for listen terminate
}

int h=0;
int heartbeat(){
  h++;
  if (h>1000){h=0;}
  return (h);
}

float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#define Power_Length 5
double Power_Vector[Power_Length];

// Indices definitions
#define H  0
#define v1 1
#define v2 2
#define i1 3
#define i2 4

const char* Power_Vector_Keys[]={
  "H","v1","v2","i1","i2"
};

// Shunt divider resistances and constants

const double I2_R_1HI = 2147.0;
const double I2_R_2HI = 986.4;

const double I2_R_1LO = 2143.0;
const double I2_R_2LO = 987.2;

const double I1_R_1HI = 2962.0;
const double I1_R_2HI = 983.7;

const double I1_R_1LO = 2961.0;
const double I1_R_2LO = 985.5;

const float I1_CHI = I1_R_2HI / (I1_R_1HI + I1_R_2HI);
const float I1_CLO = I1_R_2LO / (I1_R_1LO + I1_R_2LO);

const float I2_CHI = I2_R_2HI / (I2_R_1HI + I2_R_2HI);
const float I2_CLO = I2_R_2LO / (I2_R_1LO + I2_R_2LO);

const float ratio_1 = 0.0061;
const float ratio_2 = 0.0151;

const double I_1_FULL_SCALE = 50; // Amps
const double I_2_FULL_SCALE = 30; // Amps

const float I1_CORR       = 2.954/258.544; // Amps / ADC
const float I2_CORR       = 2.70/428; // Amps / ADC

const float I1_BIAS       = 767;//798-29;
const float I2_BIAS       = 760-12;//749-33;

const float V1_CORRECTION = 1.02443; // corrects for resistance errors
const float V2_CORRECTION = 1.00883;

class Microlink {  /// In Micro4, we don't really care about the data being down
 public:          /// linked from ADCS.
    void init(){
        Serial.setTimeout(10);
        while (Serial.available()>0){Serial.read();}
    }
    void transceive(const char* data_out_keys[], double* data_out){
       if (Serial.available()>0){
         //unsigned long int ST=micros();
         Serial.read(); // flushInput
         for (uint8_t n=0;n<Power_Length;n++){
             Serial.read(); // flush as best you can!
             Serial.print(data_out_keys[n]); // weird glitch makes zeroth element invisible
             Serial.print(":");
             Serial.print(data_out[n]);                            // SEND
             Serial.print(",");
         }   Serial.println();
         //Serial.println(micros()-ST);
      }
    }

} microlink;

double correction(float adc, float ratio){//,double R1,double R2){

  // First, convert ADC -> voltage

  // Input range is +/- 256mV
  //   Max ADC range  +/- 2^16/2 = +/- 32768
  //   256 mV / ADC * ADC = mV

  float volts = (adc * 256.0) / 32768.0; //mV
  volts = volts * ratio;                //  -> Bus mVolts correction

  return volts;

}

void power_setup(){

  ads.setGain(GAIN_SIXTEEN); // 16x gain  +/- 0.256V  1 bit = 0.125mV
  ads.begin();

}

void power_loop(){

  /* Take measurements from ADC */

  Power_Vector[H] = heartbeat();

  // Voltage dividers have R1 = 3k, R2 = 1k, such that ratio = (1/4)*(12) = 3V
  Power_Vector[v1] = map_float((float)analogRead(A0),0,1023,0,20); // 3V/12V, so 5V/20V
  Power_Vector[v2] = map_float((float)analogRead(A1),0,1023,0,20);

  Power_Vector[v1] = Power_Vector[v1] * V1_CORRECTION;
  Power_Vector[v2] = Power_Vector[v2] * V2_CORRECTION;

  Power_Vector[i1] = (double) ads.readADC_Differential_2_3();
  Power_Vector[i2] = (double) ads.readADC_Differential_0_1();

  Power_Vector[i1] = I1_CORR*(Power_Vector[i1] + I1_BIAS);
  Power_Vector[i2] = I2_CORR*(Power_Vector[i2] + I2_BIAS);

  //Power_Vector[i1] = correction((float)ads.readADC_Differential_2_3(), ratio_1);
  //Power_Vector[i2] = correction((float)ads.readADC_Differential_0_1(), ratio_2);

  //Power_Vector[i1] = (Power_Vector[i1] + I1_CLO * ads.readADC_SingleEnded(3))/ I1_CHI;
  //Power_Vector[i1]-= (Power_Vector[i1] - I1_CHI * ads.readADC_SingleEnded(2))/-I1_CLO;

  //Power_Vector[i2] = (Power_Vector[i2] + I2_CLO * ads.readADC_SingleEnded(1))/ I2_CHI;
  //Power_Vector[i2]-= (Power_Vector[i2] - I2_CHI * ads.readADC_SingleEnded(0))/-I2_CLO;

  //Power_Vector[i1] = -Power_Vector[i1]*I_1_FULL_SCALE/100; // mV -> Amps
  //Power_Vector[i2] = -Power_Vector[i2]*I_2_FULL_SCALE/100;

}

void setup() {

  bonjour();
  microlink.init();
  power_setup();

}

void loop() {
  // Approx 1-2 millis total loop time!
  power_loop();
  microlink.transceive(Power_Vector_Keys,Power_Vector);

}
