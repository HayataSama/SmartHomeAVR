#ifndef MAIN_H
#define MAIN_H

#include <inttypes.h>

typedef enum {
  NOSTATE,
  STATUS,
  PASS,
  MENU,
  CHANGE_PASS,
  CHANGE_TEMP,
  CHANGE_SPEED,
  CHANGE_TIME,
} State;

typedef struct {
  uint8_t temp;
  uint8_t motorOn;
  uint8_t maxSpeed;
} Vars;

#endif