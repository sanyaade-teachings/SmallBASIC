// -*- c-file-style: "java" -*-
// $Id: MainWindow.cpp,v 1.16 2004-12-03 19:27:47 zeeb90au Exp $
// This file is part of SmallBASIC
//
// Copyright(C) 2001-2004 Chris Warren-Smith. Gawler, South Australia
// cwarrens@twpo.com.au
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
// 

#include "sys.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include <fltk/run.h>
#include <fltk/error.h>
#include <fltk/ask.H>
#include <fltk/events.h>
#include <fltk/string.h>
#include <fltk/damage.h>
#include <fltk/Window.h>
#include <fltk/Item.h>
#include <fltk/Group.h>
#include <fltk/TabGroup.h>
#include <fltk/MenuBar.h>
#include <fltk/Button.h>

#include "MainWindow.h"
#include "EditorWindow.h"
#include "sbapp.h"

#ifdef __CYGWIN__
#include <fltk/win32.h>
#endif

using namespace fltk;

MainWindow* wnd;

enum ExecState {
    init_state,
    edit_state, 
    run_state, 
    modal_state,
    break_state, 
    quit_state
} runMode = init_state;

const char* runfile = 0;
bool reload = false;
int px,py,pw,ph;

const char aboutText[] =
    "About SmallBASIC...\n\n"
    "Copyright (c) 2000-2004 Nicholas Christopoulos,\n"
    "Chris Warren-Smith.\n\n"
    "FLTK Version 0.9.6.0\n\n"
    "http://smallbasic.sourceforge.net\n\n"
    "SmallBASIC comes with ABSOLUTELY NO WARRANTY.\n"
    "This program is free software; you can use it\n"
    "redistribute it and/or modify it under the terms of the\n"
    "GNU General Public License version 2 as published by\n"
    "the Free Software Foundation.";

void quit_cb(Widget*, void* v) {
    if (runMode == edit_state || runMode == quit_state) {
        if (wnd->editWnd->checkSave(true)) {
            exit(0);
        }
    } else {
        switch (choice("Terminate running program?",
                       "Exit", "Break", "Cancel")) {
        case 0:
            exit(0);
        case 1:
            dev_pushkey(SB_KEY_BREAK);
            runMode = break_state;
        }
    }
}

void about_cb(Widget*, void* v) {
    message(aboutText);
}

void break_cb(Widget*, void* v) {
    if (runMode == run_state || runMode == modal_state) {
        runMode = break_state;
    }
}

void fullscreen_cb(Widget *w, void* v) {
    if (w->value()) {
        // store current geometry of the window
        px = wnd->x(); 
        py = wnd->y();
        pw = wnd->w(); 
        ph = wnd->h();
        wnd->fullscreen();
    } else {
        // restore geometry to the window and turn fullscreen off
        wnd->fullscreen_off(px,py,pw,ph);
    }
}

void turbo_cb(Widget* w, void* v) {
    wnd->isTurbo = w->value();
}

void basicMain(const char* filename) {
    wnd->editWnd->readonly(true);
    wnd->tabGroup->selected_child(wnd->outputGroup);
    wnd->out->clearScreen();
    wnd->out->set_visible();
    wnd->out->activate();
    runMode = run_state;
    wnd->runStatus->label("RUN");
    wnd->runStatus->redraw();
    int success = sbasic_main(filename);
    if (runMode == quit_state) {
        exit(0);
    }
    if (success == false && gsb_last_line) {
        wnd->editWnd->gotoLine(gsb_last_line);
        wnd->fileStatus->label(gsb_last_errmsg);
    } else if (reload && 0 != strcmp(runfile, filename)) {
        runMode = run_state;
        setTitle(runfile);
        success = sbasic_main(runfile);
    }

    wnd->runStatus->label(success ? " " : "ERR");
    wnd->editWnd->readonly(false);
    wnd->editWnd->redraw();
    wnd->redraw();
    runMode = edit_state;
}

void run_cb(Widget*, void*) {
    const char* filename = wnd->editWnd->getFileName();
    if (runMode == edit_state && 
        wnd->editWnd->checkSave(false) && filename[0]) {
        basicMain(filename);
    }
}

void setTitle(const char* filename) {
    char title[256];
    char *slash;

    if (filename && filename[0]) {
        wnd->fileStatus->copy_label(filename);
        slash = strrchr(filename, '/');
        sprintf(title, "%s - SmallBASIC", (slash?slash+1:filename));
        wnd->copy_label(title);
    } else {
        wnd->fileStatus->label("Untitled");
        wnd->label("SmallBASIC");
    }
    wnd->redraw();
}

