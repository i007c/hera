
#include "hera.h"
// #define DISABLE_LOGGER
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

#include <starlight.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// #include <X11/Xft/Xft.h>
// #include <X11/keysym.h>
// #include <X11/cursorfont.h>
#include <X11/XKBlib.h>
// #include <X11/Xos.h>
#include <X11/Xatom.h>
// #include <X11/Xresource.h>


Display *dpy;
Window root;
Pixmap pixm;
int screen;
int dpy_width;
int dpy_height;
Starlight starlight;
Visual *visual;
Colormap color_map;
int depth;
Window win;
GC gc;
bool running = true;
bool bgra = true;

uint8_t downsclae_method = 0;

uint32_t win_width = 1920;
uint32_t win_height = 1080;

XImage *img;
// #define IMG_DATA_LEN 1920 * 1080 * 4
// uint8_t img_data[IMG_DATA_LEN];
uint32_t img_x = 0;
uint32_t img_y = 0;
uint32_t img_w = 0;
uint32_t img_h = 0;
uint8_t zoom = 1;
uint16_t pan_speed = 5;
uint32_t pos_x = 0;
uint32_t pos_y = 0;

int32_t fn = 0;
int32_t total_files = 0;
char **filenames;

StarlightImageBuffer image;
StarlightImageBuffer drawn;
float ratio = 0;
float ratio_speed = 0.2f;
int bg_pixel;

// events
static void expose(XEvent *e);
static void keypress(XEvent *e);

void win_toggle_fullscreen(void);
void change_image(int32_t amount);

static void (*handler[LASTEvent])(XEvent *) = {
    [Expose] = expose,
    [KeyPress] = keypress,
};

int main(int argc, char **argv) {
    log_info("--- Hera ---");

    total_files = argc - 1;
    if (total_files == 0) {
        log_warn("no input was provided");
        return 1;
    }

    filenames = ++argv;
    
    change_image(0);

    if ((dpy = XOpenDisplay(NULL)) == NULL)
        panic("con't open display :(");

    screen = DefaultScreen(dpy);
    dpy_width = DisplayWidth(dpy, screen);
    dpy_height = DisplayHeight(dpy, screen);
    visual = DefaultVisual(dpy, screen);
    color_map = DefaultColormap(dpy, screen);
    depth = DefaultDepth(dpy, screen);
    root = RootWindow(dpy, screen);

    // XVisualInfo vinfo;
    // XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo);

    // XSetWindowAttributes attr;
    // attr.colormap = XCreateColormap(
    //     dpy, DefaultRootWindow(dpy), 
    //     vinfo.visual, AllocNone
    // );
    // attr.border_pixel = 0;
    // attr.background_pixel = 0x00000000; // Red, semi-transparent
    

    bg_pixel = BlackPixel(dpy, screen);

    // uint32_t win_width = width > 1920 ? 1920 : width;
    // uint32_t win_height = height > 1080 ? 1080 : height;

    // uint32_t win_width = 1024;
    // uint32_t win_height = 1024;

    // win = XCreateSimpleWindow(
    //     dpy,
    //     XDefaultRootWindow(dpy),
    //     0, 0,
    //     win_width, win_height,
    //     0, win_b_color, win_w_color
    // );
    win = XCreateWindow(
        dpy, root, 0, 0, win_width, win_height,
        0, depth, InputOutput, visual, 0, NULL
    );

    gc = XCreateGC(dpy, win, 0, NULL);

    pixm = XCreatePixmap(dpy, win, dpy_width, dpy_height, depth);
	XSetWindowBackgroundPixmap(dpy, win, pixm);
    XSetForeground(dpy, gc, 0x00f00000);
    XFillRectangle(dpy, pixm, gc, 0, 0, 500, 200);


    // win = XCreateWindow(
    //     dpy, DefaultRootWindow(dpy), 0, 0,
    //     width, height, 0, vinfo.depth, InputOutput, vinfo.visual,
    //     CWColormap | CWBorderPixel | CWBackPixel, &attr
    // );

    // XSizeHints sh;
    // sh.min_width  = sh.max_width  = win_width;
    // sh.min_height = sh.max_height = win_height;
    // sh.flags = PMinSize | PMaxSize;
    // XSetWMNormalHints(dpy, win, &sh);

    XStoreName(dpy, win, "Hera");
    XMapWindow(dpy, win);
    XSelectInput(
        dpy, win, 
        KeyPressMask | ExposureMask
    );
    XFlush(dpy);


    // win_toggle_fullscreen();

    XEvent ev;
    XSync(dpy, false);

    while (running && !XNextEvent(dpy, &ev)) {
        if (handler[ev.type])
            handler[ev.type](&ev);
    }

    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);

    return 0;
}


