// LIDAR Lite v3 datasheet: https://static.garmin.com/pumac/LIDAR_Lite_v3_Operation_Manual_and_Technical_Specifications.pdf 

#define DEBUG_LIDAR true

//0 : Default mode, balanced performance.
//1 : Short range, high speed. Uses 0x1d maximum acquisition count.
//2 : Default range, higher speed short range. Turns on quick termination
//    detection for faster measurements at short range (with decreased
//    accuracy)
//3 : Maximum range. Uses 0xff maximum acquisition count.
//4 : High sensitivity detection. Overrides default valid measurement detection
//    algorithm, and uses a threshold value for high sensitivity and noise.
//5 : Low sensitivity detection. Overrides default valid measurement detection 
//    algorithm, and uses a threshold value for low sensitivity and noise.
#define LIDAR_CONFIG 0



#define FW_VERSION 0.5f  // firmware version





/* Network setting */
#define DISABLE_ETHERNET false  // this overrides everything! including the ethernet switch
#define OSC_UDP true            // if false, uses SLIPSerial
#define SMOOTH 0.1f             // between 0.0 and 1.0 --> closer to 0 is more smoothing. 
#define MAX_HEIGHT 150          // in cm

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>    
EthernetUDP udp;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress myIP(10,0,1,225);
//IPAddress destAddress(10,0,2,128);
IPAddress destIP(10,0,1,29);
const uint16_t destPort = 1234;

enum ConnectMode {
  AUTO,
  IP,
  IP_DNS,           // not configured
  IP_DNS_GATE,      // not configured
  IP_DNS_GATE_SUB   // not configured
};
ConnectMode DHCP = IP;

