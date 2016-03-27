#include <bcm_host.h>
#include <assert.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <SDL2/SDL.h>
#include "col.h"
#define SDL_free free
#define SDL_calloc calloc
#define SDL_malloc malloc

#define ELEMENT_CHANGE_DEST_RECT      (1<<2)

// create two resources for 'page flipping'
static DISPMANX_RESOURCE_HANDLE_T   resource0;
static DISPMANX_RESOURCE_HANDLE_T   resource1;
static DISPMANX_RESOURCE_HANDLE_T   resource_bg;

// these are used for switching between the buffers
static DISPMANX_RESOURCE_HANDLE_T cur_res;
static DISPMANX_RESOURCE_HANDLE_T prev_res;
static DISPMANX_RESOURCE_HANDLE_T tmp_res;

DISPMANX_ELEMENT_HANDLE_T dispman_element;
DISPMANX_ELEMENT_HANDLE_T dispman_element_bg;
DISPMANX_DISPLAY_HANDLE_T dispman_display;
DISPMANX_UPDATE_HANDLE_T dispman_update;

void gles2_create(int display_width, int display_height, int bitmap_width, int bitmap_height, int depth);
void gles2_destroy();
void gles2_palette_changed();
void gles2_draw(void *screen, int width, int height, int depth);

EGLDisplay display = NULL;
EGLSurface surface = NULL;
static EGLContext context = NULL;
static EGL_DISPMANX_WINDOW_T nativewindow;


static
COL_Texture* COL_TextureCreate(const unsigned int w, const unsigned int h, unsigned int d) {
  COL_Texture *const texture = (COL_Texture *) SDL_calloc(1, sizeof(*texture));
  const unsigned int s = w*h*d;
  texture->w = w;
  texture->h = h;
  texture->buffer_size = s;
  texture->buffer_count = COL_TEXTURE_BUFFER_COUNT;
  texture->current_buffer = 0;
  int i;
  for(i=0; i<texture->buffer_count; ++i){
    texture->buffers[i] = (uint8_t*)malloc(s);
    memset(texture->buffers[i], 0, s);
  }
  return texture;
}

static
void COL_TextureFree(COL_Texture *const texture) {
  int i;
  for(i=0; i<texture->buffer_count; ++i){
    free(texture->buffers[i]);
  } 
  SDL_free(texture);
}

static
void COL_TexturePresent(COL_Texture *const texture) {
  uint8_t *const buffer = texture->buffers[texture->current_buffer];


	int surface_width = texture->w;
	int surface_height = texture->h;

  	VC_RECT_T dst_rect;

	vc_dispmanx_rect_set( &dst_rect, 0, 0, surface_width, surface_height );

	vc_dispmanx_resource_write_data( 
		 cur_res,
		 VC_IMAGE_ARGB8888, 
		 surface_width*4,
		 buffer,
		 &dst_rect );
		 
	dispman_update = vc_dispmanx_update_start( 0 );
	vc_dispmanx_element_change_source( dispman_update, dispman_element, cur_res );
	vc_dispmanx_update_submit( dispman_update, 0, 0 );

	// swap current resource
	tmp_res = cur_res;
	cur_res = prev_res;
	prev_res = tmp_res;
  
  
  texture->current_buffer = (texture->current_buffer+1)%texture->buffer_count;
}

static
void COL_TextureCopyAndPresent(COL_Texture *const texture) {
  uint8_t *const b1 = texture->buffers[texture->current_buffer];
  uint8_t *const b2 = texture->buffers[(texture->current_buffer+1)%texture->buffer_count];
  memcpy(b2, b1, texture->buffer_size);
  COL_TexturePresent(texture);
}

static
void* COL_TextureGetPixels(COL_Texture *const texture) {
  return texture->buffers[texture->current_buffer];
}

static
void COL_TextureClear(COL_Texture *const texture) {
  memset(COL_TextureGetPixels(texture), 0, texture->buffer_size);
}

void COL_updateWindowPosition(COL_Renderer * renderer, const int x, const unsigned int y, const unsigned int w, const int h) {

	printf("COL_updateWindowPosition\n");

	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	vc_dispmanx_rect_set( 
		&dst_rect, 
		x, //options.display_border, 
		y, //options.display_border,
		w, 
		h);
/*
VCHPRE_ int VCHPOST_ vc_dispmanx_element_change_attributes( DISPMANX_UPDATE_HANDLE_T update, 
                                                            DISPMANX_ELEMENT_HANDLE_T element,
                                                            uint32_t change_flags,
                                                            int32_t layer,
                                                            uint8_t opacity,
                                                            const VC_RECT_T *dest_rect,
                                                            const VC_RECT_T *src_rect,
                                                            DISPMANX_RESOURCE_HANDLE_T mask,
                                                            DISPMANX_TRANSFORM_T transform );
                                                            * */
    // TODO Not working 
    /*
	vc_dispmanx_element_change_attributes(dispman_update,
                                          dispman_element,
                                          ELEMENT_CHANGE_DEST_RECT,
                                          1,
                                          255,
                                          &dst_rect,
                                          &src_rect,
                                          resource0,
                                          (DISPMANX_TRANSFORM_T) 0 );
*/
}

