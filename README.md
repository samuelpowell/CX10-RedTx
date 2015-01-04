cx10_redtx
==========

Cheerson CX10 (red) Arduino PPM TX

1. About
 
 Arduino implementation of the Cheerson CX10 (red) protocol, with PPM input. Can be used to control CX10 flashed with CX10_fnrf firmware, should also work with stock firmware. Tested with Arduino Uno and Turnigy 9XR.
 
 NOTE: In principle, the CX10 protocol sends ACKnowledgements to bind and data packets. I am unable to receive these using an nRF24 module with microstrip antenna, and receive them only intermittently using an nRF24 module with PA/LNA. This suggests that there may be problems with the CX10 RF design, or that there is interference on the fixed RF channel. This implementation does not check for ACKnowledgements, and will thus bind and fly irrespective of confirmed packet transmission. Contact me if you have any ideas....
 
2. Setup
 
 Add NRF24 and RcTrainer libraries to your Arduino environment (redistributed in ./Libraries directory), connect NRF24 module to SPI bus according to http://www.airspayce.com/mikem/arduino/NRF24/, connect PPM trainer to input capture port according to http://www.airspayce.com/mikem/arduino/RcTrainer/.
 
3. Operation
 
 + Configure TX to output PPM in TAER format, with AUX1 on channel 5.
 + Turn on CX10.
 + Toggle AUX1 (channel 5) high to bind.
 + Hold AUX1 high and apply full control sticks to allow arming and disarming on CX10_fnrf firmware, or to perform flips on original firmware.
 + Fly!
 
4. Credits
 
 + Mike McCauley (for NRF24 and RCTrainer Arduino library): http://www.airspayce.com
 + DeviationTX (for their reverse engineering of the YD717 SkyWalker protocol): http://www.deviationtx.com

