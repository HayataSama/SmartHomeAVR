#include <avr/io.h>
#include <inttypes.h>

void adcInit() {
  // Set reference voltage to AVCC
  ADMUX = (1 << REFS0);

  // Enable ADC, set prescaler to 128 (for 16MHz clock)
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

  // Perform a dummy conversion to stabilize ADC
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC))
    ;
}

uint16_t adcRead(uint8_t ch) {
  // Select ADC channel
  ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);

  // Start conversion
  ADCSRA |= (1 << ADSC);

  // Wait for conversion to complete
  while (ADCSRA & (1 << ADSC))
    ;

  // Return ADC result (10-bit value)
  return ADC;
}