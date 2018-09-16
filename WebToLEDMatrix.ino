// (c) Copyright Lyle Troxell
// Released under Apache License, version 2.0
// This project requires an Ethernet Sheild and 
// a MAX7219 Led Matrix Unit and uses
// LedControl http://wayoda.github.io/LedControl/

#include <SPI.h>
#include <HttpClient.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include "LedControl.h"
#include <avr/pgmspace.h>

//LedControl lc=LedControl(12,11,10,1);
LedControl lc=LedControl(2,4,3,1);

// https://lyle.troxell.com/beMineMessage.txt
const char kHostname[] = "lyle.github.io";
//const char kPath[] = "/dev/beMineMessage.txt";
const char kPath[] = "/dev/by-mine";

const int kPort = 80;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout =20L*1000L; //20 seconds before giving up
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000; //Between Requests to the server

byte matrixVals[] = { 0,0,0,0,0,0,0,0 };


// time between web requests
const unsigned long postingInterval = 30L * 1000L; //30seconds between requests
//No Delay the first time
unsigned long lastConnectionTime = millis() + postingInterval;

void startLEDMatrix() {
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
}

void setup()
{
  startLEDMatrix();
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
 
  Serial.println("We Are Booting Up");
  bool attemptNetwork = true;
  while (attemptNetwork)
  {
    Serial.println("About to get IP address via DHCP");
    int d = Ethernet.begin(mac);
    Serial.print("Ethernet Status:");
    Serial.println(d);
    if (d==1) {
      attemptNetwork = false;
    } else {
      delay(1500); 
    }
  }
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
}

void shiftMatrix(unsigned char movingLine) {
  byte temp;
  //Serial.println(int(movingLine) );
  for ( int i=0; i < 8; ++i ) {
    temp = matrixVals[i];
    matrixVals[i] = movingLine;
    movingLine = temp;
    lc.setRow(0,7-i, matrixVals[i]);
  }
  
//  for ( int i=0; i < 8; ++i ) {
//    //Serial.write(matrixVals[i]);
//  }
}


void loop()
{
  if (millis() - lastConnectionTime > postingInterval) {
    httpRequest();
  }
}
void httpRequest() {
  int err =0;
  unsigned char c = 0;
  EthernetClient client;
  HttpClient http(client);
  
  err = http.get(kHostname, kPort, kPath);
  if (err == 0){
    Serial.println("startedRequest ok");
    err = http.responseStatusCode();
    if (err == 200) {
      Serial.println("Response Code: " + err);
      err = http.skipResponseHeaders();
      if (err >= 0){
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        unsigned long timeoutStart = millis();
        
        byte d[1];
        byte modifyInt;
        while ( (http.connected() || http.available()) &&
               ((millis() - timeoutStart) < kNetworkTimeout) ){
            if (http.available()){
              c = http.read();
              shiftMatrix(c);
              delay(500);
              bodyLen--;
              // We read something, reset the timeout counter
              timeoutStart = millis();
            } else {
                // No data yet, wait a bit for some
                delay(kNetworkDelay);
            }
        }
        
        lastConnectionTime = millis();
      }else{
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }else{    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }else{
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
}
