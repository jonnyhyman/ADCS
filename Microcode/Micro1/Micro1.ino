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

void bonjour(){
  Serial.begin(250000);
  Serial.setTimeout(1);
  while (!Serial){} // wait for serial port to open
  delay(20);Serial.println("-->Micro1");delay(100); // wait for listen terminate
}

int h=0;
int heartbeat(){
  h++;
  if (h>1000){h=0;}
  return (h);
}

/*----------------- FROM HERE TO THE NEXT LINE COMMENT ARE STATIC DEFINITIONS */

int Control_Vector[] = {  0,  // Heartbeat         0
                          0,  // Emergency         1
                          0,  // Arm ('a')         2
                          0,  // A Relay ('A')     3 (Owned by U1)
                          0,  // B Relay           4 (Owned by U1)
                          0,  // F1 Relay          5 (Owned by U2)
                          0,  // I Relay           6 (Owned by U2)
                          0,  // Regulator State   7 (Owned by U1)
                          0,  // Fault Cause U1    8 (Owned by U1)
                          0,  // Fault Cause U2    9 (Owned by U2)
                       -999,  // Count State       10  <-- seconds
                        999,  // Count State       11 <-- milliseconds
                          0}; // Count Mode	       12 <-- mode

// Keys are only used in the uplink back to ADCS
const char *Control_Vector_Keys[]={"H","E","a","A","B","F1","I","R","Fa1","Fa2",
                                   "c1","c1_","Cm"};

// Indices definitions
#define H   0
#define E   1
#define a   2
#define A   3
#define B   4
#define F1  5
#define I   6
#define R   7
#define Fa1 8
#define Fa2 9
#define C  10
#define C_ 11
#define Cm 12

#define Controls_Length 13

int Last_Control_Vector[Controls_Length];
int Command[Controls_Length]; // Formerly ADCS_Controls

int Matrix       [42][6];   // 42 is the max length of the test matrix
                            // 6 is the length of a test matrix row

                            // Matrix Rows have: [Tsec,Tmsec,I,A,B,R]

uint8_t MAT_LENGTH=0; // this variable stores the ACTUAL downlinked length

#define INPUT_SIZE 42 // the answer to life, the universe and everything
                     // also happens to be the maximum expected input length

uint8_t update[Controls_Length];

/*----------------- FROM HERE TO THE ABOVE LINE COMMENT ARE STATIC DEFINITIONS */

//---------------------------------------------------------- Microlink

class Microlink {
 public:
    char input[INPUT_SIZE+1];
    char got  [INPUT_SIZE+1]; // stores nexus receives
    char row  [INPUT_SIZE+1]; // stores matrix rows

    int maxMicrolinkDT = 100 ; long int lastTime = millis();

    void init(){
        Serial.setTimeout(10);
    }
    void transceive(const char* data_out_keys[], int* data_out){
       if (Serial.available()>0){

          byte size = Serial.readBytesUntil('\n',input,INPUT_SIZE); // RECEIVE
          input[size]=0; // final 0 on char string

          Update_evaluate(); // figure out what to send

          for (uint8_t n=0;n<Controls_Length;n++){
              if (update[n]==1){
                Serial.print(data_out_keys[n]);
                Serial.print(":");
                Serial.print(data_out[n]);                            // SEND
                Serial.print(",");
              }
            }   Serial.println();

          parseInput(); lastTime = millis();

        } else {
          if (millis()-lastTime > maxMicrolinkDT){
            Control_Vector[Fa1] = 5;
          }
        }

    }
    void parseInput(){
      char* key = strtok(input, ","); // Read command pair (key,value)
      while (key != 0)
      {
          // Split the command in two values
          char* separator = strchr(key, ':');
          if (separator != 0)
          {
              // Actually split the string in 2: replace ':' with 0
              *separator = 0;
              ++separator;
              int value = atoi(separator);

              if (strcmp(key,"E")==0){Command[E]=value;}
              if (strcmp(key,"a")==0){Command[a]=value;}
              if (strcmp(key,"c1")==0){Command[C]=value;}
              if (strcmp(key,"c1_")==0){Command[C_]=value;}
              if (strcmp(key,"Cm")==0){Command[Cm]=value;}
              if (strcmp(key,"I")==0){Command[I]=value;} // stay up to date
              if (strcmp(key,"F1")==0){Command[F1]=value;} // with micro2 via ADCS
              if (strcmp(key,"Nexus")==0 && value==1){Nexus();}
          }

          // Find the next command in input string
          key = strtok(0, ",");
      }
    }
    void Nexus(){

      uint8_t N = 0;

      for(int b=0;b<42;b++){
       for(int c=0;c<6;c++){Matrix[b][c]=0;}
     }

      bool EnterTheNexus=true;
      while (EnterTheNexus){

        memset(got, 0, sizeof got);

        if (Serial.available()>0){

          Serial.readBytesUntil('\n',got,INPUT_SIZE);
          Serial.println(got);

          if (strcmp(got,"BYE")==0){ Nexus_Parse(N); EnterTheNexus=false; }
     else if (strcmp(got,"RGR")==0){ Nexus_Parse(N);    N+=1    ;
                                     memset(row, 0, sizeof row) ; }
     else if (strcmp(got,"NEG")==0){ memset(got, 0, sizeof got) ;
                                     memset(row, 0, sizeof row) ; }
     else  {
             memcpy(row,got,sizeof(got)); // put got into row for later
           }

        }

      } MAT_LENGTH=N ; for(int x=0;x<=MAT_LENGTH;x++){
                       for(int y=0;y<6;y++){Serial.print(Matrix[x][y]);
                                            Serial.print(' ');}Serial.print('|');}

    }
    void Nexus_Parse(int J){

      //Serial.print("J");Serial.print(J);Serial.print(" ");
      //Serial.print(row);Serial.print(" ");

      int item_=0;
      char* val_ = strtok(row, ",");
      while (val_ != NULL)
      {
          //Serial.print(val_);Serial.print(" | ");
          Matrix[J][item_]=atoi(val_);
          val_ = strtok(NULL, ",");
          item_++;
      }
    }

} microlink;

