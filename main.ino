#include "gps.h"
#include "sdReader.h"

gpsData gps;
HardwareSerial gpsSerial(2);

 // Define the RX and TX pins for Serial 2
#define RXD2 16
#define TXD2 17

#define GPS_BAUD 9600

void setup(){
  // Serial Monitor
  Serial.begin(115200);
  // Start Serial 2 with the defined RX and TX pins and a baud rate of 9600
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  
  delay(1000);
  gpsSerial.print("$PCAS02,100*1E\r\n"); 
  Serial.println("GPS started at 9600 baud rate, 10Hz");
}

void loop(){
  initGPS();
  recieveGPSData();
  displayGPSData();
  delay(1000);
  Serial.println("-------------------------------");
}