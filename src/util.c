#include <string.h>

// fill empty chars in a fixed-size string with filler
void filler(char *str, size_t size, char filler) {
  for (int i = strlen(str); i < size - 1; i++) {
    str[i] = filler;
  }
  str[size - 1] = '\0';
}

// add "<<" cursor to the end of an string
void addCursor(char *str) {
  char *c = str + strlen(str) - 1;
  *c = '<';
  c--;
  *c = '<';
}

// remove "<<" cursor at the end of an string
void removeCursor(char *str) {
  char *c = str + strlen(str) - 1;
  *c = ' ';
  c--;
  *c = ' ';
}