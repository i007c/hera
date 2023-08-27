
#include "hera.h"
#include "logger.h"
#define LS SECTOR_MAIN_HERA

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <fcntl.h>

#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <X11/Xft/Xft.h>
// #include <X11/cursorfont.h>
// #include <X11/Xatom.h>
// #include <X11/Xresource.h>


// X context
typedef struct xctx_t {
    Display *dpy;
    int screen;
    int dpy_width;
    int dpy_height;
    Visual *visual;
    Colormap color_map;
    int depth;
    Window win;
} xctx_t;

bool running = true;

void inflate(uint8_t *buffer, size_t length, uint8_t *output);

void draw(uint8_t *rgb_out, int w, int h) {
    int i = 0;

    for (i = 0; i < w * h; i += 4) {
        rgb_out[i] = 255; // blue
        rgb_out[i + 1] = 0; // green
        rgb_out[i + 2] = 255; // red
        // rgb_out[i + 3] = 10; // alpha
    }
}

XImage *create_ximage(
    Display *display, Visual *visual,
    int width, int height, uint8_t *image
) {
    // char *image32 = (char *)malloc(width * height * 4);
    // draw((uint8_t *)image32, width, height);

    return XCreateImage(
        display, visual, 24,
        ZPixmap, 0, (char *)image,
        width, height, 32, 0
    );
}

uint32_t u32(void *data) {
    uint8_t a = ((uint8_t *)data)[0];
    uint8_t b = ((uint8_t *)data)[1];
    uint8_t c = ((uint8_t *)data)[2];
    uint8_t d = ((uint8_t *)data)[3];

    return (a << 24) + (b << 16) + (c << 8) + (d);
}

uint16_t u16(void *data) {
    uint8_t a = ((uint8_t *)data)[0];
    uint8_t b = ((uint8_t *)data)[1];

    return (a << 8) + (b);
}

