// NRF24.cpp
//
// Copyright (C) 2012 Mike McCauley
// $Id: NRF24.cpp,v 1.2 2014/05/20 06:00:55 mikem Exp mikem $

#include <NRF24.h>
#include <SPI.h>

NRF24::NRF24(uint8_t chipEnablePin, uint8_t chipSelectPin)
{
    _configuration = NRF24_EN_CRC; // Default: 1 byte CRC enabled
    _chipEnablePin = chipEnablePin;
    _chipSelectPin = chipSelectPin;
}

boolean NRF24::init()
{
    // Initialise the slave select pin
    pinMode(_chipEnablePin, OUTPUT);
    digitalWrite(_chipEnablePin, LOW);
    pinMode(_chipSelectPin, OUTPUT);
    digitalWrite(_chipSelectPin, HIGH);
  
    // Added code to initilize the SPI interface and wait 100 ms
    // to allow NRF24 device to "settle".  100 ms may be overkill.
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);
    // Wait for NRF24 POR (up to 100msec)
    delay(100);

    // start the SPI library:
    // Note the NRF24 wants mode 0, MSB first and default to 1 Mbps
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
//    SPI.setClockDivider(SPI_2XCLOCK_MASK); // 1 MHz SPI clock
    SPI.setClockDivider(SPI_CLOCK_DIV2); // 8MHz SPI clock

    // Clear interrupts
    if (!spiWriteRegister(NRF24_REG_07_STATUS, NRF24_RX_DR | NRF24_TX_DS | NRF24_MAX_RT))
	return false; // Could not write to device. Not connected?

    // Make sure we are powered down
    powerDown();

    // Flush FIFOs
    flushTx();
    flushRx();

    return powerUpRx();
}

// Low level commands for interfacing with the device
uint8_t NRF24::spiCommand(uint8_t command)
{
    digitalWrite(_chipSelectPin, LOW);
    uint8_t status = SPI.transfer(command);
    digitalWrite(_chipSelectPin, HIGH);
    return status;
}

// Read and write commands
uint8_t NRF24::spiRead(uint8_t command)
{
    digitalWrite(_chipSelectPin, LOW);
    SPI.transfer(command); // Send the address, discard status
    uint8_t val = SPI.transfer(0); // The MOSI value is ignored, value is read
    digitalWrite(_chipSelectPin, HIGH);
    return val;
}

uint8_t NRF24::spiWrite(uint8_t command, uint8_t val)
{
    digitalWrite(_chipSelectPin, LOW);
    uint8_t status = SPI.transfer(command);
    SPI.transfer(val); // New register value follows
    digitalWrite(_chipSelectPin, HIGH);
    return status;
}

void NRF24::spiBurstRead(uint8_t command, uint8_t* dest, uint8_t len)
{
    digitalWrite(_chipSelectPin, LOW);
    SPI.transfer(command); // Send the start address, discard status
    while (len--)
	*dest++ = SPI.transfer(0); // The MOSI value is ignored, value is read
    digitalWrite(_chipSelectPin, HIGH);
    // 300 microsecs for 32 octet payload
}

uint8_t NRF24::spiBurstWrite(uint8_t command, uint8_t* src, uint8_t len)
{
    digitalWrite(_chipSelectPin, LOW);
    uint8_t status = SPI.transfer(command);
    while (len--)
	SPI.transfer(*src++);
    digitalWrite(_chipSelectPin, HIGH);
    return status;
}

// Use the register commands to read and write the registers
uint8_t NRF24::spiReadRegister(uint8_t reg)
{
    return spiRead((reg & NRF24_REGISTER_MASK) | NRF24_COMMAND_R_REGISTER);
}

uint8_t NRF24::spiWriteRegister(uint8_t reg, uint8_t val)
{
    return spiWrite((reg & NRF24_REGISTER_MASK) | NRF24_COMMAND_W_REGISTER, val);
}

void NRF24::spiBurstReadRegister(uint8_t reg, uint8_t* dest, uint8_t len)
{
    return spiBurstRead((reg & NRF24_REGISTER_MASK) | NRF24_COMMAND_R_REGISTER, dest, len);
}

uint8_t NRF24::spiBurstWriteRegister(uint8_t reg, uint8_t* src, uint8_t len)
{
    return spiBurstWrite((reg & NRF24_REGISTER_MASK) | NRF24_COMMAND_W_REGISTER, src, len);
}

uint8_t NRF24::statusRead()
{
    return spiReadRegister(NRF24_REG_07_STATUS);
//    return spiCommand(NRF24_COMMAND_NOP); // Side effect is to read status
}

uint8_t NRF24::flushTx()
{
    return spiCommand(NRF24_COMMAND_FLUSH_TX);
}

uint8_t NRF24::flushRx()
{
    return spiCommand(NRF24_COMMAND_FLUSH_RX);
}

boolean NRF24::setChannel(uint8_t channel)
{
    spiWriteRegister(NRF24_REG_05_RF_CH, channel & NRF24_RF_CH);
    return true;
}
boolean NRF24::setConfiguration(uint8_t configuration)
{
    _configuration = configuration;
}

boolean NRF24::setPipeAddress(uint8_t pipe, uint8_t* address, uint8_t len)
{
    spiBurstWriteRegister(NRF24_REG_0A_RX_ADDR_P0 + pipe, address, len);
    return true;
}

boolean NRF24::setRetry(uint8_t delay, uint8_t count)
{
    spiWriteRegister(NRF24_REG_04_SETUP_RETR, ((delay << 4) & NRF24_ARD) | (count & NRF24_ARC));
    return true;
}

