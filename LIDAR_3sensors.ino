#include "settings.h"

#include <Wire.h>
#include <LIDARLite.h>
#include <OSCMessage.h>

  #ifdef BOARD_HAS_USB_SERIAL
    #include <SLIPEncodedUSBSerial.h>
    SLIPEncodedUSBSerial SLIPSerial( thisBoardsSerialUSB );
  #else
    #include <SLIPEncodedSerial.h>
    SLIPEncodedSerial SLIPSerial(Serial1);
  #endif

const int numLidar = 3;
unsigned long biasCounter = 0;

LIDARLite lidars[numLidar];
uint8_t lidarAddr[] = {
  0x63,   // DO NOT USE 0x62 -> it is default and it's easiest to leave it open to set other addresses
  0x64,
  0x65
};
// BLUE:   SDA
// GREEN:  SCA
// ORANGE: EN
uint8_t enPins[] = {
  20,
  21,
  22
};



void setup() {
  Serial.begin(115200);
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
  }

  // update to new addresses (must do each time since addresses reset on power cycle)
  for (uint8_t i = 0; i < numLidar; i++) {
    uint8_t serialNum[2];
    uint8_t newAddr[1];
    digitalWrite(enPins[i], HIGH);                    // enable this sensor
    delay(25);                                        // device takes 22ms to do self test etc.
    lidars[i].begin(0, true);                         // initialize
    lidars[i].read(0x96, 2, serialNum, false, 0x62);  // get serial number
    lidars[i].write(0x18, serialNum[0], 0x62);        // write low byte of serial num back
    lidars[i].write(0x19, serialNum[1], 0x62);        // write high byte of serial num back
    lidars[i].write(0x1a, lidarAddr[i], 0x62);        // write new i2c address
    lidars[i].read(0x1a, 1, newAddr, false, 0x62);    // read back new address
    if (newAddr[0] == lidarAddr[i]) {
      Serial.printf("Success! New address: %i\n", newAddr[0]);
      lidars[i].write(0x1e, 0x08, 0x62);                // disable default i2c address
    } else {
      Serial.printf("Error! Written address: %i, intended address: %i\n", newAddr[0], lidarAddr[i]);
      while (0) {
        Serial.printf("FIX ME!\n");
        delay(250);
      }
    }
  }

  for (uint8_t i = 0; i < numLidar; i++) {
    lidars[i].configure(LIDAR_CONFIG, lidarAddr[i]);
  }
}



void loop() {

  // measure distances! (receiver bias correction only every 100th measurement...)
  // (this allows higher speed (no correction), but still calibrates to changing conditions (temp, noise, ambient, etc)).
  int lidarDist[numLidar];
  for (uint8_t i = 0; i < numLidar; i++) {
    if (biasCounter % 100 == 0) {
      lidarDist[i] = lidars[i].distance(true, lidarAddr[i]);
    }
    else {
      lidarDist[i] = lidars[i].distance(false, lidarAddr[i]);
    }
    if (DEBUG_LIDAR) Serial.printf("Lidar at %i:\t%i\t", lidarAddr[i], lidarDist[i]);
  }
  if (DEBUG_LIDAR) Serial.println();


  // construct & send OSC: addresses are lidar/0, lidar/1, etc..
  for (uint8_t i = 0; i < numLidar; i++) {
    String  oscAddr =  "/lidar/";   oscAddr += i;
    OSCMessage m(oscAddr.c_str());
    m.add((int32_t)lidarDist[i]);

    // send OSC message over serial: 
    if (OSC_UDP) {
      udp.beginPacket(destIP, destPort);
        m.send(udp);
      udp.endPacket();
      Ethernet.maintain();    // request new DHCP lease if needed (otherwise this may continue to use IP with expired lease) 
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
