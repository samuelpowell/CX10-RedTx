#include <RcTrainer.h>
#include <NRF24.h>
#include <SPI.h>

// Function prototypes
void send_packet( bool );
void write_payload(uint8_t *data, uint8_t len, bool noack);
void set_cmmd_addr( void );
void set_bind_addr( void );

// Radio and register defines
#define RF_CHANNEL      0x3C  // Stock TX fixed frequency
#define ACTIVATE        0x50  // Activate command
#define PAYLOADSIZE     9

#define ACTIVATE_DATA       0x73
#define ACTIVATE_CMD        0x50

#define NRF24_ENAA_PA (NRF24_ENAA_P0 | NRF24_ENAA_P1 | NRF24_ENAA_P2 | NRF24_ENAA_P3 | NRF24_ENAA_P4 | NRF24_ENAA_P5)
#define NRF24_ERX_PA (NRF24_ERX_P0 | NRF24_ERX_P1 | NRF24_ERX_P2 | NRF24_ERX_P3 | NRF24_ERX_P4 | NRF24_ERX_P5)
#define NRF_STATUS_CLEAR 0x70

// Channel scaling defines
#define CHAN_MAX_VALUE 1000
#define CHAN_MIN_VALUE -1000
#define chval(chin) (uint8_t) (((chin * 0xFF / CHAN_MAX_VALUE) + 0x100) >> 1);
 
// Packet state enumeration
enum packet_state_t {
    PKT_PENDING = 0,
    PKT_ACK,
    PKT_TIMEOUT,
    PKT_ERROR,
    PKT_ERROR_IN_RX
};

// Singleton instance of the radio and PPM receiver
NRF24 nrf24;
RcTrainer tx;

// Command and bind addresses (command address should be generated from random number)
uint8_t rx_tx_cmmd[5] = {0xC1, 0xC1, 0xC1, 0xC1, 0xC1};
uint8_t rx_tx_bind[5] = {0x65, 0x65, 0x65, 0x65, 0x65};

// Data packet buffer
uint8_t packet[PAYLOADSIZE];

// CX-10 scaled commands
uint8_t throttle, rudder, elevator, aileron, rudder_trim, elevator_trim, aileron_trim, flags;

// setup initalises nrf24, attempts to bind, then moves on
void setup() 
{
 
  
  // Initialise SPI bus and activate radio in RX mode
  nrf24.init();
  nrf24.setConfiguration( NRF24_EN_CRC );
  
  // Initialisation from Deviation
  nrf24.spiWriteRegister( NRF24_REG_00_CONFIG,     (NRF24_EN_CRC | NRF24_PWR_UP));  // Power up with CRC enabled
  nrf24.spiWriteRegister( NRF24_REG_01_EN_AA,      NRF24_ENAA_PA);                  // Auto ACK on all pipes
  nrf24.spiWriteRegister( NRF24_REG_02_EN_RXADDR,  NRF24_ERX_PA);                   // Enable all pipes
  nrf24.spiWriteRegister( NRF24_REG_03_SETUP_AW,   NRF24_AW_5_BYTES);               // 5-byte TX/RX address

  // Set RF power and data rate, then override REG_04/05 which it sets
  nrf24.setRF( nrf24.NRF24DataRate1Mbps, nrf24.NRF24TransmitPower0dBm);
  nrf24.spiWriteRegister( NRF24_REG_04_SETUP_RETR, 0x1A);                           // 500uS timeout, 10 retries
  nrf24.spiWriteRegister( NRF24_REG_05_RF_CH, RF_CHANNEL);                          // Channel 0x3C
  nrf24.spiWriteRegister( NRF24_REG_07_STATUS, NRF_STATUS_CLEAR);                   // Clear status
    
  nrf24.spiWriteRegister( NRF24_REG_11_RX_PW_P0, PAYLOADSIZE);                      // Set payload size on all RX pipes
  nrf24.spiWriteRegister( NRF24_REG_12_RX_PW_P1, PAYLOADSIZE);
  nrf24.spiWriteRegister( NRF24_REG_13_RX_PW_P2, PAYLOADSIZE);
  nrf24.spiWriteRegister( NRF24_REG_14_RX_PW_P3, PAYLOADSIZE);
  nrf24.spiWriteRegister( NRF24_REG_15_RX_PW_P4, PAYLOADSIZE);
  nrf24.spiWriteRegister( NRF24_REG_16_RX_PW_P5, PAYLOADSIZE);
 
  nrf24.spiWriteRegister( NRF24_REG_17_FIFO_STATUS, 0x00);                          // Clear FIFO bits (unnesseary) 

  nrf24.spiWriteRegister( NRF24_REG_1C_DYNPD, 0x3F);                                // Enable dynamic payload (all pipes)
  nrf24.spiWriteRegister( NRF24_REG_1D_FEATURE, 0x07);                              // Payloads with ACK, noack command
  
  nrf24.spiWrite(ACTIVATE_CMD, ACTIVATE_DATA);                                      // Activate feature registers
  nrf24.spiWriteRegister( NRF24_REG_1C_DYNPD, 0x3F);                                // Enable dynamic payload (all pipes)
  nrf24.spiWriteRegister( NRF24_REG_1D_FEATURE, 0x07);                              // Payloads with ACK, noack command

  // Set command address
  set_cmmd_addr();
  
  // Power up
  nrf24.setConfiguration( NRF24_EN_CRC | NRF24_PWR_UP );
  
  nrf24.flushTx();
  nrf24.flushRx();
  nrf24.spiWriteRegister(NRF24_REG_07_STATUS, NRF_STATUS_CLEAR);
  
  // White for aux1 high before binding
  while(tx.getChannel(4, 1000, 2000, 0x00, 0xFF ) < 0x40);
  
  
  set_bind_addr();

  for(int packno = 0; packno < 60; packno++)
  {
    send_packet(true);
    
    switch(packwait()) 
    {
     case PKT_ERROR:
       while(1);
       break;
     
     case PKT_ACK: 
       break;
     
     case PKT_TIMEOUT:
       break;
    }
  }
    
  set_cmmd_addr();
  
  
}


