#include <bcm_host.h>
#include <assert.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <SDL2/SDL.h>
#include "col.h"

#define SDL_free free
#define SDL_calloc calloc
#define SDL_malloc malloc

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
//  disp_wait_for_frame();    
//  disp_new_frame(ve_virt2phys(buffer), ve_virt2phys(0), 0);
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
//  disp_set_dest(x,y,w,h);
}

void COL_RendererClear(COL_Renderer *const renderer) {
  COL_TextureClear(renderer->texture);
}

void COL_TextureFlush(COL_Texture *const texture){
//  ve_flush_cache(COL_TextureGetPixels(texture), texture->buffer_size);
}

int
COL_CreateTexture(COL_Renderer * renderer, 
  const unsigned int tw, const unsigned int th,
  const int x, const unsigned int y, const unsigned int w, const unsigned int h)
{
  printf("COL_CreateTexture(const unsigned int tw %d, const unsigned int th %d, const int x %d, const unsigned int y %d, const unsigned int w %d, const unsigned int h %d)\n",
  tw,th,x,y,w,h);
  
  COL_Texture *col_texture = COL_TextureCreate(tw,th, 4);
  
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
