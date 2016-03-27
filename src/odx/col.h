#ifndef _COL_H
#define _COL_H

#define COL_TEXTURE_BUFFER_COUNT (2)

typedef struct COL_Texture {
  unsigned int buffer_count;
  unsigned int buffer_size;
  unsigned int current_buffer;
  unsigned int w;
  unsigned int h;
  uint8_t* buffers[COL_TEXTURE_BUFFER_COUNT];
  
} COL_Texture;

typedef struct COL_Renderer {
  COL_Texture *texture;
} COL_Renderer;


int COL_CreateTexture(COL_Renderer * renderer, 
  const unsigned int tw, const unsigned int th,
  const int x, const unsigned int y, const unsigned int w, const unsigned int h);
int COL_LockTexture(COL_Renderer * renderer, void **pixels, unsigned int *pitch);
void COL_UnlockTexture(COL_Renderer * renderer);
void COL_RenderPresent(COL_Renderer *const renderer);
void COL_RenderCopyAndPresent(COL_Renderer *const renderer);
void COL_DestroyTexture(COL_Renderer * renderer);
void COL_DestroyRenderer(COL_Renderer * renderer);
COL_Renderer *COL_CreateRenderer();
void COL_updateWindowPosition(COL_Renderer * renderer, const int x, const unsigned int y, const unsigned int w, const int h);
void COL_GetTextureSize(COL_Renderer * renderer, unsigned int *w, unsigned int *h);
void COL_RendererClear(COL_Renderer *const renderer);

#endif
