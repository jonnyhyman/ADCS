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

/* CHANGELOG

  R1:
    - Deleted all torque sensors from the list - not really required and slowing things down bad!

  R2:
    - Changes to tare to delete start values and increase average sample size

*/

#include "HX711.h"
#include <Wire.h>

//    sx(DOUT,SCK)    // parameter "gain" is ommited; the default value 128 is used by the library

  HX711 s0(A0,A1);
  HX711 s1(A2,A3);
  HX711 s2(A4, A5);
  HX711 s3(A6, A7);

  long signed s0_baseline =0;
  long signed s1_baseline =0;
  long signed s2_baseline =0;
  long signed s3_baseline =0;

long signed tare(HX711 scale){

  for (int f=0;f<500;f++){ // discard samples to allow stabilize,x/100 sec / cell // 500 great
     Serial.print("DISCARD:");Serial.println(scale.read());
  }

  long average=0;
  int  average_samplesize = 500; // take 500 samples @ 100Hz -~> 20s for 4

  for (int n=1;n<average_samplesize+1;n++){
   average += scale.read();
   Serial.print("AVERAGE:");Serial.println((long signed)average/n);
  }

  average = average/average_samplesize;

  return average;
}

//float Constants[] = {8678.225752,5945.765631,5679.167472,7334.167737}; // ADC / N -- integration calibrated
//float Constants[] = {7711.065377,6457.157858,5606.936117,3729.05681}; // ADC / N -- centerline calibrated
float Constants[] = {6366.703338,6396.319312,8426.327962,7491.086469}; // ADC / N -- centerline calibrated #2

double Loads[]={-1,-1,-1,-1};
double LastLoads[]={-1,-1,-1,-1};
long int LastTime=millis();

void getLoads(){

    Loads[0] = ( s0.read()-s0_baseline ) /Constants[0]; // ADC / (ADC/N) = N
    Loads[1] = ( s1.read()-s1_baseline ) /Constants[1];
    Loads[2] = ( s2.read()-s2_baseline ) /Constants[2];
    Loads[3] = ( s3.read()-s3_baseline ) /Constants[3];

/*
    Serial.print(Loads[0]);Serial.print(' ');
    Serial.print(Loads[1]);Serial.print(' ');
    Serial.print(Loads[2]);Serial.print(' ');
    Serial.print(Loads[3]);Serial.println(' ');
*/
   for (uint8_t n=0;n<4;n++){
      if (1000*abs(Loads[n]-LastLoads[n])/(millis()-LastTime)>500){
        Loads[n]=LastLoads[n];
      } // ignore typical step errors (>500 Newtons/s)
    }

  LastLoads[0] = Loads[0];LastLoads[1] = Loads[1];LastLoads[2] = Loads[2];LastLoads[3] = Loads[3];
  LastTime=millis();

}

void requestEvent(){

  for (uint8_t i=0;i<4;i++){
    char result[10];memset(result, 0, sizeof result);
    dtostrf(Loads[i],4,2,result);
    for (uint8_t s=0;s<strlen(result);s++){
      Wire.write(result[s]);
    }
    Wire.write('/');
  } Wire.write(NULL);
/*
  for (uint8_t j=0;j<4;j++){
    char result_[10];memset(result_, 0, sizeof result_);
    dtostrf(Loads[j],4,2,result_);
    for (uint8_t k=0;k<strlen(result_);k++){
      Serial.write(result_[k]);
    }
    Serial.write('/');
  } Serial.write('\n');
*/
  getLoads();
}

void setup() {
  Serial.begin(250000);

  Serial.println("Taring Thrust Sensors");
  Serial.println("s0");
  s0_baseline=tare(s0);
  Serial.println("s1");
  s1_baseline=tare(s1);
  Serial.println("s2");
  s2_baseline=tare(s2);
  Serial.println("s3");
  s3_baseline=tare(s3);
  Serial.println("done");

  Wire.begin(0x01);
  Wire.onRequest(requestEvent); // register event
}

void loop() {
 // long int st = millis();
 // getLoads();

  /*
  Serial.println(millis()-st);

z
  for (uint8_t i=0;i<4;i++){
    char result[10];memset(result, 0, sizeof result);
    dtostrf(Loads[i],4,2,result);
    for (uint8_t s=0;s<strlen(result);s++){
      Serial.write(result[s]);
    }
    Serial.write('/');
  } Serial.write('\n');
*/
}
