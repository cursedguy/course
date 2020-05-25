#include "../include/png_image.h"

Png* readPng(const char *filePath) {
    FILE *pFile = fopen(filePath, "rb");
    if (!pFile) {
        printf("Не удалось открыть файл для считывания.\n");
        return NULL;
    }

    png_byte header[8];
    fread(header, sizeof(png_byte), 8, pFile);
    if (png_sig_cmp(header, 0, 8)) {
        fclose(pFile);
        return NULL;
    }

    Png *image = malloc(sizeof(Png));
    image->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!image->png_ptr) {
        free(image);
        fclose(pFile);
        return NULL;
    }

    image->info_ptr = png_create_info_struct(image->png_ptr);
    if (!image->info_ptr) {
        png_destroy_read_struct(&image->png_ptr, NULL, NULL);
        fclose(pFile);
        free(image);
        return NULL;
    }

    if (setjmp(png_jmpbuf(image->png_ptr))) {
        png_destroy_read_struct(&image->png_ptr, NULL, NULL);
        fclose(pFile);
        free(image);
        return NULL;
    }

    png_init_io(image->png_ptr, pFile);
    png_set_sig_bytes(image->png_ptr, 8);

    png_read_info(image->png_ptr, image->info_ptr);

    image->width = png_get_image_width(image->png_ptr, image->info_ptr);
    image->height = png_get_image_height(image->png_ptr, image->info_ptr);
    image->color_type = png_get_color_type(image->png_ptr, image->info_ptr);
    image->bit_depth = png_get_bit_depth(image->png_ptr, image->info_ptr);

    image->number_of_passes = png_set_interlace_handling(image->png_ptr);
    png_read_update_info(image->png_ptr, image->info_ptr);

    if (setjmp(png_jmpbuf(image->png_ptr))) {
        png_destroy_read_struct(&image->png_ptr, NULL, NULL);
        fclose(pFile);
        free(image);
        return NULL;
    }

    if (image->bit_depth != 8 || (image->color_type != PNG_COLOR_TYPE_RGB &&
			    image->color_type != PNG_COLOR_TYPE_RGBA)) {
	printf("Данное изображение не поддерживается.\n");
        png_destroy_read_struct(&image->png_ptr, NULL, NULL);
        fclose(pFile);
        return NULL;
    }

    image->row_pointers = malloc(sizeof(png_bytep) * image->height);
    for (int y = 0; y < image->height; y++)
        image->row_pointers[y] = malloc(png_get_rowbytes(image->png_ptr, image->info_ptr));

    png_read_image(image->png_ptr, image->row_pointers);

    fclose(pFile);
    return image;
}

void writePng(const char *filePath, Png *image) {
    if (!image || !filePath)
        return;

    FILE *pFile = fopen(filePath, "wb");
    if (!pFile)
        return;

    png_structp pngStructPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
            NULL, NULL, NULL);
    if (!pngStructPtr) {
        fclose(pFile);
        return;
    }

    png_infop pngInfoStructPtr = png_create_info_struct(pngStructPtr);
    if (!pngInfoStructPtr) {
        png_destroy_write_struct(&pngStructPtr, NULL);
        fclose(pFile);
        return;
    }

    if (setjmp(png_jmpbuf(pngStructPtr))){
        png_destroy_write_struct(&pngStructPtr, &pngInfoStructPtr);
        fclose(pFile);
        return;
    }

    png_init_io(pngStructPtr, pFile);

    if (setjmp(png_jmpbuf(pngStructPtr))){
        png_destroy_write_struct(&pngStructPtr, &pngInfoStructPtr);
        fclose(pFile);
        return;
    }

    png_set_IHDR(pngStructPtr, pngInfoStructPtr, image->width, image->height, 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(pngStructPtr, pngInfoStructPtr);

    if (setjmp(png_jmpbuf(pngStructPtr))){
        png_destroy_write_struct(&pngStructPtr, &pngInfoStructPtr);
        fclose(pFile);
        return;
    }

    png_write_image(pngStructPtr, image->row_pointers);

    if (setjmp(png_jmpbuf(pngStructPtr))){
        png_destroy_write_struct(&pngStructPtr, &pngInfoStructPtr);
        fclose(pFile);
        return;
    }

    png_write_end(pngStructPtr, NULL);
    png_destroy_write_struct(&pngStructPtr, &pngInfoStructPtr);
    fclose(pFile);
}

