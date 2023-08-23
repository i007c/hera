
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

#include <X11/Xlib.h>
// #include <X11/Xutil.h>
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

void inflate(uint8_t *buffer, size_t length);

void draw(uint8_t *rgb_out, int w, int h) {
    int i = 0;

    for (i = 0; i < w * h; i += 4) {
        rgb_out[i] = 255; // blue
        rgb_out[i + 1] = 0; // green
        rgb_out[i + 2] = 255; // red
        // rgb_out[i + 3] = 10; // alpha
    }
}

XImage *create_ximage(Display *display, Visual *visual, int width, int height)
{
    char *image32 = (char *)malloc(width * height * 4);
    draw((uint8_t *)image32, width, height);

    return XCreateImage(
        display, visual, 24,
        ZPixmap, 0, image32,
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

    char filename[256] = "./sample/technoblade.png";

    if (argc > 1) {
        strncpy(filename, argv[1], 256);
    }

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
    uint8_t compression = 0;
    uint8_t filter = 0;
    uint8_t interlace = 0;

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
                compression = buffer[i]; i++;
                filter = buffer[i]; i++;
                interlace = buffer[i]; i++;

                log_verbose(
                    "\nwidth: %d\nhieght: %d\nbit_depth: %d\ncolor_type: %d\n"
                    "compression: %d\nfilter: %d\ninterlace: %d",
                    width, height, bit_depth, color_type, compression,
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
                log_info("IDAT length: %d at: %d", length, i);

                uint8_t cfm = buffer[i];
                uint8_t cm = cfm & 15; // 15 == 0b00001111
                uint8_t cinfo = cfm >> 4;
                uint32_t win = 1 << (8 + cinfo);

                log_verbose(
                    "cfm: "DFMT", cm: "DFMT", cinfo: "DFMT", win: "DFMT,
                    cfm, cm, cinfo, win
                );

                uint8_t flg = buffer[i + 1];
                uint8_t fcheck = flg & 31;   // first 5 bytes
                bool fdict = flg & 32;       // byte 6
                uint8_t flevel = flg >> 6;   // byte 7 and 8

                log_verbose(
                    "flg: "DFMT", fcheck: "DFMT", fdict: "DFMT
                    ", flevel: "DFMT", c: %d",
                    flg, fcheck, fdict, flevel,
                    (cfm * 256 + flg) % 31
                );

                inflate(&buffer[i + 2], length - 6);
                uint32_t adlr32 = buffer[i + length - 4];
                log_verbose("adlr32: %d", adlr32);

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
                log_info("-- IEND --");
            } break;

            case 0x73424954: { // sBIT
                log_info("sbit length: %d", length);
                i += length;
            } break;

            case 0x65584966: { // eXIf
                log_info("eXIf length: %d", length);
                i += length;
            } break;

            default: {
                uint8_t *ch = (uint8_t *)&type;

                log_warn(
                    "unknown type: %c%c%c%c "DFMT" 0x\033[32m%x\033[0m",
                    ch[3], ch[2], ch[1], ch[0],
                    type, type
                );
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

    return 0;

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

    xctx.win = XCreateSimpleWindow(
        xctx.dpy,
        XDefaultRootWindow(xctx.dpy),
        0, 0,
        600, 600,
        0, win_b_color, win_w_color
    );

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

    XImage *img = create_ximage(xctx.dpy, xctx.visual, 100, 100);

    // Pixmap pm = XCreatePixmap(xctx.dpy, );


    XEvent ev;
    XSync(xctx.dpy, false);
    while (running && !XNextEvent(xctx.dpy, &ev)) {
        if (ev.type == KeyPress || ev.type == KeyRelease) {
            if (ev.xkey.keycode == 24) {
                running = false;
            }
        }
        else if (ev.type == Expose) {
            XPutImage(xctx.dpy, xctx.win, gc, img, 0, 0, 200, 200, 100, 100);
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

