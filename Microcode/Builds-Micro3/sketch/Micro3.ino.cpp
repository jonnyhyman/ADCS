#line 1 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
#line 1 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
#include <Arduino.h>

#line 3 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void bonjour();
#line 11 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
int heartbeat();
#line 128 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void Salsa_start();
#line 138 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void Salsa_step(const char * send, int select);
#line 152 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void Log_step(int select);
#line 190 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
char * Make_instr_string();
#line 215 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
long readVcc();
#line 222 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void Intralink_start();
#line 229 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void Intralink_receive();
#line 304 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void setup();
#line 313 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void loop();
#line 3 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
void bonjour(){
  Serial.begin(250000);
  Serial.setTimeout(1);
  while (!Serial){} // wait for serial port to open
  delay(20);Serial.println("-->Micro3");delay(100); // wait for listen terminate
}

int h=0;
int heartbeat(){
  h++;
  if (h>1000){h=0;}
  return (h);
}
int Control_Vector_1[] = {  0, // Heartbeat         0
                          0, // Emergency         1
                          0, // Arm               2
                          0, // Fire              3
                          0, // Ignition Relay    4
                          0, // Fire Relay        5
                          0, // A Relay           6
                          0, // B Relay           7
                          0, // Regulator State   8
                          0, // Fault State 1     9
                          0, // Fault State 2     10
                        -30, // Count State       11 <-- seconds
                          0, // Count State       12<-- milliseconds
                          0};// Count Mode	      13<-- mode
int Control_Vector_2[] = {  0, // Heartbeat         0
                          0, // Emergency         1
                          0, // Arm               2
                          0, // Fire              3
                          0, // Ignition Relay    4
                          0, // Fire Relay        5
                          0, // A Relay           6
                          0, // B Relay           7
                          0, // Regulator State   8
                          0, // Fault State 1     9
                          0, // Fault State 2     10
                        -30, // Count State       11 <-- seconds
                          0, // Count State       12<-- milliseconds
                          0};// Count Mode	      13<-- mode
const char *Control_Vector_Keys[]={"H","E","A","F","I","F_","A_","B","R","F1","F2","C","C_","Cm"}; // used in the uplink back to HDCS
#define Controls_Length 14


const char* Instrumentation_Vector_Keys[]={
  "ghost","H","P0","P1","Vp","Vs","T1","T2","XF","YF","ZF","QM","AM" // very weird glitch
};                                          // makes zeroth element invisible...

#define Instrumentation_Length 12
#define Loads_Vector_Length 30
#define Gases_Vector_Length 30

double Instrumentation_Vector[Instrumentation_Length];
                  // Indices definitions

#define H 0
#define P0 1
#define P1 2
#define Vprim 3
#define Vsec 4
#define T1 5
#define T2 6
#define XF 7
#define YF 8
#define ZF 9
#define QM 10
#define ACC 11

// heartbeat already defined
#define E 1
#define A 2
#define Fi 3
#define I 4
#define F_ 5
#define A_ 6
#define B 7
#define R 8
#define Fa1 9
#define Fa2 10
#define C 11
#define C_ 12
#define Cm 13

#define U1_SS 8
#define U2_SS 9
#define U3_5_SS 10

int ADCS_Controls[Controls_Length];

//---------------------------------------------------------- Serial micrlink

class Microlink {  /// In Micro3, we don't really care about the data being down
 public:          /// linked from ADCS. We DO care about Interlink a lot more.
    void init(){
        Serial.setTimeout(10);
        while (Serial.available()>0){Serial.read();}
    }
    void transceive(const char* data_out_keys[], double* data_out){
       if (Serial.available()>0){
         //unsigned long int ST=micros();
         Serial.read(); // flushInput
         for (uint8_t n=0;n<Instrumentation_Length;n++){
             Serial.read(); // flush as best you can!
             Serial.print(data_out_keys[n+1]); // weird glitch makes zeroth element invisible
             Serial.print(":");
             Serial.print(data_out[n]);                            // SEND
             Serial.print(",");
         }   Serial.println();
         //Serial.println(micros()-ST);
      }
    }

} microlink;

//---------------------------------------------------------- SPI Instrulink

#include <SPI.h>

#define MAX_INTERSTRING_SIZE 40
#define MAX_INSTRSTRING_SIZE 40

char    salsa_string [ MAX_INTERSTRING_SIZE+1 ];
char    instr_string [ MAX_INSTRSTRING_SIZE+1 ];

void Salsa_start(){

  pinMode(U1_SS,OUTPUT); pinMode(U2_SS,OUTPUT); pinMode(U3_5_SS,OUTPUT);
  pinMode(MISO,INPUT);
  digitalWrite(U1_SS,HIGH);digitalWrite(U2_SS,HIGH);digitalWrite(U3_5_SS,HIGH);

  SPI.begin();

}

void Salsa_step(const char * send, int select){

   digitalWrite(select,LOW); // invite partner to dance
   //Serial.print("SND :");

   for (uint8_t i=0; i<strlen(send); i++ ){
        salsa_string[i] = SPI.transfer(send[i]);// Serial.print(send[i]);
   }

   digitalWrite(select,HIGH);
   Log_step(select); // finish the dance

}

