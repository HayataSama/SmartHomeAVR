#ifndef DISPLAY_H
#define DISPLAY_H

#include <string.h>

// fill empty chars in a fixed-size string with filler
void filler(char *str, size_t size, char filler);

// add "<<" cursor to the end of an string
void addCursor(char *str);

// remove "<<" cursor at the end of an string
void removeCursor(char *str);

#endif