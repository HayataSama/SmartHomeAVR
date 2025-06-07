#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>
#include <string.h>

typedef enum { NOINPUT, UP, ENTER, DOWN, BACK } Input;

// fill empty chars in a fixed-size string with filler
void filler(char *str, size_t size, char filler);

// add "<<" cursor to the end of an string
void addCursor(char *str);

// remove "<<" cursor at the end of an string
void removeCursor(char *str);

Input getKeypad();

void pmwInit();

void pwmSetDuty(uint8_t channel, uint8_t duty_cycle);

#endif