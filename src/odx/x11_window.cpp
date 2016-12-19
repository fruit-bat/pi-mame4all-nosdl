#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minimal.h"
#include "driver.h"

static struct KeyboardInfo XK_SYMS[] =
{
    { "A",            XK_A,                KEYCODE_A },
    { "B",            XK_B,                KEYCODE_B },
    { "C",            XK_C,                KEYCODE_C },
    { "D",            XK_D,                KEYCODE_D },
    { "E",            XK_E,                KEYCODE_E },
    { "F",            XK_F,                KEYCODE_F },
    { "G",            XK_G,                KEYCODE_G },
    { "H",            XK_H,                KEYCODE_H },
    { "I",            XK_I,                KEYCODE_I },
    { "J",            XK_J,                KEYCODE_J },
    { "K",            XK_K,                KEYCODE_K },
    { "L",            XK_L,                KEYCODE_L },
    { "M",            XK_M,                KEYCODE_M },
    { "N",            XK_N,                KEYCODE_N },
    { "O",            XK_O,                KEYCODE_O },
    { "P",            XK_P,                KEYCODE_P },
    { "Q",            XK_Q,                KEYCODE_Q },
    { "R",            XK_R,                KEYCODE_R },
    { "S",            XK_S,                KEYCODE_S },
    { "T",            XK_T,                KEYCODE_T },
    { "U",            XK_U,                KEYCODE_U },
    { "V",            XK_V,                KEYCODE_V },
    { "W",            XK_W,                KEYCODE_W },
    { "X",            XK_X,                KEYCODE_X },
    { "Y",            XK_Y,                KEYCODE_Y },
    { "Z",            XK_Z,                KEYCODE_Z },
    { "0",            XK_0,                KEYCODE_0 },
    { "1",            XK_1,                KEYCODE_1 },
    { "2",            XK_2,                KEYCODE_2 },
    { "3",            XK_3,                KEYCODE_3 },
    { "4",            XK_4,                KEYCODE_4 },
    { "5",            XK_5,                KEYCODE_5 },
    { "6",            XK_6,                KEYCODE_6 },
    { "7",            XK_7,                KEYCODE_7 },
    { "8",            XK_8,                KEYCODE_8 },
    { "9",            XK_9,                KEYCODE_9 },
    { "0 PAD",        XK_KP_0,             KEYCODE_0_PAD },
    { "1 PAD",        XK_KP_1,             KEYCODE_1_PAD },
    { "2 PAD",        XK_KP_2,             KEYCODE_2_PAD },
    { "3 PAD",        XK_KP_3,             KEYCODE_3_PAD },
    { "4 PAD",        XK_KP_4,             KEYCODE_4_PAD },
    { "5 PAD",        XK_KP_5,             KEYCODE_5_PAD },
    { "6 PAD",        XK_KP_6,             KEYCODE_6_PAD },
    { "7 PAD",        XK_KP_7,             KEYCODE_7_PAD },
    { "8 PAD",        XK_KP_8,             KEYCODE_8_PAD },
    { "9 PAD",        XK_KP_9,             KEYCODE_9_PAD },
    { "F1",           XK_F1,               KEYCODE_F1 },
    { "F2",           XK_F2,               KEYCODE_F2 },
    { "F3",           XK_F3,               KEYCODE_F3 },
    { "F4",           XK_F4,               KEYCODE_F4 },
    { "F5",           XK_F5,               KEYCODE_F5 },
    { "F6",           XK_F6,               KEYCODE_F6 },
    { "F7",           XK_F7,               KEYCODE_F7 },
    { "F8",           XK_F8,               KEYCODE_F8 },
    { "F9",           XK_F9,               KEYCODE_F9 },
    { "F10",          XK_F10,              KEYCODE_F10 },
    { "F11",          XK_F11,              KEYCODE_F11 },
    { "F12",          XK_F12,              KEYCODE_F12 },
    { "ESC",          XK_Escape,           KEYCODE_ESC },
    { "~",            XK_asciitilde,       KEYCODE_TILDE },
    { "-",            XK_minus,            KEYCODE_MINUS },
    { "=",            XK_equal,            KEYCODE_EQUALS },
    { "BKSPACE",      XK_BackSpace,        KEYCODE_BACKSPACE },
    { "TAB",          XK_Tab,              KEYCODE_TAB },
    { "[",            XK_bracketleft,      KEYCODE_OPENBRACE },
    { "]",            XK_bracketright,     KEYCODE_CLOSEBRACE },
    { "ENTER",        XK_Return,           KEYCODE_ENTER },
    { ";",            XK_semicolon,        KEYCODE_COLON },
    { ":",            XK_colon,            KEYCODE_QUOTE },
    { "\\",           XK_backslash,        KEYCODE_BACKSLASH },
//    { "<",            XK_less,             KEYCODE_BACKSLASH2 }, // TODO
    { ",",            XK_comma,            KEYCODE_COMMA },
    { ".",            XK_period,           KEYCODE_STOP },
    { "/",            XK_slash,            KEYCODE_SLASH },
    { "SPACE",        XK_space,            KEYCODE_SPACE },
    { "INS",          XK_Insert,           KEYCODE_INSERT },
    { "DEL",          XK_Delete,           KEYCODE_DEL },
    { "HOME",         XK_Home,             KEYCODE_HOME },
    { "END",          XK_End,              KEYCODE_END },
    { "PGUP",         XK_Page_Up,          KEYCODE_PGUP },
    { "PGDN",         XK_Page_Down,        KEYCODE_PGDN },
    { "LEFT",         XK_Left,             KEYCODE_LEFT },
    { "RIGHT",        XK_Right,            KEYCODE_RIGHT },
    { "UP",           XK_Up,               KEYCODE_UP },
    { "DOWN",         XK_Down,             KEYCODE_DOWN },
    { "/ PAD",        XK_KP_Divide,        KEYCODE_SLASH_PAD },
    { "* PAD",        XK_KP_Multiply,      KEYCODE_ASTERISK },
    { "- PAD",        XK_KP_Subtract,      KEYCODE_MINUS_PAD },
    { "+ PAD",        XK_KP_Add,           KEYCODE_PLUS_PAD },
    { ". PAD",        XK_KP_Delete,        KEYCODE_DEL_PAD },
    { "ENTER PAD",    XK_KP_Enter,         KEYCODE_ENTER_PAD },
    { "PRTSCR",       XK_Print,            KEYCODE_PRTSCR },
    { "PAUSE",        XK_Pause,            KEYCODE_PAUSE },
    { "LSHIFT",       XK_Shift_L,          KEYCODE_LSHIFT },
    { "RSHIFT",       XK_Shift_R,          KEYCODE_RSHIFT },
    { "LCTRL",        XK_Control_L,        KEYCODE_LCONTROL },
    { "RCTRL",        XK_Control_R,        KEYCODE_RCONTROL },
    { "ALT",          XK_Alt_L,            KEYCODE_LALT },
    { "ALTGR",        XK_Alt_R,            KEYCODE_RALT },
//    { "LWIN",         XK_LWIN,             KEYCODE_OTHER },
//    { "RWIN",         XK_RWIN,             KEYCODE_OTHER },
//    { "MENU",         XK_MENU,             KEYCODE_OTHER },
    { "SCRLOCK",      XK_Scroll_Lock,      KEYCODE_SCRLOCK },
    { "NUMLOCK",      XK_Num_Lock,         KEYCODE_NUMLOCK },
    { "CAPSLOCK",     XK_Caps_Lock,        KEYCODE_CAPSLOCK },
    { 0, 0, 0 }    /* end of table */
};

