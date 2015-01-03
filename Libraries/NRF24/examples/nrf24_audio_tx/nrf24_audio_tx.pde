// nrf24_audio_tx.pde
// -*- mode: C++ -*-
// Example sketch showing how to create an audio digital transmitter 
// with the NRF24 class. 
// Connect a 1Vp-p audio sigal to analog input 0, connected through a 1uF capacitor
// Works with the nrf24_audio_rx sample receiver
// The audio quality is poor: dont expect hi-fi!
//
// This code sends about 250 messages per second, each with 32 8 bit samples from analog input 0
// It uses the NRF4 in NOACK mode. The receiver never acknowledges or replies
// Tested on UNO

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
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setPayloadSize(32))
    Serial.println("setPayloadSize failed");
  if (!nrf24.setRF(NRF24::NRF24DataRate2Mbps, NRF24::NRF24TransmitPower0dBm))
    Serial.println("setRF failed");    
  // Enable the EN_DYN_ACK feature so we can use noack
  nrf24.spiWriteRegister(NRF24_REG_1D_FEATURE, NRF24_EN_DYN_ACK);
  analogReference(INTERNAL); // So we can read 1Vp-p signals
  Serial.println("initialised");
}
  
void loop()
{
  uint8_t buf[32];
  uint8_t i;
  
  // Collect 32 audio samples. At 109 microsecs per sample, we can achieve about 250
  // 32 byte packets transmitted per second: 6.4kHz sample rate
  for (i = 0; i < 32; i++)
    buf[i] = analogRead(0); // 0 - 1023 becomes 0 - 255, top 2 bits clipped. approx 109 microsecs per sample

  // Now send the samples NOACK (EN_DYN_ACK must be enabled first)
  // With 2Mbps, NOACK and 32 byte payload, can send about 1900 messages per sec
  if (!nrf24.setTransmitAddress((uint8_t*)"aurx1", 5))
   Serial.println("setTransmitAddress failed");
  // Send the data  
  if (!nrf24.send(buf, sizeof(buf), true)) // NOACK, 110 microsecs
     Serial.println("send failed");  
  // Transmission takes about 300 microsecs, of which about 130microsecs is transmitter startup time
  // and 160 microsecs is transmit time for 32 bytes+8 bytes overhead @ 2Mbps
  if (!nrf24.waitPacketSent())
     Serial.println("waitPacketSent failed");  
    digitalWrite(3, LOW);
}

