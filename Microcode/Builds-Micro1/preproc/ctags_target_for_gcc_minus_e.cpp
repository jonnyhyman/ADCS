# 1 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino"
# 1 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino"
# 2 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino" 2
# 3 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino" 2

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

int Control_Vector[] = { 0, // Heartbeat         0
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

int Last_Control_Vector[14];

double Instrumentation_Vector[] = {0,0,0,0,0,0,0,0,0};
const char *Instrumentation_Vector_Keys[]={
  "H","P0","P1","Vp","Vs","T1","T2","VF","HF"
};


                  // Indices definitions
# 67 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino"
int ADCS_Controls[14];
int Matrix [11][6]; // 12 is the max row length of the test matrix
                            // 6 is the length of a test matrix row

                            // Matrix Rows have: [Tsec,Tmsec,I,A,B,R]

uint8_t MAT_LENGTH=0; // this variable stores the ACTUAL downlinked length


                     // also happens to be the maximum expected input length

uint8_t update[14];
//---------------------------------------------------------- Microlink

class Microlink {
  //void parseInput();
 public:
    char input[42 /* the answer to life, the universe and everything*/+1];
    char nexus[42 /* the answer to life, the universe and everything*/+1];
    void init(){
        Serial.setTimeout(10);
    }
    void transceive(const char* data_out_keys[], int* data_out){
       if (Serial.available()>0){

          byte size = Serial.readBytesUntil('\n',input,42 /* the answer to life, the universe and everything*/); // RECEIVE
          input[size]=0; // final 0 on char string

            for (uint8_t n=0;n<14;n++){
              if (update[n]==1){
                Serial.print(data_out_keys[n]);
                Serial.print(":");
                Serial.print(data_out[n]); // SEND
                Serial.print(",");
              }
            } Serial.println();

          parseInput();

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

              if (strcmp(key,"E")==0){ADCS_Controls[1]=value;}
              if (strcmp(key,"A")==0){ADCS_Controls[2]=value;}
              if (strcmp(key,"F")==0){ADCS_Controls[3]=value;}
              if (strcmp(key,"C")==0){ADCS_Controls[11]=value;}
              if (strcmp(key,"C_")==0){ADCS_Controls[12]=value;}
              if (strcmp(key,"Cm")==0){ADCS_Controls[13]=value;}
              if (strcmp(key,"B")==0){ADCS_Controls[7]=value;} // stay up to date
              if (strcmp(key,"R")==0){ADCS_Controls[8]=value;} // with micro2
              if (strcmp(key,"Nexus")==0){Nexus();}
          }

          // Find the next command in input string
          key = strtok(0, ",");
      }
    }
    void Nexus(){
      /* "Nexus" uplinks have the following steps on the microcontroller side:

              # 1. Get activation trigger from sender 'Nexus:1,'

              # 2. Send 'RDY' from microcontroller

              # 3. Listen for a row of data

              # 4. Send that row of data as received by microcontroller

              # 5. Wait for checked response from sender

              # 5a. If "RGR", keep the data in the pre-assigned buffer

              # 5b. If "NEG", do nothing else with the data

              # 6. Return to step 1 until received "DUN" (rows have been sent)*/
# 147 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino"
      bool exit_nexus=false;
      int N=0; // row number
      bool RDY=true;
      while (exit_nexus==false){
          char resp[3];
          Serial.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = ("RDY"); &__c[0];}))))); // 2
          while (RDY){
            byte size = Serial.readBytesUntil('\n',nexus,42 /* the answer to life, the universe and everything*/); // 3
            nexus[size]=__null;
            if (size>1){RDY=false;}
          }

          Serial.println(nexus); //4

          int got_response=false;
          while (got_response==false){

            byte size = Serial.readBytesUntil('\n',resp,3); // 5
            resp[size]=0;
            Serial.flush();
            ///Serial.print("resp");Serial.println(resp);
            if (strcmp(resp,("RGR"))==0){got_response=true;Nexus_Parse(N);N++;} // 5a
            if (strcmp(resp,("NEG"))==0){got_response=true;} // 5b
            if (strcmp(resp,("DUN"))==0){got_response=true;Nexus_Parse(N);
                                         exit_nexus=true;} // 6
          }
          RDY=true;
      }
      MAT_LENGTH=N;
      /*for (int k=0 ; k <= N ; k++){

        for (int l=0 ; l < 11 ; l++){

            Serial.print('\t');Serial.print(Matrix[k][l]);

        }

        Serial.println();

      }*/
# 182 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino"
    }
    void Nexus_Parse(int J){

      int item=0;
      char* val = strtok(nexus, ",");
      while (val != __null)
      {
          //Serial.print(I);Serial.print(" ");Serial.print(item);Serial.print("-->");Serial.println(val);
          Matrix[J][item]=atoi(val);
          val = strtok(__null, ",");
          item++;
      }
    }

} microlink;

