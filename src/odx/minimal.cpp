/*

  Cubietruck minimal library based on ...
  GP2X minimal library v0.A by rlyeh, (c) 2005. emulnation.info@rlyeh (swap it!)

  Thanks to Squidge, Robster, snaff, Reesy and NK, for the help & previous work! :-)

  Adapted for OpenDIngux by alekmaul <alekmaul@portabledev.com> August 2012

  License
  =======

  Free for non-commercial projects (it would be nice receiving a mail from you).
  Other cases, ask me first.

  GamePark Holdings is not allowed to use this library and/or use parts from it.

*/

#include <sys/time.h>
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "minimal.h"

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
COL_Renderer *colRenderer;

SDL_Event			event;
const unsigned char 	*keystates = 0;

volatile unsigned int	odx_palette[512];
unsigned int			odx_palette_rgb[256];

int						odx_clock=366;

int						rotate_controls=0;
unsigned char			odx_keys[OD_KEY_MAX];
SDL_Joystick			*odx_joyanalog[] = {0,0};

bool ui_exit = false;

signed int axis_x[]={0, 0}, axis_y[]={0,0};

// This is used to regulate video rate in video.cpp
unsigned int            odx_video_regulator = 1100;


void odx_video_flip(void)
{
  COL_RenderPresent(colRenderer);
}

void odx_video_flip_single(void)
{
  COL_RenderPresent(colRenderer);
}

#undef printf

void odx_get_render_dest(
  int *dx,
  int *dy,
  unsigned int *dw,
  unsigned int *dh,
  unsigned int tex_w, 
  unsigned int tex_h) 
{
  int win_x, win_y;
  int win_w, win_h;
  SDL_GetWindowSize(sdlWindow, &win_w, &win_h);
  SDL_GetWindowPosition(sdlWindow, &win_x, &win_y);  
  
  printf("SDL_GetWindowPosition %d,%d\n", win_x, win_y);  
  
  int flags = SDL_GetWindowFlags(sdlWindow);
    
  // Add window border offset
  
  if(!(flags & SDL_WINDOW_BORDERLESS)) {
    win_x += 1;  // TODO Find out where to get the SDL window left border width from
    win_y += 30; // TODO Find out where to get the SDL window top border hight from
  }
  
  int x,y;
  unsigned int w,h;
  if((tex_w * win_h) > (win_w * tex_h)) {
    // scale to width of window
    printf("scale to width of window\n");
    w = win_w;
    h = (win_w * tex_h) / tex_w;
    x = win_x;
    y = win_y + ((win_h - h) / 2);
  }
  else {
    // scale to height of window
    printf("scale to height of window\n");
    w = (win_h * tex_w) / tex_h;
    h = win_h;
    x = win_x + ((win_w - w) / 2);
    y = win_y; 
  }
  
  *dx = x; 
  *dy = y;
  *dh = h;
  *dw = w;
}

void odx_clear_background() {
  SDL_RenderClear(sdlRenderer);
  SDL_RenderPresent(sdlRenderer);
}

void odx_updateWindowPosition() {
  int dx,dy;
  unsigned int dw,dh;
  unsigned int tex_w, tex_h;
  COL_GetTextureSize(colRenderer, &tex_w, &tex_h);
  odx_get_render_dest(&dx, &dy, &dw, &dh, tex_w, tex_h);    
  COL_updateWindowPosition(colRenderer, dx, dy, dw, dh);
}