// static void upscale(uint32_t iw, uint32_t ih) {
//     uint8_t w_ratio = win_width / iw;
//     uint8_t h_ratio = win_height / ih;
//     uint8_t ratio = w_ratio < h_ratio ? w_ratio : h_ratio;
//     // ratio--;
//
//     // uint8_t ratio = win_width / iw;
//     img_w = iw * ratio;
//     img_h = ih * ratio;
//     pos_x = (win_width - img_w) / 2;
//     pos_y = (win_height - img_h) / 2;
//     if (pos_x > win_width) pos_x = 0;
//     if (pos_y > win_height) pos_y = 0;
//     log_info("upsclae ratio: %d - img_w: "DFMT" h: "DFMT, ratio, img_w, img_h);
//
//     uint32_t bn = 0; // big n
//     for (uint32_t sy = 0; sy < ih; sy++) {
//         for (uint32_t sx = 0; sx < iw; sx++) {
//             uint32_t i = (sy * iw+ sx) * 4;
//
//             uint32_t base_px = (sy * img_w * ratio) + (sx * ratio);
//             for (uint8_t ry = 0; ry < ratio; ry++) {
//                 bn = base_px + (ry * img_w );
//                 for (uint8_t rx = 0; rx < ratio; rx++) {
//                     uint32_t j = bn * 4;
//                     img_data[j + 0] = starlight.out.s[i + 2];
//                     img_data[j + 1] = starlight.out.s[i + 1];
//                     img_data[j + 2] = starlight.out.s[i + 0];
//                     img_data[j + 3] = starlight.out.s[i + 3];
//                     bn++;
//                 }
//             }
//         }
//     }
// }


// static void downscale(uint32_t iw, uint32_t ih) {
//     uint8_t w_ratio = iw / win_width;
//     uint8_t h_ratio = ih / win_height;
//     uint8_t ratio = w_ratio > h_ratio ? w_ratio : h_ratio;
//     ratio++;
//
//     log_verbose("iw: \33[93m%d\33[m ih: \33[93m%d\33[m", iw, ih);
//
//     img_w = iw / ratio;
//     img_h = ih / ratio;
//     pos_x = (win_width - img_w) / 2;
//     pos_y = (win_height - img_h) / 2;
//     if (pos_x > win_width) pos_x = 0;
//     if (pos_y > win_height) pos_y = 0;
//     log_info("downsclae ratio: %d - img w: %d - h: %d", ratio, img_w, img_h);
//     log_verbose("pos x: "DFMT" y: "DFMT, pos_x, pos_y);
//
//     if (downsclae_method == 1) {
//         for (uint32_t sy = 0; sy < img_h; sy++) {
//             for (uint32_t sx = 0; sx < img_w; sx++) {
//                 uint32_t i = (sy * img_w + sx) * 4;
//
//                 uint32_t red = 0, green = 0, blue = 0, alpha = 0;
//                 uint32_t at = 1;
//
//                 uint32_t base_px = ((sy * iw * ratio) + (sx * ratio));
//
//                 for (uint8_t ry = 0; ry < ratio; ry++) {
//                     uint32_t bn = (base_px + (ry * iw)) * 4;
//
//                     for (uint8_t rx = 0; rx < ratio; rx++) {
//                         red += starlight.out.s[bn + 0];
//                         green += starlight.out.s[bn + 1];
//                         blue += starlight.out.s[bn + 2];
//                         alpha += starlight.out.s[bn + 3];
//
//                         at++;
//                         bn += 4;
//                     }
//                 }
//
//                 img_data[i + 0] = blue ? blue / at : 0;
//                 img_data[i + 1] = green ? green / at : 0;
//                 img_data[i + 2] = red ? red / at : 0;
//                 img_data[i + 3] = alpha ? 255: 0;
//             }
//         }
//
//     } else {
//         for (uint32_t sy = 0; sy < img_h; sy++) {
//             for (uint32_t sx = 0; sx < img_w; sx++) {
//                 uint32_t i = (sy * img_w + sx) * 4;
//
//                 uint32_t base_px = ((sy * iw * ratio) + (sx * ratio)) * 4;
//                 img_data[i + 0] = starlight.out.s[base_px + 2];
//                 img_data[i + 1] = starlight.out.s[base_px + 1];
//                 img_data[i + 2] = starlight.out.s[base_px + 0];
//                 img_data[i + 3] = starlight.out.s[base_px + 3];
//             }
//         }
//     }
//
// }


