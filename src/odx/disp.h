/*
 * Copyright (c) 2013 Jens Kuske <jenskuske@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef __DISP_H__
#define __DISP_H__

#include <stdint.h>

#define COLOR_YUV420   (0)
#define COLOR_YUV422   (2)
#define COLOR_ARGB8888 (3)
#define COLOR_BGRA8888 (4)
#define COLOR_RGB565   (5)

int disp_open(void);

int disp_wait_for_frame();

int disp_set_para(
      const uint32_t buffer1,
      const uint32_t buffer2,
			const int color_format,       
      const int buf_w, const int buf_h,
      const int src_x, const int src_y, const int src_w, const int src_h,
			const int out_x, const int out_y, const int out_width, const int out_height);
      
int disp_new_frame(const uint32_t luma_buffer, const uint32_t chroma_buffer, const int frame_rate);
      
void disp_set_dest(
  const int out_x, const int out_y, 
  const int out_width, const int out_height);
  
void disp_set_src(
  const int x, const int y, const int w, const int h);
  
void disp_close(void);

#endif
