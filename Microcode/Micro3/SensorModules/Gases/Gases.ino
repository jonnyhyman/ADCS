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

#include <SPI.h>
#include <Wire.h>
#include "Adafruit_MAX31855.h"

#define MAXDO   12
#define MAXCS_T1 11
#define MAXCS_T2 10
#define MAXCLK  13

#define T1 0
#define T2 1
#define P1 2

double Gases_Vector[3];

// initialize the Thermocouple
Adafruit_MAX31855 TC1(MAXCLK, MAXCS_T1, MAXDO);
Adafruit_MAX31855 TC2(MAXCLK, MAXCS_T2, MAXDO);

void setup(){
  Serial.begin(250000);
  Wire.begin(0x02);
  Wire.onRequest(requestEvent); // register event
  delay(500);   // wait for MAX chip to stabilize
}

void tc_loop(){

   double c = TC1.readCelsius();
   if (isnan(c)) {Gases_Vector[T1]=-1;} else {      Gases_Vector[T1]=c;    }
          c = TC2.readCelsius();
   if (isnan(c)) {Gases_Vector[T2]=-1;} else {      Gases_Vector[T2]=c;    }

}

void pt_loop(){

// minimum voltage for PTs = 0.5, max is 4.5 for SL kPa to max kPa (500psig = 3548.703 kPa)

  Gases_Vector[P1] = (analogRead(A0) - 1024*(0.5/5))*(3548.703 - 101.325)/(1024*(4.5/5)-1024*(0.5/5))+0;

}

void requestEvent(){

  for (uint8_t i=0;i<3;i++){
    char result[10];memset(result, 0, sizeof result);
    dtostrf(Gases_Vector[i],4,2,result);
    for (uint8_t s=0;s<strlen(result);s++){
      Wire.write(result[s]);
    }
    Wire.write('/');
  } Wire.write(NULL);

}

void loop(){

  tc_loop();
  pt_loop();

  for (uint8_t i=0;i<3;i++){
    char result_[10];memset(result_, 0, sizeof result_);
    dtostrf(Gases_Vector[i],4,2,result_);
    for (uint8_t s=0;s<strlen(result_);s++){
      Serial.write(result_[s]);
    }
    Serial.write('/');
  } Serial.println();


}
