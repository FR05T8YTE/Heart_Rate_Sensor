#include "I2C_master.h"

void _USCI_I2C_init() {
  P1SEL  |= SDA_PIN + SCL_PIN;              // Assign P1.6, P1.7 to I2C duty
  P1SEL2 |= SDA_PIN + SCL_PIN;

  BCSCTL1 = CALBC1_1MHZ;                    // Calibrate SMCLK, 1MHz
  DCOCTL  = CALDCO_1MHZ;

  UCB0CTL1  = UCSWRST;                      // Enable SW reset
  UCB0CTL0  = UCMST + UCMODE_3 + UCSYNC;    // I2C Master, synchronous mode
  UCB0CTL1  = UCSSEL_2 + UCSWRST;           // Use SMCLK, keep SW reset
  UCB0BR0   = 10;                           // Set prescaler (can decrease this)
  UCB0BR1   = 0;
  UCB0I2CSA = SLAVE_ADDR;                   // Set slave address
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
}


unsigned int _no_ack() {

  int err = 0;

  // Check NACK flag for ACK 
  if (UCB0STAT & UCNACKIFG) {  // if UNACKIFG is set, master got NACK
    UCB0CTL1 |= UCTXSTP;       // Send STOP condition
    UCB0STAT &= ~UCNACKIFG;    // Clear NACK flag
    err = 1;                   // This means there was a NACK
  }

  return err;                  // 0: ACK received, 1: NACK received
}


unsigned int _transmit(const uint8_t *buf, unsigned int nbytes) {
  
  int err = 0;

  UCB0CTL1 |= UCTR | UCTXSTT;  // Send START condition

  // Wait for the start condition to be sent and until TX not busy
  while ((UCB0CTL1 & UCTXSTT) && ((IFG2 & UCB0TXIFG) == 0));

  err = _no_ack();

  // If no error and nonzero bytes to send, transmit the data
  while ((err == 0) && (nbytes > 0)) {
    UCB0TXBUF = *buf;  // add data to TXBUF
    while ((IFG2 & UCB0TXIFG) == 0) {
      err = _no_ack();
      if (err) {
        break;
      }
    }

    buf++;
    nbytes--;
  }

  return err;
}

unsigned int _receive(uint8_t *buf, unsigned int nbytes){

  int err = 0;

  UCB0CTL1 &= ~UCTR;    // Clear write bit
  UCB0CTL1 |= UCTXSTT;  // Send START condition

  // Wait for the start condition to be sent 
  while (UCB0CTL1 & UCTXSTT);

  // If there is only one byte to receive, send STOP
  // as soon as START has been sent
  if (nbytes == 1) {
    UCB0CTL1 |= UCTXSTP;
  }

  err = _no_ack();  // Check for ACK

  /* If no error and bytes left to receive, receive the data */
  while ((err == 0) && (nbytes > 0)) {
      
    while ((IFG2 & UCB0RXIFG) == 0);

    *buf = UCB0RXBUF;  // Take data from RXBUF
    buf++;
    nbytes--;

    // If there is only one byte left to receive
    // send the stop condition
    if (nbytes == 1) {
      UCB0CTL1 |= UCTXSTP;
    }
  }

  return err;
}
