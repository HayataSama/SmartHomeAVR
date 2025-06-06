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
Vars vars;
char buffer[16]; // generic buffer representing one line on display

uint16_t password = 1234;
int c = 0;
int myC = 0;
Input keyInput = 0;
int flag = 1;

/* in isr check state and only if it was STATUS respond to it*/

int main() {
  // configure PC0 "pin change" interrupt and enable global interrupt
  PCICR |= (1 << PCIE1);
  PCMSK1 |= (1 << PCINT8);
  // sei();

  // initializations
  lcd = lcdInit(DDB0, DDB1, DDD4, DDD5, DDD6, DDD7, 16, 2, LCD_5x8DOTS);
  lcdClear(lcd);
  adcInit();

  // setup default state
  currentState = STATUS;
  lastState = NOSTATE;
  vars.maxSpeed = 50;
  vars.motorOn = 0;

  // create menu
  for (int i = 0; i < 5; i++) {
    filler(menu[i], sizeof(menu[i]), ' ');
  }

  while (1) {
    vars.temp = (uint8_t)(adcRead(1) * 50 / 1023);

    // State machine
    if (currentState != lastState) {
      switch (currentState) {
      case STATUS:
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        sprintf(buffer, "Temp:%dC Motor:%d", vars.temp, vars.motorOn);
        lcdPrint(lcd, buffer);
        lcdSetCursor(lcd, 1, 0);
        sprintf(buffer, "Speed:%d%%", vars.maxSpeed);
        lcdPrint(lcd, buffer);
        lastState = currentState;

        /* for testing purposes */
        _delay_ms(500);
        currentState = PASS;

      case PASS:
        lcdClear(lcd);
        lcdSetCursor(lcd, 0, 0);
        lcdPrint(lcd, "Enter Password:");
        lcdSetCursor(lcd, 1, 0);
        lcdPrint(lcd, "*");
        _delay_ms(500);
        lcdPrint(lcd, "*");
        _delay_ms(500);
        lcdPrint(lcd, "*");
        _delay_ms(500);
        lcdPrint(lcd, "*");
        _delay_ms(500);

        /* user types password and presses enter */

        if (password == PASSWORD) {
          currentState = MENU;
          lastState = PASS;
        }

      case MENU:
        if (c == -1) {
          c = 4;
        }
        if (c == 5) {
          c = 0;
        }
        if (flag == 1) {
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
            /* check c is on which item and change state to that one */
          } else if (keyInput == BACK) {
            flag = 1;
            currentState = STATUS;
            lastState = MENU;
            break;
          }
        }

      default:
        break;
      }
    }
  }

  return 0;
}