static void pan_image(int16_t x, int16_t y) {
    drawn.x += x;
    drawn.y += y;
    if (drawn.x + win_width > drawn.w) {
        drawn.x = drawn.w - win_width;
    }
    if (drawn.x < 0) drawn.x = 0;

    if (drawn.y + win_height > drawn.h) {
        drawn.y = drawn.h - win_height;
    }
    if (drawn.y < 0) drawn.y = 0;
}

static void draw_image(bool rescale) {

    // StarlightImageBuffer output = {
    //     .b = { .s = img_data, .c = img_data, .e = &img_data[IMG_DATA_LEN], .l = IMG_DATA_LEN },
    //     .w = win_width,
    //     .h = win_height,
    //     .x = 0,
    //     .y = 0
    // };

    log_debug(
        "ww: %5d | wh: %5d - dw: %5d - dh: %5d | r: %f - rs: %f",
        win_width, win_height, drawn.w, drawn.h, ratio, ratio_speed
    );
    if (rescale || !drawn.w) {
        if (drawn.b.s) free(drawn.b.s);

        // drawn.x = drawn.w - (image.w * ratio);
        // drawn.y = drawn.h - (image.h * ratio);
        drawn.w = image.w * ratio;
        drawn.h = image.h * ratio;
        if (drawn.w > 20000 || drawn.h > 20000) {
            ratio = 0.2;
            ratio_speed = (((float)win_width / image.w) / image.w) * 16;
            drawn.w = image.w * ratio;
            drawn.h = image.h * ratio;
        }
        pan_image(0, 0);
        drawn.b.l = drawn.w * drawn.h * 4;
        drawn.b.s = malloc(drawn.b.l);
        assert(drawn.b.s != NULL);
        drawn.b.c = drawn.b.s;
        drawn.b.e = drawn.b.s + drawn.b.l - 1;
        starlight_scale(&image, &drawn, bgra);
        img = XCreateImage(
            dpy, visual, depth, ZPixmap,
            0, (char *)drawn.b.s, drawn.w, drawn.h, 32, 0
        );
        // img_w = output.w;
        // img_h = output.h;
    }
        // if (win_width > starlight.width && win_height > starlight.height) {
        //     upscale(starlight.width, starlight.height);
        // } else {
        //     downscale(starlight.width, starlight.height);
        // }
    // }


    // pos_x = (win_width - img_w) / 2;
    // pos_y = (win_height - img_h) / 2;

    // if (img_x > img_w) img_x = img_w;
    // if (img_y > img_h) img_y = img_h;

    // log_debug("img x: %u - y: %u - w: %u - h: %u", img_x, img_y, img_w, img_h);

    // XClearWindow(dpy, win);
    // XClearArea(dpy, win, 0, 0, win_width, win_height, false);
    // XClearArea(dpy, win, 0, 0, pos_x, 0, false);
    // XClearArea(dpy, win, img_w + pos_x, 0, pos_x, 0, false);

    // log_debug(
    //     "ww: %5d | wh: %5d - dw: %5d - dh: %5d",
    //     win_width, win_height, drawn.w, drawn.h
    // );
    
    uint32_t pw = win_width;
    uint32_t ph = win_height;
    pos_x = 0;
    pos_y = 0;


    XSetForeground(dpy, gc, bg_pixel);
    XFillRectangle(dpy, pixm, gc, 0, 0, win_width, win_height);


    if (drawn.w < pw) {
        pw = drawn.w;
        pos_x = (win_width - pw) / 2;
        // XClearArea(dpy, win, 0, 0, pos_x + 1, win_height, false);
        // XClearArea(dpy, win, pw + pos_x - 1, 0, pos_x + 2, win_height, false);
    }

    if (drawn.h < ph) {
        ph = drawn.h;
        pos_y = (win_height - ph) / 2;

        // XClearArea(dpy, win, 0, 0, win_width, pos_y + 1, false);
        // XClearArea(dpy, win, 0, ph + pos_y - 1, win_width, pos_y + 2, false);
    }

    XPutImage(
        dpy, pixm, gc, img,
        drawn.x, drawn.y, pos_x, pos_y, pw, ph
    );
    XClearWindow(dpy, win);
	// XSetWindowBackgroundPixmap(dpy, win, pixm);
    // XSync(dpy, true);
    // XFlush(dpy);
}