class OdxX11Window {

private:

    Display *display;
    Window window;
    Window root;
    int fullscreen_flag;
    int x, y, w, h;
    int s;
    void (*position_listener_ptr)(int x, int y, int w, int h);
    KeyboardInfo *key_info;
    bool key_state[256];
    
public:

    void update_window_pos() {
        Window unused;
        XTranslateCoordinates(
            display,
            window,  // get position for this window...
            root,    // ...relative to this one
            0, 0,    // local left top coordinates of the wnd
            &x,      // these is position of wnd in root_window
            &y,      // ...
          &unused);       
    }
    
    void setup_keyinfo() {
		unsigned int n = sizeof(XK_SYMS)/sizeof(KeyboardInfo);
		key_info = new KeyboardInfo[n];
		for(int i = 0; i < n; ++i) {
			key_info[i].name = XK_SYMS[i].name;
			key_info[i].code = XKeysymToKeycode(display, XK_SYMS[i].code);
			key_info[i].standardcode = XK_SYMS[i].standardcode;
		}
		for(int i = 0; i < 256; ++i) {
			key_state[i] = false;
		}
	}
    
    ~OdxX11Window() {
        if(window) XDestroyWindow(display, window);
        if(display != NULL) XCloseDisplay(display);
        delete key_info;
    }
    
