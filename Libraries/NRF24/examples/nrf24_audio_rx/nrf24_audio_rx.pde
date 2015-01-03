// nrf24_audio_rx.pde
// -*- mode: C++ -*-
// Example sketch showing how to create an audio digital receiver
// with the NRF24 class. 
// Works with the nrf24_audio_tx sample transmitter
// Connect audio output to pin 6, through a low pass filter consisting of a 1k resister and series followed by a 
// 0.0033 microfarad capacitor to ground (48kHz filter).
// The audio quality is poor: dont expect hi-fi!
// We have to change the PWM frequency to 62 kHz so we can get bandwidth reasonable 
// audio out through the low pass filter
// Tested on UNO

#include <NRF24.h>
#include <SPI.h>


// Singleton instance of the radio
NRF24 nrf24;
// NRF24 nrf24(8, 7); // use this to be electrically compatible with Mirf
// NRF24 nrf24(8, 10);// For Leonardo, need explicit SS pin

/**
 * Divides a given PWM pin frequency by a divisor.
 *
 * The resulting frequency is equal to the base frequency divided by
 * the given divisor:
 *   - Base frequencies:
 *      o The base frequency for pins 3, 9, 10, and 11 is 31250 Hz.
 *      o The base frequency for pins 5 and 6 is 62500 Hz.
 *   - Divisors:
 *      o The divisors available on pins 5, 6, 9 and 10 are: 1, 8, 64,
 *        256, and 1024.
 *      o The divisors available on pins 3 and 11 are: 1, 8, 32, 64,
 *        128, 256, and 1024.
 *
 * PWM frequencies are tied together in pairs of pins. If one in a
 * pair is changed, the other is also changed to match:
 *   - Pins 5 and 6 are paired on timer0
 *   - Pins 9 and 10 are paired on timer1
 *   - Pins 3 and 11 are paired on timer2
 *
 * Note that this function will have side effects on anything else
 * that uses timers:
 *   - Changes on pins 3, 5, 6, or 11 may cause the delay() and
 *     millis() functions to stop working. Other timing-related
 *     functions may also be affected.
 *   - Changes on pins 9 or 10 will cause the Servo library to function
 *     incorrectly.
 *
 * Thanks to macegr of the Arduino forums for his documentation of the
 * PWM frequency divisors. His post can be viewed at:
 *   http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1235060559/0#4
 */
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

void setup() 
{
  Serial.begin(9600);
  while (!Serial) 
    ; // wait for serial port to connect. Needed for Leonardo only
  if (!nrf24.init())
    Serial.println("NRF24 init failed");
  if (!nrf24.setChannel(1))
    Serial.println("setChannel failed");
  if (!nrf24.setThisAddress((uint8_t*)"aurx1", 5))
    Serial.println("setThisAddress failed");

  if (!nrf24.setPayloadSize(32))
    Serial.println("setPayloadSize failed");
  if (!nrf24.setRF(NRF24::NRF24DataRate2Mbps, NRF24::NRF24TransmitPower0dBm))
    Serial.println("setRF failed");    
  if (!nrf24.powerUpRx())
      Serial.println("powerOnRx failed");    
  // Here we change the PWM frequency so we can render audio with better quality
  setPwmFrequency(6, 1); // PWM output on pin 6 is 62 khz
  Serial.println("initialised");
}

unsigned long count = 0;
void loop()
{
  uint8_t buf[32];
  uint8_t len = sizeof(buf);

  nrf24.waitAvailable();
  if (nrf24.recv(buf, &len)) // 140 microsecs
  {
    uint8_t i;
    for (i = 0; i < 32; i++)
    {
      analogWrite(6, buf[i]); // 15 microsecs
      // This delay was established experimentally to make sure the
      // buffer was exhausted just in time for the next packet to arrive     
      delayMicroseconds(65);
    }
  }
}