unsigned int odx_keyboard_read()
{
	unsigned int res = 0;

	while(SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_WINDOWEVENT:
      {
        SDL_WindowEvent *window_event = &event.window;
        switch (window_event->event) {
          case SDL_WINDOWEVENT_SHOWN:
              printf("Window %d shown\n", window_event->windowID);
              break;
          case SDL_WINDOWEVENT_HIDDEN:
              printf("Window %d hidden\n", window_event->windowID);
              break;
          case SDL_WINDOWEVENT_EXPOSED:
              printf("Window %d exposed\n", window_event->windowID);
              odx_clear_background();
              break;
          case SDL_WINDOWEVENT_MOVED:
              printf("Window %d moved to %d,%d\n",
                      window_event->windowID, window_event->data1,
                      window_event->data2);
              odx_updateWindowPosition();                    
              break;
          case SDL_WINDOWEVENT_RESIZED:
              printf("Window %d resized to %dx%d\n",
                      window_event->windowID, window_event->data1,
                      window_event->data2);
              odx_updateWindowPosition();       
              odx_clear_background();
                           
              break;
          case SDL_WINDOWEVENT_MINIMIZED:
              printf("Window %d minimized\n", window_event->windowID);
              break;
          case SDL_WINDOWEVENT_MAXIMIZED:
              printf("Window %d maximized\n", window_event->windowID);
              break;
          case SDL_WINDOWEVENT_RESTORED:
              printf("Window %d restored\n", window_event->windowID);
              break;
          case SDL_WINDOWEVENT_ENTER:
              printf("Mouse entered window %d\n",
                      window_event->windowID);
              break;
          case SDL_WINDOWEVENT_LEAVE:
              printf("Mouse left window %d\n", window_event->windowID);
              break;
          case SDL_WINDOWEVENT_FOCUS_GAINED:
              printf("Window %d gained keyboard focus\n",
                      window_event->windowID);
              break;
          case SDL_WINDOWEVENT_FOCUS_LOST:
              printf("Window %d lost keyboard focus\n",
                      window_event->windowID);
              break;
          case SDL_WINDOWEVENT_CLOSE:
              printf("Window %d closed\n", window_event->windowID);
              ui_exit = true;
              break;
          default:
              printf("Window %d got unknown event %d\n",
                      window_event->windowID, window_event->event);
              break;
          }
        }  
        break;
      default:
      break;
    }
  }
  
	keystates = SDL_GetKeyboardState(NULL);

	if (keystates[SDL_SCANCODE_Q] == SDL_PRESSED) {
        ui_exit = true;
	}

	// Keys for file chooser only...
	if(keystates[SDL_SCANCODE_LEFT] == SDL_PRESSED) res |= OD_LEFT;
	if(keystates[SDL_SCANCODE_RIGHT] == SDL_PRESSED)res |= OD_RIGHT;
	if(keystates[SDL_SCANCODE_UP] == SDL_PRESSED) res |= OD_UP;
	if(keystates[SDL_SCANCODE_DOWN] == SDL_PRESSED) res |= OD_DOWN;
	if(keystates[SDL_SCANCODE_RETURN] == SDL_PRESSED) res |= OD_A;
	if(keystates[SDL_SCANCODE_ESCAPE] == SDL_PRESSED) res |= OD_B;
	if(keystates[SDL_SCANCODE_SPACE] == SDL_PRESSED) res |= OD_START;
	
	return res;
}

bool odx_key_pressed(int keycode) {
	if(keystates == 0) return false;
	return keystates[keycode] == SDL_PRESSED;
}

bool odx_is_joy_button_pressed(int index, int button) {
    SDL_Joystick *joystick = odx_joyanalog[index];
    return (joystick != NULL) && SDL_JoystickGetButton(joystick, button);
}

bool odx_is_joy_axis_pressed (int index, int axis, int dir){
    
    SDL_Joystick *joystick = odx_joyanalog[index];
    if(joystick == NULL) return 0;
    int v = SDL_JoystickGetAxis(joystick, axis);
    switch (dir)
    {
        case 2: return v > +32; break;
        case 1: return v < -32; break;
        default: break;
    }
 	return 0;   
}

