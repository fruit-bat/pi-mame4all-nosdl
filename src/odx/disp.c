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
#include <stdio.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include "disp.h"
#include "sunxi_disp_ioctl.h"

#define DRAM_OFFSET (0x40000000)

static int fd = -1;
static int layer = -1;
static int last_id = -1;

int disp_open(void)
{
	fd = open("/dev/disp", O_RDWR);
	if (fd == -1)
		return 0;

	uint32_t args[4];

	args[0] = 0;
	args[1] = DISP_LAYER_WORK_MODE_SCALER;
	args[2] = 0;
	args[3] = 0;
	layer = ioctl(fd, DISP_CMD_LAYER_REQUEST, args);

	if (layer > 0)
		return 1;

	close(fd);
	return 0;
}

void disp_set_dest(
  const int out_x, const int out_y, 
  const int out_width, const int out_height)  
{
	__disp_layer_info_t layer_info;
 	uint32_t args[4];
   
  args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&layer_info);
	args[3] = 0;  
  	
  ioctl(fd, DISP_CMD_LAYER_GET_PARA, args);
  
 	layer_info.scn_win.x = out_x;
	layer_info.scn_win.y = out_y;
	layer_info.scn_win.width = out_width;
	layer_info.scn_win.height = out_height; 
  

	ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
}

void disp_set_src(
  const int x, const int y, const int w, const int h)  
{
	__disp_layer_info_t layer_info;
 	uint32_t args[4];
   
  args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&layer_info);
	args[3] = 0;  
  	
  ioctl(fd, DISP_CMD_LAYER_GET_PARA, args);
  
 	layer_info.src_win.x = x;
	layer_info.src_win.y = y;
	layer_info.src_win.width = w;
	layer_info.src_win.height = h; 

	ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
}

int disp_set_para(
      const uint32_t buffer1,
      const uint32_t buffer2,
			const int color_format,       
      const int buf_w, const int buf_h,
      const int src_x, const int src_y, const int src_w, const int src_h,
			const int out_x, const int out_y, const int out_width, const int out_height)
{
	uint32_t args[4];
	__disp_layer_info_t layer_info;
	memset(&layer_info, 0, sizeof(layer_info));
	layer_info.pipe = 1;
	layer_info.mode = DISP_LAYER_WORK_MODE_SCALER;
	switch (color_format)
	{
	case COLOR_YUV420:
    layer_info.fb.mode = DISP_MOD_MB_UV_COMBINED;
		layer_info.fb.format = DISP_FORMAT_YUV420;
    layer_info.fb.seq = DISP_SEQ_UVUV;
		break;
	case COLOR_YUV422:
    layer_info.fb.mode = DISP_MOD_MB_UV_COMBINED;
		layer_info.fb.format = DISP_FORMAT_YUV422;
    layer_info.fb.seq = DISP_SEQ_UVUV;
		break;
  case COLOR_ARGB8888:
    layer_info.fb.mode = DISP_MOD_INTERLEAVED;
    layer_info.fb.format = DISP_FORMAT_ARGB8888;
    layer_info.fb.seq = DISP_SEQ_ARGB;
    break;
   case COLOR_BGRA8888:
    layer_info.fb.mode = DISP_MOD_INTERLEAVED;
    layer_info.fb.format = DISP_FORMAT_ARGB8888;
    layer_info.fb.seq = DISP_SEQ_BGRA;
    break;
  case COLOR_RGB565:
    layer_info.fb.mode = DISP_MOD_INTERLEAVED;
    layer_info.fb.format = DISP_FORMAT_RGB565;
    layer_info.fb.seq = DISP_SEQ_P10;
    break; 
	default:
		return 0;
		break;
	}
	layer_info.fb.br_swap = 0;
	layer_info.fb.addr[0] = buffer1 + DRAM_OFFSET;
	layer_info.fb.addr[1] = buffer2 + DRAM_OFFSET;

	layer_info.fb.cs_mode = DISP_BT601;// DISP_BT709, DISP_BT601;
	layer_info.fb.size.width = buf_w;
	layer_info.fb.size.height = buf_h;
	layer_info.src_win.x = src_x;
	layer_info.src_win.y = src_y;
	layer_info.src_win.width = src_w;
	layer_info.src_win.height = src_h;
	layer_info.scn_win.x = out_x;
	layer_info.scn_win.y = out_y;
	layer_info.scn_win.width = out_width;
	layer_info.scn_win.height = out_height;

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&layer_info);
	args[3] = 0;
	ioctl(fd, DISP_CMD_LAYER_SET_PARA, args);
	ioctl(fd, DISP_CMD_LAYER_TOP, args);
	ioctl(fd, DISP_CMD_VIDEO_START, args);
	ioctl(fd, DISP_CMD_LAYER_OPEN, args);
 // ioctl(fd, DISP_CMD_LAYER_ENHANCE_OFF, args);

	return 1;
}

int disp_wait_for_frame() {
  
  
  uint32_t args[4];
	args[0] = 0;
	args[1] = layer;
	args[2] = 0;
	args[3] = 0;
	int tmp, i = 0;
  
	while ((tmp = ioctl(fd, DISP_CMD_VIDEO_GET_FRAME_ID, args)) != last_id)
	{
//    printf("X tmp=%d last_id=%d\n", tmp, last_id);
		usleep(1000);
		if (i++ > 100) {
      last_id = tmp;
//      printf("E tmp=%d last_id=%d\n", tmp, last_id);
			return 0;
    }
	}  
//  printf("Y tmp=%d last_id=%d\n", tmp, last_id);
  return 1;
}

int disp_new_frame(
  const uint32_t buffer1, 
  const uint32_t buffer2,
  const int frame_rate)
{
	uint32_t args[4], i = 0;

	__disp_video_fb_t video;
	memset(&video, 0, sizeof(__disp_video_fb_t));
	video.id = last_id+1;
	video.frame_rate = frame_rate;
	video.addr[0] = buffer1 + DRAM_OFFSET;
	video.addr[1] = buffer2 + DRAM_OFFSET;

	args[0] = 0;
	args[1] = layer;
	args[2] = (unsigned long)(&video);
	args[3] = 0;

	i = ioctl(fd, DISP_CMD_VIDEO_SET_FB, args);

	last_id++;

	return 1;
}

void disp_close(void)
{
	uint32_t args[4];

	args[0] = 0;
	args[1] = layer;
	args[2] = 0;
	args[3] = 0;
	ioctl(fd, DISP_CMD_LAYER_CLOSE, args);
	ioctl(fd, DISP_CMD_VIDEO_STOP, args);
	ioctl(fd, DISP_CMD_LAYER_RELEASE, args);

	close(fd);
}
