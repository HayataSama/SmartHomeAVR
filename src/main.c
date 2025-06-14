#include "include/main.h"
#include "include/adc.h"
#include "include/lcd.h"
#include "include/util.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

LCD lcd;
State currentState;
State lastState;
char buffer[BUFFER_SIZE];             // text buffer
char passBuffer[PASSWORD_LENGTH + 1]; // password buffer
Input keyInput = 0;
Vars vars = {
    .currentTemp = 0,
    .lastTemp = 0,
    .tempDiff = 0,
    .motorOn = 0,
    .speed = 0,
};
volatile uint8_t seconds = 0;
volatile uint8_t timeoutFlag = 0;

ISR(TIMER1_COMPA_vect) {
  seconds++;
  // emulate an RTC
  vars.time[2]++;
  if (vars.time[2] == 60) {
    vars.time[2] = 0;
    vars.time[1]++;
  }
  if (vars.time[1] == 60) {
    vars.time[1] = 0;
    vars.time[0]++;
  }

  // Emulating a TIMEOUT second watchdog
  if (seconds >= TIMEOUT) {
    currentState = STATUS;
    lastState = NOSTATE;
    timeoutFlag = 1;
    seconds = 0;
  }
}

// ISR for PC0 (Keypad)
ISR(PCINT1_vect) {
  if (!(PINC & PINC0)) { // PC0 is low
    if (currentState == STATUS) {
      currentState = PASS;
      lastState = STATUS;
    }
  }
}

int main() {
  systemInit();

  while (1) {
    // read temperature from sensor and adjust the motor
    motorControl();

    // check if the alarm has went off
    checkAlarm();

    // State machine
    if (currentState != lastState) {
      switch (currentState) {
      case STATUS:
        displayStatus();
        break;

      case PASS:
        passwordHandler();
        break;

      case MENU:
        displayMenu();
        break;

      case CHANGE_PASS:
        changePassword();
        break;

      case CHANGE_TEMP:
        changeTemp();
        break;

      case CHANGE_SPEED:
        changeSpeed();
        break;

      case CHANGE_TIME:
        changeTime();
        break;

      case SET_ALARM:
        setAlarm();
        break;
      }
    }
  }

  return 0;
}

void systemInit() {
  /*
    // write default values to eeprom for the very first time
    eeprom_write_byte((uint8_t *)0x00, vars.maxSpeed);
    eeprom_write_byte((uint8_t *)0x01, vars.tempThreshold);
    eeprom_write_block((const void *)vars.time, (void *)0x02,
    sizeof(vars.time)); eeprom_write_block((const void *)vars.alarm, (void
    *)0x05,
                       sizeof(vars.alarm));
    eeprom_write_block((const void *)vars.password, (void *)0x08,
                       sizeof(vars.password));
  */

  vars.maxSpeed = eeprom_read_byte((uint8_t *)0x00);
  vars.tempThreshold = eeprom_read_byte((uint8_t *)0x01);
  eeprom_read_block((void *)vars.time, (const void *)0x02, sizeof(vars.time));
  eeprom_read_block((void *)vars.alarm, (const void *)0x05, sizeof(vars.alarm));
  eeprom_read_block((void *)vars.password, (const void *)0x08,
                    sizeof(vars.password));

  // configure PC0 "pin change" interrupt
  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT8);

  // configure timer 1 to generate an interrupt every 1s
  timerInit();
  startTimer();

  // enable global interrupt
  sei();

  // for debugging purposes
  DDRD |= (1 << DDD0);

  // init display
  lcd = lcdInit(DDB0, DDB1, DDD4, DDD5, DDD6, DDD7, 16, 2, LCD_5x8DOTS);
  lcdClear(lcd);

  // init adc
  adcInit();

  // init timer 2 pwm (ch0: PB3, ch1: PD3)
  pmwInit();

  // set default state
  currentState = STATUS;
  lastState = NOSTATE;

  // clear buffers
  buffer[0] = '\0';
  passBuffer[0] = '\0';
}

