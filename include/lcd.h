#ifndef lcd_h
#define lcd_h

#include <inttypes.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

typedef struct {
  uint8_t rs_pin;       // LOW: command. HIGH: character.
  uint8_t enable_pin;   // activated by a HIGH pulse.
  uint8_t data_pins[8]; // only data_pin[4] to data_pin[7] is used

  uint8_t displayfunction; // mode, no. of rows, font
  uint8_t displaycontrol;  // on/off, cursor, blink
  uint8_t displaymode;     // ltr/rtl

  uint8_t numlines;       // no. of rows
  uint8_t row_offsets[4]; // DDRAM address offsets
} LCD;

// initialize LCD
LCD lcdInit(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6,
            uint8_t d7, uint8_t cols, uint8_t rows, uint8_t charsize);

// return cursor to home (0, 0)
void lcdHome(LCD lcd);

// clear display
void lcdClear(LCD lcd);

// move cursor to (row, col)
void lcdSetCursor(LCD lcd, uint8_t row, uint8_t col);

// print an string
void lcdPrint(LCD lcd, const char *str);

// print a number
void lcdPrintNum(LCD lcd, uint32_t num);

// turn on display
void lcdDisplayOn(LCD *lcd);

// turn off display without losing data
void lcdDisplayOff(LCD *lcd);

// send LCD commands
void sendCommand(LCD lcd, uint8_t cmd);

// send 1 byte of data
void sendData(LCD lcd, uint8_t data);

// send 4 bits (used by sendData())
static void write4bits(LCD lcd, uint8_t value);

// pulse EN pin to let LCD know of new incoming data/command
static void pulse(LCD lcd);

// set the starting DDRAM address offset for each row of the LCD
static void setRowOffsets(LCD *lcd, uint8_t row0, uint8_t row1, uint8_t row2,
                          uint8_t row3);

#endif