int isMatch(png_bytep colorA, png_bytep colorB) {
    if (!colorA || !colorB)
        return 0;
    for (int i = 0; i < 3; i++)
        if (colorA[i] != colorB[i])
            return 0;
    return 1;
}

void setPixel(png_bytep pixel, png_bytep color) {
    for (int i = 0; i < 3; i++)
        pixel[i] = color[i];
}

void changeColor(png_bytep colorTo, png_bytep colorFrom, Png *image) {
    for (int y = 0; y < image->height; y++)
        for (int x = 0; x < image->width; x++) {
            png_bytep px = &(image->row_pointers[y][x*4]);
            if (isMatch(px, colorFrom))
                setPixel(px, colorTo);
        }
}

void drawLine(int x1, int y1, int x2, int y2, png_bytep color, Png* image) {
    const int deltaX = abs(x2 - x1);
    const int deltaY = abs(y2 - y1);
    const int signX = x1 < x2 ? 1 : -1;
    const int signY = y1 < y2 ? 1 : -1;
    int error = deltaX - deltaY;

    setPixel(&(image->row_pointers[y2][x2*4]), color);
    while (x1 != x2 || y1 != y2) {
        setPixel(&(image->row_pointers[y1][x1*4]), color);
        const int error2 = error * 2;
        if (error2 > -deltaY) {
            error -= deltaY;
            x1 += signX;
        }
        if (error2 < deltaX) {
            error += deltaX;
            y1 += signY;
        }
    }
}

void drawLineThick(int x1, int y1, int x2, int y2, int thick, png_bytep color, Png* image) {
    if (x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0 || x1 > image->width || x2 > image->width || y2 > image->height ||
        y1 > image->height || x1-thick/2 < 0 || x2-thick/2 < 0 || y1-thick/2 < 0 || y2-thick/2 < 0 ||
        x1+thick/2 > image->width || x2+thick/2 > image->width || y1+thick/2 > image->height ||
        y2+thick/2 > image->height)
        return;
    if (abs(y2-y1) > abs(x2-x1))
        for (int i = -thick/2; i < thick/2; i++)
            drawLine(x1+i, y1, x2+i, y2, color, image);
    else
        for (int i = -thick/2; i < thick/2; i++)
            drawLine(x1, y1+i, x2, y2+i, color, image);
}

void gradient(png_bytep color, int width, Png *image) {
    if (width <= 0 || !color || width >= image->height ||
        width >= image->width) {
        printf("Неверные аргументы для создания рамки.\n");
        return;
    }

    for (int y = 0; y < width; y++)
        for (int x = 0; x < image->width; x++) {
            double pos = (double)y/image->height;
            color[0] = pos * 255;
            setPixel(&(image->row_pointers[y][x*4]), color);
        }

    for (int y = width; y < image->height; y++)
        for (int x = 0; x < width; x++) {
            double pos = (double)y/image->height;
            color[0] = pos * 255;
            setPixel(&(image->row_pointers[y][x*4]), color);
        }

    for (int y = image->height-width; y < image->height; y++)
        for (int x = width; x < image->width; x++) {
            double pos = (double)y/image->height;
            color[0] = pos * 255;
            setPixel(&(image->row_pointers[y][x*4]), color);

        }

    for (int y = width; y < image->height-width; y++)
        for (int x = image->width-width; x < image->width; x++) {
            double pos = (double)y/image->height;
            color[0] = pos * 255;
            setPixel(&(image->row_pointers[y][x*4]), color);
        }
}

