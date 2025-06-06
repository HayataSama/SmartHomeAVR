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