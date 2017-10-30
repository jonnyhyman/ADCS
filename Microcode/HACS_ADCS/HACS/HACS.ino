#include <SPI.h>
#include <RH_RF95.h>

/* HOW IT WORKS:

      - Here we take digital states and then send those with magic to receiver!
      - We simply loop through each digital channel and pump each out individually
      - We also update the 7 segment display with the RSSI reading

*/

uint8_t ipins[] = {13,12,11,10,9,6,5}; bool now[7]; uint8_t inputs = 7;
uint8_t opins[] = {23}; uint8_t outputs = 1;

void panelsetup(){

  memset(now,0,sizeof now);

  for (int i_=0;i_<inputs;i_++){
    pinMode(ipins[i_],INPUT);
  }
  for (int o_=0;o_<outputs;o_++){
    pinMode(opins[o_],OUTPUT);
  }

}

void panelsweep(){

  for (int i=0;i<inputs;i++){
      now[i] = digitalRead(ipins[i]);
      RF_STATEUPDATE(i);
  }
}

void panelupdate(uint8_t code_){

  uint8_t pinid = round(code_/10);  // pin index id
  uint8_t pinst = code_ -10*pinid; // state
  digitalWrite(opins[pinid],pinst);

  Serial.print("  code");Serial.print(code_);
  Serial.print("  pinid");Serial.print(pinid);
  Serial.print("  pinst");Serial.println(pinst);Serial.println();
}


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

uint8_t incode[2];  uint8_t len = sizeof(incode);
void RF_STATEUPDATE(int i){
  uint8_t code = i*10 + now[i]; // looks like switchstate -> 01 means index 0 has state 1
  char code_chr[3];
  itoa(code,code_chr,10);
  Serial.print("sent");Serial.println(code_chr);
  rf95.send(code_chr, 2);  // rf95.waitPacketSent();
  if (rf95.waitAvailableTimeout(100)){
    if (rf95.recv(incode, &len)){
      int incodeint = atoi(incode);
      panelupdate(incodeint);
    }
  }
}

//----------------------------------------------------------------- DISPLAY

#include <Wire.h> // Include the Arduino SPI library
const byte s7sAddress = 0x71;
char tempString[10];  // Will be used with sprintf to create strings
void segmentsetup()
{
  Wire.begin();  // Initialize hardware I2C pins

  // Clear the display, and then turn on all segments and decimals
  clearDisplayI2C();  // Clears display, resets cursor
  s7sSendStringI2C("HACS");
  setBrightnessI2C(255);  // High brightness
  delay(1000);
  clearDisplayI2C(); // Clear the display before jumping into loop
}

void segmentupdate()
{ //

  // Magical sprintf creates a string for us to send to the s7s.
  //  The %4d option creates a 4-digit integer.
  //memset(tempString,0,sizeof tempString);
  //sprintf(tempString, "%4d", ());
  int rssi = (rf95.lastRssi());
  itoa(rssi,tempString,10);

  // This will output the tempString to the S7S
  s7sSendStringI2C(tempString);
  
  //    setDecimalsI2C(0b00000100);  // Sets digit 3 decimal on
}

// This custom function works somewhat like a serial.print.
//  You can send it an array of chars (string) and it'll print
//  the first 4 characters in the array.
void s7sSendStringI2C(String toSend)
{
  Wire.beginTransmission(s7sAddress);
  for (int i=0; i<4; i++)
  {
    Wire.write(toSend[i]);
  }
  Wire.endTransmission();
}

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplayI2C()
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x76);  // Clear display command
  Wire.endTransmission();
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightnessI2C(byte value)
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x7A);  // Set brightness command byte
  Wire.write(value);  // brightness data byte
  Wire.endTransmission();
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimalsI2C(byte decimals)
{
  Wire.beginTransmission(s7sAddress);
  Wire.write(0x77);
  Wire.write(decimals);
  Wire.endTransmission();
}

//----------------------------------------------------------------- MAIN

void setup(){

  Serial.begin(250000);

  RFsetup();
  panelsetup();
  segmentsetup();

}

void loop(){

  panelsweep();
  segmentupdate();

}
