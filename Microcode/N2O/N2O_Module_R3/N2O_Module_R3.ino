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

#include <adafruit_feather.h>

#include <SPI.h>
#include "Adafruit_MAX31855.h"

#include <Wire.h>
#include "HX711.h"

#define MAXDO  PA6
#define MAXCS  PA7
#define MAXCLK PA5

HX711 m0(PA3,PC2); // DT, SCK

long signed m0_baseline =0;

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

const int V0_PIN = PA1; // The pin where the voltage divider is set
const int P0_PIN = PA2; // The pin where the voltage divider is set

int   V0_ADC   = 0;           // The raw ADC value off the voltage div
float vbatLSB  = 0.80566F;   // mV per LSB to convert raw values to volts
float V0_F     = 0.0F;      // The ADC equivalent in millivolts

#define WLAN_SSID_0            "ControlNet"
#define WLAN_PASS_0            "!controlnet!"

#define LOCAL_PORT             44013
#define MAC_ADDR               "6C:0B:84:CA:21:EA"

AdafruitUDP udp;

char packetBuffer[255];
bool connectAP(void)
{
  // Attempt to connect to an AP
  Serial.print("Attempting to connect to: ");
  Serial.print(WLAN_SSID_0);
  Serial.print(" or ");
  Serial.print(WLAN_SSID_1);
  Serial.print(" or ");
  Serial.println(WLAN_SSID_2);

  if ( Feather.connect(WLAN_SSID_0, WLAN_PASS_0) )
  {
    Serial.print("Connected to "); Serial.print(WLAN_SSID_0); Serial.println("!");
  }
  else if ( Feather.connect(WLAN_SSID_1, WLAN_PASS_1) )
  {
    Serial.print("Connected to "); Serial.print(WLAN_SSID_1); Serial.println("!");
  }
  else if ( Feather.connect(WLAN_SSID_2, WLAN_PASS_2) )
  {
    Serial.print("Connected to "); Serial.print(WLAN_SSID_2); Serial.println("!");
  }
  else
  {
    Serial.printf("Failed!");// %s (%d)", Feather.errstr(), Feather.errno());
    Serial.println();
  }
  Serial.println();

  return Feather.connected();
}

void setup()
{

  Serial.begin(115200);

  // THE FOLLOWING LINE MUST BE THE FIRST THING TO RUN!
  // Otherwise the HX711 will enter sleep mode... Lazy bastard!

  load_cell_setup();

  pinMode(V0_PIN, INPUT_ANALOG);
  pinMode(P0_PIN, INPUT_ANALOG);

  // wait for Serial port to connect. Needed for native USB port only
  //while (!Serial) delay(1);

  Serial.println("UDP Nitrous Oxide Instrumentation Module");
  Serial.println();

  while ( !connectAP() )
  {
    delay(500);
  }

  // Tell the UDP client to auto print error codes and halt on errors
  udp.err_actions(true, true);
  udp.setReceivedCallback(received_callback);

  Serial.printf("Openning UDP at port %d ... ", LOCAL_PORT);
  udp.begin(LOCAL_PORT);
  Serial.println("OK");

  Serial.println("Please use your PC/mobile and send any text to ");
  Serial.print( IPAddress(Feather.localIP()) );
  Serial.print(" UDP port ");
  Serial.println(LOCAL_PORT);

}

void loop()
{ // nothing to do here! Everything handled by the handlers. Thanks handlers!
}

/**************************************************************************/
/*!
    @brief  Temperature
*/
/**************************************************************************/

Adafruit_MAX31855 TC0(MAXCLK, MAXCS, MAXDO);

double temperature = -1;

void temperature_loop() {

  double c = TC0.readCelsius();
  if (isnan(c)) {
    temperature = -1 ;
  } else {
    temperature = c ;
  }

}

/**************************************************************************/
/*!
    @brief  Load Cells
*/
/**************************************************************************/

float calibration = -68.13906424; // ADC / gram
double mass = -1; double mass_filt;

const int numload_readings = 3;

double load_readings[numload_readings];      // the load_readings from the analog input
int load_readIndex = 0;              // the index of the current reading
double load_total = 0;                  // the running load_total

void load_cell_setup() {
  /////// KEEP FIRST
  pinMode(PA3, INPUT);
  pinMode(PC2, OUTPUT);

  // Tare the load cell
  m0_baseline = tare(m0);
  ////// KEEP FIRST

  for (int thisReading = 0; thisReading < numload_readings; thisReading++) {
    load_readings[thisReading] = 0;
  }

}
void load_cell_loop() {

  load_total = load_total - load_readings[load_readIndex];
  // read from the sensor:
  load_readings[load_readIndex] = ( m0.read()-m0_baseline ) / calibration ;
  // add the reading to the load_total:
  load_total = load_total + load_readings[load_readIndex];
  // advance to the next position in the array:
  load_readIndex = load_readIndex + 1;

  // if we're at the end of the array...
  if (load_readIndex >= numload_readings) {
    // ...wrap around to the beginning:
    load_readIndex = 0;
  }

  // calculate the average:
  mass = load_total / numload_readings;

}

/**************************************************************************/
/*!
    @brief  Received something from the UDP port
*/
/**************************************************************************/

float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; }

int   P0_ADC = 0;
float P0_F   = 0.0F;
float P0_filtered;
float maxADC_divider = 5 * 6.187/(9.858+6.187) ; // Voutmax * R2/(R1+R2)

void received_callback(void)
{
  int packetSize = udp.parsePacket();

  if ( packetSize )
  {
    long int started = millis();

    udp.read(packetBuffer, sizeof(packetBuffer)); //takes like 250 ms!

    //memset(packetBuffer, 0, sizeof packetBuffer);

    V0_ADC = analogRead(V0_PIN);
    V0_F   = ((float)V0_ADC * vbatLSB) * 2.0F * 1.52F; // 1.52 is correction factor

    P0_ADC = analogRead(P0_PIN);
    float P0_V = map_float((float)P0_ADC,0,4095,0,maxADC_divider);

    P0_F  = map_float(P0_V,
                      maxADC_divider*(0.5/5),
                      maxADC_divider*(4.5/5),
                      101.325,
                      20785.596); // kPa

    temperature_loop();

    load_cell_loop();

    String massS     = String(mass);
    String batteryS  = String(V0_F/1000);
    String pressureS = String(P0_F/1000); // kPa -> MPa
    String temperatureS = String(temperature);

    String packetS   = "V0:" + batteryS;
           packetS  += ",P0:"+pressureS;
           packetS  += ",T0:"+temperatureS;
           packetS  += ",M0:"+massS;

    packetS.toCharArray(packetBuffer, sizeof(packetBuffer));
    packetSize = (packetS).length();

    Serial.print("Send  : ");  Serial.print(packetS);
    Serial.print("  |-> w/Size: ");  Serial.print(packetSize);
    Serial.print("  |-> w/Time: ");  Serial.println(millis()-started);

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(packetBuffer, packetSize);
    udp.endPacket();

  }
}