void change_image(int32_t amount) {
    log_debug("total files: %d - amount: %d", total_files, amount);
    if (!total_files) exit(1);

    fn += amount;
    if (fn >= total_files) {
        fn = 0;
    } else if (fn < 0){
        fn = total_files - 1;
    }

    char *filename = filenames[fn];

    if (access(filename, F_OK)) {
        log_error("file '%s' does not exists", filename);
        change_image(1);
        return;
    }

    log_verbose("filename: "SFMT, filename);

    if (starlight.out.s) free(starlight.out.s);
    memset(&starlight, 0, sizeof(Starlight));

    uint8_t *buffer = NULL;

    int fd = open(filename, O_RDONLY);
    off_t filesize = lseek(fd, 0, SEEK_END);
    buffer = malloc(filesize);
    lseek(fd, 0, SEEK_SET);
    read(fd, buffer, filesize);

    starlight.raw.s = buffer;
    starlight.raw.l = filesize;

    starlight_status_t starlight_status = starlight_load(&starlight);
    if (starlight_status) {
        log_error(
            "Starlight status: %s",
            starlight_status_string(starlight_status)
        );
        // total_files--;
        // filenames++;
        change_image(1);
        free(starlight.raw.s);
        return;
    }

    log_info(
        "image info:\nformat: %d\nwidth: %d | height: %d\n",
        starlight.format, starlight.width, starlight.height
    );

    starlight.out.s = malloc(starlight.out.l);
    starlight.cmp.s = malloc(starlight.cmp.l);

    starlight_status = starlight.loader(&starlight);

    if (starlight_status) {
        log_error(
            "Starlight loader status: %s",
            starlight_status_string(starlight_status)
        );
        // total_files--;
        // filenames++;
        change_image(1);
        free(starlight.raw.s);
        free(starlight.out.s);
        free(starlight.cmp.s);
        return;
    }

    free(starlight.cmp.s);
    free(starlight.raw.s);
    image.b = starlight.out;
    image.w = starlight.width;
    image.h = starlight.height;
    // image.a = image.w / image.h;
    image.x = 0;
    image.y = 0;
    drawn.w = 0;
    drawn.h = 0;
    ratio_speed = (((float)win_width / image.w) / image.w) * 16;
    // image.z = 1;
}

static void expose(XEvent *e) {
    XExposeEvent *ev = &e->xexpose;
    win_width = ev->width;
    win_height = ev->height;


    ratio_speed = (((float)win_width / image.w) / image.w) * 16;
    if (!ratio) ratio = (float)win_width / image.w;

    draw_image(true);
}

void win_toggle_fullscreen(void) {
    XEvent ev;
    XClientMessageEvent *cm;

    memset(&ev, 0, sizeof(ev));
    ev.type = ClientMessage;

    cm = &ev.xclient;
    cm->window = win;
    cm->message_type = XInternAtom(dpy, "_NET_WM_STATE", false);
    cm->format = 32;
    cm->data.l[0] = 2; // toggle
    cm->data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", false);

    XSendEvent(
        dpy, DefaultRootWindow(dpy), false,
        SubstructureNotifyMask | SubstructureRedirectMask, &ev
    );
}

static void keypress(XEvent *e) {
    XKeyEvent *ev = &e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(
        dpy, ev->keycode, 0, ev->state & ShiftMask ? 1 : 0
    );
    uint8_t rsx = 1;
    if (ev->state & ControlMask) rsx = 16;

    switch (keysym) {
        case XK_q:
            running = false;
            break;

        case XK_e:
            downsclae_method++;
            if (downsclae_method > 1) downsclae_method = 0;
            draw_image(true);
            break;

        case XK_r:
            bgra = !bgra;
            draw_image(true);
            break;

        case XK_f:
            win_toggle_fullscreen();
            break;

        case XK_g:
            pan_speed += 10;
            break;

        case XK_h:
            pan_speed -= 10;
            break;

        case XK_d:
        case XK_Right:
            // change_image(1);
            // draw_image(true);
            pan_image(pan_speed, 0);
            draw_image(false);
            break;

        case XK_a:
        case XK_Left:
            // change_image(-1);
            // draw_image(true);
            pan_image(-pan_speed, 0);
            draw_image(false);
            break;

        case XK_w:
        case XK_Up:
            pan_image(0, -pan_speed);
            draw_image(false);
            break;

        case XK_s:
        case XK_Down:
            pan_image(0, pan_speed);
            draw_image(false);
            break;

        case XK_1:
            ratio -= ratio_speed * rsx;
            if (ratio < 0) ratio = ratio_speed;
            draw_image(true);
            break;

        case XK_2:
            ratio = 1;
            draw_image(true);
            break;
            
        case XK_3:
            ratio += ratio_speed * rsx;
            draw_image(true);
            break;

        case XK_W:
            ratio = (float)win_width / image.w;
            draw_image(true);
            break;

        case XK_H:
            ratio = (float)win_height / image.h;
            draw_image(true);
            break;
    }
}

