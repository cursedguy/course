#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include "../include/png_image.h"
#include "../include/structures.h"

static struct option longOpts[] = {
        {"info",      no_argument,       NULL, 'i'},
        {"help",      no_argument,       NULL, 'h'},
        {"rect",      no_argument,       NULL, 'r'},
        {"color",     no_argument,       NULL, 'c'},
        {"paint",     no_argument,       NULL, 'p'},
        {"width",     required_argument, NULL, 's'},
        {"write",     required_argument, NULL, 'w'},
        {"frame",     required_argument, NULL, 'F'},
        {"from",      required_argument, NULL, 'f'},
        {"to",        required_argument, NULL, 't'},
        {NULL,        0,                 NULL,  0}
};

void printHelp() {
    printf("Справка по использованию программы: \n"
           "По умолчанию изображение будет сохранено в тот же файл, из которого оно было считано."
           " Для выбора другого файла, пожалуйста, воспользуйтесь флагом -w / --write.\n"
           "Информация об изображении:                         -i / --info\n"
           "Справка:                                           -h / --help\n"
           "Выбор цвета:                                       -c / --color\n"
           "Цвет, который требуется заменить:                  -f / --from\n"
           "Цвет, на который требуется заменить:               -t / --to\n"
           "Поиск всех прямоугольников заданного цвета:        -r / --rect\n"
           "Замена всех пикселей одного цвета на другой:       -p / --paint\n"
           "Рамка вокруг изображения:                          -F / --frame\n"
           "Толщина рамки:                                     -s / --width\n"
           "Толщина прямой для обводки прямоугольников:        -T\n"
           "Цвет для обводки прямоугольников выбирается флагом -c, например: -r --color -f white -t navy\n"
           "Доступные цвета:         Доступные рамки: \n"
           "red                      1 - градиент\n"
           "green                    2 - выколотые точки\n"
           "blue\n"
           "navy\n"
           "aqua\n"
           "white\n"
           "ivory\n");
}

void cleanMemory(Png *image, Opts *options) {
    for (int y = 0; y < image->height; y++)
        free(image->row_pointers[y]);
    free(image->row_pointers);
    png_destroy_read_struct(&image->png_ptr, NULL, NULL);
    free(image->info_ptr);
    free(image);
    if (options->colorFrom)
        free(options->colorFrom);
    if (options->colorTo)
        free(options->colorTo);
    if (options->frameColor)
        free(options->frameColor);
}

png_bytep initColor(const char *name) {
    png_bytep color = calloc(3, sizeof(unsigned int));
    if (!name) {
        free(color);
        return NULL;
    }
    if (!strcmp(name, "red")) {
        color[0] = 255;
        return color;
    }
    if (!strcmp(name, "green")) {
        color[1] = 255;
        return color;
    }
    if (!strcmp(name, "blue")) {
        color[2] = 255;
        return color;
    }
    if (!strcmp(name, "navy")) {
        color[2] = 128;
        return color;
    }
    if (!strcmp(name, "white")) {
        color[0] = 255;
        color[1] = 255;
        color[2] = 255;
        return color;
    }
    if (!strcmp(name, "aqua")) {
        color[1] = 255;
        color[2] = 255;
        return color;
    }
    if (!strcmp(name, "ivory")) {
        color[0] = 255;
        color[1] = 255;
        color[2] = 240;
        return color;
    }
    free(color);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printHelp();
        return 0;
    }

    const char *shortOpts = "irchF:s:p:w:f:t:d:C:T:";
    int opt = -1;
    int longIndex = 0;

    Opts options;
    memset(&options, 0, sizeof(Opts));
    options.thick = 1;

    while ((opt = getopt_long(argc, argv, shortOpts, longOpts, &longIndex)) != -1) {
        switch(opt) {
            case 'T':
                if (sscanf(optarg, "%d", &options.thick) != 1) {
                    printHelp();
                    return 1;
                }
                break;
            case 'r':
		if (!argv[optind]) {
                    printHelp();
                    return 1;
                }
                if (!strcmp(argv[optind], "--color") && !strcmp(argv[optind], "-c")) {
                    printHelp();
                    return 1;
                }
                options.rect = 1;
                break;
            case 'F':
                if (sscanf(optarg, "%d", &options.frameType) != 1) {
                    printHelp();
                    return 1;
                }
                if (options.frameType != GRADIENT && options.frameType != LINES) {
                   printHelp();
                   return 1;
                }
                options.frame = 1;
                break;
            case 'w':
                if (!optarg) {
                    printHelp();
                    return 1;
                }
                options.fileWrite = optarg;
                break;
            case 'p':
		if (!argv[optind]) {
                    printHelp();
                    return 1;
                }
                if (!strcmp(argv[optind], "--color") && !strcmp(argv[optind], "-c")) {
                    printHelp();
                    return 1;
                }
                options.color = 1;
                options.repaint = 1;
                break;
            case 's':
                if (sscanf(optarg, "%d", &options.width) != 1) {
                    printHelp();
                    return 1;
                }
                break;
            case 'i':
                options.info = 1;
                break;
            case 'C':
                if (!options.frame) {
                    printHelp();
                    return 1;
                }
                options.frameColor = initColor(optarg);
                if (!options.frameColor) {
                    printHelp();
                    return 1;
                }
                break;
            case 'c':
                if (!options.repaint && !options.rect) {
                    printHelp();
                    return 1;
                }
                options.color = 1;
                break;
            case 'f':
                if (!options.color && !optarg) {
                    printHelp();
                    return 1;
                }
                options.colorFrom = initColor(optarg);
                if (!options.colorFrom) {
                    printHelp();
                    return 1;
                }
                options.to = 1;
                break;
            case 't':
                if (!options.to || !optarg) {
                    printHelp();
                    return 1;
                }
                options.colorTo = initColor(optarg);
                if (!options.colorTo) {
                    printHelp();
                    free(options.colorFrom);
                    return 1;
                }
		break;
	    case '?':
	    case 'h':
            default:
		printHelp();
                return 1;
        }
    }

    const char *savePath = argv[argc-1];
    Png *image = readPng(savePath);
    if (!image) {
        printf("Изображение не удалось считать.\n");
        printHelp();
        return 1;
    }

    if (options.repaint)
        changeColor(options.colorTo, options.colorFrom, image);
    if (options.info)
        printInfo(image);
    if (options.frame) {
        switch (options.frameType) {
            case GRADIENT:
                gradient(options.frameColor, options.width, image);
                break;
            case LINES:
                frameLine(options.frameColor, options.width, image);
                break;
            default:
                printHelp();
                break;
        }
    }
    if (options.rect && options.colorTo && options.colorFrom)
        rectangles(options.thick, options.colorTo, options.colorFrom, image);
    if (options.fileWrite)
        savePath = options.fileWrite;

    writePng(savePath, image);
    cleanMemory(image, &options);
    return 0;
}