void setRowCol(int row, int col) {
    char t[20];
    sprintf(t, "%d", row);
    wnd->rowStatus->copy_label(t);
    wnd->rowStatus->redraw();
    sprintf(t, "%d", col);
    wnd->colStatus->copy_label(t);
    wnd->colStatus->redraw();
}

void setModified(bool dirty) {
    wnd->modStatus->label(dirty?"MOD":"");
    wnd->modStatus->redraw();
}

int arg_cb(int argc, char **argv, int &i) {
    if (i+1 >= argc) {
        return false;
    }

    switch (argv[i][1]) {
    case 'e':
        runfile = strdup(argv[i+1]);
        runMode = edit_state;
        i+=2;
        return 1;
    case 'a':
        reload = true;
        //fallthru
    case 'r':
        runfile = strdup(argv[i+1]);
        runMode = run_state;
        i+=2;
        return 1;
    case 'm':
        opt_loadmod = 1;
        strcpy(opt_modlist, argv[i+1]);
        i+=2;
        return 1;
    }
    return 0;
}

#if defined(__CYGWIN__)
// see http://www.sysinternals.com/ntw2k/utilities.shtml
// for the free DebugView program
#include <windows.h>
void trace(const char *format, ...) {
    char    buf[4096], *p = buf;
    va_list args;
    
    va_start(args, format);
    p += vsnprintf(p, sizeof buf - 1, format, args);
    va_end(args);
    
    while (p > buf && isspace(p[-1])) {
        *--p = '\0';
    }
    
    *p++ = '\r';
    *p++ = '\n';
    *p   = '\0';
    OutputDebugString(buf);
}
#endif

int main(int argc, char **argv) {
    int i=0;
    if (args(argc, argv, i, arg_cb) < argc) {
        fatal("Options are:\n -r[un] file.bas\n"
              " -a[uto-run] file.bas\n"
              " -e[dit] file.bas\n"
              " -m module-home\n\n%s", help);
    }

    wnd = new MainWindow(600, 400);
    wnd->show(argc, argv);

#ifdef __CYGWIN__
    wnd->icon((char *)LoadIcon(xdisplay, MAKEINTRESOURCE(101)));
#endif

    check();
    switch (runMode) {
    case run_state:
        wnd->editWnd->loadFile(runfile, -1);
        basicMain(runfile);
        break;
    case edit_state:
        wnd->editWnd->loadFile(runfile, -1);
        break;
    default:
        setTitle(0);
        runMode = edit_state;
    }

    return run();
}

struct TabPage : public Group {
    TabPage(int x, int y, int w, int h, const char * s) : 
        Group(x, y, w, h, s) {}
    int handle(int event) {
        // TextDisplay::layout() does nothing when not visible
        if (event == SHOW) {
            Widget* w = child(0);
            w->layout();
            w->take_focus();
        }
        return Group::handle(event);
    }
};

