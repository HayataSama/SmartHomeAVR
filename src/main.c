#include "include/main.h"
#include "include/adc.h"
#include "include/lcd.h"
#include "include/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

LCD lcd;
State currentState;
State lastState;
Vars vars = {.currentTemp = 0,
             .lastTemp = 0,
             .tempDiff = 0,
             .motorOn = 0,
             .speed = 0,
             .maxSpeed = 10,
             .password = "1212",
             .tempThreshold = 10,
             .time = {0, 0, 0},
             .alarm = {0, 0, 0}};
char buffer[16]; // generic buffer representing one line on display
char tmp[4];     // for password
Input chr;
uint8_t superTmp;

char password[4] = "1212";
int c = 0;
int myC = 0;
Input keyInput = 0;
int flag = 1;

/* in isr check state and only if it was STATUS respond to it*/
// ISR for PC0 (Keypad)
ISR(PCINT1_vect) {
  if (PINC & 0x01) {
    // PC0 is HIGH
  } else {
    // PC0 is LOW
    if (currentState == STATUS) {
      currentState = PASS;
      lastState = STATUS;
    }
  }
}

int main() {
  // configure PC0 "pin change" interrupt and enable global interrupt
  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT8);
  sei();

  // initializations
  lcd = lcdInit(DDB0, DDB1, DDD4, DDD5, DDD6, DDD7, 16, 2, LCD_5x8DOTS);
  lcdClear(lcd);
  adcInit();

  // setup default state
  currentState = STATUS;
  lastState = NOSTATE;

  // create menu
  for (int i = 0; i < 5; i++) {
    filler(menu[i], sizeof(menu[i]), ' ');
  }

  while (1) {
    vars.lastTemp = vars.currentTemp;
    vars.currentTemp = (uint8_t)(adcRead(1) * 50 / 1023);
    vars.tempDiff = vars.currentTemp - vars.lastTemp;

    // turn the motor on
    if ((vars.currentTemp > vars.tempThreshold) && (vars.motorOn == 0)) {
      vars.motorOn = 1;
      vars.speed = 5;
    } else if (vars.currentTemp < vars.tempThreshold) {
      vars.motorOn = 0;
      vars.speed = 0;
    }

    // make motor go brrrrrr
    if ((vars.motorOn) && (vars.tempDiff > 0)) {
      vars.speed += 5;
      if (vars.speed > vars.maxSpeed) {
        vars.speed = vars.maxSpeed;
      }
    } else if ((vars.motorOn) && (vars.tempDiff < 0)) {
      vars.speed -= 5;
      if (vars.speed < 5 || vars.speed > 100) {
        vars.speed = 0;
      }
    }

    // State machine
    if (currentState != lastState) {
      switch (currentState) {
      case STATUS:
        // FIXME: when you go back from menu to status screen the first line
        // doesn't get printed and speed is 37% for some reason lol
        // FIXME: when you go back from menu to status screen it doesn't want
        // require password again even if password is changed
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        sprintf(buffer, "Temp:%dC Motor:%d", vars.currentTemp, vars.motorOn);
        lcdPrint(lcd, buffer);
        lcdSetCursor(lcd, 1, 0);
        sprintf(buffer, "Speed:%d%%of%d%%", vars.speed, vars.maxSpeed);
        lcdPrint(lcd, buffer);
        _delay_ms(500); // so screen doesn't get updated very frequently
        break;

      case PASS:
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Enter Password:");
        lcdSetCursor(lcd, 1, 0);

        /* user types password and presses enter */

        if (strcmp(password, vars.password)) {
          lcdClear(lcd);
          lcdSetCursor(lcd, 0, 0);
          lcdPrint(lcd, "uwu");
          currentState = MENU;
          lastState = PASS;
        } else {
          // FIXME: make this show a failure screen
          lcdClear(lcd);
          lcdSetCursor(lcd, 0, 0);
          lcdPrint(lcd, "not uwu");
          _delay_ms(2000);
          currentState = STATUS;
          lastState = PASS;
        }
        break;

      case MENU:
        if (c == -1) {
          c = 4;
        }
        if (c == 5) {
          c = 0;
        }
        if (flag == 1) {
          c = 0;
          lcdClear(lcd);
          // print first line
          lcdSetCursor(lcd, 0, 0);
          addCursor(menu[c]);
          lcdPrint(lcd, menu[c]);

          // print second line
          lcdSetCursor(lcd, 1, 0);
          lcdPrint(lcd, menu[(c + 1) % (sizeof(menu) / sizeof(menu[0]))]);
          flag = 0;
        }

        while (1) {
          keyInput = getKeypad();
          // prevents very fast scrolling
          // CAUTION: too much delay causes unresponsiveness
          _delay_ms(100);
          if (keyInput == UP) {
            if (myC == 1) {
              // "<<" is on second line
              lcdClear(lcd);

              // remove "<<" from second line
              lcdSetCursor(lcd, 1, 0);
              removeCursor(menu[c]);
              lcdPrint(lcd, menu[c]);

              // add "<<" to first line
              lcdSetCursor(lcd, 0, 0);
              // FIXME: when c = 0 it duplicates menu[0] because there is no
              // menu[-1]
              addCursor(menu[(c - 1) % (sizeof(menu) / sizeof(menu[0]))]);
              lcdPrint(lcd, menu[(c - 1) % (sizeof(menu) / sizeof(menu[0]))]);
              myC = 0;
            } else {
              // "<<" is on first line

              lcdClear(lcd);
              // fix
              removeCursor(menu[c]);
              lcdSetCursor(lcd, 1, 0);
              lcdPrint(lcd, menu[c]);

              addCursor(menu[(c - 1) % (sizeof(menu) / sizeof(menu[0]))]);
              lcdSetCursor(lcd, 0, 0);
              lcdPrint(lcd, menu[(c - 1) % (sizeof(menu) / sizeof(menu[0]))]);
            }
            c--;
            break;
          } else if (keyInput == DOWN) {
            if (myC == 0) {
              // "<<" is on first line
              lcdClear(lcd);

              // remove "<<" from first line
              lcdSetCursor(lcd, 0, 0);
              removeCursor(menu[c]);
              lcdPrint(lcd, menu[c]);

              // add "<<" to second line
              lcdSetCursor(lcd, 1, 0);
              addCursor(menu[(c + 1) % (sizeof(menu) / sizeof(menu[0]))]);
              lcdPrint(lcd, menu[(c + 1) % (sizeof(menu) / sizeof(menu[0]))]);
              myC = 1;
            } else {
              // "<<" is on second line

              lcdClear(lcd);
              // fix
              removeCursor(menu[c]);
              lcdSetCursor(lcd, 0, 0);
              lcdPrint(lcd, menu[c]);

              addCursor(menu[(c + 1) % (sizeof(menu) / sizeof(menu[0]))]);
              lcdSetCursor(lcd, 1, 0);
              lcdPrint(lcd, menu[(c + 1) % (sizeof(menu) / sizeof(menu[0]))]);
            }

            c++;
            break;
          } else if (keyInput == ENTER) {
            flag = 1;
            switch (c + 4) {
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
            flag = 1;
            currentState = STATUS;
            lastState = MENU;
            break;
          }
        }
        break;
      case CHANGE_PASS:
        // FIXME: handle the case where user inputs more than 4 chars
        // probably reset the tmp buffer and start over.
        // clear previously set password
        tmp[0] = '\0';
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        // FIXME: some other stuff get printed after old pass
        sprintf(buffer, "Old Pass:%s", vars.password);
        lcdPrint(lcd, buffer);

        // wait for user input
        do {
          chr = getKeypad();
          _delay_ms(100);
          if (chr == UP) {
            // chr is 1
            sprintf(tmp + strlen(tmp), "%d", 1);
          } else if (chr == DOWN) {
            // chr is 2
            sprintf(tmp + strlen(tmp), "%d", 2);
          } else if (chr == ENTER && (strlen(tmp) == 4)) {
            strcpy(vars.password, tmp);
            currentState = SUCCESS;
            lastState = CHANGE_PASS;
          } else if (chr == BACK) {
            currentState = MENU;
            lastState = CHANGE_PASS;
          }

          lcdSetCursor(lcd, 1, 0);
          sprintf(buffer, "New Pass:%s", tmp);
          lcdPrint(lcd, buffer);
        } while ((chr != ENTER) && (chr != BACK));

        break;
      case CHANGE_TEMP:
        // FIXME: when we return from this to menu, 2 items have cursor on them.
        // it's probably because we don't erase cursor before switching to
        // another state so it remains in menu array.
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        sprintf(buffer, "Thresh(C):%d", vars.tempThreshold);
        lcdPrint(lcd, buffer);
        superTmp = vars.tempThreshold;

        while (1) {
          keyInput = getKeypad();
          _delay_ms(100);
          if (keyInput == UP) {
            superTmp++;
            lcdClear(lcd);
            lcdSetCursor(lcd, 0, 0);
            sprintf(buffer, "Thresh(C):%d", superTmp);
            lcdPrint(lcd, buffer);
          }
          if (keyInput == DOWN) {
            superTmp--;
            lcdClear(lcd);
            lcdSetCursor(lcd, 0, 0);
            sprintf(buffer, "Thresh(C):%d", superTmp);
            lcdPrint(lcd, buffer);
          }
          if (keyInput == ENTER) {
            vars.tempThreshold = superTmp;
            break;
          }
          if (keyInput == BACK) {
            break;
          }
        }

        currentState = MENU;
        lastState = CHANGE_TEMP;
        break;

      case CHANGE_SPEED:
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        sprintf(buffer, "Max Speed:%d", vars.maxSpeed);
        lcdPrint(lcd, buffer);
        superTmp = vars.maxSpeed;

        while (1) {
          keyInput = getKeypad();
          _delay_ms(100);
          if (keyInput == UP) {
            superTmp++;
            lcdClear(lcd);
            lcdSetCursor(lcd, 0, 0);
            sprintf(buffer, "Max Speed:%d", superTmp);
            lcdPrint(lcd, buffer);
          }
          if (keyInput == DOWN) {
            superTmp--;
            lcdClear(lcd);
            lcdSetCursor(lcd, 0, 0);
            sprintf(buffer, "Max Speed:%d", superTmp);
            lcdPrint(lcd, buffer);
          }
          if (keyInput == ENTER) {
            vars.maxSpeed = superTmp;
            break;
          }
          if (keyInput == BACK) {
            break;
          }
        }

        currentState = MENU;
        lastState = CHANGE_SPEED;
        break;
      case CHANGE_TIME:
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Set Time:");
        lcdSetCursor(lcd, 1, 0);
        sprintf(buffer, "%02d:%02d:%02d", vars.time[0], vars.time[1],
                vars.time[2]);
        lcdPrint(lcd, buffer);
        uint8_t myTmp[3];
        for (int i = 0; i < 3; i++) {
          myTmp[i] = vars.time[i];
        }

        for (int i = 0; i < 3; i++) {
          while (1) {
            keyInput = getKeypad();
            _delay_ms(100);

            if (keyInput == UP) {
              myTmp[i]++;
              lcdClear(lcd);
              lcdSetCursor(lcd, 0, 0);
              lcdPrint(lcd, "Set Time:");
              lcdSetCursor(lcd, 1, 0);
              sprintf(buffer, "%02d:%02d:%02d", myTmp[0], myTmp[1], myTmp[2]);
              lcdPrint(lcd, buffer);
            }
            if (keyInput == DOWN) {
              myTmp[i]--;
              lcdClear(lcd);
              lcdSetCursor(lcd, 0, 0);
              lcdPrint(lcd, "Set Time:");
              lcdSetCursor(lcd, 1, 0);
              sprintf(buffer, "%02d:%02d:%02d", myTmp[0], myTmp[1], myTmp[2]);
              lcdPrint(lcd, buffer);
            }
            if (keyInput == ENTER) {
              vars.time[i] = myTmp[i];
              break;
            }
            if (keyInput == BACK) {
              i -= 2;
              break;
            }
          }
          // if cursor is on first digit, back means go back to menu
          if (i < 0) {
            break;
          }
        }
        currentState = MENU;
        lastState = CHANGE_TIME;
        break;
      case SET_ALARM:
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Set Alarm:");
        lcdSetCursor(lcd, 1, 0);
        sprintf(buffer, "%02d:%02d:%02d", vars.alarm[0], vars.alarm[1],
                vars.alarm[2]);
        lcdPrint(lcd, buffer);
        uint8_t myTmpAlarm[3];
        for (int i = 0; i < 3; i++) {
          myTmpAlarm[i] = vars.alarm[i];
        }

        for (int i = 0; i < 3; i++) {
          while (1) {
            keyInput = getKeypad();
            _delay_ms(100);

            if (keyInput == UP) {
              myTmpAlarm[i]++;
              lcdClear(lcd);
              lcdSetCursor(lcd, 0, 0);
              lcdPrint(lcd, "Set Alarm:");
              lcdSetCursor(lcd, 1, 0);
              sprintf(buffer, "%02d:%02d:%02d", myTmpAlarm[0], myTmpAlarm[1],
                      myTmpAlarm[2]);
              lcdPrint(lcd, buffer);
            }
            if (keyInput == DOWN) {
              myTmpAlarm[i]--;
              lcdClear(lcd);
              lcdSetCursor(lcd, 0, 0);
              lcdPrint(lcd, "Set Alarm:");
              lcdSetCursor(lcd, 1, 0);
              sprintf(buffer, "%02d:%02d:%02d", myTmpAlarm[0], myTmpAlarm[1],
                      myTmpAlarm[2]);
              lcdPrint(lcd, buffer);
            }
            if (keyInput == ENTER) {
              vars.alarm[i] = myTmpAlarm[i];
              break;
            }
            if (keyInput == BACK) {
              i -= 2;
              break;
            }
          }
          // if cursor is on first digit, back means go back to menu
          if (i < 0) {
            break;
          }
        }
        currentState = MENU;
        lastState = SET_ALARM;
        break;
      case SUCCESS:
        if (lastState == CHANGE_PASS) {
          lcdClear(lcd);
          lcdSetCursor(lcd, 0, 0);
          lcdPrint(lcd, "Password Changed");
          _delay_ms(500);
        }
        currentState = MENU;
        lastState = SUCCESS;
        break;
      }
    }
  }

  return 0;
}

// TODO: make a function for handling keypad input depending on state
// instead of re-writing ifs everytime

// every time we press back, we want to go to the lastState (probably)
// and also if we pressed back and we are going to land on menu we should
// show the first two items. (if flag == 1)
