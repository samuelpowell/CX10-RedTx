// crazyflie.pde
// -*- mode: C++ -*-
//
// This sketch act like a Crazyflie quadcopter http://www.bitcraze.se/
// using the CRTP radiolink protocol:
// http://wiki.bitcraze.se/projects:crazyflie:firmware:comm_protocol
//
// Requires 
// - NRF24 radio module such as the sparkfun WRL-00691 http://www.sparkfun.com/products/691
// - Arduino such as Uno
// - A Crazyflie transmitter, such as the Carzyflie PC client+CrazyRadio module
// or
// the NRF24 crazyflie client part of the NRF24 library
//
// Uses NRF24 library to comunicate with the Crazyflie, 
// http://www.airspayce.com/mikem/arduino/NRF24
//
// Receives and decodes varion message types from teh Crazyflie transmitter, although
// only the link echo and commander messages are fully implemented
//
// Author: Mike McCauley
// Copyright (C) 2012 Mike McCauley

#include <NRF24.h>
#include <SPI.h>

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

// The address to use for this Crazyflie
uint8_t address[] = { 0xe7, 0xe7, 0xe7, 0xe7, 0xe7 };

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
  if (!nrf24.setPipeAddress(0, address, sizeof(address)))
     Serial.println("setPipeAddress failed");  
  // Be compatible with Crazyflie: No interrupts, 2 bytes CRC
  nrf24.setConfiguration(NRF24_MASK_RX_DR | NRF24_MASK_TX_DS | NRF24_MASK_MAX_RT | NRF24_EN_CRC | NRF24_CRCO);
  nrf24.spiWriteRegister(NRF24_REG_1D_FEATURE, NRF24_EN_DPL | NRF24_EN_ACK_PAY);   // Dynamic size payload + ack
  nrf24.spiWriteRegister(NRF24_REG_1C_DYNPD, NRF24_DPL_P0);     // Dynamic payload on pipe 0
  if (!nrf24.setRetry(6, 3)) // 1500us and 3 retries
     Serial.println("setRetry failed");  

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

// Send an ACK with a payload on pipe 0
void sendAckPayload(uint8_t* data, uint8_t len)
{
  nrf24.spiBurstWrite(NRF24_COMMAND_W_ACK_PAYLOAD(0), data, len);
  nrf24.waitPacketSent();
}

// ACK with no payload on pipe 0
void sendAck()
{
  sendAckPayload(0, 0);
}

void loop()
{
  uint8_t buf[100];
  uint8_t buflen = sizeof(buf);
  static uint32_t last_second = 0;
  
  // enable receiver again, after transmit and wait for a message
  nrf24.waitAvailable();
  if (nrf24.recv(buf, &buflen))
  {
    // Decode incoming messages from client based on port number in the header byte
 //   dump("msg", buf, buflen);
    if (CRTP_HEADER_PORT(buf[0]) == CRTP_PORT_LINK) // Link Echo
    {
      sendAck(); // Just ack, no payload
    }
    else if (CRTP_HEADER_PORT(buf[0]) == CRTP_PORT_COMMANDER) // Commander
    {
      // Commander message to set control positions
      CommanderCrtpValues *p = (CommanderCrtpValues*)(buf+1);
      Serial.println(p->roll); // -30.0 to 30.0
      Serial.println(p->pitch); // -28.0 to 32.0
      Serial.println(p->yaw); // -200.0 to 200.0
      Serial.println(p->thrust); // 0 to 45755
      sendAck(); // Just ack, no payload
//      dump("commander", buf, buflen);
    }
    else if (CRTP_HEADER_PORT(buf[0]) == CRTP_PORT_PARAM) // Parameter
    {
      if (buf[1] == CMD_GET_INFO) // Param GET_INFO
      {
        // pc client only fetches item data if the CRC changes
        // If you change the contents of your TOC, you must change the CRC
        // Crazyflie PC client doesnt relaly believe it if you say there are no TOC entries
        uint8_t reply[] = { CRTP_HEADER(CRTP_PORT_PARAM, PARAM_TOC_CH), CMD_GET_INFO, 1, 0, 0, 2, 0}; // 1 params in toc
        sendAckPayload(reply, sizeof(reply));
      }
      else if (buf[1] == CMD_GET_ITEM) // Param GET_ITEM
      { 
        // Set up a fax param as item 0
        uint8_t reply[] = { CRTP_HEADER(CRTP_PORT_PARAM, PARAM_TOC_CH), CMD_GET_ITEM, 0, 1, 'x', 0, 'y', 0}; // bogus item 0 uint8_t param
        sendAckPayload(reply, sizeof(reply));
      }
      else
      {
        Serial.print("unknown param type ");
        Serial.println(buf[1]);
      }
 //     dump("param", buf, buflen);
    }
    else if (buf[0] == CRTP_HEADER_PORT(buf[0]) == CRTP_PORT_PARAM) // Log
    {
      // http://wiki.bitcraze.se/projects:crazyflie:crtp:log
      if (buf[1] == CMD_GET_INFO) // Log GET_INFO
      {
        // pc client only fetches item data if the CRC changes
        // If you change the contents of your TOC, you must change the CRC
        // Crazyflie PC client doesnt relaly believe it if you say there are no TOC entries
       uint8_t reply[] = { CRTP_HEADER(CRTP_PORT_LOG, LOG_TOC_CH), CMD_GET_INFO, 1, 0, 0, 2, 1, 8, 64}; // 1 log item in toc
        sendAckPayload(reply, sizeof(reply));
      }   
      else if (buf[1] == CMD_GET_ITEM) // Log GET_ITEM
      {
        // Set up a fake battery voltage as item 0
        uint8_t reply[] = { CRTP_HEADER(CRTP_PORT_LOG, LOG_TOC_CH), CMD_GET_ITEM, 0, 7, 'p', 'm', 0, 'v', 'b', 'a', 't', 0}; // item 0 float pm.vbat
        sendAckPayload(reply, sizeof(reply));
      }
//      dump("log", buf, buflen);
    }
    else
    {
      // Sometimes get bogus type 0x93
 //     dump("unknown", buf, buflen);
    }
  }
  
  // Do once per second tasks
  uint32_t this_second = millis() / 1000;
  if (this_second != last_second)
  {
    // This is how you would send value data for a log block
    // with current real-time data
    // if a log block has ben set up by the client
//    uint8_t msg[] = {CRTP_HEADER(CRTP_PORT_LOG, LOG_LOG_CH), 0, 0, 0, 0, 11, 11, 11, 11};
//    sendAckPayload(msg, sizeof(msg));

    last_second = this_second;
  }
}