void motorControl() {
  // read new temperature from sensor
  vars.lastTemp = vars.currentTemp;
  vars.currentTemp = (uint8_t)((adcRead(1) * MAX_TEMP) >> 10);
  vars.tempDiff = vars.currentTemp - vars.lastTemp;

  // check if motor should be turned on
  if ((vars.currentTemp > vars.tempThreshold) && (vars.motorOn == 0)) {
    vars.motorOn = 1;
    vars.speed = MIN_SPEED;
  } else if (vars.currentTemp < vars.tempThreshold) {
    vars.motorOn = 0;
    vars.speed = 0;
  }

  // adjust motor speed
  if ((vars.motorOn) && (vars.tempDiff > 0)) {
    // if temp is increasing speed up the motor
    vars.speed += SPEED_STEP_SIZE;
    if (vars.speed > vars.maxSpeed) {
      vars.speed = vars.maxSpeed;
    }
  } else if ((vars.motorOn) && (vars.tempDiff < 0)) {
    // if temp is decreasing speed down the motor
    vars.speed -= SPEED_STEP_SIZE;
    if (vars.speed < MIN_SPEED || vars.speed > MAX_SPEED) {
      vars.speed = 0;
    }
  }
  pwmSetDuty(1, vars.speed * 255 / 100);
}

void displayStatus() {
  seconds = 0;
  timeoutFlag = 0;
  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  snprintf(buffer, BUFFER_SIZE, "Temp:%dC Motor:%d", vars.currentTemp,
           vars.motorOn);
  lcdPrint(lcd, buffer);
  lcdSetCursor(lcd, 1, 0);
  snprintf(buffer, BUFFER_SIZE, "Speed:%d%%of%d%%", vars.speed, vars.maxSpeed);
  lcdPrint(lcd, buffer);
  // to prevent screen refreshing very fast
  _delay_ms(500);
}

void passwordHandler() {
  // clear password buffer
  passBuffer[0] = '\0';
  Input input;

  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  lcdPrint(lcd, "Enter Password:");
  lcdSetCursor(lcd, 1, 0);

  // to prevent accidentally pressing enter or back
  _delay_ms(750);

  seconds = 0;
  timeoutFlag = 0;
  while (!(timeoutFlag)) {
    input = getKeypad();
    _delay_ms(100);

    if (input == UP) {
      if (strlen(passBuffer) < PASSWORD_LENGTH) {
        // input is 1
        snprintf(passBuffer + strlen(passBuffer),
                 sizeof(passBuffer + strlen(passBuffer)), "%d", 1);

        // update the screen
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Enter Password:");
        lcdSetCursor(lcd, 1, 0);
        lcdPrint(lcd, passBuffer);
      }
    } else if (input == DOWN) {
      if (strlen(passBuffer) < PASSWORD_LENGTH) {
        // input is 2
        snprintf(passBuffer + strlen(passBuffer),
                 sizeof(passBuffer + strlen(passBuffer)), "%d", 2);

        // update the screen
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Enter Password:");
        lcdSetCursor(lcd, 1, 0);
        lcdPrint(lcd, passBuffer);
      }
    } else if (input == ENTER) {
      if (strlen(passBuffer) == PASSWORD_LENGTH) {
        if (strcmp(passBuffer, vars.password) == 0) {
          currentState = MENU;
          lastState = PASS;
        } else {
          displayFailure("Incorrect Pass");
          currentState = STATUS;
          lastState = PASS;
        }
      } else {
        displayFailure("Incorrect Pass");
        currentState = STATUS;
        lastState = PASS;
      }
      break;
    } else if (input == BACK) {
      currentState = STATUS;
      lastState = PASS;
      break;
    }
  }
}

void displayFailure(char *msg) {
  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  snprintf(buffer, BUFFER_SIZE, "%s", msg);
  lcdPrint(lcd, buffer);
  _delay_ms(1000);
}

void displaySuccess(char *msg) {
  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  snprintf(buffer, BUFFER_SIZE, "%s", msg);
  lcdPrint(lcd, buffer);
  _delay_ms(1000);
}