//---------------------------------------------------------- END OF Microlink
//-----------------------------------------------------------------------------

uint8_t cindex=0;
unsigned long int LT_ms=0; // 49 days worth of +integer milliseconds
unsigned long int LT_s =0; // 9 hours worth of +integer seconds
         double DT_s_i;

//---------------------------------------------------------- ACTIONS
//-----------------------------------------------------------------------------

int * actions(int* Control_Vector,int* ADCS_Controls){

    Control_Vector[0] = heartbeat();

    /// ADCS Induced Changes ----------------------------------------------
                                                      // Emergency Changes
    if (ADCS_Controls[1]==1 && Control_Vector[1]==0) {
      Control_Vector[1]=ADCS_Controls[1];
    }
    else if (ADCS_Controls[1]==0 && Control_Vector[1]==1) {
      Control_Vector[1]=ADCS_Controls[1];
    }
                                                        // Arming Changes
    if (ADCS_Controls[2]==1 && Control_Vector[2]==0) {
      Control_Vector[2]=ADCS_Controls[2];
      cindex=0;
    }
    else if (ADCS_Controls[2]==0 && Control_Vector[2]==1) {
      Control_Vector[2]=ADCS_Controls[2];
      cindex=0;
    }
                                                      // Countdown Changes

    if (ADCS_Controls[13]==2 && Control_Vector[13]!=1) { // changes while counting
      Control_Vector[11] =ADCS_Controls[11]; // not permitted!
      Control_Vector[12]=ADCS_Controls[12];
      Control_Vector[13]=0;
    }
    else if (ADCS_Controls[13]==1 && Control_Vector[13]==0) {
      Control_Vector[13]=1;
      cindex=0;
      LT_ms = millis();
      LT_s = millis();
      Control_Vector[12]=0;
    }
    else if (ADCS_Controls[13]==0 && Control_Vector[13]==1) {
      Control_Vector[13]=0;
    }

    /// Self Induced Changes ----------------------------------------------
                                                      // Countdown Changes
    if (Control_Vector[13]==1){

        unsigned int DT_ms = (millis()-LT_ms);
        unsigned int DT_s = (millis()-LT_s);

        if (DT_ms > 1){
            Control_Vector[12]+= (DT_ms);
            LT_ms = millis();
        } if (Control_Vector[12]>=1000){ Control_Vector[12] = 0; } // reset to 0

        if (DT_s > 1000){
            modf(double(DT_s/1000),&DT_s_i); // put DT/1000's integer part into DT_s_i
            Control_Vector[11]+=(DT_s_i); // increment by the DT_s increment
            LT_s = millis();
        }
    }
                                                // Fault Detected Changes
    if (Control_Vector[9]==1 || Control_Vector[10]==1){
      Control_Vector[2]=0; // disarm and halt the count
      Control_Vector[13]=0;
      ADCS_Controls[2]=0;
      ADCS_Controls[13]=0;
    }
    return Control_Vector;
}

//---------------------------------------------------------- ACTUATIONS
//-----------------------------------------------------------------------------
int * actuate(int* Control_Vector){

  // Countdown timed actuations

  if (Control_Vector[2]==1 && Control_Vector[13]==1){

    if ( (Control_Vector[11] >= Matrix[cindex+1][0] ) && // if count > event time
         (Control_Vector[12] >= Matrix[cindex+1][1] ) && // go to next event index
         (cindex+1<=MAT_LENGTH) ) { cindex++; }

    if (Control_Vector[4] !=Matrix[cindex][2]){Control_Vector[4] =Matrix[cindex][2];} // Ignition<-> item 2
    if (Control_Vector[6]!=Matrix[cindex][3]){Control_Vector[6]=Matrix[cindex][3];} // A relay <-> item 3

  } else { // DISARM or HOLD actions

    ADCS_Controls[2]=0;
    Control_Vector[4]=0;
    Control_Vector[6]=0;
    cindex=0;

  }

  // Manually activated actuations

  if (Control_Vector[1]==1){ // Emergency                                   // keep at the end of this loop to OVERRIDE others
    Control_Vector[4] =0;
    Control_Vector[6]=0;
    Control_Vector[13]=0; // hold mode
    Control_Vector[2]=0;

    ADCS_Controls[13]=0;
    ADCS_Controls[2]=0;
  } // if no emergency, no overrides required

  return Control_Vector;

}
//---------------------------------------------------------- I2C Interlink
//---------------------------------------------------------Micro1 & Micro2


