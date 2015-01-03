// nrf24_ping_client.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messageing client
// with the NRF24 class. 
// It is designed to work with the example nrf24_ping_server
// It also works with ping_server from the Mirf library

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
  // Now be compatible with Mirf ping_server
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setThisAddress((uint8_t*)"clie1", 5))
    Serial.println("setThisAddress failed");
  if (!nrf24.setPayloadSize(sizeof(unsigned long)))
    Serial.println("setPayloadSize failed");
  if (!nrf24.setRF(NRF24::NRF24DataRate2Mbps, NRF24::NRF24TransmitPower0dBm))
    Serial.println("setRF failed");    

  Serial.println("initialised");
}

// With printing commented and delay removed, this can achieve about 666 round trips per second
void loop()
{
//  Serial.println("send");
  
  // Send some data to the server
  if (!nrf24.setTransmitAddress((uint8_t*)"serv1", 5))
    Serial.println("setTransmitAddress failed");

  unsigned long time = millis();
  if (!nrf24.send((uint8_t*)&time, sizeof(time)))
      Serial.println("send failed");  
   if (!nrf24.waitPacketSent())
      Serial.println("waitPacketSent failed"); 
//     Serial.println("Sent");   
  
  if (nrf24.waitAvailableTimeout(100))
  {
    // Server should send the same time back to us

     //    Serial.println("available");
    unsigned long data;
    uint8_t len = sizeof(data);
    if (!nrf24.recv((uint8_t*)&data, &len))
      Serial.println("read failed");

   // Compute the round trip time. This is the time to send one (acknowledged) message (with data) 
   // to the server and 
   // for the server to send one (acknowledged) reply (with data) back to us
   // Typical reported RTT is 1- 2 ms
   unsigned long rtt = millis() - data;
   Serial.print("Ping: ");
   Serial.println(rtt);

  }
  else
    Serial.println("No reply from server");

  delay(1000);  
}

