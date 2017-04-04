#include "settings.h"

//#include <Wire.h>
//#include <i2c_t3.h>
#include <LIDARLite.h>
#include <OSCMessage.h>

#ifdef BOARD_HAS_USB_SERIAL
#include <SLIPEncodedUSBSerial.h>
SLIPEncodedUSBSerial SLIPSerial( thisBoardsSerialUSB );
#else
#include <SLIPEncodedSerial.h>
SLIPEncodedSerial SLIPSerial(Serial1);
#endif

const int numLidar = 2;
unsigned long biasCounter = 0;

float lidarDistAvg[numLidar];
LIDARLite lidars[numLidar];
const unsigned char lidarAddr[] = {
  100,   // DO NOT USE 0x62 -> it is default and it's easiest to leave it open to set other addresses
  102,
  104
};
// BLUE:   SDA
// GREEN:  SCA
// ORANGE: EN
const uint8_t enPins[] = {
  33,
  34,
  35
};
const uint8_t hardResetPin = 32;
const uint8_t ethernetDisablePin = 31;
boolean ethernetDisable = false;



void setup() {
  // wiz reset + setup:
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);    // begin reset the WIZ820io
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);  // de-select WIZ820io
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);   // de-select the SD Card
  digitalWrite(9, HIGH);   // end reset pulse
  delay(100);

  Serial.begin(115200);
  delay(1000);
  Serial.printf("\n\nLaser Instrument\nc.2017 Tangible Interaction Design Inc.\n");
  Serial.printf("v%f-beta\n", FW_VERSION);

  // only checks ethernet disable on startup:
  pinMode(ethernetDisablePin, INPUT);
  ethernetDisable = digitalRead(ethernetDisablePin);
  if (ethernetDisable || DISABLE_ETHERNET) Serial.printf("Ethernet Disabled\n");
  else Serial.printf("Ethernet Enabled\n");
  
  // power cycle the lidars (they are hanging the bus)
  pinMode(hardResetPin, OUTPUT);
  digitalWrite(hardResetPin, HIGH);
  delay(1000);
  digitalWrite(hardResetPin, LOW);
  delay(200);
  

  if (OSC_UDP) {
    initNetwork();
  }
  else {
    SLIPSerial.begin(115200);
  }

  // disable all LIDAR units (in order to set i2c addrs)
  for (uint8_t i = 0; i < numLidar; i++) {
    pinMode(enPins[i], OUTPUT);
    digitalWrite(enPins[i], LOW);
    delay(100);
  }
  //  digitalWrite(enPins[2], LOW);                    // enable this sensor
  //  delay(100);

  // update to new addresses (must do each time since addresses reset on power cycle)
  for (uint8_t i = 0; i < numLidar; i++) {
    unsigned char serialNum[2];
    unsigned char newAddr[1];
    unsigned char disableStatus[1];
    digitalWrite(enPins[i], HIGH);                    // enable this sensor
    delay(50);                                        // device takes 22ms to do self test etc.
    //lidars[i].reset(0x62);
    delay(50);
    
    Serial.printf("Configuring lidar %i...", i);
    lidars[i].begin(LIDAR_CONFIG, true, 0x62);       // initialize
    Serial.printf("done.\n Getting serial number: ");
    lidars[i].read(0x96, 2, serialNum, false, 0x62); // get serial number
    Serial.print(serialNum[0], HEX); Serial.println(serialNum[1], HEX);
    Serial.printf("Writing serial back to 0x18 & 0x19...");
    lidars[i].write(0x18, serialNum[0], 0x62);        // write low byte of serial num back
    lidars[i].write(0x19, serialNum[1], 0x62);        // write high byte of serial num back
    Serial.printf("done.\n Writing new address...");
    lidars[i].write(0x1a, lidarAddr[i], 0x62);        // write new i2c address
    lidars[i].read(0x1a, 1, newAddr, false, 0x62);    // read back new address

    if (newAddr[0] == lidarAddr[i]) {
      Serial.printf("Success!\nWritten Address: %i, Read address: %i\n", lidarAddr[i], newAddr[0]);
      delay(500);
      lidars[i].write(0x1e, 0x08, 0x62);                // disable default i2c address
      //Serial.printf("Default Address Disabled\n");
      delay(500);
      lidars[i].read(0x1e, 1, disableStatus, false, lidarAddr[i]);
      Serial.printf("Default address disable register: %i\n", disableStatus[0]);
    } else {
      Serial.printf("Error! Written address: %i, intended address: %i\n", newAddr[0], lidarAddr[i]);
      while (1) {
        Serial.printf("FIX ME!\n");
        delay(3000);
      }
    }
    //    digitalWrite(enPins[i], LOW);                    // disable this sensor
    //    delay(100);
  }
  delay(500);

  for (uint8_t i = 0; i < numLidar; i++) {
    lidars[i].configure(LIDAR_CONFIG, lidarAddr[i]);
  }

  Serial.printf("READY\n");
  delay(3000);
}



void loop() {

  // measure distances! (receiver bias correction only every 100th measurement...)
  // (this allows higher speed (no correction), but still calibrates to changing conditions (temp, noise, ambient, etc)).
  float lidarDist[numLidar];
  for (uint8_t i = 0; i < numLidar; i++) {
    if (biasCounter % 100 == 0) {
      lidarDist[i] = (float)lidars[i].distance(true, lidarAddr[i]);
    }
    else {
      lidarDist[i] = (float)lidars[i].distance(false, lidarAddr[i]);
    }
    lidarDist[i] = map(lidarDist[i], 0, MAX_HEIGHT, 100, 0);
    lidarDist[i] /= 100.0f;
    lidarDist[i] = constrain(lidarDist[i], 0.0f, 1.0f);
    lidarDistAvg[i] = SMOOTH * lidarDist[i] + (1.0 - SMOOTH) * lidarDistAvg[i];
    if (DEBUG_LIDAR) Serial.printf("Lidar at %i:\t%f\t", lidarAddr[i], lidarDistAvg[i]);
  }
  if (DEBUG_LIDAR) Serial.println();


  // construct & send OSC: addresses are lidar/0, lidar/1, etc..
  for (uint8_t i = 0; i < numLidar; i++) {
    String  oscAddr =  "/lidar/";   oscAddr += i;
    OSCMessage m(oscAddr.c_str());
    m.add((float)lidarDistAvg[i]);

    // send OSC message over serial:
    if (OSC_UDP) {
      if (!DISABLE_ETHERNET && !ethernetDisable) {
        udp.beginPacket(destIP, destPort);
        m.send(udp);
        udp.endPacket();
        Ethernet.maintain();    // request new DHCP lease if needed (otherwise this may continue to use IP with expired lease)
      }
    }
    else {
      SLIPSerial.beginPacket();
      m.send(SLIPSerial);
      SLIPSerial.endPacket();
    }
    m.empty();
  }

  biasCounter++;
  //delay(10);
}
