// nrf24_test.pde
// -*- mode: C++ -*-
// Test suite for the NRF24 class. 

#include <NRF24.h>
#include <SPI.h>


// Singleton instance of the radio
NRF24 nrf24;

void setup() 
{
  Serial.begin(9600);
  if (!nrf24.init())
    Serial.println("NRF24 init failed");
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setThisAddress((uint8_t*)"clie1", 5))
    Serial.println("setThisAddress failed");
  if (!nrf24.setPayloadSize(4))
    Serial.println("setPayloadSize failed");
  if (!nrf24.setRF(NRF24::NRF24DataRate2Mbps, NRF24::NRF24TransmitPower0dBm))
    Serial.println("setRF failed");  
}

void loop()
{
  Serial.println("start");
 
  // Configure for carrier wave:
  nrf24.spiWriteRegister(NRF24_REG_06_RF_SETUP, NRF24_CONT_WAVE | NRF24_PLL_LOCK | NRF24_PWR_0dBm);
  if (!nrf24.powerUpTx())
    Serial.println("powerUpTx failed");  

  delay(5000);
  if (!nrf24.powerDown())
    Serial.println("powerDown failed");  
 
  delay(5000);
 }

