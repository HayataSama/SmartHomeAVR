#include "include/lcd.h"
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

LCD myLCD;

int main(void) {
  myLCD = lcdInit(DDB0, DDB1, DDD4, DDD5, DDD6, DDD7, 16, 2, LCD_5x8DOTS);

  while (1) {
  }
  return 0;
}