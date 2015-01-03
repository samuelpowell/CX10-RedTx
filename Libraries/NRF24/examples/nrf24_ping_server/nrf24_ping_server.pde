// nrf24_ping_server.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing server
// with the NRF24 class. 
// It is designed to work with the example nrf24_ping_client. 
// It also works with ping_client from the Mirf library

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
  // Defaults after init are 2.402 GHz (channel 2)
  // Now be compatible with Mirf ping_client
 if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setThisAddress((uint8_t*)"serv1", 5))
    Serial.println("setThisAddress failed");
  if (!nrf24.setPayloadSize(sizeof(unsigned long)))
    Serial.println("setPayloadSize failed");
  if (!nrf24.setRF(NRF24::NRF24DataRate2Mbps, NRF24::NRF24TransmitPower0dBm))
    Serial.println("setRF failed");    
  Serial.println("initialised");
}

void loop()
{
//  Serial.println("waiting");
  nrf24.waitAvailable();
  // ping_client sends us an unsigned long containing its timestamp
  unsigned long data;
  uint8_t len = sizeof(data);
  if (!nrf24.recv((uint8_t*)&data, &len))
    Serial.println("read failed");
//   Serial.println(data);

  // Now send the same data back
  // Need to set the address of the detination each time, since auto-ack changes the TX address
  if (!nrf24.setTransmitAddress((uint8_t*)"clie1", 5))
   Serial.println("setTransmitAddress failed");
  // Send the same data back   
  if (!nrf24.send((uint8_t*)&data, sizeof(data)))
     Serial.println("send failed");  
  if (!nrf24.waitPacketSent())
  {
     Serial.println("waitPacketSent failed");  
  }
}

