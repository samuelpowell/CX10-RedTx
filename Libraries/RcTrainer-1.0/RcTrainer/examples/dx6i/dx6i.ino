// dx61.ino
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2013 Mike McCauley
//
// Print out servo positions from a Spektrum DX6i in trainer mode

#include <RcTrainer.h>

// Set up to listen on interrupt 0 which is digital input pin D2
// See http://arduino.cc/en/Reference/attachInterrupt for mapping
// interrupt number to pin number
RcTrainer tx;

void setup()
{
    Serial.begin(115200);	
}

void loop()
{
    Serial.println("----------------------");
    // Default mapping is used, suitable for DX6i, which has 6 channels
    uint8_t i;
    for (i = 0; i < 6; i++)
	Serial.println(tx.getChannel(i));
    
    delay(1000);
}

