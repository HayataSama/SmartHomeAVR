#ifndef ADC_H
#define ADC_H

#include <inttypes.h>

void adcInit();
uint16_t adcRead(uint8_t ch);

#endif