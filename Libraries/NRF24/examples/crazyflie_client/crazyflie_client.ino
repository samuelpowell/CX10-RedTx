// crazyflie_client.pde
// -*- mode: C++ -*-
//
// This sketch act like a Crazyflie client (ie the transmitter) using the CRTP radiolink protocol:
// http://wiki.bitcraze.se/projects:crazyflie:firmware:comm_protocol
// to control a Crazyflie quadcopter http://www.bitcraze.se/
//
// Requires 
// - RC Transmitter that can act in trainer mode (eg Spektrum DX6i and others)
// - NRF24 radio module such as the sparkfun WRL-00691 http://www.sparkfun.com/products/691
// - Arduino such as Uno
//
// Uses NRF24 library to comunicate withthe Crazyflie, 
// http://www.airspayce.com/mikem/arduino/NRF24
// and uses
// the  RcTrainer library to get servo positions from an RC transmitter running as a trainer
// see http://www.airspayce.com/mikem/arduino/RcTrainer
// Transmits the servo posiiotns to the Crazyflie, therefore allowing you to control the Crazyflie from 
// a conventional RC transmitter (ie no PC and no CrazyRadio module required).
//
// Only uses the Link ECHO and commander messages. No Log or Param messages are used 
// Author: Mike McCauley
// Copyright (C) 2012 Mike McCauley

#include <NRF24.h>
#include <SPI.h>
#include <RcTrainer.h>

// Structure of Crazyflie commander messages
#pragma pack(1);
typedef struct 
{
  float roll;
  float pitch;
  float yaw;
  uint16_t thrust;
} CommanderCrtpValues;
#pragma pack()

// Useful macros for CRTP message contents and formatting
#define CRTP_HEADER(port, channel) (((port & 0x0F) << 4) | (channel & 0x0F))
#define CRTP_HEADER_PORT(h) ((h >> 4) & 0xf)
#define CRTP_HEADER_CHANNEL(h) (h & 0x3)

// Param channels
#define PARAM_TOC_CH 0
#define PARAM_READ_CH 1
#define PARAM_WRITE_CH 2

// Log channels
#define LOG_TOC_CH 0
#define LOG_CONTROL_CH 1
#define LOG_LOG_CH 2
// Log packet parameters storage
#define LOG_MAX_OPS 64
#define LOG_MAX_BLOCKS 8

// Port definitions
typedef enum {
  CRTP_PORT_CONSOLE     = 0x00,
  CRTP_PORT_PARAM       = 0x02,
  CRTP_PORT_COMMANDER   = 0x03,
  CRTP_PORT_LOG         = 0x05,
  CRTP_PORT_LINK        = 0x0F,
} CRTPPort;

// Common command numbers
#define CMD_GET_ITEM 0
#define CMD_GET_INFO 1

// Singleton instance of the radio
NRF24 nrf24;
// NRF24 nrf24(8, 7); // use this to be electrically compatible with Mirf
// NRF24 nrf24(8, 10);// For Leonardo, need explicit SS pin

// The currently used radio channel
uint8_t channel;

// The expected address of the Crazyflie server (copter)
// This is in fact also the default address of the NRF24
uint8_t address[] = { 0xe7, 0xe7, 0xe7, 0xe7, 0xe7 };
uint32_t failed_packet_count = 0;

// Object to get servo positions from RC trainer on pin D2
RcTrainer rc;

void setup() 
{
  Serial.begin(115200);
  while (!Serial)
    ; // Wait for serial port, only required for Leonardo

  if (!nrf24.init())
    Serial.println("NRF24 init failed");
  
  // Be Crazyflie radiolink compatible
  // We use the NRF24 library for convenience, but
  // we use a different configuration to the default NRF24
  if (!nrf24.setChannel(13))
     Serial.println("setChannel failed");
  // Set data rate to 250k and low power
  if (!nrf24.setRF(NRF24::NRF24DataRate250kbps, NRF24::NRF24TransmitPower0dBm))
     Serial.println("setRF failed");  
  // Be compatible with Crazyflie: No interrupts, 2 bytes CRC
  nrf24.setConfiguration(NRF24_MASK_RX_DR | NRF24_MASK_TX_DS | NRF24_MASK_MAX_RT | NRF24_EN_CRC | NRF24_CRCO);
  nrf24.spiWriteRegister(NRF24_REG_1D_FEATURE, NRF24_EN_DPL | NRF24_EN_ACK_PAY);   // Dynamic size payload + ack
  nrf24.spiWriteRegister(NRF24_REG_1C_DYNPD, NRF24_DPL_P0);     // Dynamic payload on pipe 0
  if (!nrf24.setRetry(6, 3)) // 1500us and 3 retries
     Serial.println("setRetry failed");  
  if (!nrf24.setTransmitAddress(address, sizeof(address)))
     Serial.println("setTransmitAddress failed");  

  Serial.println("initialised");
}

// Debugging data dumper
void dump(char* prompt, uint8_t* data, uint8_t len)
{
  Serial.print(prompt);
  Serial.print(": ");	
  for (int i = 0; i < len; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
}

// Scan for a Crazyflie server (ie a copter) that responds
// If one is found, set the channel and return true
boolean scan()
{
  // Standard LINK echo message
  // If there is a Crazyflie present on a channel it will reply with an ACK and no payload
  uint8_t msg[] = { CRTP_HEADER(CRTP_PORT_LINK, 3) };
  for (channel = 0; channel < 125; channel++)
  {
    nrf24.setChannel(channel);
   if (!nrf24.send(msg, sizeof(msg)))
      Serial.println("scan send failed");
    if (nrf24.waitPacketSent())
      return true; // Got an ACK, expect no payload. This channel works, use it
  }
  return false;
}

// Send a message to the copter and detect failures
void send(uint8_t* data, uint8_t len)
{
  if (!nrf24.send(data, len))
    Serial.println("send failed");
    
  if (nrf24.waitPacketSent())
  {
    // Analyze payload?
  }
  else
  {
    failed_packet_count++;
  } 
}

void loop()
{
  // Poll until we have a positive response from a Crazyflie
  // and then use its channel
  while (!scan())
    ;
    
  // Now run a session with the one we found...
  failed_packet_count = 0;
  Serial.print("found a crazyflie on channel: ");
  Serial.println(channel);
  
  // ...until we get too many comms failures
  while (failed_packet_count < 50)
  {
      // Send a commander message
      uint8_t buf[32];
      buf[0] = CRTP_HEADER(CRTP_PORT_COMMANDER, 0);
      CommanderCrtpValues* msg = (CommanderCrtpValues*)(buf+1);
      // Mode 2 stick layout, map standard stick values to suitable Crazyflie commander ranges
      msg->roll   = map(rc.getChannel(1), 0, 1023, 30.0, -30.0);
      msg->pitch  = map(rc.getChannel(2), 0, 1023, 30.0, -30.0);
      msg->yaw    = map(rc.getChannel(3), 0, 1023, 200.0, -200.0);
      msg->thrust = map(rc.getChannel(0), 0, 1023, 0, 65000);
      send(buf, sizeof(CommanderCrtpValues) + 1);
      delay(20); // 50 per sec
  } 
  Serial.println("connection failed");
  
  // Too many failed messages, fall out here and rescan for a new Crazyflie
}