void frameLine(png_bytep color, int width, Png *image) {
    if (width <= 0 || !color || width >= image->height ||
        width >= image->width) {
        printf("Неверные аргументы для создания рамки.\n");
        return;
    }

    for (int y = 0; y < width; y++)
        for (int x = 0; x < image->width; x++) {
            if (y % 2)
                setPixel(&(image->row_pointers[y][x*4]), color);
            if (x % 2) {
                double pos = (double) y / image->height;
                color[0] = pos * 255;
                setPixel(&(image->row_pointers[y][x*4]), color);
            }
        }

    for (int y = width; y < image->height; y++)
        for (int x = 0; x < width; x++) {
            if (y % 2)
                setPixel(&(image->row_pointers[y][x*4]), color);
            if (x % 2) {
                double pos = (double) y / image->height;
                color[0] = pos * 255;
                setPixel(&(image->row_pointers[y][x*4]), color);
            }
        }

    for (int y = image->height-width; y < image->height; y++)
        for (int x = width; x < image->width; x++) {
            if (y % 2)
                setPixel(&(image->row_pointers[y][x*4]), color);
            if (x % 2) {
                double pos = (double) y / image->height;
                color[0] = pos * 255;
                setPixel(&(image->row_pointers[y][x*4]), color);
            }
        }

    for (int y = width; y < image->height-width; y++)
        for (int x = image->width-width; x < image->width; x++) {
            if (y % 2)
                setPixel(&(image->row_pointers[y][x*4]), color);
            if (x % 2) {
                double pos = (double) y / image->height;
                color[0] = pos * 255;
                setPixel(&(image->row_pointers[y][x*4]), color);
            }
        }
}

void rectangles(int thick, png_bytep colorTo, png_bytep colorFrom, Png* image) {
    if (!image || !colorTo || !colorFrom || thick <= 0 || thick >= 10) {
        printf("Неверные аргументы для обводки прямоугольников.\n");
        return;
    }

    int height = image->height;
    int width = image->width;
    int count = 0;

    Rectangle_t *rectangles = malloc(sizeof(Rectangle_t));

    int **array = calloc(height, sizeof(int*));
    for (int i = 0; i < height; i++)
        array[i] = calloc(width, sizeof(int));

    for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
            for (int r = row; r < height && isMatch(&(image->row_pointers[r][col*4]), colorFrom); r++)
                array[r][col] = 1;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            if (array[i][j] == IN_RECTANGLE) {
                int flag = 0;
                int y = i, x = j;

		while (array[y][x] == IN_RECTANGLE && x+1 < width) {
                    if (array[y][x+1] == CHECKED) flag = 1;
                    array[y][x++] = CHECKED;
                }

                if (flag) continue;
                x--;
                y++;
                if (y >= height) continue;

                while (array[y][x] == IN_RECTANGLE && y+1 < height) {
                    if (array[y+1][x] == CHECKED) flag = 1;
                    array[y++][x] = CHECKED;
                }
                if (flag) continue;
		    
                rectangles[count].x1 = j, rectangles[count].y1 = i;
                rectangles[count].x2 = x, rectangles[count++].y2 = y;
                rectangles = realloc(rectangles, (count+1) * sizeof(Rectangle_t));
            }

    for (int i = 0; i < count; i++) {
        drawLineThick(rectangles[i].x1, rectangles[i].y1,
                      rectangles[i].x2, rectangles[i].y1, thick, colorTo, image);
        drawLineThick(rectangles[i].x1, rectangles[i].y1,
                      rectangles[i].x1, rectangles[i].y2, thick, colorTo, image);
        drawLineThick(rectangles[i].x1, rectangles[i].y2,
                      rectangles[i].x2, rectangles[i].y2, thick, colorTo, image);
        drawLineThick(rectangles[i].x2, rectangles[i].y1,
                      rectangles[i].x2, rectangles[i].y2, thick, colorTo, image);
    }

    for (int i = 0; i < height; i++)
        free(array[i]);
    free(array);
    free(rectangles);
}

void printInfo(Png *image) {
    if (!image)
        return;
    printf(""
           "Информация о считанном файле: \n"
           "Тип файла: PNG изображение\n"
           "Ширина картинки(в пикселях): %d\n"
           "Высота картинки(в пикселях): %d\n"
           "Глубина цвета: %d\n", image->width, image->height, image->bit_depth);
}
