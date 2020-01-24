#include <X11/Xlib.h>
#include <iostream>
#include "X11/keysym.h"
#include "util/os_time.h"

double elapsedF2, elapsedF12;
uint64_t last_f2_press, last_f12_press;
pthread_t f2;
char *displayid = getenv("DISPLAY");
Display *dpy = XOpenDisplay(displayid);

bool key_is_pressed(KeySym ks) {
    char keys_return[32];
    XQueryKeymap(dpy, keys_return);
    KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
    return isPressed;
}