//------------------------------------------------ Microlink Update Scheme

void Update_setup(){
    for (int v=0; v<Controls_Length; v++){
      update[v]=-1; // negative 1 if DONT CHANGE
    }
}
void Update_evaluate(){

  for (int u=0; u<Controls_Length; u++){
    if (Control_Vector[u]!=Last_Control_Vector[u]){
      update[u]=1;
    } else { update[u]=0; }
  }

  memcpy(Last_Control_Vector,Control_Vector,sizeof(Control_Vector));
}


//---------------------------------------------------------- END OF Microlink
//-----------------------------------------------------------------------------

uint8_t cindex=0; // Maintains the position within the test matrix
unsigned long long int LT_ms=0; // 49 days worth of +integer milliseconds
         double DT_s_i;
         double DT_ms_i;

//---------------------------------------------------------- ACTIONS
//-----------------------------------------------------------------------------

int * actions(int* Control_Vector,int* Command){

    Control_Vector[H] = heartbeat();

    /// ADCS Induced Changes ----------------------------------------------
                                                      // Emergency Changes
    if (Command[E]==1 && Control_Vector[E]==0) {
      Control_Vector[E]=Command[E];
    }
    else if (Command[E]==0 && Control_Vector[E]==1) {
      Control_Vector[E]=Command[E];
    }
                                                        // Arming Changes
    if (Command[a]==1 && Control_Vector[a]==0) {
      Control_Vector[a]=Command[a];
      cindex=0;
    }
    else if (Command[a]==0 && Control_Vector[a]==1) {
      Control_Vector[a]=Command[a];
      cindex=0;
    }
                                                      // Countdown Changes

    if (Command[Cm]==2 && Control_Vector[Cm]!=1) { // changes while counting
      Control_Vector[C] =Command[C];               // not permitted!
      Control_Vector[C_]=Command[C_];
      Control_Vector[Cm]=0;
    }
    else if (Command[Cm]==1 && (Control_Vector[Cm]==0 || Control_Vector[Cm]==2)) {
      Control_Vector[Cm]=1;
      Control_Vector[C_]=0;
      cindex=0;
      LT_ms = millis();
    }
    else if (Command[Cm]==0 && Control_Vector[Cm]==1) {
      Control_Vector[Cm]=0;
    }

    /// Self Induced Changes ----------------------------------------------
                                                      // Countdown Changes
    if (Control_Vector[Cm]==1){

        uint16_t _dt_ = millis()-LT_ms;

        if (_dt_ >= 1){
          Control_Vector[C_] += (_dt_);
          LT_ms=millis();
        }

        if (Control_Vector[C_] >= 1000){
          Control_Vector[C] += Control_Vector[C_]/1000;
          Control_Vector[C_] = Control_Vector[C_]-1000;
        }

    }
                                                // Fault Detected Changes
    if (Control_Vector[Fa1] || Control_Vector[Fa2]){
      Control_Vector[E]=1; // disarm and halt the count
      Control_Vector[a]=0;
      Control_Vector[Cm]=0;
      Command[a]=0;
      Command[Cm]=0;
    }
    return Control_Vector;
}

