#include "../include/lcd.h"
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>

LCD lcdInit(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6,
            uint8_t d7, uint8_t cols, uint8_t rows, uint8_t charsize) {
  LCD lcd = {
      .rs_pin = rs,
      .enable_pin = enable,
      .data_pins = {0, 0, 0, 0, d4, d5, d6, d7},
      .displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS,
      .numlines = rows,
  };

  // set the starting DDRAM address offset for each row of the LCD
  setRowOffsets(&lcd, 0x00, 0x40, 0x00, 0x40);

  // set register select and enable pin as output
  PORTB = 0x00;
  DDRB |= (1 << rs) | (1 << enable);
  // set data pins as output
  PORTD = 0x00;
  DDRD |= (1 << d4) | (1 << d5) | (1 << d6) | (1 << d7);

  // wait 50ms before sending commands
  _delay_ms(50);

  // set LCD to 4-bit mode
  PORTB &= ~((1 << rs) | (1 << enable));
  write4bits(lcd, 0x03);
  _delay_ms(5);
  write4bits(lcd, 0x03);
  _delay_ms(5);
  write4bits(lcd, 0x03);
  _delay_us(150);
  write4bits(lcd, 0x02);

  // set number of lines, font size, etc
  sendCommand(lcd, LCD_FUNCTIONSET | lcd.displayfunction);

  // true on display with no cursor and blinking then clear it
  lcd.displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  lcdDisplayOn(&lcd);
  lcdClear(lcd);

  // set entry mode
  lcd.displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  sendCommand(lcd, LCD_ENTRYMODESET | lcd.displaymode);

  _delay_ms(50);
  return lcd;
}

void lcdHome(LCD lcd) {
  sendCommand(lcd, LCD_RETURNHOME);
  _delay_ms(2);
}

void lcdClear(LCD lcd) {
  sendCommand(lcd, LCD_CLEARDISPLAY);
  _delay_ms(2);
}

void lcdSetCursor(LCD lcd, uint8_t row, uint8_t col) {
  const size_t max_rows = sizeof(lcd.row_offsets) / sizeof(lcd.row_offsets[0]);
  if (row >= max_rows) {
    row = max_rows - 1;
  }
  if (row >= lcd.numlines) {
    row = lcd.numlines - 1;
  }
  sendCommand(lcd, LCD_SETDDRAMADDR | (col + lcd.row_offsets[row]));
}

void lcdPrint(LCD lcd, const char *str) {
  while (*str) {
    sendData(lcd, *str++);
  }
}

void lcdPrintNum(LCD lcd, uint32_t num) {
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%lu", num);
  lcdPrint(lcd, buffer);
}

void lcdDisplayOn(LCD *lcd) {
  lcd->displaycontrol |= LCD_DISPLAYON;
  sendCommand(*lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void lcdDisplayOff(LCD *lcd) {
  lcd->displaycontrol &= ~LCD_DISPLAYON;
  sendCommand(*lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void sendCommand(LCD lcd, uint8_t cmd) {
  PORTB &= ~(1 << lcd.rs_pin);
  write4bits(lcd, cmd >> 4);
  write4bits(lcd, cmd);
}

void sendData(LCD lcd, uint8_t data) {
  PORTB |= (1 << lcd.rs_pin);
  write4bits(lcd, data >> 4);
  write4bits(lcd, data);
}

static void write4bits(LCD lcd, uint8_t value) {
  PORTD = (PORTD & (~0xF0)) | ((((value >> 0) & 0x01) << lcd.data_pins[4]) |
                               (((value >> 1) & 0x01) << lcd.data_pins[5]) |
                               (((value >> 2) & 0x01) << lcd.data_pins[6]) |
                               (((value >> 3) & 0x01) << lcd.data_pins[7]));
  pulse(lcd);
}

static void pulse(LCD lcd) {
  // to set a bit LOW: AND the register with INV of the desired MASK
  // to set a bit HIGH: OR the register with the desired MASK
  PORTB &= ~(1 << lcd.enable_pin);
  _delay_us(1);
  PORTB |= (1 << lcd.enable_pin);
  _delay_us(1);
  PORTB &= ~(1 << lcd.enable_pin);
  _delay_us(100);
}

static void setRowOffsets(LCD *lcd, uint8_t row0, uint8_t row1, uint8_t row2,
                          uint8_t row3) {
  lcd->row_offsets[0] = row0;
  lcd->row_offsets[1] = row1;
  lcd->row_offsets[2] = row2;
  lcd->row_offsets[3] = row3;
}