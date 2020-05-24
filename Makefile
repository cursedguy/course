SRC = source/
CC = gcc
CFLAGS = -Wall -lpng
EXECUTABLE = png_edit

all: $(EXECUTABLE)

$(EXECUTABLE): $(SRC)main.c $(SRC)png_image.c
	$(CC) $(SRC)main.c $(SRC)png_image.c $(CFLAGS) -o $(EXECUTABLE)
