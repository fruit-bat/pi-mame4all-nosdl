#include "driver.h"
#include "dirty.h"
#include "minimal.h"

/* from video.c */
extern int gfx_display_lines;
extern int gfx_display_columns;
extern int skiplines;
extern int skipcolumns;

#define FLIP_VIDEO odx_video_flip();

extern int video_scale;

#include "minimal.h"

#ifdef printf
#undef printf
#endif

//#define WHICH_BLIT 1
#define ADJ32(X) ((32-((X)&0x1f))&0x1f)

UINT32 *palette_16bit_lookup;

INLINE void blitscreen_color8_exact(struct osd_bitmap *bitmap)
{
#ifdef WHICH_BLIT
  printf("blitscreen_color8_exact\n");
#endif  
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0]);
	unsigned char *lb=bitmap->line[skiplines] + skipcolumns;

	void *k; unsigned int pitch; COL_LockTexture(colRenderer, &k, &pitch);
	unsigned int adj = ADJ32(pitch);
	
	register unsigned int *address=(unsigned int *)k;

	for (y = 0; y < gfx_display_lines; ++y)
	{
		for (x = 0; x < pitch; ++x)
		{
			*(address++) = odx_palette_rgb[*(lb + x)];
		}
		lb += width;
		address += adj;
	}

	COL_UnlockTexture(colRenderer);
	FLIP_VIDEO
}

void blitscreen_dirty1_color8(struct osd_bitmap *bitmap)
{
  blitscreen_color8_exact(bitmap);
}

void blitscreen_dirty0_color8(struct osd_bitmap *bitmap)
{
	blitscreen_color8_exact(bitmap);
}

INLINE void blitscreen_palettized16_exact(struct osd_bitmap *bitmap)
{
#ifdef WHICH_BLIT  
  printf("blitscreen_palettized16_exact\n");
#endif  
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;

	void *k; unsigned int pitch; COL_LockTexture(colRenderer, &k, &pitch);
	unsigned int adj = ADJ32(pitch);
	register unsigned int *address=(unsigned int *)k;

	for (y = 0; y < gfx_display_lines; ++y)
	{
		for (x = 0; x < pitch; ++x)
		{
			*(address++) =  palette_16bit_lookup[*(lb + x)];  
		}
		address += adj;
		lb += width;
	}

	COL_UnlockTexture(colRenderer);
	FLIP_VIDEO   
}

void blitscreen_dirty0_palettized16(struct osd_bitmap *bitmap)
{
  blitscreen_palettized16_exact(bitmap); 
}

void blitscreen_dirty1_palettized16(struct osd_bitmap *bitmap)
{
  blitscreen_palettized16_exact(bitmap); 
}

INLINE void blitscreen_color16_exact(struct osd_bitmap *bitmap)
{
#ifdef WHICH_BLIT  
  printf("blitscreen_color16_exact\n");
#endif
	int x, y;
	int width=(bitmap->line[1] - bitmap->line[0])>>1;
	unsigned short *lb=((unsigned short*)(bitmap->line[skiplines])) + skipcolumns;

	void *k; unsigned int pitch; COL_LockTexture(colRenderer, &k, &pitch);
	unsigned int adj = ADJ32(pitch);
	register unsigned int *address=(unsigned int *)k;
  
	for (y = 0; y < gfx_display_lines; ++y)
	{
		for (x = 0; x < pitch; ++x)
		{
			unsigned int c = (unsigned int)lb[x];
			*(address++) =  ((c&0xF800)<<8) | ((c&0x7e0)<<5) | ((c&0x1f)<<3) | 0xff000000  ;
		}
 		address += adj;
		lb += width;
	}

	COL_UnlockTexture(colRenderer);
	FLIP_VIDEO   
}

void blitscreen_dirty0_color16(struct osd_bitmap *bitmap)
{
  blitscreen_color16_exact(bitmap); 
}

void blitscreen_dirty1_color16(struct osd_bitmap *bitmap)
{
  blitscreen_color16_exact(bitmap); 
}