    OdxX11Window(
        bool fullscreen,
        void (*pl)(int x, int y, int w, int h)) : 
        
        display(NULL), 
        window(0),
        position_listener_ptr(pl),
        key_info(0) {
       
       
        const int initial_width = 300;
        const int initial_height = 300;
       
        w = initial_width;
        h = initial_height;
       
        fullscreen_flag = fullscreen;

        display = XOpenDisplay(NULL);
       
        if (display == NULL) {
          fprintf(stderr, "Cannot open display\n");
          exit(1);
        }
        
        setup_keyinfo();

        s = DefaultScreen(display);
        root = RootWindow(display, s);
        XWindowAttributes getWinAttr;
        XGetWindowAttributes(display, root, &getWinAttr);
        XSetWindowAttributes wa;
     
        window = XCreateWindow(
           display,
           root,
           0,
           0,
           initial_width,
           initial_height,
           0,
           CopyFromParent,
           InputOutput,
           CopyFromParent,
           0,
           &wa );

        Atom atom_fullscreen = XInternAtom ( display, "_NET_WM_STATE_FULLSCREEN", True );
        Atom atom_state   = XInternAtom ( display, "_NET_WM_STATE", True );

        XChangeProperty (
          display,
          window,
          atom_state,
          XA_ATOM,  
          32,  
          PropModeReplace,
          (unsigned char*) &atom_fullscreen,  
          fullscreen_flag ? 1 : 0 );
    
        XSelectInput(
          display,
          window, 
          ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);
         
        XMapWindow(
          display, 
          window);
          
        update_window_pos(); 
    }
    
    void process_events() {
        XEvent e;
        while (XPending(display) > 0) {
            XNextEvent(display, &e);
            if (e.type == ConfigureNotify) {
                XConfigureEvent xce = e.xconfigure;                  
                update_window_pos();
                w = xce.width;
                h = xce.height;
                if(position_listener_ptr) {
                    position_listener_ptr(x, y, w, h);
                }
            }
            if (e.type == Expose) {
                XFillRectangle(
                    display, 
                    window,
                    DefaultGC(display, s), 0, 0, w, h); // Clear the screen
            }
            if (e.type == KeyPress) {
                 XKeyEvent xke = e.xkey;
                 int keycode = xke.keycode;
                 key_state[keycode] = true;
            }
            if (e.type == KeyRelease) {
                 XKeyEvent xke = e.xkey;
                 int keycode = xke.keycode;
                 key_state[keycode] = false;
           }            
        }        
    }
    
    void get_window_geometry(int *xp, int *yp, int *wp, int *hp) {
        *xp = x; *yp = y; *wp = w; *hp = h;
    }
    
    KeyboardInfo* get_keyboard_info() {
		return key_info;
	}
	
	bool is_key_pressed(unsigned int keycode) {
		if(keycode > 255) return false;
		return key_state[keycode];
	}
};

OdxX11Window *odxX11Window = NULL;

void odx_window_pos(int *x, int *y, int *w, int *h) {
    odxX11Window->get_window_geometry(x, y, w, h);
}

void odx_window_create(
    bool fullscreen,
    void (*position_listener)(int x, int y, int w, int h)) {
        
    odxX11Window = new OdxX11Window(fullscreen, position_listener);   
}

void odx_window_destroy() {
    if(odxX11Window != NULL) delete odxX11Window;   
}

void odx_window_process_events() {
    odxX11Window->process_events();
}

bool odx_window_is_key_pressed(unsigned int keycode) {
	return odxX11Window->is_key_pressed(keycode);
}

KeyboardInfo* odx_window_get_keyboard_info() {
	return odxX11Window->get_keyboard_info();
}
