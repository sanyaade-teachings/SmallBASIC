/**
 * -*- c-file-style: "java" -*-
 * SmallBASIC for eBookMan
 * Copyright(C) 2001-2002 Chris Warren-Smith. Gawler, South Australia
 * cwarrens@twpo.com.au
 *
 *                  _.-_:\
 *                 /      \
 *                 \_.--*_/
 *                       v
 *
 * This program is distributed under the terms of the GPL v2.0 or later
 * Download the GNU Public License (GPL) from www.gnu.org
 * 
 */

#include "sys.h"
#include "device.h"
#include "smbas.h"
#include "ebm_main.h"
#include <evnt_fun.h>

const int longSleep = 200000;
const int turboSleep = 4000;
const int menuSleep = 100000;

EXTERN_C_BEGIN
#include <piezo.h>

extern SBWindow out;
bool needUpdate=false;
int ticks=0;
const int ticksPerRedraw=5; // update text every 1/4 second

int osd_devinit() {
    needUpdate=false;
    out.resetPen();
    return 1;
}

void osd_setcolor(long color) {
    out.setColor(color);
}

void osd_settextcolor(long fg, long bg) {
    out.setTextColor(fg, bg);
}

void osd_refresh() {}

int osd_devrestore() {
    return 1;
}

/**
 *   system event-loop
 *   return value:
 *     0 continue 
 *    -1 close sbpad application
 *    -2 stop running basic application
 */
int osd_events(int wait_flag) {
    if (wait_flag == 1) {
        usleep(out.isInShell() ? menuSleep : 
               out.isTurbo() ? turboSleep : longSleep);
    }
    GUIFLAGS *guiFlags = GUI_Flags_ptr();
    if (needUpdate) {
        needUpdate=false;
        imgUpdate(); // update graphics operation
        *guiFlags &= ~GUIFLAG_NEED_FLUSH;
    }
    if (!EVNT_IsWaiting()) {
        if (*guiFlags & GUIFLAG_NEED_FLUSH == 0) {
            return 0; // no redraw required
        }
        if (wait_flag == 1 || ++ticks==ticksPerRedraw) {
            ticks=0;
            *guiFlags &= ~GUIFLAG_NEED_FLUSH;
            imgUpdate(); // defered text update
        }
        return 0;
    }
    GUI_EventLoop(0);
    while (out.isRunning() && out.isMenuActive()) {
        while (!EVNT_IsWaiting()) {
            usleep(menuSleep);
        }
        GUI_EventLoop(0);
    }
    if (out.wasBreakEv()) {
        return -2;
    }
    return dev_kbhit();
}

void osd_setpenmode(int enable) {
    out.penState = (enable ? 2 : 0);
}

int osd_getpen(int code) {
    if (out.penState == 0) {
        usleep(out.isTurbo() ? turboSleep : longSleep);
        return 0;
    }
    int r = 0;
    switch (code) {
    case 0:  // bool: status changed
        if (osd_events(1) < 0) {
            brun_break();
            return 0;
        }
        r = out.penUpdate;
        out.penUpdate = false;
        return r;
    case 1:  // last pen-down x
        return out.penDownX;
    case 2:  // last pen-down y
        return out.penDownY;
    case 3:  // vert. 1 = down, 0 = up
        return out.penDown;
    case 4:  // last x
        return out.penX;
    case 5:  // last y
        return out.penY;
    }
    return 0;
}

int osd_getx() {
    return out.getX();
}

int osd_gety() {
    return out.getY();
}

void osd_setxy(int x, int y) {
    out.setXY(x,y);
}

void osd_cls() {
    out.clearScreen();
}

int osd_textwidth(const char *str) {
    return out.textWidth(str);
}

int osd_textheight(const char *str) {
    return out.textHeight();
}

void osd_setpixel(int x, int y) {
    imgPixelSetColor(imgGetBase(), x, y, 15-dev_fgcolor);
    needUpdate=true;
}

long osd_getpixel(int x, int y) {
    return imgPixelGetColor(imgGetBase(), x,y);
}

void osd_line(int x1, int y1, int x2, int y2) {
    out.drawLine(x1, y1, x2, y2);
    GUIFLAGS *guiFlags = GUI_Flags_ptr();
    *guiFlags &= ~GUIFLAG_NEED_FLUSH;
    needUpdate=false;
    imgUpdate(); 
}

void osd_rect(int x1, int y1, int x2, int y2, int bFill) {
    if (bFill) {
        out.drawFGRectFilled(x1, y1, x2-x1, y2-y1);
    } else {
        out.drawFGRect(x1, y1, x2-x1, y2-y1);
    }
    needUpdate=true;
}

void osd_beep() {
    GUI_Beep();
}

void osd_sound(int frq, int ms, int vol, int bgplay) {
    piezo_tone t;
    t.tone = ((32768/frq)-2)/2;
    // interval in seconds = (man*4^15-exp)/1000000
    // 4^10 = 1024 -> /= 1000000 -> ~1ms
    t.man = ms;
    t.exp = 10;
    t.unused = 0;
    piezo_play_1tone(t, 0);
}

void osd_write(const char *s) {
    out.write(s);
}

EXTERN_C_END