void changePassword() {
  passBuffer[0] = '\0';
  Input input;

  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  snprintf(buffer, BUFFER_SIZE, "Old Pass:%s", vars.password);
  lcdPrint(lcd, buffer);
  lcdSetCursor(lcd, 1, 0);
  snprintf(buffer, BUFFER_SIZE, "New Pass:%s", passBuffer);
  lcdPrint(lcd, buffer);

  // to prevent accidentally pressing enter or back
  _delay_ms(750);

  seconds = 0;
  timeoutFlag = 0;
  while (!(timeoutFlag)) {
    input = getKeypad();
    _delay_ms(100);

    if (input == UP) {
      if (strlen(passBuffer) < PASSWORD_LENGTH) {
        // input is 1
        snprintf(passBuffer + strlen(passBuffer),
                 sizeof(passBuffer + strlen(passBuffer)), "%d", 1);
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        snprintf(buffer, BUFFER_SIZE, "Old Pass:%s", vars.password);
        lcdPrint(lcd, buffer);
        lcdSetCursor(lcd, 1, 0);
        snprintf(buffer, BUFFER_SIZE, "New Pass:%s", passBuffer);
        lcdPrint(lcd, buffer);
      }
    } else if (input == DOWN) {
      if (strlen(passBuffer) < PASSWORD_LENGTH) {
        // input is 2
        snprintf(passBuffer + strlen(passBuffer),
                 sizeof(passBuffer + strlen(passBuffer)), "%d", 2);
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        snprintf(buffer, BUFFER_SIZE, "Old Pass:%s", vars.password);
        lcdPrint(lcd, buffer);
        lcdSetCursor(lcd, 1, 0);
        snprintf(buffer, BUFFER_SIZE, "New Pass:%s", passBuffer);
        lcdPrint(lcd, buffer);
      }
    } else if (input == ENTER) {
      if (strlen(passBuffer) == PASSWORD_LENGTH) {
        strcpy(vars.password, passBuffer);
        // update EEPROM
        eeprom_write_block((const void *)vars.password, (void *)0x08,
                           sizeof(vars.password));
        displaySuccess("Pass Changed");
        currentState = MENU;
        lastState = CHANGE_PASS;
        break;
      }
    } else if (input == BACK) {
      currentState = MENU;
      lastState = CHANGE_PASS;
      break;
    }
  }
}

void changeTime() {
  uint8_t timeBuffer[3];

  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  lcdPrint(lcd, "Set Time:");
  lcdSetCursor(lcd, 1, 0);
  snprintf(buffer, BUFFER_SIZE, "%02d:%02d:%02d", vars.time[0], vars.time[1],
           vars.time[2]);
  lcdPrint(lcd, buffer);

  // to prevent accidentally pressing enter or back
  _delay_ms(750);

  for (int i = 0; i < 3; i++) {
    timeBuffer[i] = vars.time[i];
  }

  seconds = 0;
  timeoutFlag = 0;
  for (int i = 0; i < 3; i++) {
    while (!(timeoutFlag)) {
      keyInput = getKeypad();
      _delay_ms(100);

      if (keyInput == UP) {
        timeBuffer[i]++;
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Set Time:");
        lcdSetCursor(lcd, 1, 0);
        snprintf(buffer, BUFFER_SIZE, "%02d:%02d:%02d", timeBuffer[0],
                 timeBuffer[1], timeBuffer[2]);
        lcdPrint(lcd, buffer);

      } else if (keyInput == DOWN) {
        timeBuffer[i]--;
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Set Time:");
        lcdSetCursor(lcd, 1, 0);
        snprintf(buffer, BUFFER_SIZE, "%02d:%02d:%02d", timeBuffer[0],
                 timeBuffer[1], timeBuffer[2]);
        lcdPrint(lcd, buffer);

      } else if (keyInput == ENTER) {
        if (i == 2) {
          // if cursor is on last digit, enter means save changes
          for (int i = 0; i < 3; i++) {
            vars.time[i] = timeBuffer[i];
          }
          displaySuccess("Time Changed");
          // update EEPROM
          eeprom_write_block((const void *)vars.time, (void *)0x02,
                             sizeof(vars.time));
        }
        break;

      } else if (keyInput == BACK) {
        i -= 2;
        break;
      }
    }
    if (i == -2) {
      // if cursor is on first digit, back means go back to menu
      break;
    }
  }
  currentState = MENU;
  lastState = CHANGE_TIME;
}

