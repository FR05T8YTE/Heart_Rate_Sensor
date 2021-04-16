#include "MAX30102.h"

uint16_t data[DATA_SIZE];
uint8_t data_pointer = 0;

uint32_t DC_signal_avg = 0;
int16_t AC_signal_prev = 0;
int16_t AC_max = 0;
int16_t AC_min = 0;
int16_t AC_cycle_max = 0;
int16_t AC_cycle_min = 0;
uint8_t positive_edge = 0;
uint8_t negative_edge = 0;

uint16_t miliseconds = 0;
uint16_t rates[RATES_SIZE];
uint8_t rates_pointer = 0;

int16_t FIR_buf[32];
uint8_t FIR_pointer = 0;
static const uint16_t FIR_coeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};

uint8_t setup() {
  // Timer interrupt setup
  CCTL0 = CCIE;                 // CCR0 interrupt enabled
  CCR0 = 1000;				          // 1ms at 1MHz
  TACTL = TASSEL_2 + MC_1;      // SMCLK, upmode
  __enable_interrupt();
  
  uint8_t err = 0;
  
  // Soft reset
  uint8_t TX_buf0[2] = {0x09, 0x40};                
  err |= _transmit((const uint8_t *) TX_buf0, 2);
  
  // Poll for reset to clear
  uint8_t addr = 0x09;
  uint8_t RX_buf = 0x40;
  while (RX_buf == 0x40) {
    err |= _transmit((const uint8_t *) &addr, 1);
    err |= _receive((uint8_t *) &RX_buf, 1);
    __delay_cycles(1000);
  }
  
  // 8 sample averaging, FIFO rollover, no free FIFO interrupt
  uint8_t TX_buf1[2] = {0x08, 0x70};                 
  err |= _transmit((const uint8_t *) TX_buf1, 2);
  
  // Heart rate mode
  uint8_t TX_buf2[2] = {0x09, 0x02};                
  err |= _transmit((const uint8_t *) TX_buf2, 2);
  
  // ADC LSB 15.63pA, 800 samples/s, 118us LED pulse, 16 bit ADC resolution
  uint8_t TX_buf3[2] = {0x0A, 0x31};          
  err |= _transmit((const uint8_t *) TX_buf3, 2);
  
  // Red LED power level 6.2mA
  uint8_t TX_buf4[2] = {0x0C, 0x1F};                  
  err |= _transmit((const uint8_t *) TX_buf4, 2);
  
  // Enable slot 1 (red led)
  uint8_t TX_buf5[2] = {0x11, 0x01};                 
  err |= _transmit((const uint8_t *) TX_buf5, 2);
}

// Calc DC_signal_avg
void calc_DC_avg() {
  uint32_t sum = 0;
  for (uint16_t i = 0; i < DATA_SIZE; i++) {
    sum += data[i];
  }
  DC_signal_avg = sum / DATA_SIZE;
}

// Calc and return BPM avg
uint16_t get_BPM_avg() {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < RATES_SIZE; i++) {
    sum += rates[i];
  }
  return (sum / RATES_SIZE);
}


// Low Pass FIR Filter
int16_t lowpass_FIR_filter(int16_t signal) {  
  FIR_buf[FIR_pointer] = signal;
  int32_t z = (int32_t)FIR_coeffs[11] * (int32_t)FIR_buf[(FIR_pointer - 11) & 0x1F];
  
  for (uint8_t i = 0 ; i < 11 ; i++) {
    z += (int32_t)FIR_coeffs[i] * (int32_t)(FIR_buf[(FIR_pointer - i) & 0x1F] + FIR_buf[(FIR_pointer - 22 + i) & 0x1F]);
  }

  FIR_pointer = (FIR_pointer + 1) % 32;

  return (z >> 15);
}

void process_sample(uint16_t sample) {  
  int16_t AC_signal = lowpass_FIR_filter(sample - DC_signal_avg);
  
  // Rising edge
  if ((AC_signal_prev < 0) & (AC_signal >= 0)) {
    positive_edge = 1;
    negative_edge = 0;
    AC_min = AC_cycle_min;
    AC_cycle_min = 0;
    if ((AC_max - AC_min > 80) & (AC_max - AC_min < 1000)) {
      uint16_t BPM = 60000 / miliseconds;
      miliseconds = 0;
      if ((BPM > 20) & (BPM < 250)) {
        rates[rates_pointer] = BPM;
        rates_pointer = (rates_pointer + 1) % RATES_SIZE;
      }
    }
  }
  
  // Falling edge
  if ((AC_signal_prev > 0) & (AC_signal <= 0)) {
    positive_edge = 0;
    negative_edge = 1;
    AC_max = AC_cycle_max;
    AC_cycle_max = 0;
  }
  
  // Find max in positive cycle
  if (positive_edge & (AC_signal > AC_cycle_max)) {
    AC_cycle_max = AC_signal;
  }
  
  // Find min in positive cycle
  if (negative_edge & (AC_signal < AC_cycle_min)) {
    AC_cycle_min = AC_signal;
  }
  
  AC_signal_prev = AC_signal;
}

uint8_t get_data() {
  uint8_t err = 0;
  
  // Get read pointer
  uint8_t rd_ptr_addr = 0x06;              
  uint8_t rd_prt;                 
  err |= _transmit((const uint8_t *) &rd_ptr_addr, 1);
  err |= _receive((uint8_t *) &rd_prt, 1);
  
  // Get write pointer
  uint8_t wr_ptr_addr = 0x04;                  
  uint8_t wr_prt;                       
  err |= _transmit((const uint8_t *) &wr_ptr_addr, 1);
  err |= _receive((uint8_t *) &wr_prt, 1);

  if (rd_prt != wr_prt) {
    int num_samples = wr_prt - rd_prt;
    if (num_samples < 0) {
      num_samples += FIFO_SIZE;
    }
    uint8_t data_addr = 0x07;
    uint8_t RX_buf[3];
    uint16_t data_pointer_temp = data_pointer;
    for (uint16_t i = 0; i < num_samples; i++) {
      err |= _transmit((const uint8_t *) &data_addr, 1);
      err |= _receive((uint8_t *) &RX_buf, 3);
      data[data_pointer] = RX_buf[0];
      data[data_pointer] <<= 8;
      data[data_pointer] += RX_buf[1];
      data[data_pointer] <<= 6;
      data[data_pointer] += (RX_buf[2] >> 2);
      data_pointer = (data_pointer + 1) % DATA_SIZE;
    }
    
    calc_DC_avg();
    
    for (uint16_t i = 0; i < num_samples; i++) {
      process_sample(data[data_pointer_temp]);
      data_pointer_temp = (data_pointer_temp + 1) % DATA_SIZE;
    }
  }
  return err;
}

// Timer
#if defined(__TI_COMPILER_VERSION__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
#else
  void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer_A (void)
#endif
{
  miliseconds++;
}