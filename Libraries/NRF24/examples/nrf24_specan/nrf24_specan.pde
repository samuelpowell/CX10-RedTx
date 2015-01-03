// nrf24_specan.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a primitive spectrum analyser
// with the NRF24 class. 
// The nRF24L01 received power detector is only one bit, but
// this will show which channels have more than -64dBm present

#include <NRF24.h>
#include <SPI.h>


// Singleton instance of the radio
NRF24 nrf24;
// NRF24 nrf24(8, 7); // use this to be electrically compatible with Mirf
// NRF24 nrf24(8, 10);// For Leonardo, need explicit SS pin

void setup() 
{
  Serial.begin(9600);
  while (!Serial)
  	; // wait for serial port to connect. Needed for Leonardo only
  if (!nrf24.init())
    Serial.println("NRF24 init failed");
  // Narrow the receiver bandwidth
  if (!nrf24.setRF(NRF24::NRF24DataRate250kbps, NRF24::NRF24TransmitPowerm18dBm))
    Serial.println("setRF failed");    

  // Start the receiver
  if (!nrf24.powerUpRx())
    Serial.println("powerUpRx failed");

  Serial.println("initialised");
}

// Here we only scan the first 50 channels
void loop()
{
  uint8_t i;
  for (i = 0; i < 50; i++)
  {
      if (!nrf24.setChannel(i))
        Serial.println("setChannel failed");
      delay(1); // need at least 170microsecs
      uint8_t rpd = nrf24.spiReadRegister(NRF24_REG_09_RPD); // 1 bit only :-(
      Serial.print(i, DEC);
      Serial.print(": ");
      Serial.println(rpd);
  }
  Serial.println("-------------------------");
  delay(1000);
}