MainWindow::MainWindow(int w, int h) : Window(w, h, "SmallBASIC") {
    int mnuHeight = 20;
    int statusHeight = mnuHeight;
    int groupHeight = h-mnuHeight-statusHeight-3;
    int tabHeight = mnuHeight;
    // if tabBegin is zero tabs will be at the bottom
    //int tabBegin = runMode == run_state ? 0 : mnuHeight; 
    int tabBegin = 0;
    int pageHeight = groupHeight-tabHeight;

    isTurbo = 0;
    opt_graphics = 1;
    opt_quite = 1;
    opt_nosave = 1;
    opt_ide = IDE_NONE; // for sberr.c
    opt_command[0] = '\0';
    opt_pref_width = 0;
    opt_pref_height = 0;
    opt_pref_bpp = 0;
    
    begin();
    MenuBar* m = new MenuBar(0, 0, w, mnuHeight);
    m->add("&File/&New File",       0,        (Callback*)EditorWindow::new_cb);
    m->add("&File/&Open File...",   CTRL+'o', (Callback*)EditorWindow::open_cb);
    m->add("&File/_&Insert File...",CTRL+'i', (Callback*)EditorWindow::insert_cb);
    m->add("&File/&Save File",      CTRL+'s', (Callback*)EditorWindow::save_cb);
    m->add("&File/Save File &As...",CTRL+SHIFT+'S', (Callback*)EditorWindow::saveas_cb);
    m->add("&File/E&xit",        CTRL+'q', (Callback*)quit_cb);
    m->add("&Edit/_&Undo",       CTRL+'z', (Callback*)EditorWindow::undo_cb);
    m->add("&Edit/Cu&t",         CTRL+'x', (Callback*)EditorWindow::cut_cb);
    m->add("&Edit/&Copy",        CTRL+'c', (Callback*)EditorWindow::copy_cb);
    m->add("&Edit/&Paste",       CTRL+'v', (Callback*)EditorWindow::paste_cb);
    m->add("&Edit/_&Delete",     0, (Callback*)EditorWindow::delete_cb);
    m->add("&Edit/&Full Screen", 0, (Callback*)fullscreen_cb)->type(Item::TOGGLE);
    m->add("&Edit/&Turbo",       0, (Callback*)turbo_cb)->type(Item::TOGGLE);
    //m->add("&Edit/&Settings",   0, (Callback*)EditorWindow::settings_cb);
    m->add("&Program/&Run",      CTRL+'r', (Callback*)run_cb);
    m->add("&Program/&Break",    CTRL+'b', (Callback*)break_cb);
    m->add("&Search/&Find...",   CTRL+'f', (Callback*)EditorWindow::find_cb);
    m->add("&Search/Find A&gain",CTRL+'g', (Callback*)EditorWindow::find2_cb);
    m->add("&Search/&Replace...",0,        (Callback*)EditorWindow::replace_cb);
    m->add("&Search/Replace &Again",CTRL+'t', (Callback*)EditorWindow::replace2_cb);
    m->add("&Help/About...",     CTRL+'f',    (Callback*)about_cb);

    callback(quit_cb);

    tabGroup = new TabGroup(0, mnuHeight, w, groupHeight);
    tabGroup->begin();

    TabPage* eg = new TabPage(0, tabBegin, w, pageHeight, "Editor");
    editGroup = eg;
    editGroup->begin();
    editWnd = new EditorWindow(2, 2, w-4, pageHeight-4);
    m->user_data(editWnd); // the EditorWindow is callback user data (void*)

    editGroup->resizable(editWnd);
    editGroup->end();
    tabGroup->resizable(editGroup);

    TabPage* og = new TabPage(0, tabBegin, w, pageHeight, "Output");
    outputGroup = og;
    outputGroup->hide();
    outputGroup->begin();
    out = new Fl_Ansi_Window(2, 2, w-4, pageHeight-4);
    outputGroup->resizable(out);
    outputGroup->end();

    tabGroup->end();
    resizable(tabGroup);

    Group* statusBar = new Group(0, h-mnuHeight+1, w, mnuHeight-2);
    statusBar->begin();
    statusBar->box(NO_BOX);
    fileStatus = new Widget(0,0, w-122, mnuHeight-2);
    modStatus = new Widget(w-120, 0, 28, mnuHeight-2);
    runStatus = new Widget(w-90, 0, 28, mnuHeight-2);
    rowStatus = new Widget(w-60, 0, 28, mnuHeight-2);
    colStatus = new Widget(w-30, 0, 28, mnuHeight-2);

    for (int n=0; n<statusBar->children(); n++) {
        Widget* w = statusBar->child(n);
        w->labelfont(HELVETICA);
        w->box(THIN_DOWN_BOX);
        w->color(GRAY75);
    }

    fileStatus->align(ALIGN_INSIDE_LEFT);
    statusBar->resizable(fileStatus);
    statusBar->end();
    end();
}

MainWindow::~MainWindow() {}

bool MainWindow::isBreakExec(void) {
    return (runMode == break_state || runMode == quit_state);
}

void MainWindow::setModal(bool modal) {
    runMode = modal ? modal_state : run_state;
}

bool MainWindow::isModal() {
    return (runMode == modal_state);
}

bool MainWindow::isEdit() {
    return (runMode == edit_state);
}

void MainWindow::resetPen() {
    penDownX = 0;
    penDownY = 0;
    penState = 0;
}

void MainWindow::execLink(const char* file) {
    if (reload == false) {
        wnd->editWnd->loadFile(file, -1);
    } else {
        setTitle(file);
    }
    basicMain(file);
}

int MainWindow::handle(int e) {
    if (runMode == run_state && e == KEY && !event_key_state(RightCtrlKey)) {
        int k;
        k = event_key();
        switch (k) {
        case PageUpKey:
            dev_pushkey(SB_KEY_PGUP);
            break;
        case PageDownKey:
            dev_pushkey(SB_KEY_PGDN);
            break;
        case UpKey:
            dev_pushkey(SB_KEY_UP);
            break;
        case DownKey:
            dev_pushkey(SB_KEY_DN);
            break;
        case LeftKey:
            dev_pushkey(SB_KEY_LEFT);
            break;
        case RightKey:
            dev_pushkey(SB_KEY_RIGHT);
            break;
        case BackSpaceKey: 
        case DeleteKey:
            dev_pushkey(SB_KEY_BACKSPACE);
            break;
        case ReturnKey:
            dev_pushkey(13);
            break;
        default:
            dev_pushkey(k);
            break;
        }
        return 1;
    }
    return Window::handle(e);
}