// For front end only
unsigned int odx_joystick_read(unsigned int index)
{
 	unsigned int res=0;
  
	// manage joystick
  SDL_Joystick *joystick = odx_joyanalog[index];
	if (joystick) {
		if (!rotate_controls) {
			axis_x[index] = SDL_JoystickGetAxis(joystick, 0)/256;
			axis_y[index] = SDL_JoystickGetAxis(joystick, 1)/256;
			if (axis_x[index] < -32) { res |=  OD_LEFT;  } // LEFT
			if (axis_x[index] > 32) { res |=  OD_RIGHT; } // RIGHT
			if (axis_y[index] < -32) { res |=  OD_UP;  } // UP
			if (axis_y[index] > 32) { res |=  OD_DOWN;  } // DOWN
		}
		else {
			axis_x[index] = SDL_JoystickGetAxis(joystick, 1)/256;
			axis_y[index] = SDL_JoystickGetAxis(joystick, 0)/256;
			if (axis_y[index] < -32) res |= OD_LEFT;
			if (axis_y[index] >  32) res |= OD_RIGHT;
			if (axis_x[index] < -32) res |= OD_UP;
			if (axis_x[index] >  32) res |= OD_DOWN;
		}
		
		if (SDL_JoystickGetButton(joystick,0)) { res |=  OD_A;  }  // BUTTON A
		if (SDL_JoystickGetButton(joystick,1)) { res |=  OD_B; }  // BUTTON B

		if (SDL_JoystickGetButton(joystick,2)) { res |=  OD_X;  }  // BUTTON X
		if (SDL_JoystickGetButton(joystick,3))  { res |=  OD_Y;  }   // BUTTON Y

		if (SDL_JoystickGetButton(joystick,4))  { res |=  OD_R;  }  // BUTTON R
		if (SDL_JoystickGetButton(joystick,5))  { res |=  OD_L;  }  // BUTTON L

		if (SDL_JoystickGetButton(joystick,6))  { res |=  OD_START; } // START
		if (SDL_JoystickGetButton(joystick,7)) { res |=  OD_SELECT; } // SELECT
	}
  // printf("keystat %#010x \n", res);   
	return res;
}

unsigned int odx_joystick_press ()
{
	unsigned int ExKey=0;
	for(int i = 0; i< 20 && !ui_exit; ++i) if  ((odx_joystick_read(0) | odx_keyboard_read()) != 0 ) { usleep (10000); }
	while (!ui_exit && ((ExKey=(odx_joystick_read(0) | odx_keyboard_read())) == 0 )) { usleep (10000); }
//	printf("keystat %#010x \n", ExKey);  
	return ExKey;
}

unsigned long odx_timer_read(void)
{
	struct timeval tval;
	gettimeofday(&tval, 0);
	return ((tval.tv_sec*1000000)+tval.tv_usec);
}

void odx_video_wait_vsync(void) 
{
}

void odx_init(int ticks_per_second, int bpp, int rate, int bits, int stereo, int Hz, bool fullscreen)
{
  printf("odx-init\n");

	/* All keys unpressed. */
	for(int i = 0 ; i < OD_KEY_MAX ; i++ ) {
		odx_keys[i] = 0;
	}
  // Initialize SDL.
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_AUDIO) < 0) {
          exit(1);
  }
  
  SDL_RendererInfo rendererInfo;
  int sdlRendererIndex;
  for(sdlRendererIndex=0; ;++sdlRendererIndex) {
    if(0!=SDL_GetRenderDriverInfo(sdlRendererIndex, &rendererInfo)) {
      printf("Could not find required driver\n");
      exit(1);
    }
    printf("Found software driver at index %d with name %s\n", sdlRendererIndex, rendererInfo.name);  

    if(strcmp(rendererInfo.name, "software")==0) {
        break;
    }
  }  

  printf("Using software driver at index %d with name %s\n", sdlRendererIndex, rendererInfo.name);  
   
  sdlWindow = SDL_CreateWindow(
    "Mame4Cubie", 
    SDL_WINDOWPOS_UNDEFINED, 
    SDL_WINDOWPOS_UNDEFINED, 
    fullscreen ? 1280 : 1024, 
    fullscreen ? 800 : 768, 
    fullscreen ? SDL_WINDOW_BORDERLESS|SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE);
      
  printf("Created window\n"); 
     
  sdlRenderer = SDL_CreateRenderer(sdlWindow, sdlRendererIndex, 0);
  
  printf("Created renderer %ld\n", sdlRenderer);
  
  SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
  
  odx_clear_background();
  
  colRenderer = COL_CreateRenderer(); 
  printf("Created col renderer %ld\n", colRenderer);
  
	/* General video & audio stuff */

  printf("Found %d joysticks\n", SDL_NumJoysticks());
	
  for(int i = 0; i < 2; ++i) {
    odx_joyanalog[i] = SDL_JoystickOpen(i);
    if (odx_joyanalog[i] != NULL )  {
          printf("Opened Joystick 0\n");
          printf("Name: %s\n", SDL_JoystickNameForIndex(i));
          printf("Number of Axes: %d\n", SDL_JoystickNumAxes(odx_joyanalog[i]));
          printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(odx_joyanalog[i]));
          printf("Number of Balls: %d\n", SDL_JoystickNumBalls(odx_joyanalog[i]));
    }     
  }
  
	SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN,SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP,SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT,SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT,SDL_IGNORE);
	SDL_ShowCursor(SDL_DISABLE);

	odx_sound_init(rate, bits, stereo);

	odx_set_video_mode(bpp,ODX_SCREEN_WIDTH,ODX_SCREEN_HEIGHT);

	odx_video_color8(0,0,0,0);
	odx_video_color8(255,255,255,255);
	odx_video_setpalette();
	
	odx_clear_video();
}

