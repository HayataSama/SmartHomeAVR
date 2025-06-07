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
  SET_ALARM,
  SUCCESS,
  FAILURE
} State;

typedef struct {
  uint8_t currentTemp;
  uint8_t lastTemp;
  int8_t tempDiff;
  uint8_t motorOn;
  uint8_t speed;

  uint8_t maxSpeed;
  char password[4 + 1];
  uint8_t tempThreshold;
  uint8_t time[3];
  uint8_t alarm[3];
} Vars;

char menu[5][16 + 1] = {
    // +1 is for '\0' null terminator
    "1.Change Pass", "2.Temp Thresh", "3.Motor Speed",
    "4.Set Time",    "5.Set Alarm",
};

// read temperature from sensor and adjust the motor
void motorControl();

#endif