#include <SPI.h>
#include <RH_RF95.h>

/* HOW IT WORKS:

      - Here we take digital out states from ADCS, then when we get a ping from
      HACS, we send our new digital states, and then send the switch code HACS
      gave us to ADCS bu changing a digital pin state.

*/

uint8_t ipins[] = {13,12,11,10,9,6,5}; uint8_t inow[7]; uint8_t inputs = 7; // I means FROM HACS to ADCS
uint8_t opins[] = {3}; uint8_t onow[1]; uint8_t outputs = 1;// O means FROM ADCS to HACS
uint8_t oindex  = 0;
//----------------------------------------------------------------- ADCS pinchks


void ADCSsetup(){

  memset(inow,0,sizeof inow); memset(onow,0,sizeof onow);

  for (int i_=0;i_<inputs;i_++){
    pinMode(ipins[i_],OUTPUT);
  }
  for (int o_=0;o_<outputs;o_++){
    pinMode(opins[o_],INPUT);
  }

}

void ADCScheck(){
    for (int o=0;o<outputs;o++){
        onow[o] = o*10 + int(digitalRead(opins[o]));
    }
}

void ADCSupdt(uint8_t code){
  uint8_t pinid = round(code/10);  // pin index id
  uint8_t pinst = code -10*pinid; // state
  digitalWrite(ipins[pinid],pinst);
  
  Serial.print("  code");Serial.print(code);
  Serial.print("  pinid");Serial.print(pinid);
  Serial.print("  pinst");Serial.println(pinst);Serial.println();
}

//----------------------------------------------------------------- MAGIC

/* for feather32u4 */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define RF95_FREQ 433.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

void RFsetup(){

  digitalWrite(RFM95_RST, LOW);delay(10);
  digitalWrite(RFM95_RST, HIGH);delay(10);

  while(!rf95.init()){
    Serial.println("LoRa radio init failed");
    while (1);
  }
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  } Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  rf95.setTxPower(23, false); // set power to 23dBm

}

void RFUpdt(){
    if (rf95.available()){
      uint8_t incode[2]; uint8_t len = sizeof(incode);
      if (rf95.recv(incode, &len)){
        int incodeint = atoi(incode);
        ADCSupdt(incodeint); // send ADCS HACS states
      }
      char code_chr[3];
      itoa(onow[oindex],code_chr,10);
      Serial.print("sent");Serial.println(code_chr);
      rf95.send(code_chr, 2); rf95.waitPacketSent(); oindex++;

    } if ( oindex > outputs-1 ){ oindex=0; }
}
//----------------------------------------------------------------- MAIN

void setup(){

  Serial.begin(250000);

  RFsetup();
  ADCSsetup();

}

void loop(){

  ADCScheck();// check ADCS output states
  RFUpdt(); // check code from HACS

}