void odx_deinit(void)
{
printf("odx_deinit(void).\n");	

	odx_sound_thread_stop();
	COL_DestroyRenderer(colRenderer);
	SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK);
}

void odx_set_clock(int mhz)
{

}
 
void odx_set_video_mode(int bpp,int width,int height)
{
  printf("void odx_set_video_mode(int bpp %d,int width %d,int height %d)\n", bpp, width, height);
  
  int x,y,w,h;
  SDL_GetWindowSize(sdlWindow, &w, &h);
  SDL_GetWindowPosition(sdlWindow, &x, &y);  
  
  int dx,dy;
  unsigned int dw,dh;
  odx_get_render_dest(&dx, &dy, &dw, &dh, width, height);
  COL_CreateTexture(colRenderer, width, height, dx, dy, dw, dh);
                               
  printf("Created COL texture format %d\n",  SDL_PIXELFORMAT_ARGB8888);

  odx_clear_video();
}

void odx_clear_video() {
  printf("odx_clear_video()\n");
  COL_RendererClear(colRenderer);
  odx_video_flip();
}

// Font: THIN8X8.pf : Exported from PixelFontEdit 2.7.0
static const unsigned char fontdata8x8[2048] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Char 000 (.)
	0x7E, 0x81, 0xA5, 0x81, 0xBD, 0x99, 0x81, 0x7E,	// Char 001 (.)
	0x7E, 0xFF, 0xDB, 0xFF, 0xC3, 0xE7, 0xFF, 0x7E,	// Char 002 (.)
	0x6C, 0xFE, 0xFE, 0xFE, 0x7C, 0x38, 0x10, 0x00,	// Char 003 (.)
	0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x10, 0x00,	// Char 004 (.)
	0x38, 0x7C, 0x38, 0xFE, 0xFE, 0x7C, 0x38, 0x7C,	// Char 005 (.)
	0x10, 0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x7C,	// Char 006 (.)
	0x00, 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00,	// Char 007 (.)
	0xFF, 0xFF, 0xE7, 0xC3, 0xC3, 0xE7, 0xFF, 0xFF,	// Char 008 (.)
	0x00, 0x3C, 0x66, 0x42, 0x42, 0x66, 0x3C, 0x00,	// Char 009 (.)
	0xFF, 0xC3, 0x99, 0xBD, 0xBD, 0x99, 0xC3, 0xFF,	// Char 010 (.)
	0x0F, 0x07, 0x0F, 0x7D, 0xCC, 0xCC, 0xCC, 0x78,	// Char 011 (.)
	0x3C, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x7E, 0x18,	// Char 012 (.)
	0x3F, 0x33, 0x3F, 0x30, 0x30, 0x70, 0xF0, 0xE0,	// Char 013 (.)
	0x7F, 0x63, 0x7F, 0x63, 0x63, 0x67, 0xE6, 0xC0,	// Char 014 (.)
	0x99, 0x5A, 0x3C, 0xE7, 0xE7, 0x3C, 0x5A, 0x99,	// Char 015 (.)
	0x80, 0xE0, 0xF8, 0xFE, 0xF8, 0xE0, 0x80, 0x00,	// Char 016 (.)
	0x02, 0x0E, 0x3E, 0xFE, 0x3E, 0x0E, 0x02, 0x00,	// Char 017 (.)
	0x18, 0x3C, 0x7E, 0x18, 0x18, 0x7E, 0x3C, 0x18,	// Char 018 (.)
	0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x00,	// Char 019 (.)
	0x7F, 0xDB, 0xDB, 0x7B, 0x1B, 0x1B, 0x1B, 0x00,	// Char 020 (.)
	0x3E, 0x63, 0x38, 0x6C, 0x6C, 0x38, 0xCC, 0x78,	// Char 021 (.)
	0x00, 0x00, 0x00, 0x00, 0x7E, 0x7E, 0x7E, 0x00,	// Char 022 (.)
	0x18, 0x3C, 0x7E, 0x18, 0x7E, 0x3C, 0x18, 0xFF,	// Char 023 (.)
	0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, 0x00,	// Char 024 (.)
	0x18, 0x18, 0x18, 0x18, 0x7E, 0x3C, 0x18, 0x00,	// Char 025 (.)
	0x00, 0x18, 0x0C, 0xFE, 0x0C, 0x18, 0x00, 0x00,	// Char 026 (.) right arrow
	0x00, 0x30, 0x60, 0xFE, 0x60, 0x30, 0x00, 0x00,	// Char 027 (.)
	0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xFE, 0x00, 0x00,	// Char 028 (.)
	0x00, 0x24, 0x66, 0xFF, 0x66, 0x24, 0x00, 0x00,	// Char 029 (.)
	0x00, 0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x00, 0x00,	// Char 030 (.)
	0x00, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00, 0x00,	// Char 031 (.)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Char 032 ( )
	0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x40, 0x00,	// Char 033 (!)
	0x90, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Char 034 (")
	0x50, 0x50, 0xF8, 0x50, 0xF8, 0x50, 0x50, 0x00,	// Char 035 (#)
	0x20, 0x78, 0xA0, 0x70, 0x28, 0xF0, 0x20, 0x00,	// Char 036 ($)
	0xC8, 0xC8, 0x10, 0x20, 0x40, 0x98, 0x98, 0x00,	// Char 037 (%)
	0x70, 0x88, 0x50, 0x20, 0x54, 0x88, 0x74, 0x00,	// Char 038 (&)
	0x60, 0x20, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,	// Char 039 (')
	0x20, 0x40, 0x80, 0x80, 0x80, 0x40, 0x20, 0x00,	// Char 040 (()
	0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00,	// Char 041 ())
	0x00, 0x20, 0xA8, 0x70, 0x70, 0xA8, 0x20, 0x00,	// Char 042 (*)
	0x00, 0x00, 0x20, 0x20, 0xF8, 0x20, 0x20, 0x00,	// Char 043 (+)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x20, 0x40,	// Char 044 (,)
	0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00,	// Char 045 (-)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60, 0x00,	// Char 046 (.)
	0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00,	// Char 047 (/)
	0x70, 0x88, 0x98, 0xA8, 0xC8, 0x88, 0x70, 0x00,	// Char 048 (0)
	0x40, 0xC0, 0x40, 0x40, 0x40, 0x40, 0xE0, 0x00,	// Char 049 (1)
	0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0xF8, 0x00,	// Char 050 (2)
	0x70, 0x88, 0x08, 0x10, 0x08, 0x88, 0x70, 0x00,	// Char 051 (3)
	0x08, 0x18, 0x28, 0x48, 0xFC, 0x08, 0x08, 0x00,	// Char 052 (4)
	0xF8, 0x80, 0x80, 0xF0, 0x08, 0x88, 0x70, 0x00,	// Char 053 (5)
	0x20, 0x40, 0x80, 0xF0, 0x88, 0x88, 0x70, 0x00,	// Char 054 (6)
	0xF8, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40, 0x00,	// Char 055 (7)
	0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70, 0x00,	// Char 056 (8)
	0x70, 0x88, 0x88, 0x78, 0x08, 0x08, 0x70, 0x00,	// Char 057 (9)
	0x00, 0x00, 0x60, 0x60, 0x00, 0x60, 0x60, 0x00,	// Char 058 (:)
	0x00, 0x00, 0x60, 0x60, 0x00, 0x60, 0x60, 0x20,	// Char 059 (;)
	0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x00,	// Char 060 (<)
	0x00, 0x00, 0xF8, 0x00, 0xF8, 0x00, 0x00, 0x00,	// Char 061 (=)
	0x80, 0x40, 0x20, 0x10, 0x20, 0x40, 0x80, 0x00,	// Char 062 (>)
	0x78, 0x84, 0x04, 0x08, 0x10, 0x00, 0x10, 0x00,	// Char 063 (?)
	0x70, 0x88, 0x88, 0xA8, 0xB8, 0x80, 0x78, 0x00,	// Char 064 (@)
	0x20, 0x50, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x00,	// Char 065 (A)
	0xF0, 0x88, 0x88, 0xF0, 0x88, 0x88, 0xF0, 0x00,	// Char 066 (B)
	0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 0x00,	// Char 067 (C)
	0xF0, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x00,	// Char 068 (D)
	0xF8, 0x80, 0x80, 0xE0, 0x80, 0x80, 0xF8, 0x00,	// Char 069 (E)
	0xF8, 0x80, 0x80, 0xE0, 0x80, 0x80, 0x80, 0x00,	// Char 070 (F)
	0x70, 0x88, 0x80, 0x80, 0x98, 0x88, 0x78, 0x00,	// Char 071 (G)
	0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88, 0x00,	// Char 072 (H)
	0xE0, 0x40, 0x40, 0x40, 0x40, 0x40, 0xE0, 0x00,	// Char 073 (I)
	0x38, 0x10, 0x10, 0x10, 0x10, 0x90, 0x60, 0x00,	// Char 074 (J)
	0x88, 0x90, 0xA0, 0xC0, 0xA0, 0x90, 0x88, 0x00,	// Char 075 (K)
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8, 0x00,	// Char 076 (L)
	0x82, 0xC6, 0xAA, 0x92, 0x82, 0x82, 0x82, 0x00,	// Char 077 (M)
	0x84, 0xC4, 0xA4, 0x94, 0x8C, 0x84, 0x84, 0x00,	// Char 078 (N)
	0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00,	// Char 079 (O)
	0xF0, 0x88, 0x88, 0xF0, 0x80, 0x80, 0x80, 0x00,	// Char 080 (P)
	0x70, 0x88, 0x88, 0x88, 0xA8, 0x90, 0x68, 0x00,	// Char 081 (Q)
	0xF0, 0x88, 0x88, 0xF0, 0xA0, 0x90, 0x88, 0x00,	// Char 082 (R)
	0x70, 0x88, 0x80, 0x70, 0x08, 0x88, 0x70, 0x00,	// Char 083 (S)
	0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,	// Char 084 (T)
	0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00,	// Char 085 (U)
	0x88, 0x88, 0x88, 0x50, 0x50, 0x20, 0x20, 0x00,	// Char 086 (V)
	0x82, 0x82, 0x82, 0x82, 0x92, 0x92, 0x6C, 0x00,	// Char 087 (W)
	0x88, 0x88, 0x50, 0x20, 0x50, 0x88, 0x88, 0x00,	// Char 088 (X)
	0x88, 0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x00,	// Char 089 (Y)
	0xF8, 0x08, 0x10, 0x20, 0x40, 0x80, 0xF8, 0x00,	// Char 090 (Z)
	0xE0, 0x80, 0x80, 0x80, 0x80, 0x80, 0xE0, 0x00,	// Char 091 ([)
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00,	// Char 092 (\)
	0xE0, 0x20, 0x20, 0x20, 0x20, 0x20, 0xE0, 0x00,	// Char 093 (])
	0x20, 0x50, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00,	// Char 094 (^)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00,	// Char 095 (_)
	0x40, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Char 096 (`)
	0x00, 0x00, 0x70, 0x08, 0x78, 0x88, 0x74, 0x00,	// Char 097 (a)
	0x80, 0x80, 0xB0, 0xC8, 0x88, 0xC8, 0xB0, 0x00,	// Char 098 (b)
	0x00, 0x00, 0x70, 0x88, 0x80, 0x88, 0x70, 0x00,	// Char 099 (c)
	0x08, 0x08, 0x68, 0x98, 0x88, 0x98, 0x68, 0x00,	// Char 100 (d)
	0x00, 0x00, 0x70, 0x88, 0xF8, 0x80, 0x70, 0x00,	// Char 101 (e)
	0x30, 0x48, 0x40, 0xE0, 0x40, 0x40, 0x40, 0x00,	// Char 102 (f)
	0x00, 0x00, 0x34, 0x48, 0x48, 0x38, 0x08, 0x30,	// Char 103 (g)
	0x80, 0x80, 0xB0, 0xC8, 0x88, 0x88, 0x88, 0x00,	// Char 104 (h)
	0x20, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70, 0x00,	// Char 105 (i)
	0x10, 0x00, 0x30, 0x10, 0x10, 0x10, 0x90, 0x60,	// Char 106 (j)
	0x80, 0x80, 0x88, 0x90, 0xA0, 0xD0, 0x88, 0x00,	// Char 107 (k)
	0xC0, 0x40, 0x40, 0x40, 0x40, 0x40, 0xE0, 0x00,	// Char 108 (l)
	0x00, 0x00, 0xEC, 0x92, 0x92, 0x92, 0x92, 0x00,	// Char 109 (m)
	0x00, 0x00, 0xB0, 0xC8, 0x88, 0x88, 0x88, 0x00,	// Char 110 (n)
	0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x70, 0x00,	// Char 111 (o)
	0x00, 0x00, 0xB0, 0xC8, 0xC8, 0xB0, 0x80, 0x80,	// Char 112 (p)
	0x00, 0x00, 0x68, 0x98, 0x98, 0x68, 0x08, 0x08,	// Char 113 (q)
	0x00, 0x00, 0xB0, 0xC8, 0x80, 0x80, 0x80, 0x00,	// Char 114 (r)
	0x00, 0x00, 0x78, 0x80, 0x70, 0x08, 0xF0, 0x00,	// Char 115 (s)
	0x40, 0x40, 0xE0, 0x40, 0x40, 0x50, 0x20, 0x00,	// Char 116 (t)
	0x00, 0x00, 0x88, 0x88, 0x88, 0x98, 0x68, 0x00,	// Char 117 (u)
	0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20, 0x00,	// Char 118 (v)
	0x00, 0x00, 0x82, 0x82, 0x92, 0x92, 0x6C, 0x00,	// Char 119 (w)
	0x00, 0x00, 0x88, 0x50, 0x20, 0x50, 0x88, 0x00,	// Char 120 (x)
	0x00, 0x00, 0x88, 0x88, 0x98, 0x68, 0x08, 0x70,	// Char 121 (y)
	0x00, 0x00, 0xF8, 0x10, 0x20, 0x40, 0xF8, 0x00,	// Char 122 (z)
	0x10, 0x20, 0x20, 0x40, 0x20, 0x20, 0x10, 0x00,	// Char 123 ({)
	0x40, 0x40, 0x40, 0x00, 0x40, 0x40, 0x40, 0x00,	// Char 124 (|)
	0x40, 0x20, 0x20, 0x10, 0x20, 0x20, 0x40, 0x00,	// Char 125 (})
	0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Char 126 (~)
	0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0x00	// Char 127 (.)
};

static void odx_text(unsigned int *scr, int x, int y, char *text, int color, int w)
{
	unsigned int i,l;

	scr=scr+x+(y*w);
  

	for (i=0;i<strlen(text);i++) {
		
		for (l=0;l<8;l++) {
			scr[l*w+0]=(fontdata8x8[((text[i])*8)+l]&0x80)?odx_palette_rgb[color]:scr[l*w+0];
			scr[l*w+1]=(fontdata8x8[((text[i])*8)+l]&0x40)?odx_palette_rgb[color]:scr[l*w+1];
			scr[l*w+2]=(fontdata8x8[((text[i])*8)+l]&0x20)?odx_palette_rgb[color]:scr[l*w+2];
			scr[l*w+3]=(fontdata8x8[((text[i])*8)+l]&0x10)?odx_palette_rgb[color]:scr[l*w+3];
			scr[l*w+4]=(fontdata8x8[((text[i])*8)+l]&0x08)?odx_palette_rgb[color]:scr[l*w+4];
			scr[l*w+5]=(fontdata8x8[((text[i])*8)+l]&0x04)?odx_palette_rgb[color]:scr[l*w+5];
			scr[l*w+6]=(fontdata8x8[((text[i])*8)+l]&0x02)?odx_palette_rgb[color]:scr[l*w+6];
			scr[l*w+7]=(fontdata8x8[((text[i])*8)+l]&0x01)?odx_palette_rgb[color]:scr[l*w+7];
		}
		scr+=6;
	} 
}

void odx_gamelist_text_out(int x, int y, char *eltexto)
{
 void *k; unsigned int pitch; COL_LockTexture(colRenderer, &k, &pitch);

	unsigned int *address=(unsigned int *)k;

	char texto[43];
	strncpy(texto,eltexto,42);
	texto[42]=0;
	if (texto[0]!='-')
		odx_text(address,x+1,y+1,texto,0, pitch);
	odx_text(address,x,y,texto,255, pitch);
}

/* Variadic functions guide found at http://www.unixpapa.com/incnote/variadic.html */
void odx_gamelist_text_out_fmt(int x, int y, char* fmt, ...)
{
	char strOut[128];
	va_list marker;
	
	va_start(marker, fmt);
	vsprintf(strOut, fmt, marker);
	va_end(marker);	

	odx_gamelist_text_out(x, y, strOut);
}

static int logy=0;

void odx_printf_init(void)
{
	logy=0;
}

static void odx_text_log(char *texto)
{
	if (!logy) {
		odx_clear_video();
	}
  
	void *k; unsigned int pitch; COL_LockTexture(colRenderer, &k, &pitch);

	unsigned int *address=(unsigned int *)k;

	odx_text(address,0,logy,texto,255, pitch); 	
  
	COL_UnlockTexture(colRenderer);
	COL_RenderCopyAndPresent(colRenderer);  
  
	logy+=8;
	if(logy>239) logy=0;
}

/* Variadic functions guide found at http://www.unixpapa.com/incnote/variadic.html */
void odx_printf(char* fmt, ...)
{
	int i,c;
	char strOut[4096];
	char str[41];
	va_list marker;
	
	va_start(marker, fmt);
	vsprintf(strOut, fmt, marker);
	va_end(marker);	

	c=0;
	for (i=0;i<strlen(strOut);i++)
	{
		str[c]=strOut[i];
		if (str[c]=='\n')
		{
			str[c]=0;
			odx_text_log(str);
			c=0;
		}
		else if (c==39)
		{
			str[40]=0;
			odx_text_log(str);
			c=0;
		}		
		else
		{
			c++;
		}
	}
}