void COL_RendererClear(COL_Renderer *const renderer) {
  COL_TextureClear(renderer->texture);
}

void COL_TextureFlush(COL_Texture *const texture){
//  ve_flush_cache(COL_TextureGetPixels(texture), texture->buffer_size);
}

static uint32_t display_adj_width, display_adj_height;		//display size minus border


int
COL_CreateTexture(COL_Renderer * renderer, 
  const unsigned int tw, const unsigned int th,
  const int x, const unsigned int y, const unsigned int w, const unsigned int h)
{
  printf("COL_CreateTexture(const unsigned int tw %d, const unsigned int th %d, const int x %d, const unsigned int y %d, const unsigned int w %d, const unsigned int h %d)\n",
  tw,th,x,y,w,h);
  
  COL_Texture *col_texture = COL_TextureCreate(tw,th, 4);
  
  
  
  	int ret;

  
  
 	uint32_t display_width = w, display_height = h;
    
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect; 
  
 // 	graphics_get_display_size(0 /* LCD */, &display_width, &display_height);
    
	dispman_display = vc_dispmanx_display_open( 0 );
	assert( dispman_display != 0 );
        
	// Add border around bitmap for TV
//	display_width -= options.display_border * 2;
//	display_height -= options.display_border * 2;
    
	//Create two surfaces for flipping between
	//Make sure bitmap type matches the source for better performance
	uint32_t crap;
	resource0 = vc_dispmanx_resource_create(VC_IMAGE_ARGB8888, tw, th, &crap);
	resource1 = vc_dispmanx_resource_create(VC_IMAGE_ARGB8888, tw, th, &crap);
    
	vc_dispmanx_rect_set( 
		&dst_rect, 
		x, //options.display_border, 
		y, //options.display_border,
		display_width, 
		display_height);
		
	vc_dispmanx_rect_set( &src_rect, 0, 0, tw << 16, th << 16);
    
	//Make sure mame and background overlay the menu program
	dispman_update = vc_dispmanx_update_start( 0 );
    
	// create the 'window' element - based on the first buffer resource (resource0)
	dispman_element = vc_dispmanx_element_add(dispman_update,
                                              dispman_display,
                                              1,
                                              &dst_rect,
                                              resource0,
                                              &src_rect,
                                              DISPMANX_PROTECTION_NONE,
                                              0,
                                              0,
                                              (DISPMANX_TRANSFORM_T) 0 );
    
	ret = vc_dispmanx_update_submit_sync( dispman_update );
    
	// setup swapping of double buffers
	cur_res = resource1;
	prev_res = resource0;
  
  
  
  
  
  
//  gles2_create(w, h, tw, th, 32);
//	disp_set_para(ve_virt2phys(COL_TextureGetPixels(col_texture)), 0,	COLOR_ARGB8888, tw, th, 0, 0, tw, th, x, y, w, h);

  if( renderer->texture) {  
    COL_TextureFree( renderer->texture);
  }
  
  renderer->texture = col_texture;

  return 0;
}

void COL_GetTextureSize(COL_Renderer * renderer, unsigned int *w, unsigned int *h) {
  COL_Texture *const col_texture = renderer->texture;  
  *w = col_texture->w;
  *h = col_texture->h;
}

int
COL_LockTexture(COL_Renderer * renderer, void **pixels, unsigned int *pitch)
{
  COL_Texture *const col_texture = renderer->texture;
  *pixels = COL_TextureGetPixels(col_texture);
  *pitch = col_texture->w;
  return 0;
}

void
COL_UnlockTexture(COL_Renderer * renderer)
{
  COL_Texture *const col_texture = renderer->texture;
  COL_TextureFlush(col_texture);
}

void
COL_RenderPresent(COL_Renderer *const renderer)
{
  if(renderer->texture) {
    COL_Texture *const texture = renderer->texture;
    COL_TexturePresent(texture);
  }
}

void
COL_RenderCopyAndPresent(COL_Renderer *const renderer)
{
  if(renderer->texture) {
    COL_Texture *const texture = renderer->texture;
    COL_TextureCopyAndPresent(texture);
  }
}

void
COL_DestroyTexture(COL_Renderer * renderer)
{
  COL_Texture *const col_texture = renderer->texture;
  if(col_texture) {  
//    disp_close();
    COL_TextureFree(col_texture);
    renderer->texture = 0;
  }
}

void
COL_DestroyRenderer(COL_Renderer * renderer)
{
  if(renderer) {
    bcm_host_deinit();
    COL_DestroyTexture(renderer);
    SDL_free(renderer);
  }
}

COL_Renderer *COL_CreateRenderer()
{
    //Initialise dispmanx
    bcm_host_init();

  COL_Renderer *col_renderer;
  
  col_renderer = (COL_Renderer *) SDL_calloc(1, sizeof(*col_renderer));
  
  if(!col_renderer) {
      bcm_host_deinit();
      SDL_OutOfMemory();
      return NULL;    
  }

  col_renderer->texture = NULL;

  return col_renderer;
}  
