//******************************************************************************
// Note: External pull-ups are needed for SDA & SCL! 
// For the MAX30102, pullups should be ~1kOhm.
// Tested with MAX30102 VIN @ 3.3V.
//
// Also: Remove P1.6 jumper to use SCL!
//
// Much of this was taken from the TI USCI I2C Master library,
// which is located here: http://www.ti.com/lit/zip/slaa382,
// and the SimplyEmbedded tutorial: 
// http://www.simplyembedded.org/tutorials/msp430-i2c-basics/
//
// March 2021
// Colby DeLisle, UBC
//
// Minor modification by Jethro Sinclair, March 2021
//******************************************************************************

#include "msp430g2553.h"        // Device-specific header
#include "stdint.h"             // Contains def'ns of fixed-width ints

#define SDA_PIN BIT7            // MSP430G2553 SDA is P1.7
#define SCL_PIN BIT6            // MSP430G2553 SCL is P1.6

#define MAX30102_SLAVE_ADDR 0x57
#define SLAVE_ADDR MAX30102_SLAVE_ADDR

/*
  Slave address for MAX30102 is 0x57, see Table 17 of datasheet.
  Note that this doesn't include the R/W bit. So 0x57 represents
  only the leading 7 bits of the address.
*/

void _USCI_I2C_init();
unsigned int _no_ack();
unsigned int _transmit(const uint8_t*, unsigned int); 
unsigned int _receive(uint8_t*, unsigned int);