//---------------------------------------------------------- ACTUATIONS
//-----------------------------------------------------------------------------

int * actuate(int* Control_Vector){

  // Countdown timed actuations // Matrix Rows have: [Tsec,Tmsec,I,A,B,R]

  if (Control_Vector[a]==1 && Control_Vector[Cm]==1){

    if ( (Control_Vector[C]  >= Matrix[cindex+1][0] ) && // if count > event time
         (Control_Vector[C_] >= Matrix[cindex+1][1] ) && // go to next event index
         (cindex+1<=MAT_LENGTH) )                       {  cindex++; }

    //if (Control_Vector[I] !=Matrix[cindex][2]){Control_Vector[I] =Matrix[cindex][2];} // Ignition<-> item 2
    if (Control_Vector[A]!=Matrix[cindex][3]){Control_Vector[A]=Matrix[cindex][3];} // A relay <-> item 3
    if (Control_Vector[B]!=Matrix[cindex][4]){Control_Vector[B]=Matrix[cindex][4];} // B relay <-> item 4
    if (Control_Vector[R]!=Matrix[cindex][5]){Control_Vector[R]=Matrix[cindex][5];} // Regulator <-> item 5

  } else { // DISARM or HOLD actions

    Control_Vector[A]=0;
    Control_Vector[B]=0;
    Control_Vector[R]=0;
    cindex=0;

  }

  // Manually activated actuations

  if (Control_Vector[E]==1){     // Emergency                                   // keep at the end of this loop to OVERRIDE others
    Control_Vector[A] = 0;
    Control_Vector[B] = 0;
    Control_Vector[R] = 0;
    Control_Vector[Cm]= 0; // hold mode
    Control_Vector[a] = 0; // disarm
    Command[Cm]=0;
    Command[a]=0;
  } // if no emergency, no overrides required

  // Relay actuations, keep AFTER Emergency actions

       if (Control_Vector[A]==1){  PORTD |=   1<<3  ;    }
  else if (Control_Vector[A]==0){  PORTD &= ~(1<<3) ;    }

       if (Control_Vector[B]==1){  PORTD |=   1<<2  ;    }
  else if (Control_Vector[B]==0){  PORTD &= ~(1<<2) ;    }

  // REGULATOR CONTROL GOES HERE, PIN 4

  return Control_Vector;

}

//---------------------------------------------------------- I2C Interlink
//---------------------------------------------------------Micro1 & Micro2

#define MAX_INTERSTRING_SIZE 40
int strikes=0;
void Interlink_start(){
  Wire.begin(); // No address so this is a an I2C Master
}
void Interlink_transmit(){
  char interstring[MAX_INTERSTRING_SIZE+1];
  for (uint8_t n=0;n<Controls_Length;n++){
      char numstr[5];
      itoa(Control_Vector[n],numstr,10);

      if (n==0) { strcpy(interstring,numstr);}
      else      { strcat(interstring,numstr);}
      strcat(interstring,",");
  }   strcat(interstring,NULL);
  //Serial.println(interstring);

  Wire.beginTransmission(2); // send to address 2 (Micro2)
  Wire.write(interstring);
  Wire.endTransmission();

}
bool Interlink_check_core(){
  Wire.requestFrom(2,1); // requesting 1 byte -> the state byte - if 0, stop all!
  delayMicroseconds(10);
  if (Wire.available()){
    byte Check_Fault_State = Wire.read();
    if (Check_Fault_State==0x06){     Control_Vector[Fa2]=0; // all good no problemo!
                                      Control_Vector[Fa1]=0; return 1;}
    else if (Check_Fault_State==0x46){Control_Vector[Fa1]=1; // turns out I'm the faulty one!
                                      Control_Vector[Fa2]=0; return 1;}
  } else {return 0;}
}
void Interlink_check(){
  bool check = Interlink_check_core();
  if (!check){strikes+=1;} else {strikes=0;}
  if (!check&&strikes>=2){Control_Vector[Fa2]=1;} // 2 strikes you're out!
}

//---------------------------------------------------------- Main

void setup() {

  bonjour();
  Interlink_start();
  microlink.init();
  Update_setup();

}

void loop() {

  microlink.transceive(Control_Vector_Keys,Control_Vector);
  *Control_Vector = *actions(Control_Vector,Command);
  *Control_Vector = *actuate(Control_Vector);

  Interlink_transmit();
  Interlink_check();

}