int main(int argc, char **argv) {
    log_info("--- Hera ---");

    // char filename[256] = "./sample/png/technoblade.png";
    char filename[256] = "./sample/png/x1.4019952.png";
    // char filename[256] = "./sample/png/x8.4171534.png";
    // char filename[256] = "./sample/png/nn.png";
    // char filename[256] = "./sample/png/x7.4162641.png";
    // char filename[256] = "./sample/r/4191220.png";

    if (argc > 1) {
        strncpy(filename, argv[1], 256);
    }

    uint8_t *idat_buffer = NULL;
    uint32_t idat_length = 0;

    // char *filename = "./sample/blade.jpg";
    // char *filename = "./sample/technoblade.png";
    // char *filename = "./test.png";
    // char *filename = "./sample/cat.gif";

    log_verbose("filename: "SFMT, filename);

    uint8_t *buffer = NULL;

    int fd = open(filename, O_RDONLY);
    off_t filesize = lseek(fd, 0, SEEK_END);
    buffer = malloc(filesize);
    lseek(fd, 0, SEEK_SET);
    read(fd, buffer, filesize);

    // uint8_t *data = malloc();


    const uint8_t PNG_SIGNATURE[8] = {
        0x89, 0x50, 0x4E, 0x47,
        0x0D, 0x0A, 0x1A, 0x0A
    };

    for (off_t x = 0; x < 8; x++) {
        if (buffer[x] != PNG_SIGNATURE[x]) 
            panic("invalid png signature");
    }


    // uint32_t g = u32(&buffer[8]);
    // printf("g: %d\n", g);
    // g = get_u32(&g);
    // printf("g: %d - %ld\n", g, sizeof(u32));

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t bit_depth = 0;
    uint8_t color_type = 0;
    uint8_t cb = 4;
    uint8_t compression = 0;
    uint8_t filter = 0;
    uint8_t interlace = 0;

    uint8_t *image_output = NULL;

    log_info("filesize: %d", filesize);

    for (off_t i = 8; i < filesize;) {
        uint32_t length = u32(&buffer[i]);
        i += 4;

        uint32_t type = u32(&buffer[i]);
        i += 4;

        switch (type) {
            case 0x49484452: // IHDR
                if (length != 13) {
                    log_error(
                        "invalid IHDR length: %d != 13", length
                    );
                    return 1;
                }

                width = u32(&buffer[i]); i += 4;
                height = u32(&buffer[i]); i += 4;
                bit_depth = buffer[i]; i++;
                color_type = buffer[i]; i++;
                assert(color_type == 2 || color_type == 6);
                cb = color_type == 2 ? 3 : 4;
                compression = buffer[i]; i++;
                filter = buffer[i]; i++;
                interlace = buffer[i]; i++;

                image_output = malloc(width * height * cb + height);
                assert(image_output != NULL);

                log_verbose(
                    "\nwidth: %d\nhieght: %d\nwidth * height * %d + height: "DFMT
                    "\nbit_depth: %d\ncolor_type: %d\n"
                    "compression: %d\nfilter: %d\ninterlace: %d",
                    width, height, cb, width * height * cb + height, bit_depth,
                    color_type, compression,
                    filter, interlace
                );

                break;
            case 0x69434350: // iCCP
                log_verbose("iCCP: length: %d", length);
                /*
                char iccp_name[80];
                uint8_t j = 0;
                for (; j < 80; j++) {
                    iccp_name[j] = buffer[i + j];
                    if (!iccp_name[j]) break;
                }
                uint8_t icc_compression = buffer[i+j+1];
                log_verbose("name: "SFMT", len: %d", iccp_name, j);
                log_verbose("icc_compression: %d", icc_compression);

                off_t n = 0;
                for (off_t h=0; h < length-(j + 2); h++) {

                    if (!buffer[i + j + 2 + h])
                        printf("\033[31m00\033[0m ");
                    else
                        printf("%02x ", buffer[i + j + 2 + h]);

                    n++;
                    if (n > 20) {
                        printf("\n");
                        n = 0;
                    }
                }

                printf("\n"); */

                i += length;
                break;
            case 0x624b4744: // bKGD
                switch (color_type) {
                    case 4:
                    case 0: {
                        uint16_t bg = u16(&buffer[i]);
                        log_verbose("bKGD: gray scale: %d", bg);
                    } break;

                    case 6:
                    case 2: {
                        uint16_t red = u16(&buffer[i]);
                        uint16_t green = u16(&buffer[i + 2]);
                        uint16_t blue = u16(&buffer[i + 4]);

                        log_verbose(
                            "bKGD rgb: %d %d %d",
                            red, green, blue
                        );
                    } break;

                    case 3:
                        log_verbose("bKGD: palette index: %d", buffer[i]);
                        break;

                    default:
                        log_warn("invalid color type");
                        break;
                }
                i += length;
                break;

            case 0x70485973: { // pHYs 
                uint32_t x_axis = u32(&buffer[i]); i += 4;
                uint32_t y_axis = u32(&buffer[i]); i += 4;
                uint8_t specifier = buffer[i]; i++;
                log_info(
                    "pHYs: x: %d, y: %d, specifier: %d",
                    x_axis, y_axis, specifier
                );
            } break;

            case 0x74494d45: { // tIME
                uint16_t year = u16(&buffer[i]); i += 2;
                uint8_t month = buffer[i]; i++;
                uint8_t day = buffer[i]; i++;
                uint8_t hour = buffer[i]; i++;
                uint8_t minute = buffer[i]; i++;
                uint8_t second = buffer[i]; i++;

                log_info(
                    "tIME: %d-%02d-%02d %02d:%02d:%02d",
                    year, month, day, hour, minute, second
                );
            } break;

            case 0x49444154: { // IDAT
                // log_info("WidthxHeight: %d", width * height);
                // log_info("IDAT length: %d at: %d", length, i);

                idat_length += length;
                if (idat_buffer == NULL) {
                    idat_buffer = malloc(idat_length);
                } else {
                    idat_buffer = realloc(idat_buffer, idat_length);
                }
                memcpy(&idat_buffer[idat_length - length], &buffer[i], length);


                // off_t n=0;
                // for (off_t j = 2; j < length ; j++) {
                //     uint8_t b = buffer[i + j];
                //     bool final = b & 1;
                //     uint8_t method = (b >> 1) & 3;
                //
                //     log_info("b: %d, final: %d, method: %d", b, final, method);
                //     break;
                //
                //
                //     if (!buffer[i + j])
                //         printf("\033[31m00\033[0m ");
                //     else
                //         printf("%02x ", buffer[i + j]);
                //
                //     n++;
                //     if (n > 20) {
                //         printf("\n");
                //         n = 0;
                //     }
                // }
                // printf("\n");
                i += length;
            } break;

            case 0x49454e44: { // IEND
                inflate(idat_buffer, idat_length, image_output);
                free(idat_buffer);
                log_info("-- IEND -- "DFMT, i);
                i = filesize; // break from parent loop
            } break;

            case 0x73424954: { // sBIT
                log_info("sbit length: %d", length);
                i += length;
            } break;

            case 0x65584966: { // eXIf
                log_info("eXIf length: %d", length);
                i += length;
            } break;

            case 0x73524742: { // sRGB
                log_info("sRGV length: %d", length);

                uint8_t rendering_intent = buffer[i];
                printf(
                    "Rendering Intent: %d\n0: Perceptual\n"
                    "1: Relative colorimetric\n"
                    "2: Saturation\n3: Absolute colorimetric\n",
                    rendering_intent
                );

                i += length;
            } break;

            case 0x67414d41: { // gAMA
                log_verbose("gAMA length: "DFMT, length);

                uint32_t gamma = u32(&buffer[i]);
                log_verbose("gamma value: "DFMT, gamma);

                i += length;
            } break;

            case 0x69545874: { // iTXt
                i += length;
                break;

                char *text_buffer = malloc(length);
                memcpy(text_buffer, &buffer[i], length);

                for (off_t n=0; n < length; n++) {
                    if (!text_buffer[n]) {
                        text_buffer[n] = '^';
                    } else if (text_buffer[n] == '\n') {
                    } else if (text_buffer[n] < 32) {
                        text_buffer[n] = 'X';
                    }
                }
                text_buffer[length] = 0;

                printf(
                    "iTXt length: "DFMT", content: %s\n",
                    length, text_buffer
                );

                i += length;
            } break;


            case 0x74455874: { // tEXt
                i += length;
                break;

                char *text_buffer = malloc(length);
                memcpy(text_buffer, &buffer[i], length);

                for (off_t n=0; n < length; n++) {
                    if (!text_buffer[n]) {
                        text_buffer[n] = '\n';
                    } else if (text_buffer[n] < 32) {
                        text_buffer[n] = 'X';
                    }
                }
                text_buffer[length] = 0;
                printf(
                    "tEXt length: "DFMT", content: %s\n",
                    length, text_buffer
                );

                i += length;
            } break;

            case 0x6348524d: { // cHRM
                i += length;
                break;

                char *text_buffer = malloc(length);
                memcpy(text_buffer, &buffer[i], length);

                for (off_t n=0; n < length; n++) {
                    if (!text_buffer[n]) {
                        text_buffer[n] = '~';
                    } else if (text_buffer[n] < 32) {
                        text_buffer[n] = 'X';
                    }
                }
                text_buffer[length] = 0;
                printf(
                    "cHRM length: "DFMT", content: %s\n",
                    length, text_buffer
                );

                i += length;
            } break;
            default: {
                uint8_t *ch = (uint8_t *)&type;


                printf("\033[38;2;255;0;0m--------------------\033[0m\n");
                log_error(
                    "i: "DFMT"/"DFMT", unknown type: %c%c%c%c "
                    DFMT" 0x\033[32m%x\033[0m", i, filesize,
                    ch[3], ch[2], ch[1], ch[0],
                    type, type
                );
                printf("\033[38;2;255;0;0m--------------------\033[0m\n");
                return 1;
            } break;
        }

        // ignore crc for now
        i += 4;

        // break;


        /*
IHDR 1229472850 0x49484452
PLTE 1347179589 0x504c5445
IDAT 1229209940 0x49444154
IEND 1229278788 0x49454e44
bKGD 1649100612 0x624b4744
cHRM 1665684045 0x6348524d
dSIG 1683179847 0x64534947
eXIf 1700284774 0x65584966
gAMA 1732332865 0x67414d41
hIST 1749635924 0x68495354
iCCP 1766015824 0x69434350
iTXt 1767135348 0x69545874
pHYs 1883789683 0x70485973
sBIT 1933723988 0x73424954
sPLT 1934642260 0x73504c54
sRGB 1934772034 0x73524742
sTER 1934902610 0x73544552
tEXt 1950701684 0x74455874
tIME 1950960965 0x74494d45
tRNS 1951551059 0x74524e53
zTXt 2052348020 0x7a545874
         */


        // if (!buffer[i])
        //     printf("\033[31m00\033[0m ");
        // else
        //     printf("%02x ", buffer[i]);
        // printf("%3c ", buffer[i]);
        //
        // uint32_t type = buffer[i] << 24;
        // type += buffer[i + 1] << 16;
        // type += buffer[i + 2] << 8;
        // type += buffer[i + 3];
        //
        // // uint32_t type = *(uint32_t *)&buffer[i];
        // printf("type: 0x%x - %c %c %c %c\n", type, buffer[i], buffer[i+1], buffer[i+2], buffer[i+3]);
        // printf("type: 0x%x == 0x49484452\n", type);
        // break;

        // switch (type) {
        //     case 1
        // }


    }

    printf("\n");
    free(buffer);

    uint8_t *image_data = malloc(width * height * cb);

    uint64_t n = 0;
    uint64_t m = 0;

    // for (uint64_t i=0; i < width * height * 4 + height; i++) {
    //
    // }



    // uint8_t k = 0;
    for (uint32_t iy=0; iy < height; iy++) {
        // printf("[%d]: ", iy);
        uint8_t filter_type = 0;

        for (uint32_t ix=0; ix < width * cb + 1; ix++) {
            if (ix == 0) {
                filter_type = image_output[n];
                // if (filter_type)
                //     printf("[%ld] filter: %d\n", n, image_output[n]);
            } else {
                // image_data[m] = image_output[n];

                switch (filter_type) {
                    case 0: image_data[m] = image_output[n]; break;
                    case 1: {
                        // image_data[m] = image_output[n];
                        // if (!image_data[m]) image_data[m] = image_data[m-4];
                        // printf("%d ",image_data[m-1]);
                        if (ix < cb+1u)
                            image_data[m] = image_output[n];
                        else
                            image_data[m] = image_output[n] + image_data[m-cb];
                    } break;
                    case 2: {
                        if (iy > 0)
                            image_data[m] = image_output[n] + image_data[m - width * cb];
                        else
                            image_data[m] = image_output[n];

                            
                        // if (!image_data[m])
                        //     image_data[m] = image_data[m-width*4];

                    } break;
                    // case 1: image_data[m] = image_output[n] + image_data[m - 1]; break;
                    // case 2: image_data[m] = image_output[n] + image_data[m - width]; break;
                    case 3: {
                        uint8_t a = image_data[m - cb];
                        uint8_t b = image_data[m - width * cb];

                        if (ix < (cb+1u)) a = 0;
                        if (iy < 1) b = 0;

                        // if (ix < 1 || iy < 1) {
                        //     image_data[m] = image_output[n];
                        //     break;
                        // }
                        image_data[m] = image_output[n] + ((a + b) >> 1);
                        // if (iy == 0) b = 0;
                        // if (ix < 1) a = 0;

                        // if (!iy)
                        //     image_data[m] = image_output[n];
                        // image_data[m] = image_output[n] + ((a + b) >> 1);
                    } break;
                    case 4: {
                        uint8_t a, b, c;
                        a = image_data[m - cb];
                        b = image_data[m - width * cb];
                        c = image_data[m - (width * cb) - cb];


                        // if (ix < 1 || iy < 1) {
                        //     image_data[m] = image_output[n];
                        //     break;
                        // }
                        if (ix < cb+1u) {
                            a = 0;
                            c = 0;
                        }
                        if (iy < 1) {
                            b = 0; c = 0;
                        }
                        // if (iy == 0 || ix < 1) c = 0;

                        int32_t  p = a + b - c;
                        int32_t pa = abs(p - a);
                        int32_t pb = abs(p - b);
                        int32_t pc = abs(p - c);
                        int32_t pr;

                        if (pa <= pb && pa <= pc) {
                            pr = a;
                        } else if (pb <= pc) {
                            pr = b;
                        } else {
                            pr = c;
                        }
                        // if (pr)
                        // printf("pr: %d\n", pr);

                        image_data[m] = image_output[n] + pr;
                    } break;
                    // default: assert(false && "invalid filter");
                }
                // image_data[m] = image_output[n];
                m++;
                // k++;
                // if (cb == 3 && k == 3) {
                //     m++;
                //     k=0;
                // }
            }
            n++;
            // image_data[n] = image_output[iy * (width + 1) * 4 + ix + 2]; n++;
            // image_data[n] = image_output[iy * (width + 1) * 4 + ix + 1]; n++;
            // image_data[n] = image_output[iy * (width + 1) * 4 + ix + 0]; n++;
            // image_data[n] = 255; n++;

            // image_data[n] = 0; n++;
            // image_data[n] = 255; n++;
            // image_data[n] = 12; n++;
            // image_data[n] = 0; n++;

            // image_data[iy * width * 4 + ix] = image_output[iy * (width + 1) * 4 + ix];
            // printf("%d ", iy * width + ix);
            // if (ix == 0) {
            //     printf("[%d]: %d\n", iy, image_output[iy * width * 4]);
            // } else {
            //     image_data[iy * width * 4 + ix - 1] = image_output[iy * width * 4 + ix ];
            // }
            // if (ix == 0) {
            //     printf("filter type: %d\n", image_output[sl * width]);
            // } else {
            //     image_data[n] = image_output[n+sl];
            //     n++;
            // }
        }
        // printf("\n");
    }



    uint8_t *org = image_data;
    image_data = malloc(width * height * 4);

    // uint8_t red;
    n = 0;
    for (m=0; m < width * height * cb;) {
        image_data[n] = org[m+2];   // blue
        image_data[n+1] = org[m+1]; // green
        image_data[n+2] = org[m];   // red

        if (cb == 4) {
            image_data[n+3] = org[m+3];
        } else {
            image_data[n+3] = 255;
        }

        m += cb;
        n += 4;
    }

    free(org);

    /* {{{ */
    if (false && (width > 512 || height > 512)) {
        uint8_t *org = image_data;
        uint32_t old_width = width;
        uint32_t old_height = height;

        if (old_width > old_height) {
            width = 1024;
            height = 1024 * (double)old_height / old_width;
        } else {
            height = 1024;
            width = 1024 * (double)old_width / old_height;
        }

        uint8_t ratio = 10;
        image_data = malloc(width * height * 4);
        printf(
            "width: %d - height: %d - ratio: %d\n",
            width, height, ratio
        );

        m = 0;
        n = 0;
        for (uint64_t iy=0; iy < height; iy++) {
            for (uint64_t ix=0; ix < width; ix++) {
                image_data[(iy * 4) * width + (ix * 4)] = org[(iy * 4 * ratio) * width + (ix * 4 * ratio)];
                image_data[(iy * 4) * width + (ix * 4) + 1] = org[(iy * 4 * ratio) * width + (ix * 4 * ratio) + 1];
                image_data[(iy * 4) * width + (ix * 4) + 2] = org[(iy * 4 * ratio) * width + (ix * 4 * ratio) + 2];
                // image_data[m + 1] = org[n + 1];
                // image_data[m + 2] = org[n + 2];
                // image_data[m + 3] = org[n + 3];
                // image_data[m+1] = 255;
                // m += 4;
                // n += ratio * ratio + 4;
                // image_data[iy * width + ix] = org[iy * width * ratio + ix];
                // image_data[iy * width + ix + 1] = org[iy * width * ratio + ix + 1];
                // image_data[iy * width + ix + 2] = org[iy * width * ratio + ix + 2];
                // image_data[iy * width + ix + 3] = org[iy * width * ratio + ix + 3];
                // image_data[iy * width + ix] = org[iy * width * ratio + ix];
                // image_data[iy * width + ix] = org[iy * width * ratio + ix];
                // image_data[iy * width + ix] = org[iy * width * ratio + ix];
                // image_data[m+1] = org[m + 1];
                // image_data[m+2] = org[m + 2];
                // image_data[m+3] = org[m + 3];
                //
                // m += 4;
            }
        }
        free(org);
    }
    /* }}} */



    free(image_output);


    // return 0;

    xctx_t xctx;
    memset(&xctx, 0, sizeof(xctx_t));

    if ((xctx.dpy = XOpenDisplay(NULL)) == NULL)
        panic("con't open display :(");


    xctx.screen = DefaultScreen(xctx.dpy);
    xctx.dpy_width = DisplayWidth(xctx.dpy, xctx.screen);
    xctx.dpy_height = DisplayHeight(xctx.dpy, xctx.screen);
    xctx.visual = DefaultVisual(xctx.dpy, xctx.screen);
    xctx.color_map = DefaultColormap(xctx.dpy, xctx.screen);
    xctx.depth = DefaultDepth(xctx.dpy, xctx.screen);

    int win_b_color = BlackPixel(xctx.dpy, xctx.screen);
    int win_w_color = BlackPixel(xctx.dpy, xctx.screen);

    uint32_t win_width = width > 1920 ? 1920 : width;
    uint32_t win_height = height > 1080 ? 1080 : height;


    // if (width > 1920 || height > 1080) {
    //     if (width > height) {
    //         win_width = 1920;
    //         win_height = 1920 * (double)height / width;
    //     } else {
    //         win_height = 1080;
    //         win_width = 1080 * (double)width / height;
    //     }
    // }

    // printf("width: %d - height: %d\n", win_width, win_height);

    xctx.win = XCreateSimpleWindow(
        xctx.dpy,
        XDefaultRootWindow(xctx.dpy),
        (1920 - win_width) / 2, (1080 - win_height) / 2,
        win_width, win_height,
        0, win_b_color, win_w_color
    );

    XSizeHints sh;
    sh.min_width  = sh.max_width  = win_width;
    sh.min_height = sh.max_height = win_height;
    sh.flags = PMinSize | PMaxSize;
    XSetWMNormalHints(xctx.dpy, xctx.win, &sh);

    XStoreName(xctx.dpy, xctx.win, "Hera");
    XMapWindow(xctx.dpy, xctx.win);
    XSelectInput(
        xctx.dpy, xctx.win, 
        KeyPressMask | ExposureMask | LeaveWindowMask
    );
    XFlush(xctx.dpy);

    GC gc = XCreateGC(xctx.dpy, xctx.win, 0, NULL);

    // uint8_t data[] = { 250, 0, 0, 250, 0, 0, 250, 0, 0, 250, 0, 0 };

    // XImage *img = XCreateImage(
    //     xctx.dpy, xctx.visual, xctx.depth, ZPixmap, 0,
    //     (char *)data, 2, 2, 32, 0
    // );

    XImage *img = create_ximage(
        xctx.dpy, xctx.visual,
        width, height, image_data
    );

    // Pixmap pm = XCreatePixmap(xctx.dpy, );

    int dstx = 0; // (1920 - win_width) / 2;
    int dsty = 0; // (1080 - win_height) / 2;
    int imgx = 0;
    int imgy = 0;
    // bool redraw = false;
    int factor = 10;

    XEvent ev;
    XSync(xctx.dpy, false);
    while (running && !XNextEvent(xctx.dpy, &ev)) {
        if (ev.type == KeyPress || ev.type == KeyRelease) {
            // printf("keycode: %d\n", ev.xkey.keycode);
            switch (ev.xkey.keycode) {
                case 24: // q
                    running = false;
                    break;

                case 42: // g
                    factor += 10;
                    break;

                case 43: // h
                    factor -= 10;
                    break;

                case 40:
                case 114: // right
                    imgx += factor;
                    if (imgx + win_width > width) {
                        imgx = width - win_width;
                    }
                    // redraw = true;
                    XPutImage(xctx.dpy, xctx.win, gc, img, imgx, imgy, dstx, dsty, width, height);
                    break;

                case 38:
                case 113: // left
                    imgx -= factor;
                    imgx = imgx > 0 ? imgx : 0;
                    // redraw = true;
                    XPutImage(xctx.dpy, xctx.win, gc, img, imgx, imgy, dstx, dsty, width, height);
                    break;

                case 25:
                case 111: // up
                    imgy -= factor;
                    imgy = imgy > 0 ? imgy : 0;
                    // redraw = true;
                    XPutImage(xctx.dpy, xctx.win, gc, img, imgx, imgy, dstx, dsty, width, height);
                    break;

                case 39:
                case 116: // down
                    imgy += factor;
                    if (imgy + win_height > height) {
                        imgy = height - win_height;
                    }
                    // redraw = true;
                    XPutImage(xctx.dpy, xctx.win, gc, img, imgx, imgy, dstx, dsty, width, height);
                    break;
            }
        } 
        // else if (ev.type == KeyRelease && redraw) {
        //     XPutImage(xctx.dpy, xctx.win, gc, img, imgx, imgy, 0, 0, width, height);
        //     redraw = false;
        // } 
        else if (ev.type == Expose) {
            XPutImage(xctx.dpy, xctx.win, gc, img, imgx, imgy, dstx, dsty, width, height);
        }
    }
        // if (handler[ev.type])
        //     handler[ev.type](&ev); /* call handler */

    // XftDrawDestroy(xftdraw);
    // drw_free(drw);
    XDestroyWindow(xctx.dpy, xctx.win);
    XCloseDisplay(xctx.dpy);

    return 0;
}