boolean NRF24::setThisAddress(uint8_t* address, uint8_t len)
{
    // Set pipe 1 for this address
    setPipeAddress(1, address, len); 
    // RX_ADDR_P2 is set to RX_ADDR_P1 with the LSbyte set to 0xff, for use as a broadcast address
    return true;
}

boolean NRF24::setTransmitAddress(uint8_t* address, uint8_t len)
{
    // Set both TX_ADDR and RX_ADDR_P0 for auto-ack with Enhanced shockwave
    spiBurstWriteRegister(NRF24_REG_0A_RX_ADDR_P0, address, len);
    spiBurstWriteRegister(NRF24_REG_10_TX_ADDR, address, len);
    return true;
}

boolean NRF24::setPayloadSize(uint8_t size)
{
    spiWriteRegister(NRF24_REG_11_RX_PW_P0, size);
    spiWriteRegister(NRF24_REG_12_RX_PW_P1, size);
    return true;
}

boolean NRF24::setRF(uint8_t data_rate, uint8_t power)
{    
    uint8_t value = (power << 1) & NRF24_PWR;
    // Ugly mapping of data rates to noncontiguous 2 bits:
    if (data_rate == NRF24DataRate250kbps)
	value |= NRF24_RF_DR_LOW;
    else if (data_rate == NRF24DataRate2Mbps)
	value |= NRF24_RF_DR_HIGH;
    // else NRF24DataRate1Mbps, 00
    spiWriteRegister(NRF24_REG_06_RF_SETUP, value);

    if (data_rate == NRF24DataRate250kbps)
	spiWriteRegister(NRF24_REG_04_SETUP_RETR, 0x43); // 1250usecs, 3 retries
    else
	spiWriteRegister(NRF24_REG_04_SETUP_RETR, 0x03); // 250us, 3 retries
	
    return true;
}

boolean NRF24::powerDown()
{
    spiWriteRegister(NRF24_REG_00_CONFIG, _configuration);
    digitalWrite(_chipEnablePin, LOW);
    return true;
}

boolean NRF24::powerUpRx()
{
    boolean status = spiWriteRegister(NRF24_REG_00_CONFIG, _configuration | NRF24_PWR_UP | NRF24_PRIM_RX);
    digitalWrite(_chipEnablePin, HIGH);
    return status;
}

boolean NRF24::powerUpTx()
{
    // Its the pulse high that puts us into TX mode
    digitalWrite(_chipEnablePin, LOW);
    boolean status = spiWriteRegister(NRF24_REG_00_CONFIG, _configuration | NRF24_PWR_UP);
    digitalWrite(_chipEnablePin, HIGH);
    return status;
}

boolean NRF24::send(uint8_t* data, uint8_t len, boolean noack)
{
    powerUpTx();
    spiBurstWrite(noack ? NRF24_COMMAND_W_TX_PAYLOAD_NOACK : NRF24_COMMAND_W_TX_PAYLOAD, data, len);
    // Radio will return to Standby II mode after transmission is complete
    return true;
}

boolean NRF24::waitPacketSent()
{
    // If we are currently in receive mode, then there is no packet to wait for
    if (spiReadRegister(NRF24_REG_00_CONFIG) & NRF24_PRIM_RX)
	return false;

    // Wait for either the Data Sent or Max ReTries flag, signalling the 
    // end of transmission
    uint8_t status;
    while (!((status = statusRead()) & (NRF24_TX_DS | NRF24_MAX_RT)))
	;

    // Must clear NRF24_MAX_RT if it is set, else no further comm
    spiWriteRegister(NRF24_REG_07_STATUS, NRF24_TX_DS | NRF24_MAX_RT);
    if (status & NRF24_MAX_RT)
	flushTx();
    // Return true if data sent, false if MAX_RT
    return status & NRF24_TX_DS;
}

boolean NRF24::isSending()
{
    return !(spiReadRegister(NRF24_REG_00_CONFIG) & NRF24_PRIM_RX) && !(statusRead() & (NRF24_TX_DS | NRF24_MAX_RT));
}

boolean NRF24::printRegisters()
{
    uint8_t registers[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0d, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x1c, 0x1d};

    uint8_t i;
    for (i = 0; i < sizeof(registers); i++)
    {
	Serial.print(i, HEX);
	Serial.print(": ");
	Serial.println(spiReadRegister(i), HEX);
    }
    return true;
}

boolean NRF24::available()
{
    if (spiReadRegister(NRF24_REG_17_FIFO_STATUS) & NRF24_RX_EMPTY)
	return false;
    // Manual says that messages > 32 octets should be discarded
    if (spiRead(NRF24_COMMAND_R_RX_PL_WID) > 32)
    {
	flushRx();
	return false;
    }
    return true;
}

void NRF24::waitAvailable()
{
    powerUpRx();
    while (!available())
	;
}

// Blocks until a valid message is received or timeout expires
// Return true if there is a message available
// Works correctly even on millis() rollover
bool NRF24::waitAvailableTimeout(uint16_t timeout)
{
    powerUpRx();
    unsigned long starttime = millis();
    while ((millis() - starttime) < timeout)
        if (available())
           return true;
    return false;
}

boolean NRF24::recv(uint8_t* buf, uint8_t* len)
{
    // Clear read interrupt
    spiWriteRegister(NRF24_REG_07_STATUS, NRF24_RX_DR);

    // 0 microsecs @ 8MHz SPI clock
    if (!available())
	return false;
    // 32 microsecs (if immediately available)
    *len = spiRead(NRF24_COMMAND_R_RX_PL_WID);
    // 44 microsecs
    spiBurstRead(NRF24_COMMAND_R_RX_PAYLOAD, buf, *len);
    // 140 microsecs (32 octet payload)

    return true;
}
