#ifndef MAIN_H
#define MAIN_H

#include <inttypes.h>

#define PASSWORD_LENGTH 4  // password buffer size
#define BUFFER_SIZE 16 + 1 // text buffer size + \0
#define MENU_ITEMS 5       // no. of menu items
#define MAX_TEMP 50        // max temperature reported by sensor
#define MIN_TEMP 0         // min temperature reported by sensor
#define MAX_SPEED 100      // max motor duty cycle (%)
#define MIN_SPEED 5        // min motor duty cycle (%)
#define SPEED_STEP_SIZE 5  // step size for motor control

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
  char password[PASSWORD_LENGTH + 1];
  uint8_t tempThreshold;
  uint8_t time[3];
  uint8_t alarm[3];
} Vars;

char menu[MENU_ITEMS][BUFFER_SIZE] = {
    // +1 is for '\0' null terminator
    "1.Change Pass", "2.Temp Thresh", "3.Motor Speed",
    "4.Set Time",    "5.Set Alarm",
};

// read temperature from sensor and adjust the motor
void motorControl();

#endif