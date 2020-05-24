#ifndef PNG_IMAGE_H
#define PNG_IMAGE_H

#include <png.h>
#include <stdlib.h>
#include "structures.h"

#define GRADIENT 1
#define LINES 2
#define IN_RECTANGLE 1
#define CHECKED 2

typedef struct {
    int width, height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep *row_pointers;
} Png;

Png* readPng(const char*);
int isMatch(png_bytep, png_bytep);
void changeColor(png_bytep, png_bytep, Png*);
void printInfo(Png*);
void rectangles(int, png_bytep, png_bytep, Png*);
void setPixel(png_bytep, png_bytep);
void drawLine(int, int, int, int, png_bytep, Png*);
void drawLineThick(int, int, int, int, int, png_bytep, Png*);
void writePng(const char*, Png*);
void frameLine(png_bytep, int, Png*);
void gradient(png_bytep, int, Png*);

#endif
