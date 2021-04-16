#include "I2C_master.h"
#include "msp430g2553.h"
#include "stdint.h"

#define FIFO_SIZE 32
#define DATA_SIZE 128
#define RATES_SIZE 16

uint8_t setup(void);
void calc_DC_avg(void);
uint16_t get_BPM_avg(void);
int16_t lowpass_FIR_filter(int16_t signal);
void process_sample(uint16_t sample);
uint8_t get_data(void);
