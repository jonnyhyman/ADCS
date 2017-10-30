#include <adafruit_feather.h>

const int GATE_PIN = PA15; // The pin where the TIP142 gate is driven
unsigned int GATE_STATE = 0;

#include <adafruit_featherap.h>

#define WLAN_SSID            "F2_WLAN"
#define WLAN_PASS            "BeSafe_UseOften"
#define WLAN_ENCRYPTION       ENC_TYPE_WPA2_AES
#define WLAN_CHANNEL          1

#define LOCAL_PORT             18015 // Molecular weight of water :)

IPAddress apIP     (192, 168, 2, 1);
IPAddress apGateway(192, 168, 2, 1);
IPAddress apNetmask(255, 255, 255, 0);

AdafruitUDP udp(WIFI_INTERFACE_SOFTAP);

char packetBuffer[255];

bool connectAP(void)
{
  // Attempt to connect to an AP
  Serial.print("Attempting to connect to: ");
  Serial.print(WLAN_SSID);
 
  if ( Feather.connect(WLAN_SSID, WLAN_PASS) )
  {
    Serial.print("Connected to "); Serial.print(WLAN_SSID); Serial.println("!");
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

  pinMode(GATE_PIN, OUTPUT);

  Serial.begin(115200);

  // wait for Serial port to connect. Needed for native USB port only
  //while (!Serial) delay(1); //*** COMMENT IN PRODUCTION

  Serial.println("UDP Fire Suppression Module");
  Serial.println();

  Serial.println("Configuring SoftAP\r\n");
  FeatherAP.err_actions(true, true);
  FeatherAP.begin(apIP, apGateway, apNetmask, WLAN_CHANNEL);

  Serial.println("Starting SoftAP\r\n");
  FeatherAP.start(WLAN_SSID, WLAN_PASS, WLAN_ENCRYPTION);
  FeatherAP.printNetwork();

  // Tell the UDP client to auto print error codes and halt on errors
  //udp.err_actions(true, true);
  udp.setReceivedCallback(received_callback);

  Serial.printf("Openning UDP at port %d ... ", LOCAL_PORT);
  udp.begin(LOCAL_PORT);
  Serial.println("OK");

  Serial.println("Please use your PC/mobile and send any text to ");
  Serial.print( apIP );
  Serial.print(" UDP port ");
  Serial.println(LOCAL_PORT);

}

void loop()
{ // nothing to do here! Everything handled by the handlers. Thanks handlers!
}

/**************************************************************************/
/*!
    @brief  Received input parsing
*/
/**************************************************************************/

void parse_input(){
      char* key = strtok(packetBuffer, ","); // Read command pair (key,value)
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
              if (strcmp(key,"F2")==0){GATE_STATE = value;}
          }

          // Find the next command in input string
          key = strtok(0, ",");
      }
}

/**************************************************************************/
/*!
    @brief  Received something from the UDP port
*/
/**************************************************************************/

void received_callback(void)
{
  int packetSize = udp.parsePacket();

  if ( packetSize )
  {

    udp.read(packetBuffer, sizeof(packetBuffer));

    Serial.print("Got: "); Serial.println(packetBuffer);

    parse_input();

    digitalWrite(GATE_PIN,GATE_STATE);
    
    String stateS    = String(GATE_STATE);
    String packetS   = "F2:" + stateS;

    packetS.toCharArray(packetBuffer, sizeof(packetBuffer));
    packetSize = (packetS).length();

    Serial.print("Send  : ");  Serial.print(packetS);

    Serial.print("  |-> w/Size: ");  Serial.println(packetSize);

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(packetBuffer, packetSize);
    udp.endPacket();

  }
}
