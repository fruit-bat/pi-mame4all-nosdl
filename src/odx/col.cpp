#include <bcm_host.h>
#include <assert.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>
#include "col.h"

#define ELEMENT_CHANGE_DEST_RECT      (1<<2)

static DISPMANX_RESOURCE_HANDLE_T   resource0;

DISPMANX_ELEMENT_HANDLE_T dispman_element;
DISPMANX_DISPLAY_HANDLE_T dispman_display;
DISPMANX_UPDATE_HANDLE_T dispman_update;

EGLDisplay display = NULL;
EGLSurface surface = NULL;

static COL_Renderer *col_renderer = NULL;
#define RU32(X) (((X)+31)&~0x1f)

static
COL_Texture* COL_TextureCreate(const unsigned int w, const unsigned int h, unsigned int d) {
  COL_Texture *const texture = (COL_Texture *) calloc(1, sizeof(*texture));
  const unsigned int s = RU32(w)*h*d;
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
  free(texture);
}

void
vsync_callback(
    DISPMANX_UPDATE_HANDLE_T update,
    void *arg)
{
    if(col_renderer) {
        COL_Texture *const texture = col_renderer->texture; 
        if(texture) {
            int buffer_index = (texture->current_buffer+texture->buffer_count-1)%texture->buffer_count;
            uint8_t *const buffer = texture->buffers[buffer_index];
            
            int surface_width = RU32(texture->w);
            int surface_height = texture->h;

            VC_RECT_T dst_rect;

            vc_dispmanx_rect_set( &dst_rect, 0, 0, surface_width, surface_height );

            vc_dispmanx_resource_write_data( 
                 resource0,
                 VC_IMAGE_ARGB8888, 
                 surface_width*4,
                 buffer,
                 &dst_rect );
                 
            vc_dispmanx_element_change_source( dispman_update, dispman_element, resource0 );
        }
    }
}

static
void COL_TexturePresent(COL_Texture *const texture) {
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
	
	dispman_update = vc_dispmanx_update_start( 0 );

	vc_dispmanx_rect_set( 
		&dst_rect, 
		x, //options.display_border, 
		y, //options.display_border,
		w, 
		h);
    
	vc_dispmanx_element_change_attributes(dispman_update,
                                          dispman_element,
                                          ELEMENT_CHANGE_DEST_RECT,
                                          1,
                                          255,
                                          &dst_rect,
                                          0,
                                          resource0,
                                          (DISPMANX_TRANSFORM_T) 0 );

	vc_dispmanx_update_submit( dispman_update, 0, 0 );
}

void COL_RendererClear(COL_Renderer *const renderer) {
  COL_TextureClear(renderer->texture);
}

void COL_TextureFlush(COL_Texture *const texture){
}

int
COL_CreateTexture(COL_Renderer * renderer, 
  const unsigned int tw, const unsigned int th,
  const int x, const unsigned int y, const unsigned int w, const unsigned int h)
{
  printf("COL_CreateTexture(const unsigned int tw %d, const unsigned int th %d, const int x %d, const unsigned int y %d, const unsigned int w %d, const unsigned int h %d)\n",
  tw,th,x,y,w,h);
  
  COL_Texture *col_texture = COL_TextureCreate(tw,th, 4);
   
 	uint32_t display_width = w, display_height = h;
    
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect; 
  
    vc_dispmanx_vsync_callback(dispman_display, vsync_callback, NULL);
          
	//Make sure bitmap type matches the source for better performance
	uint32_t crap;
	resource0 = vc_dispmanx_resource_create(VC_IMAGE_ARGB8888, RU32(tw), th, &crap);
    
	vc_dispmanx_rect_set( 
		&dst_rect, 
		x, //options.display_border, 
		y, //options.display_border,
		display_width, 
		display_height);
		
	vc_dispmanx_rect_set( &src_rect, 0, 0, RU32(tw) << 16, th << 16);
    
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
    
	vc_dispmanx_update_submit_sync( dispman_update );
 
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
  if(col_texture != NULL) {  
    renderer->texture = 0;
    
	int ret;
	printf("Shuting down dispmanx\n");
    vc_dispmanx_vsync_callback(dispman_display, NULL, NULL);
	dispman_update = vc_dispmanx_update_start( 0 );
	ret = vc_dispmanx_element_remove( dispman_update, dispman_element );
	ret = vc_dispmanx_update_submit_sync( dispman_update );
	dispman_update = vc_dispmanx_update_start( 0 );
	ret = vc_dispmanx_resource_delete( resource0 );
	assert(ret == 0);
	ret = vc_dispmanx_update_submit_sync( dispman_update );
	assert(ret == 0);

    COL_TextureFree(col_texture);
  }
}

void
COL_DestroyRenderer(COL_Renderer * renderer)
{
	printf("COL_DestroyRenderer\n");

    COL_DestroyTexture(renderer);
	if(renderer) {
        
		printf("vc_dispmanx_display_close\n");
        vc_dispmanx_display_close( dispman_display );
        dispman_display = 0;
        
		printf("bcm_host_deinit\n");
		bcm_host_deinit();
		free(renderer);
	}
}

COL_Renderer *COL_CreateRenderer()
{
  //Initialise dispmanx
  bcm_host_init();

  col_renderer = (COL_Renderer *) calloc(1, sizeof(*col_renderer));
  
  if(!col_renderer) {
      bcm_host_deinit();
      return NULL;    
  }
  col_renderer->texture = NULL;
  
  dispman_display = vc_dispmanx_display_open( 0 );
  
  if(dispman_display == 0) {
     printf("failed vc_dispmanx_display_open\n"); 
     bcm_host_deinit();
     free(col_renderer);
     return NULL;    
  }
  
  return col_renderer;
}  
