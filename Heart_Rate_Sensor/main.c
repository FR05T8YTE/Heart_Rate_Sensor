//*********************************************************************
// By: Jethro Sinclair
// Date: March 2021
//
// Connects to a MAX30102 and using red light data calculates heart 
// rate in beats per minute. The heart rate is displayed on a 4-digit 
// 7-segment display. Uses a I2C library deveploped by Colby DeLisle.
// MAX30102 library insipered by SparkFun Electronics arduino library 
// for the MAX30105. https://github.com/sparkfun/SparkFun_MAX3010x_
// Sensor_Library/archive/master.zip
//*********************************************************************

#include "I2C_master.h"
#include "MAX30102.h"

// display num uses pins P2.0 to P2.5
// P2.0 to P2.3 = D0 to D3
// P2.4 to P2.5 = A0 to A1
void display(int num) {
  for (int i = 0; i < 4; i++) {
    P2OUT = (P2OUT & 0xc0) | i << 4;      // set place and zero digit
    P2OUT |= num % 10;                    // set digit
    num /= 10;
  }
}

// Check for slave presence
uint8_t check_slave() {
  uint8_t err = 0;
  UCB0CTL1 |= UCTR + UCTXSTT + UCTXSTP;   // I2C TX, START/STOP conditions
  while (UCB0CTL1 & UCTXSTP);             // Wait for STOP condition to be sent
  if (_no_ack()) {                        // If slave NACK'd,
    err = 1;                              // set error
  }
  return err;
}

void main(void) {
  WDTCTL = WDTPW + WDTHOLD;               // Stop WDT
  P2DIR |= 0x3F;                          // Set display pins
  _USCI_I2C_init();          
  
  uint8_t err = 0;               
  err |= check_slave();                   // Confirm MAX30102 is connected
  err |= setup();                         // Configure settings for MAX30102
  
  while(1) {
    err |= get_data();                    // Get data from the MAX30102
    display(get_BPM_avg());               // Display heart rate
    __delay_cycles(4000);                 // Delay so the I2C is not being spammed
    if (err) {
      display(3053);
      while(1);
    }
  }
}


