 /* -*- c-file-style: "java" -*-
 * $Id: output_write.c,v 1.4 2006-02-08 05:56:04 zeeb90au Exp $
 * This file is part of SmallBASIC
 *
 * Copyright(C) 2001-2006 Chris Warren-Smith. Gawler, South Australia
 * cwarrens@twpo.com.au
 *
 * This program is distributed under the terms of the GPL v2.0 or later
 * Download the GNU Public License (GPL) from www.gnu.org
 */ 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "output_model.h"

extern OutputModel output;
#define INITXY 2

void drawtext() {
    //    layout = gtk_widget_create_pango_layout (widget, text);
    //    fontdesc = pango_font_description_from_string ("Luxi Mono 12");
    //    pango_layout_set_font_description (layout, fontdesc); 
    //    gdk_draw_layout (..., layout);
    //    pango_font_description_free (fontdesc);
    //    g_object_unref (layout);
}

void new_line() {
    gint height = output.widget->allocation.height;
    gint fontHeight = om_font_height();

    output.curX = INITXY;
    if (output.curY+(fontHeight*2) >= height) {
        /* shift image up fontHeight pixels */
        gdk_draw_drawable(output.pixmap,
                          output.gc,
                          output.pixmap,
                          0, fontHeight, /* src x,y */
                          0, 0,          /* dest x,y */
                          output.widget->allocation.width,
                          output.widget->allocation.width-fontHeight);
        /* erase bottom line */
        gdk_draw_rectangle(output.pixmap, output.gc, TRUE,
                           output.widget->allocation.x,
                           output.widget->allocation.y,
                           output.widget->allocation.width,
                           output.widget->allocation.height);
        osd_refresh();
    } else {
        output.curY += fontHeight;
    }
}

int calc_tab(int x) {
    int c = 1;
    while (x > output.tabSize) {
        x -= output.tabSize;
        c++;
    }
    return c * output.tabSize;
}

long ansi_to_gtk(long c) {
    if (c < 0) {
        // assume color is windows style RGB packing
        // RGB(r,g,b) ((COLORREF)((BYTE)(r)|((BYTE)(g) << 8)|((BYTE)(b) << 16)))
        c = -c;
        int b = (c>>16) & 0xFF;
        int g = (c>>8) & 0xFF;
        int r = (c) & 0xFF;
        //return fltk::color(r, g, b);
    }

    //return (c > 16) ? WHITE : colors[c];
    return 0;
}

int set_graphics_rendition(char c, int escValue) {
    switch (c) {
    case 'K': // \e[K - clear to eol
        gdk_draw_rectangle(output.pixmap, output.gc, TRUE, output.curX, output.curY,
                           output.widget->allocation.width-output.curX, om_font_height());
        break;
    case 'G': // move to column
        output.curX = escValue;
        break;
    case 'T': // non-standard: move to n/80th of screen width
        output.curX = escValue*output.widget->allocation.width/80;
        break;
    case 's': // save cursor position
        output.curYSaved = output.curX;
        output.curXSaved = output.curY;
        break;
    case 'u': // restore cursor position
        output.curX = output.curYSaved;
        output.curY = output.curXSaved;
        break;
    case ';': // fallthru
    case 'm': // \e[...m  - ANSI terminal
        switch (escValue) {
        case 0:  // reset
            //reset();
            break;
        case 1: // set bold on
            pango_font_description_set_weight(output.font, PANGO_WEIGHT_BOLD);
            return 1;
        case 2: // set faint on
            pango_font_description_set_weight(output.font, PANGO_WEIGHT_ULTRALIGHT);
            break;
        case 3: // set italic on
            pango_font_description_set_style(output.font, PANGO_STYLE_ITALIC);
            return 1;
        case 4: // set underline on
            output.underline = 1;
            break;
        case 5: // set blink on
            break;
        case 6: // rapid blink on
            break;
        case 7: // reverse video on
            output.invert = 1;
            break;
        case 8: // conceal on
            break;
        case 21: // set bold off
            pango_font_description_set_weight(output.font, PANGO_WEIGHT_NORMAL);
            return 1;
        case 23: // italic off
            pango_font_description_set_style(output.font, PANGO_STYLE_NORMAL);
            return 1;
        case 24: // set underline off
            output.underline = 0;
            break;
        case 27: // reverse video off
            output.invert = 0;
            break;
            // colors - 30..37 foreground, 40..47 background
        case 30: // set black fg
            //labelcolor(ansi_to_gtk(0));
            return 1;
        case 31: // set red fg
            //labelcolor(ansi_to_gtk(4));
            return 1;
        case 32: // set green fg
            //labelcolor(ansi_to_gtk(2));
            return 1;
        case 33: // set yellow fg
            //labelcolor(ansi_to_gtk(6));
            return 1;
        case 34: // set blue fg
            //labelcolor(ansi_to_gtk(1));
            return 1;
        case 35: // set magenta fg
            //labelcolor(ansi_to_gtk(5));
            return 1;
        case 36: // set cyan fg
            //labelcolor(ansi_to_gtk(3));
            return 1;
        case 37: // set white fg
            //labelcolor(ansi_to_gtk(7));
            return 1;
        case 40: // set black bg
            //color(ansi_to_gtk(0));
            return 1;
        case 41: // set red bg
            //color(ansi_to_gtk(4));
            return 1;
        case 42: // set green bg
            //color(ansi_to_gtk(2));
            return 1;
        case 43: // set yellow bg
            //color(ansi_to_gtk(6));
            return 1;
        case 44: // set blue bg
            //color(ansi_to_gtk(1));
            return 1;
        case 45: // set magenta bg
            //color(ansi_to_gtk(5));
            return 1;
        case 46: // set cyan bg
            //color(ansi_to_gtk(3));
            return 1;
        case 47: // set white bg
            //color(ansi_to_gtk(15));
            return 1;
        case 48: // subscript on
            break;
        case 49: // superscript
            break;
        };                        
    }
    return 0;
}

