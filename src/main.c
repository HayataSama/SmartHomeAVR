#include "include/main.h"
#include "include/lcd.h"
#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

LCD lcd;
State currentState;
State lastState;
Vars vars;
char buffer[16];
char menu[5][16 + 1] = {
    // +1 is for '\0' null terminator
    "1.Change Pass", "2.Temp Thresh", "3.Motor Speed",
    "4.Set Time",    "5.Set Alarm",
};

void filler() {
  for (int i = 0; i < sizeof(menu) / sizeof(menu[0]); i++) {
    for (int j = strlen(menu[i]); j < 16; j++) {
      menu[i][j] = ' ';
      menu[i][16] = '\0';
    }
  }
}

void addCursor(int i) {
  char *c = menu[i] + strlen(menu[i]) - 1;
  *c = '<';
  c--;
  *c = '<';
}

void removeCursor(int i) {
  char *c = menu[i] + strlen(menu[i]) - 1;
  *c = ' ';
  c--;
  *c = ' ';
}

int main(void) {
  // set default state of program
  currentState = STATUS;
  lastState = NOSTATE;
  vars.temp = 25;
  vars.motorOn = 0;
  vars.maxSpeed = 50;

  // initialize lcd
  lcd = lcdInit(DDB0, DDB1, DDD4, DDD5, DDD6, DDD7, 16, 2, LCD_5x8DOTS);

  // create menu items
  filler();

  while (1) {
    lcdSetCursor(lcd, 1, 0);
    removeCursor(1);
    lcdPrint(lcd, menu[1]);
    lcdSetCursor(lcd, 0, 0);
    addCursor(0);
    lcdPrint(lcd, menu[0]);
    _delay_ms(1000);

    lcdSetCursor(lcd, 0, 0);
    removeCursor(0);
    lcdPrint(lcd, menu[0]);
    lcdSetCursor(lcd, 1, 0);
    addCursor(1);
    lcdPrint(lcd, menu[1]);
    _delay_ms(1000);
  }

  // while (1) {
  //   if (currentState != lastState) {
  //     switch (currentState) {
  //     case STATUS:
  //       lcdClear(lcd);
  //       lcdSetCursor(lcd, 0, 4);
  //       lcdPrint(lcd, "Welcome");
  //       lcdSetCursor(lcd, 1, 0);
  //       sprintf(buffer, "Temp:%dC Motor:%d", vars.temp, vars.motorOn);
  //       lcdPrint(lcd, buffer);
  //       lastState = currentState;
  //       break;

  //     default:
  //       break;
  //     }
  //   }
  // }

  return 0;
}