void setAlarm() {
  uint8_t alarmBuffer[3];

  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  lcdPrint(lcd, "Set Alarm:");
  lcdSetCursor(lcd, 1, 0);
  snprintf(buffer, BUFFER_SIZE, "%02d:%02d:%02d", vars.alarm[0], vars.alarm[1],
           vars.alarm[2]);
  lcdPrint(lcd, buffer);

  // to prevent accidentally pressing enter or back
  _delay_ms(750);

  for (int i = 0; i < 3; i++) {
    alarmBuffer[i] = vars.alarm[i];
  }

  seconds = 0;
  timeoutFlag = 0;
  for (int i = 0; i < 3; i++) {
    while (!(timeoutFlag)) {
      keyInput = getKeypad();
      _delay_ms(100);

      if (keyInput == UP) {
        alarmBuffer[i]++;
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Set Alarm:");
        lcdSetCursor(lcd, 1, 0);
        snprintf(buffer, BUFFER_SIZE, "%02d:%02d:%02d", alarmBuffer[0],
                 alarmBuffer[1], alarmBuffer[2]);
        lcdPrint(lcd, buffer);

      } else if (keyInput == DOWN) {
        alarmBuffer[i]--;
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Set Alarm:");
        lcdSetCursor(lcd, 1, 0);
        snprintf(buffer, BUFFER_SIZE, "%02d:%02d:%02d", alarmBuffer[0],
                 alarmBuffer[1], alarmBuffer[2]);
        lcdPrint(lcd, buffer);

      } else if (keyInput == ENTER) {
        if (i == 2) {
          for (int i = 0; i < 3; i++) {
            vars.alarm[i] = alarmBuffer[i];
          }
          displaySuccess("Alarm Changed");
          // update EEPROM
          eeprom_write_block((const void *)vars.alarm, (void *)0x05,
                             sizeof(vars.alarm));
        }
        break;
        ;

      } else if (keyInput == BACK) {
        i -= 2;
        break;
      }
    }
    if (i == -2) {
      // if cursor is on first digit, back means go back to menu
      break;
    }
  }
  currentState = MENU;
  lastState = SET_ALARM;
}

void changeSpeed() {
  uint8_t tempSpeed = vars.maxSpeed;

  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  snprintf(buffer, BUFFER_SIZE, "Max Speed:%d", vars.maxSpeed);
  lcdPrint(lcd, buffer);

  // to prevent accidentally pressing enter or back
  _delay_ms(750);

  seconds = 0;
  timeoutFlag = 0;
  while (!(timeoutFlag)) {
    keyInput = getKeypad();
    _delay_ms(100);

    if (keyInput == UP) {
      tempSpeed++;
      lcdClear(lcd);
      lcdSetCursor(lcd, 0, 0);
      snprintf(buffer, BUFFER_SIZE, "Max Speed:%d", tempSpeed);
      lcdPrint(lcd, buffer);

    } else if (keyInput == DOWN) {
      tempSpeed--;
      lcdClear(lcd);
      lcdSetCursor(lcd, 0, 0);
      snprintf(buffer, BUFFER_SIZE, "Max Speed:%d", tempSpeed);
      lcdPrint(lcd, buffer);

    } else if (keyInput == ENTER) {
      displaySuccess("Speed Changed");
      vars.maxSpeed = tempSpeed;
      // update EEPROM
      eeprom_write_byte((uint8_t *)0x00, vars.maxSpeed);
      break;

    } else if (keyInput == BACK) {
      break;
    }
  }

  currentState = MENU;
  lastState = CHANGE_SPEED;
}

void changeTemp() {
  uint8_t tempTemp = vars.tempThreshold;

  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  snprintf(buffer, BUFFER_SIZE, "Thresh(C):%d", vars.tempThreshold);
  lcdPrint(lcd, buffer);

  // to prevent accidentally pressing enter or back
  _delay_ms(750);

  seconds = 0;
  timeoutFlag = 0;
  while (!(timeoutFlag)) {
    keyInput = getKeypad();
    _delay_ms(100);

    if (keyInput == UP) {
      tempTemp++;
      lcdClear(lcd);
      lcdSetCursor(lcd, 0, 0);
      snprintf(buffer, BUFFER_SIZE, "Thresh(C):%d", tempTemp);
      lcdPrint(lcd, buffer);

    } else if (keyInput == DOWN) {
      tempTemp--;
      lcdClear(lcd);
      lcdSetCursor(lcd, 0, 0);
      snprintf(buffer, BUFFER_SIZE, "Thresh(C):%d", tempTemp);
      lcdPrint(lcd, buffer);

    } else if (keyInput == ENTER) {
      vars.tempThreshold = tempTemp;
      // update EEPROM
      eeprom_write_byte((uint8_t *)0x01, vars.tempThreshold);
      displaySuccess("Temp Changed");
      break;

    } else if (keyInput == BACK) {
      break;
    }
  }

  currentState = MENU;
  lastState = CHANGE_TEMP;
}