int doEscape(unsigned char** p) {
    int escValue = 0;
    while (isdigit(**p)) {
        escValue = (escValue * 10) + (**p - '0');
        *p++;
    }

    if (set_graphics_rendition(**p, escValue)) {
        //fltk::Font* font = labelfont();
        //        if (bold) {
        //            font = font->bold();
        //        }
        //        if (italic) {
        //            font = font->italic();
        //        }
        //        setfont(font, labelsize());
    }
    
    if (**p == ';') {
        *p++; // next rendition
        return 1;
    }
    return 0;
}

/**
 *   Supported control codes:
 *   \t      tab (20 px)
 *   \a      beep
 *   \r      return
 *   \n      next line
 *   \xC     clear screen
 *   \e[K    clear to end of line
 *   \e[0m   reset all attributes to their defaults
 *   \e[1m   set bold on
 *   \e[4m   set underline on
 *   \e[7m   reverse video
 *   \e[21m  set bold off
 *   \e[24m  set underline off
 *   \e[27m  set reverse off
 */
void osd_write(const char *str) {
    int len = strlen(str);
    if (len <= 0) {
        return;
    }

    int ascent = om_getascent();
    int fontHeight = om_font_height();
    unsigned char *p = (unsigned char*)str;
    int numChars, cx, width;

    while (*p) {
        switch (*p) {
        case '\a':   // beep
            drvsound_beep();
            break;
        case '\t':
            output.curX = calc_tab(output.curX+1);
            break;
        case '\xC':
            //init();
            //setcolor(color());
            //fillrect(Rectangle(output.widget->allocation.width, output.widget->allocation.height));
            break;
        case '\033':  // ESC ctrl chars
            if (*(p+1) == '[' ) {
                p += 2;
                while(1) {
                    if (!doEscape(&p)) {
                        break;
                    }
                }
            }
            break;
        case '\n': // new line
            new_line();
            break;
        case '\r': // return
            output.curX = INITXY;
            //setcolor(color());
            //fillrect(Rectangle(0, curY, output.widget->allocation.width, fontHeight));
            break;
        default:
            numChars = 1; // print minimum of one character
            cx = 0; //(int)getwidth((const char*)p, 1);
            width = output.widget->allocation.width-1;

            if (output.curX + cx >= width) {
                new_line();
            }

            // print further non-control, non-null characters 
            // up to the width of the line
            while (p[numChars] > 31) {
                cx += 0; //(int)getwidth((const char*)p+numChars, 1);
                if (output.curX + cx < width) {
                    numChars++;
                } else {
                    break;
                }
            }
            
            if (output.invert) {
                //setcolor(labelcolor());
                //fillrect(Rectangle(curX, curY, cx, fontHeight));
                //setcolor(color());
                //drawtext((const char*)p, numChars, curX, curY+ascent);
            } else {
                //setcolor(color());
                //fillrect(Rectangle(curX, curY, cx, fontHeight));
                //setcolor(labelcolor());
                //drawtext((const char*)p, numChars, curX, curY+ascent);
            }

            if (output.underline) {
                //drawline(curX, curY+ascent+1, curX+cx, curY+ascent+1);
            }
            
            // advance
            p += numChars-1; // allow for p++ 
            output.curX += cx;
        };
        
        if (*p == '\0') {
            break;
        }
        p++;
    }

    osd_refresh();
}

/* End of "$Id: output_write.c,v 1.4 2006-02-08 05:56:04 zeeb90au Exp $". */

