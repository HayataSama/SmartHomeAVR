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
  CHANGE_TIME
} State;

typedef struct {
  uint8_t temp;
  uint8_t motorOn;
  uint8_t maxSpeed;
} Vars;

const uint16_t PASSWORD = 1234;

char menu[5][16 + 1] = {
    // +1 is for '\0' null terminator
    "1.Change Pass", "2.Temp Thresh", "3.Motor Speed",
    "4.Set Time",    "5.Set Alarm",
};

#endif