void displayMenu() {
  int menuIndex = 0;

  // fill menu items with spaces
  for (int i = 0; i < 5; i++) {
    filler(menu[i], BUFFER_SIZE, ' ');
  }

  // print first line
  lcdClear(lcd);
  lcdSetCursor(lcd, 0, 0);
  addCursor(menu[menuIndex]);
  lcdPrint(lcd, menu[menuIndex]);
  // print second line
  lcdSetCursor(lcd, 1, 0);
  lcdPrint(lcd, menu[(menuIndex + 1) % MENU_ITEMS]);

  // to prevent accidentally pressing enter or back
  _delay_ms(750);

  seconds = 0;
  timeoutFlag = 0;
  while (!(timeoutFlag)) {
    keyInput = getKeypad();
    _delay_ms(100);

    if (keyInput == UP) {
      lcdClear(lcd);
      removeCursor(menu[menuIndex]);
      lcdSetCursor(lcd, 1, 0);
      lcdPrint(lcd, menu[menuIndex]);

      addCursor(menu[(menuIndex + MENU_ITEMS - 1) % MENU_ITEMS]);
      lcdSetCursor(lcd, 0, 0);
      lcdPrint(lcd, menu[(menuIndex + MENU_ITEMS - 1) % MENU_ITEMS]);

      menuIndex--;
      // set lowerboundary
      if (menuIndex == -1) {
        menuIndex = 4;
      }

    } else if (keyInput == DOWN) {
      lcdClear(lcd);
      removeCursor(menu[menuIndex]);
      lcdSetCursor(lcd, 0, 0);
      lcdPrint(lcd, menu[menuIndex]);

      addCursor(menu[(menuIndex + 1) % MENU_ITEMS]);
      lcdSetCursor(lcd, 1, 0);
      lcdPrint(lcd, menu[(menuIndex + 1) % MENU_ITEMS]);

      menuIndex++;
      // set upper boundary
      if (menuIndex == 5) {
        menuIndex = 0;
      }

    } else if (keyInput == ENTER) {
      removeCursor(menu[menuIndex]);

      switch (menuIndex + 4) {
      case CHANGE_PASS:
        currentState = CHANGE_PASS;
        lastState = MENU;
        break;
      case CHANGE_TEMP:
        currentState = CHANGE_TEMP;
        lastState = MENU;
        break;
      case CHANGE_SPEED:
        currentState = CHANGE_SPEED;
        lastState = MENU;
        break;
      case CHANGE_TIME:
        currentState = CHANGE_TIME;
        lastState = MENU;
        break;
      case SET_ALARM:
        currentState = SET_ALARM;
        lastState = MENU;
        break;
      }
      break;

    } else if (keyInput == BACK) {
      removeCursor(menu[menuIndex]);

      currentState = STATUS;
      lastState = MENU;
      break;
    }
  }
}

void checkAlarm() {
  if (memcmp(vars.time, vars.alarm, sizeof(vars.time)) == 0) {
    PORTD |= (1 << PORTD0);
  } else {
    PORTD &= ~(1 << PORTD0);
  }
}

void timerInit() {
  // configure timer 1 to generate an interrput every 1s

  // Clear Timer1 registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  // Set Timer1 to CTC (Clear Timer on Compare Match) mode
  TCCR1B |= (1 << WGM12);
  // Set prescaler to 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // Calculate OCR1A value for 30 seconds
  // OCR1A = (F_CPU * time_in_seconds) / prescaler - 1
  // OCR1A = (16000000 * 1) / 1024 - 1 = 15624
  OCR1A = 15624;
}

void startTimer() {
  // Enable Timer1 Compare Match A interrupt
  TIMSK1 |= (1 << OCIE1A);

  // Start counting from 0 to OCR1A
  TCNT1 = 0;
}

void stopTimer() {
  // Disable Timer1 Compare Match A interrupt
  TIMSK1 &= ~(1 << OCIE1A);

  // clear TCNT1
  TCNT1 = 0;
}

// TODO: search for 7-segment solution
