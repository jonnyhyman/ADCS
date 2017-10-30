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
  delay(20);Serial.println("-->Micro3.5");delay(100); // wait for listen terminate
}

int h=0;
int heartbeat(){
  h++;
  if (h>1000){h=0;}
  return (h);
}

int Control_Vector[] = {  0, // Heartbeat         0
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
int Last_Control_Vector[Controls_Length];

double Instrumentation_Vector[] = {0,0,0,0,0,0,0,0,0};
const char *Instrumentation_Vector_Keys[]={
  "H","P0","P1","Vp","Vs","T1","T2","VF","HF"
};
#define Instrumentation_Length 9

                  // Indices definitions
#define H 0
#define P0 1
#define P1 2
#define Vprim 3
#define Vsec 4
#define T1 5
#define T2 6
#define VForce 7
#define HForce 8

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

//---------------------------------------------------------- SPI Instrulink
//--------------------------------------------------Micro3, Micro1 & Micro2

#include "SPI.h"

#define MAX_CTRLSTRING_SIZE 72
#define MAX_INSTRUSTRING_SIZE 60  // sum is 202

char SPI_String[MAX_INSTRUSTRING_SIZE+(MAX_CTRLSTRING_SIZE*2)];

unsigned int pos      = 0;
unsigned int take_log = 0;

void SPI_Start(){

  pinMode(12, OUTPUT); // SET MISO as OUTPUT

  PORT->Group[PORTA].PINCFG[16].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM1 SPI PA16 Arduino PIN11
  PORT->Group[PORTA].PMUX[8].bit.PMUXE = 0x2; //SERCOM 1 is selected for peripherial use of this pad
  PORT->Group[PORTA].PINCFG[17].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM1 SPI PA17 Arduino PIN13
  PORT->Group[PORTA].PMUX[8].bit.PMUXO = 0x2; //SERCOM 1 is selected for peripherial use of this pad
  PORT->Group[PORTA].PINCFG[18].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM1 SPI PA18 Arduino PIN10
  PORT->Group[PORTA].PMUX[9].bit.PMUXE = 0x2; //SERCOM 1 is selected for peripherial use of this pad
  PORT->Group[PORTA].PINCFG[19].bit.PMUXEN = 0x1; //Enable Peripheral Multiplexing for SERCOM1 SPI PA19 Arduino PIN12
  PORT->Group[PORTA].PMUX[9].bit.PMUXO = 0x2; //SERCOM 1 is selected for peripherial use of this pad

	//Disable SPI 1
	SERCOM1->SPI.CTRLA.bit.ENABLE =0;
	while(SERCOM1->SPI.SYNCBUSY.bit.ENABLE);

	//Reset SPI 1
	SERCOM1->SPI.CTRLA.bit.SWRST = 1;
	while(SERCOM1->SPI.CTRLA.bit.SWRST || SERCOM1->SPI.SYNCBUSY.bit.SWRST);

	//Setting up NVIC
	NVIC_EnableIRQ(SERCOM1_IRQn);
	NVIC_SetPriority(SERCOM1_IRQn,2);

	//Setting Generic Clock Controller!!!!
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_SERCOM1_CORE) | //Generic Clock 0
						GCLK_CLKCTRL_GEN_GCLK0 | // Generic Clock Generator 0 is the source
						GCLK_CLKCTRL_CLKEN; // Enable Generic Clock Generator

	while(GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY); //Wait for synchronisation

	//Set up SPI Control A Register
	SERCOM1->SPI.CTRLA.bit.DORD = 0; //MSB first
	SERCOM1->SPI.CTRLA.bit.CPOL = 0; //SCK is low when idle, leading edge is rising edge
	SERCOM1->SPI.CTRLA.bit.CPHA = 0; //data sampled on leading sck edge and changed on a trailing sck edge
	SERCOM1->SPI.CTRLA.bit.FORM = 0x0; //Frame format = SPI
	SERCOM1->SPI.CTRLA.bit.DIPO = 0; //DATA PAD0 MOSI is used as input (slave mode)
	SERCOM1->SPI.CTRLA.bit.DOPO = 0x2; //DATA PAD3 MISO is used as output
	SERCOM1->SPI.CTRLA.bit.MODE = 0x2; //SPI in Slave mode
	SERCOM1->SPI.CTRLA.bit.IBON = 0x1; //Buffer Overflow notification
	SERCOM1->SPI.CTRLA.bit.RUNSTDBY = 1; //wake on receiver complete

	//Set up SPI control B register
	//SERCOM1->SPI.CTRLB.bit.RXEN = 0x1; //Enable Receiver
	SERCOM1->SPI.CTRLB.bit.SSDE = 1; //Slave Select Detection Enabled
	SERCOM1->SPI.CTRLB.bit.CHSIZE = 0; //character size 8 Bit
	//SERCOM1->SPI.CTRLB.bit.PLOADEN = 0x1; //Enable Preload Data Register
	//while (SERCOM1->SPI.SYNCBUSY.bit.CTRLB);

	//Set up SPI interrupts
	SERCOM1->SPI.INTENSET.bit.SSL = 0x1; //Enable Slave Select low interrupt
	SERCOM1->SPI.INTENSET.bit.RXC = 0x1; //Receive complete interrupt
	SERCOM1->SPI.INTENSET.bit.TXC = 0x0; //Transmit complete interrupt
	SERCOM1->SPI.INTENSET.bit.ERROR = 0x0; //Receive complete interrupt
	SERCOM1->SPI.INTENSET.bit.DRE = 0x1; //Data Register Empty interrupt
	//init SPI CLK
	//SERCOM1->SPI.BAUD.reg = SERCOM_FREQ_REF / (2*4000000u)-1;
	//Enable SPI
	SERCOM1->SPI.CTRLA.bit.ENABLE = 1;
	while(SERCOM1->SPI.SYNCBUSY.bit.ENABLE);
	SERCOM1->SPI.CTRLB.bit.RXEN = 0x1; //Enable Receiver, this is done here due to errate issue
	while(SERCOM1->SPI.SYNCBUSY.bit.CTRLB); //wait until receiver is enabled

}

