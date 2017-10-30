# 1 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
# 1 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
# 2 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino" 2

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
int Control_Vector_1[] = { 0, // Heartbeat         0
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
int Control_Vector_2[] = { 0, // Heartbeat         0
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



const char* Instrumentation_Vector_Keys[]={
  "ghost","H","P0","P1","Vp","Vs","T1","T2","XF","YF","ZF","QM","AM" // very weird glitch
}; // makes zeroth element invisible...





double Instrumentation_Vector[12];
                  // Indices definitions
# 72 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
// heartbeat already defined
# 91 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino"
int ADCS_Controls[14];

//---------------------------------------------------------- Serial micrlink

class Microlink { /// In Micro3, we don't really care about the data being down
 public: /// linked from ADCS. We DO care about Interlink a lot more.
    void init(){
        Serial.setTimeout(10);
        while (Serial.available()>0){Serial.read();}
    }
    void transceive(const char* data_out_keys[], double* data_out){
       if (Serial.available()>0){
         //unsigned long int ST=micros();
         Serial.read(); // flushInput
         for (uint8_t n=0;n<12;n++){
             Serial.read(); // flush as best you can!
             Serial.print(data_out_keys[n+1]); // weird glitch makes zeroth element invisible
             Serial.print(":");
             Serial.print(data_out[n]); // SEND
             Serial.print(",");
         } Serial.println();
         //Serial.println(micros()-ST);
      }
    }

} microlink;

//---------------------------------------------------------- SPI Instrulink

# 121 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino" 2




char salsa_string [ 40 +1 ];
char instr_string [ 40 +1 ];

void Salsa_start(){

  pinMode(8,0x1); pinMode(9,0x1); pinMode(10,0x1);
  pinMode(MISO,0x0);
  digitalWrite(8,0x1);digitalWrite(9,0x1);digitalWrite(10,0x1);

  SPI.begin();

}

void Salsa_step(const char * send, int select){

   digitalWrite(select,0x0); // invite partner to dance
   //Serial.print("SND :");

   for (uint8_t i=0; i<strlen(send); i++ ){
        salsa_string[i] = SPI.transfer(send[i]);// Serial.print(send[i]);
   }

   digitalWrite(select,0x1);
   Log_step(select); // finish the dance

}

void Log_step(int select){

  if (select!=-1){

    if (strcmp(salsa_string,"")!=0){

      strcat(salsa_string," FRM ");

      if (select==8){ strcat(salsa_string, "U1" ); }
      if (select==9){ strcat(salsa_string, "U2" ); }

      digitalWrite(10,0x0);

      for (uint8_t i=0; i<strlen(salsa_string); i++ ){
           SPI.transfer(salsa_string[i]); delayMicroseconds(50); // this is an empirically tuned delay
      } SPI.transfer('\n');

      digitalWrite(10,0x1);

    } memset(salsa_string, 0, sizeof salsa_string);

  } else {

    if (strcmp(instr_string,"")!=0){
      strcat(instr_string," FRM U3");
      digitalWrite(10,0x0);

      for (uint8_t i=0; i<strlen(instr_string); i++ ){
           SPI.transfer(instr_string[i]); delayMicroseconds(50); // this is an empirically tuned delay
      } SPI.transfer('\n');

      digitalWrite(10,0x1);
    }

  }

}

char* Make_instr_string() {

  memset(instr_string, 0, sizeof instr_string);

  for (uint8_t n=0;n<12;n++){
      char numstr[5];
      itoa(Instrumentation_Vector[n],numstr,10);
      if (n==0) { strcpy(instr_string,numstr);}
      else { strcat(instr_string,numstr);}
      strcat(instr_string,",");
  } strcat(instr_string,"\n");

  return instr_string;
}

//---------------------------------------------------------- I2C Intralink
//----------------------------------------------------- To DAQ controllers

# 209 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino" 2
# 210 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino" 2
# 211 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V5\\Microcode\\Micro3\\Micro3.ino" 2

Adafruit_MMA8451 acc = Adafruit_MMA8451();
uint8_t accstate=0;

long readVcc() {
  long result; // Read 1.1V reference against AVcc
  (*(volatile uint8_t *)(0x7C)) = (1 << (6)) | (1 << (3)) | (1 << (2)) | (1 << (1));//delayMicroseconds(500);
  (*(volatile uint8_t *)(0x7A)) |= (1 << (6)); // Convert
  while (((*(volatile uint8_t *)(((uint16_t) &((*(volatile uint8_t *)(0x7A)))))) & (1 << (6)))); result = (*(volatile uint8_t *)(0x78)); result |= (*(volatile uint8_t *)(0x79))<<8; result = 1126400L / result; // Back-calculate AVcc in mV
  return (result) ; }

void Intralink_start(){

   accstate = acc.begin();
   acc.setRange(MMA8451_RANGE_4_G);
   Wire.begin(); // No address so this is the I2C Master

}
void Intralink_receive(){

  Instrumentation_Vector[0] = heartbeat();

  //************************************************** LOADS

//  Wire.requestFrom(0x01, Loads_Vector_Length);  // request bytes from slave device
  char intra_string [ 30 ]; memset(intra_string, 0, sizeof intra_string);

  uint8_t p = 0;
/*  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read();
    intra_string[p]=(c); p++;
    if (c =='\n') break;
  }
  //Serial.print("                                                    ");Serial.println(intra_string);
*/ uint8_t index=0;
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

  Wire.requestFrom(0x02, 30); // request bytes from slave device
  memset(intra_string, 0, sizeof intra_string);
  p = 0;
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read();
    intra_string[p]=(c); p++;
    if (c =='\n') break;
  }
//  Serial.print(" ");Serial.println(intra_string);
  char* value_ = strtok(intra_string, "/");index=0;
  while (value_ != __null)
  {

      if (index==0) Instrumentation_Vector[5] = atof(value_);
      if (index==1) Instrumentation_Vector[6] = atof(value_);
      if (index==2) Instrumentation_Vector[1] = atof(value_);
      if (index==3) Instrumentation_Vector[2] = atof(value_);

      value_ = strtok(__null, "/");
      index++;

  }

  //************************************************** THYSELF


  double Vprim_reading = 5*analogRead(A0)*3.2;
         Vprim_reading = Vprim_reading/1024;
  Instrumentation_Vector[3] = Vprim_reading;
  Instrumentation_Vector[4] = 5;// read vcc too slow, awaiting new method readVcc()/1000;

  sensors_event_t event;
  acc.getEvent(&event);
  double acceleration_magnitude=sqrt(pow(event.acceleration.x,2)
                                    +pow(event.acceleration.y,2)
                                    +pow(event.acceleration.z,2)
                                  ) / 9.795 ;

  Instrumentation_Vector[11] = acceleration_magnitude;
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

  Salsa_step( Make_instr_string(), 8 ); // takes 840us if no indata, else 1240
  Salsa_step( Make_instr_string(), 9 );
  Log_step (-1);

//  delayMicroseconds(50); // 100 is OK, 1000 is beautiful (if single board)

}