// loop repeatedly sends data read by PPM to the device, every 8ms
void loop()
{
  uint8_t aux1 = 0;
  
  // Get RX values by PPM, convert to range 0x00 to 0xFF
  throttle = (uint8_t) (tx.getChannel(0, 1000, 2000, 0x00, 0xFF ));
  aileron  = (uint8_t) (tx.getChannel(1, 1000, 2000, 0x00, 0xFF ));
  elevator = (uint8_t) (tx.getChannel(2, 1000, 2000, 0x00, 0xFF ));
  rudder   = (uint8_t) (tx.getChannel(3, 1000, 2000, 0x00, 0xFF ));
  aux1     = (uint8_t) (tx.getChannel(4, 1000, 2000, 0x00, 0xFF ));
  
  // Add command values to trim to get real full scale response 
  // in original CX-10 firmware (FN firmware ignores the trims, so
  // no problems).
  rudder_trim = rudder >> 1;
  aileron_trim = aileron >> 1;
  elevator_trim = elevator >> 1;
  
  // If the AUX1 is high, we set the flags, allowing flips in original
  // firmware, or arming (via elevator) in FN firmware
  if(aux1 > 0x80) {
    flags = 0x0F;      
  }
  else {
    flags = 0x00;
  }
   
  
  // Send a data packet and find out what happens
  send_packet(false);
  switch(packwait()) 
  {
   // Device not in TX mode, how did this happen?
   case PKT_ERROR_IN_RX:
     while(1);
     break;
     
    // Packet ACKed, move on
   case PKT_ACK: 
     break;
     
   // No ACK received, and we tried hard, so time out. 
   case PKT_TIMEOUT:
     break;
  }
  
  // Wait for 8ms, before sending next data
  delay(8);
  
}

// send_packet constructs a packet and dispatches to radio
void send_packet( bool bind )
{
    // bind: send first four bytes of command address in the packet
    //       the final byte is set automatically to 0xC1 by CX-10.
    if (bind) {
        packet[0]= rx_tx_cmmd[0];
        packet[1]= rx_tx_cmmd[1];
        packet[2]= rx_tx_cmmd[2];
        packet[3]= rx_tx_cmmd[3];
        packet[4] = 0x56;
        packet[5] = 0xAA;
        packet[6] = 0x32;
        packet[7] = 0x00;
    }
    // cmnd: send RX commands present in the global variables 
    else {
        
        packet[0] = throttle;
        packet[1] = rudder;
        packet[3] = elevator;
        packet[4] = aileron;
        packet[2] = rudder_trim;
        packet[5] = elevator_trim;
        packet[6] = aileron_trim;
        packet[7] = flags;
    }

    // clear packet status bits and TX FIFO
    nrf24.spiWriteRegister( NRF24_REG_07_STATUS, NRF_STATUS_CLEAR );
    nrf24.flushTx();
    
    // Form checksum (my modified CX-10 firmware doesnt care)
    packet[8] = packet[0];  
    for(uint8_t i=1; i < 8; i++) packet[8] += packet[i];
    packet[8] = ~packet[8];

    // Transmit, requesting acknowledgement
    write_payload(packet, 9, false);

}

// write_payload issues the send command to the nrf24 library
void write_payload(uint8_t *data, uint8_t len, bool noack)
{
  nrf24.spiBurstWrite(noack ? NRF24_COMMAND_W_TX_PAYLOAD_NOACK : NRF24_COMMAND_W_TX_PAYLOAD, data, len);
}

// packwait polls the nrf24 to determine what's happened to our data
int packwait()
{
    // If we are currently in receive mode, then there is no packet to wait for
    if (nrf24.spiReadRegister(NRF24_REG_00_CONFIG) & NRF24_PRIM_RX)
	return PKT_ERROR_IN_RX;

    // Wait for either the Data Sent or Max ReTries flag, signalling the 
    // end of transmission
    uint8_t status;
    
    while (!((status = nrf24.statusRead()) & (NRF24_TX_DS | NRF24_MAX_RT)));
    
    nrf24.spiWriteRegister(NRF24_REG_07_STATUS, NRF24_TX_DS | NRF24_MAX_RT);
    
    switch(status &  (NRF24_TX_DS | NRF24_MAX_RT)) {
    
    case NRF24_TX_DS:
      return PKT_ACK;  
      break;
    
    case NRF24_MAX_RT:
      nrf24.flushTx();
      return PKT_TIMEOUT;
      break;
    }
    

   return PKT_ERROR;
}

 
void set_cmmd_addr( void )
{
  nrf24.spiBurstWriteRegister( NRF24_REG_0A_RX_ADDR_P0,  rx_tx_cmmd, 5); 
  nrf24.spiBurstWriteRegister( NRF24_REG_10_TX_ADDR, rx_tx_cmmd, 5);                // Set command address  
  
}

void set_bind_addr( void )
{
  nrf24.spiBurstWriteRegister( NRF24_REG_0A_RX_ADDR_P0,  rx_tx_bind, 5);
  nrf24.spiBurstWriteRegister( NRF24_REG_10_TX_ADDR, rx_tx_bind, 5);                // Set command address  
}