void Log_step(int select){

  if (select!=-1){

    if (strcmp(salsa_string,"")!=0){

      strcat(salsa_string," FRM ");

      if (select==U1_SS){ strcat(salsa_string, "U1" ); }
      if (select==U2_SS){ strcat(salsa_string, "U2" ); }

      digitalWrite(U3_5_SS,LOW);

      for (uint8_t i=0; i<strlen(salsa_string); i++ ){
           SPI.transfer(salsa_string[i]);  delayMicroseconds(50); // this is an empirically tuned delay
      }    SPI.transfer('\n');

      digitalWrite(U3_5_SS,HIGH);

    } memset(salsa_string, 0, sizeof salsa_string);

  } else {

    if (strcmp(instr_string,"")!=0){
      strcat(instr_string," FRM U3");
      digitalWrite(U3_5_SS,LOW);

      for (uint8_t i=0; i<strlen(instr_string); i++ ){
           SPI.transfer(instr_string[i]);  delayMicroseconds(50); // this is an empirically tuned delay
      }    SPI.transfer('\n');

      digitalWrite(U3_5_SS,HIGH);
    }

  }

}

char* Make_instr_string() {

  memset(instr_string, 0, sizeof instr_string);

  for (uint8_t n=0;n<Instrumentation_Length;n++){
      char numstr[5];
      itoa(Instrumentation_Vector[n],numstr,10);
      if (n==0) { strcpy(instr_string,numstr);}
      else      { strcat(instr_string,numstr);}
      strcat(instr_string,",");
  }   strcat(instr_string,"\n");

  return instr_string;
}

//---------------------------------------------------------- I2C Intralink
//----------------------------------------------------- To DAQ controllers

#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

Adafruit_MMA8451 acc = Adafruit_MMA8451();
uint8_t accstate=0;

long readVcc() {
  long result; // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);//delayMicroseconds(500);
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC)); result = ADCL; result |= ADCH<<8; result = 1126400L / result; // Back-calculate AVcc in mV
  return (result) ; }

void Intralink_start(){

   accstate = acc.begin();
   acc.setRange(MMA8451_RANGE_4_G);
   Wire.begin(); // No address so this is the I2C Master

}
void Intralink_receive(){

  Instrumentation_Vector[H] = heartbeat();

  //************************************************** LOADS

//  Wire.requestFrom(0x01, Loads_Vector_Length);  // request bytes from slave device
  char intra_string [ Loads_Vector_Length ]; memset(intra_string, 0, sizeof intra_string);

  uint8_t p = 0;
/*  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read();
    intra_string[p]=(c); p++;
    if (c =='\n') break;
  }
  //Serial.print("                                                    ");Serial.println(intra_string);
*/  uint8_t index=0;
  /*char* value = strtok(intra_string, "/");
  while (value != NULL)
  {

      if (index==0) Instrumentation_Vector[XF] = atof(value);
      if (index==1) Instrumentation_Vector[YF] = atof(value);
      if (index==2) Instrumentation_Vector[ZF] = atof(value);
      if (index==3) Instrumentation_Vector[QM] = atof(value);

      value = strtok(NULL, "/");
      index++;

  }
*/
  //************************************************** GASES

  Wire.requestFrom(0x02, Gases_Vector_Length);  // request bytes from slave device
  memset(intra_string, 0, sizeof intra_string);
  p = 0;
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read();
    intra_string[p]=(c); p++;
    if (c =='\n') break;
  }
//  Serial.print(" ");Serial.println(intra_string);
  char* value_ = strtok(intra_string, "/");index=0;
  while (value_ != NULL)
  {

      if (index==0) Instrumentation_Vector[T1] = atof(value_);
      if (index==1) Instrumentation_Vector[T2] = atof(value_);
      if (index==2) Instrumentation_Vector[P0] = atof(value_);
      if (index==3) Instrumentation_Vector[P1] = atof(value_);

      value_ = strtok(NULL, "/");
      index++;

  }

  //************************************************** THYSELF


  double Vprim_reading = 5*analogRead(A0)*3.2;
         Vprim_reading = Vprim_reading/1024;
  Instrumentation_Vector[Vprim] = Vprim_reading;
  Instrumentation_Vector[Vsec]  = 5;// read vcc too slow, awaiting new method readVcc()/1000;

  sensors_event_t event;
  acc.getEvent(&event);
  double acceleration_magnitude=sqrt(pow(event.acceleration.x,2)
                                    +pow(event.acceleration.y,2)
                                    +pow(event.acceleration.z,2)
                                  ) / 9.795 ;

  Instrumentation_Vector[ACC] = acceleration_magnitude;
  // update the rest here
}

void setup() {

  bonjour();
  Intralink_start();
  microlink.init();
  Salsa_start();

}

void loop() {

  microlink.transceive(Instrumentation_Vector_Keys,Instrumentation_Vector);

  Intralink_receive();

  Salsa_step( Make_instr_string(), U1_SS ); // takes 840us if no indata, else 1240
  Salsa_step( Make_instr_string(), U2_SS );
  Log_step  (-1);

//  delayMicroseconds(50); // 100 is OK, 1000 is beautiful (if single board)

}

