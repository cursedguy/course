#ifndef STRUCTURES_H
#define STRUCTURES_H

typedef struct {
    int width;
    int repaint;
    int info;
    int frame;
    int color;
    int to;
    int frameType;
    int rect;
    int thick;
    png_bytep frameColor;
    png_bytep colorTo;
    png_bytep colorFrom;
    char *fileRead;
    char *fileWrite;
} Opts;

typedef struct {
    int x1;
    int y1;
    int x2;
    int y2;
} Rectangle_t;

#endif