void SERCOM1_Handler(){

  uint8_t data = 0;
  uint8_t interrupts = SERCOM1->SPI.INTFLAG.reg; //Read SPI interrupt register

  // Clear slave select interrupt flag
  if(interrupts & (1<<3)){ SERCOM1->SPI.INTFLAG.bit.SSL = 1; }
	if(interrupts & (1<<2)){
     data = SERCOM1->SPI.DATA.reg    ;  // Read data register
		 SERCOM1->SPI.INTFLAG.bit.RXC = 1; // Clear receive complete interrupt
	}
	if(interrupts & (1<<0)){ SERCOM1->SPI.DATA.reg = 0xAA; }

              SD_Loop((char)data); // = the heart of the loop
}

//---------------------------------------------------------- Log stuff

#include <SD.h>
uint8_t sd_go=0; File dataFile; const int chipSelect = 4; uint8_t written=0;
String write_file; String file_name(){
  char filename[15];
  strcpy(filename, "F00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[1] = '0' + i/10;
    filename[2] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }
  return filename;
}

int SD__Setup(){

    Serial.println("Initializing SD card...");

    if (!SD.begin(chipSelect)) {
            Serial.println("Card failed, or not present");
            return 0;
    }

    write_file = file_name();
    dataFile   = SD.open(write_file, FILE_WRITE);

    return 1;

}
void SD_Flush(){
    dataFile.flush();
    written=0;
}
void SD_Loop(char data) {
  if (sd_go && dataFile){
      Serial.write(data);
      written += dataFile.write(data);
      if (written>128){SD_Flush();}
    }
}


//---------------------------------------------------------- Main

void setup() {

          bonjour();
          SPI_Start();
  sd_go = SD__Setup();

}

void loop() { ; }
