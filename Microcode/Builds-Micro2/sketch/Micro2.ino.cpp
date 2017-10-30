#line 1
#line 1 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V2\\Microcode\\Micro2\\Micro2.ino"
#include <Arduino.h>

#line 3 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V2\\Microcode\\Micro2\\Micro2.ino"
void bonjour();
#line 11 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V2\\Microcode\\Micro2\\Micro2.ino"
String heartbeat();
#line 31 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V2\\Microcode\\Micro2\\Micro2.ino"
void setup();
#line 35 "C:\\Users\\Jonny Hyman\\Dropbox\\Rocket Drone\\Engineering & Design\\Control_Station_0\\Auto DCS\\V2\\Microcode\\Micro2\\Micro2.ino"
void loop();
#line 3
void bonjour(){
  Serial.begin(250000);
  Serial.setTimeout(10);
  while (!Serial){} // wait for serial port to open
  delay(20);Serial.println("-->Micro2");delay(100); // wait for listen terminate
}

long int h=0;
String heartbeat(){
  h++;
  if (h>1000){h=0;}
  return ("H:"+String(h)+",");
}
class Microlink {
 public:
    void init(){
        Serial.setTimeout(10);
    }
    void transceive(String data){
       if (Serial.available()){
       String in = Serial.readString();
                   Serial.print(heartbeat());
                   Serial.println(data);
      }
    }

} microlink;

void setup() {
  bonjour();
  microlink.init();
}
void loop() {
  microlink.transceive("Caaan you feeel the loooove tonight?");
}

