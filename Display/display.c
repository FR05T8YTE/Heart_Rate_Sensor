#include  <msp430.h>

// display num uses pins P2.0 to P2.5
void display(int num) {
  for (int i = 0; i < 4; i++) {
    P2OUT = (P2OUT & 0xc0) | i << 4;  // set place and zero digit
    P2OUT |= num % 10;                // set digit
    num /= 10;
  }
}

void main(void) {
  WDTCTL = WDTPW + WDTHOLD;           // Stop WDT
  P2DIR = 0x3F;                       // Set P1 output direction
  while (1) {                         // Loop forever
    for(int i = 0; i < 9999; i++) { 
      display(i);
      __delay_cycles(500000);
    }
  }
}