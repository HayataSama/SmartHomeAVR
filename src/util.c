#include "include/util.h"
#include "include/adc.h"
#include <avr/io.h>
#include <inttypes.h>
#include <string.h>

// fill empty chars in a fixed-size string with filler
void filler(char *str, size_t size, char filler) {
  for (int i = strlen(str); i < size - 1; i++) {
    str[i] = filler;
  }
  str[size - 1] = '\0';
}

// add "<<" cursor to the end of an string
void addCursor(char *str) {
  char *c = str + strlen(str) - 1;
  *c = '<';
  c--;
  *c = '<';
}

// remove "<<" cursor at the end of an string
void removeCursor(char *str) {
  char *c = str + strlen(str) - 1;
  *c = ' ';
  c--;
  *c = ' ';
}

Input getKeypad() {
  uint16_t adc = adcRead(0);
  if (adc > 510) {
    return NOINPUT;
  } else if (adc > 450 && adc < 510) {
    return BACK;
  } else if (adc > 280 && adc < 340) {
    return DOWN;
  } else if (adc > 100 && adc < 160) {
    return UP;
  } else if ((adc >= 0) && (adc < 50)) {
    return ENTER;
  } else {
    return NOINPUT;
  }
}

void pmwInit() {
  // Set OC2A (PB3) and OC2B (PD3) as output pins
  DDRB |= (1 << DDB3); // OC2A output
  DDRD |= (1 << DDD3); // OC2B output

  // Configure Timer2 for Fast PWM mode
  TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);

  // Set prescaler to 64
  TCCR2B = (1 << CS22);

  // Initialize duty cycle to 0 (LED off)
  OCR2A = 0; // Channel A duty cycle (0-255)
  OCR2B = 0; // Channel B duty cycle (0-255)
             // PWM frequency = 16MHz / (64 * 256) = ~976 Hz
}

void pwmSetDuty(uint8_t channel, uint8_t duty_cycle) {
  if (channel == 0) {
    // Channel A (PB3)
    OCR2A = duty_cycle;
  } else if (channel == 1) {
    // Channel B (PD3)
    OCR2B = duty_cycle;
  }
}