void Interlink_start(){
  Wire.begin(); // No address so this is a an I2C Master
}
void Interlink_transmit(){
  char interstring[40 +1];
  for (uint8_t n=0;n<14;n++){
      char numstr[5];
      itoa(Control_Vector[n],numstr,10);

      if (n==0) { strcpy(interstring,numstr);}
      else { strcat(interstring,numstr);}
      strcat(interstring,",");
  } strcat(interstring,__null);
  //Serial.println(interstring);

  Wire.beginTransmission(2); // send to address 2 (Micro2)
  Wire.write(interstring);
  Wire.endTransmission();

}

bool Interlink_check_core(){
  byte returned = Wire.requestFrom(2,1); // requesting 1 byte -> the state byte - if 0, stop all!
  delayMicroseconds(10);
  if (Wire.available()){
    byte Check_Fault_State = Wire.read();
    if (Check_Fault_State==0x06){Control_Vector[10]=0; // all good no problemo!
                                 Control_Vector[9]=0; return 1;}
    if (Check_Fault_State==0x46){Control_Vector[9]=1; // turns out I'm the faulty one!
                                 Control_Vector[10]=0; return 1;}
  } else {return 0;}
}

void Interlink_check(){
  bool check = Interlink_check_core();
  if (!check){check=Interlink_check_core();} // check twice!
  if (!check){
    Control_Vector[10]=1;
    ///Serial.print("FAULT ");Serial.println(check);
  }
}
//---------------------------------------------------------- SPI Instrulink
//--------------------------------------------------Micro3, Micro1 & Micro2
# 363 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V3\\Microcode\\Micro1\\Micro1.ino" 2


char instrustring[75 +1];
char vectorstring[40 +1];

volatile int pos;
volatile int vpos;
volatile boolean youvegotSPI;
volatile boolean active;

void Instrulink_start(){
  pinMode(MISO, 0x1); pos = 0; youvegotSPI = false;
  (*(volatile uint8_t *)((0x2C) + 0x20)) |= (1UL << (6)); // turn on SPI in slave mode
  (*(volatile uint8_t *)((0x2C) + 0x20)) |= (1UL << (7));
}
extern "C" void __vector_17 /* SPI Serial Transfer Complete */ (void) __attribute__ ((signal,used, externally_visible)) ; void __vector_17 /* SPI Serial Transfer Complete */ (void){ // behold, the world's longest ISR!

  char c = (*(volatile uint8_t *)((0x2E) + 0x20));

  if (pos>75 ){pos=0;}
  if (c == 1){
    youvegotSPI = true; active = true; vpos = 0;
    (*(volatile uint8_t *)((0x2E) + 0x20)) = vectorstring[vpos++]; // send first byte
    return;
  } else if (c!=0 && c!='\n'){
    instrustring[pos++] = c;
  }

  if (!active){
    (*(volatile uint8_t *)((0x2E) + 0x20)) = 0;
    return; }

   (*(volatile uint8_t *)((0x2E) + 0x20)) = vectorstring [vpos];
   if (vectorstring [vpos] == '\n' || ++vpos >= sizeof (vectorstring))
   { active = false; }

}
void Instrulink_check(){
  if (youvegotSPI){

    int item=0;
    char* val = strtok(instrustring, ",");
    while (val != __null)
    {
        Instrumentation_Vector[item]=atoi(val);
        val = strtok(__null, ",");
        item++;
        if (item >= 9) break;
    }
    pos = 0; youvegotSPI = false;
  }
}
void Instrulink_transmit_prepare(){

  for (uint8_t n=0;n<14;n++){
      char numstr[5];
      itoa(Control_Vector[n],numstr,10); //base 10
      if (n==0) { strcpy(vectorstring,numstr);}
      else { strcat(vectorstring,numstr);}
      strcat(vectorstring,",");
    } strcat(vectorstring,"\n");
}

void Update_setup(){
    for (int a=0; a<14; a++){
      update[a]=-1; // negative 1 if DONT CHANGE
    }
}

void Update_evaluate(){

  for (int a=0; a<14; a++){
    if (Control_Vector[a]!=Last_Control_Vector[a]){
      update[a]=1;
    } else { update[a]=0; }
  }

  memcpy(Last_Control_Vector,Control_Vector,sizeof(Control_Vector));
}

//---------------------------------------------------------- Main

void setup() {

  bonjour();
  microlink.init();

  Interlink_start();
  Instrulink_start();
  Instrulink_transmit_prepare();

  Update_setup();

}
void loop() {

  Update_evaluate();

  microlink.transceive(Control_Vector_Keys,Control_Vector);
  *Control_Vector = *actions(Control_Vector,ADCS_Controls);
  *Control_Vector = *actuate(Control_Vector);

  Interlink_transmit();
  Interlink_check();

  Instrulink_check();
  Instrulink_transmit_prepare();

}
