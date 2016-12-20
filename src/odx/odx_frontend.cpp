#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "minimal.h"

#include "odx_frontend_list.h"

#define ODX_SCREEN_WIDTH 320
#define ODX_SCREEN_HEIGHT 240
#define BMP_SIZE ((ODX_SCREEN_WIDTH*ODX_SCREEN_HEIGHT*4)+(256*4)+54)

#define COMPATCORES 1

char frontend_build_version[] = "GCW0 V1.1";

static unsigned char splash_bmp[BMP_SIZE];
static unsigned char menu_bmp[BMP_SIZE];
static void odx_exit(char *param);
static bool fullscreen=false;

int game_num_avail=0;
static int last_game_selected=0;
char playemu[16] = "mame\0";
char playgame[16] = "builtinn\0";

char mamedir[512];

int odx_freq=336;       /* default dingoo Mhz */ 
int odx_video_depth=32; /* MAME video depth */
int odx_video_aspect=2; /* Scale best*/
int odx_video_sync=0;   /* No vsync */
int odx_frameskip=-1;
int odx_sound = 4;
int odx_volume = 3;
int odx_clock_cpu=100;
int odx_clock_sound=100;
int odx_cpu_cores=1;
int odx_ramtweaks=0;
int odx_cheat=0;
int odx_gsensor=0;

char romdir[512];

extern bool ui_exit;

inline bool isJoyPressed_A(const int ExKey) {
	if(ExKey & OD_A) printf("Joy A\n");
	return ExKey & OD_A;
}

inline bool isJoyPressed_B(const int ExKey) {
	if(ExKey & OD_B) printf("Joy B\n");
	return ExKey & OD_B;
}

inline bool isJoyPressed_X(const int ExKey) {
	if(ExKey & OD_X) printf("Joy X\n");
	return ExKey & OD_X;
}

inline bool isJoyPressed_Y(const int ExKey) {
	if(ExKey & OD_Y) printf("Joy Y\n");
	return ExKey & OD_Y;
}

inline bool isJoyPressed_START(const int ExKey) {
	if(ExKey & OD_START) printf("Joy START\n");
	return ExKey & OD_START;
}

inline bool isJoyPressed_SELECT(const int ExKey) {
	if(ExKey & OD_SELECT) printf("Joy SELECT\n");
	return ExKey & OD_SELECT;
}

inline bool isJoyPressed_L(const int ExKey) {
	if(ExKey & OD_L) printf("Joy L\n");
	return ExKey & OD_L;
}

inline bool isJoyPressed_R(const int ExKey) {
	if(ExKey & OD_R) printf("Joy R\n");
	return ExKey & OD_R;
}

inline bool isJoyPressed_LEFT(const int ExKey) {
	if(ExKey & OD_LEFT) printf("Joy LEFT\n");
	return ExKey & OD_LEFT;
}

inline bool isJoyPressed_RIGHT(const int ExKey) {
	if(ExKey & OD_RIGHT) printf("Joy RIGHT\n");
	return ExKey & OD_RIGHT;
}

inline bool isJoyPressed_UP(const int ExKey) {
	if(ExKey & OD_UP) printf("Joy UP\n");
	return ExKey & OD_UP;
}

inline bool isJoyPressed_DOWN(const int ExKey) {
	if(ExKey & OD_DOWN) printf("Joy DOWN\n");
	return ExKey & OD_DOWN;
}

// TODO should this jist be odx_joystick_press?
unsigned int frontend_joystick_press ()
{
	unsigned int ExKey = odx_joystick_press();
	if(ui_exit) {
		odx_exit("");
	}
	return ExKey;
}

static void blit_bmp_8bpp(unsigned char *in) 
{
    void *k; unsigned int pitch; 
    COL_LockTexture(colRenderer, &k, &pitch);
    
	unsigned int *t =(unsigned int *)k;
	unsigned int *r = t + ((ODX_SCREEN_HEIGHT - 1) * pitch);
	
	register unsigned char *src = in + 71;

	for(int y = 0; y < ODX_SCREEN_HEIGHT; ++y) {
		register unsigned int *p = r;
	
		for(int x = 0; x < ODX_SCREEN_WIDTH; ++x) {
			unsigned int r  = *src++;
			unsigned int g  = *src++;
			unsigned int b  = *src++;
			*src++;
			
			*p++ = 
			  0xff000000 | 
			  (r  << 0) | 
			  (g  << 8) |
			  (b  << 16) ;
		}
		r -= pitch;
	}
	COL_UnlockTexture(colRenderer);
}

static void odx_intro_screen(void) {
printf("odx_intro_screen(void)\n");  
  
	char name[256];
	FILE *f;
	odx_video_flip();
	sprintf(name,"skins/splash.bmp");
	
	f=fopen(name,"rb");
	if (f) {
		fread(splash_bmp,1,BMP_SIZE,f);
		fclose(f);
printf("odx_intro_screen(void) - read splash image ok\n");  
	}
    else {
printf("odx_intro_screen(void) - failed to read splash image\n");          
    }
    
printf("odx_intro_screen(void) - 1\n");
	blit_bmp_8bpp(splash_bmp);
printf("odx_intro_screen(void) - 2\n");
	odx_gamelist_text_out(1,230,frontend_build_version);
printf("odx_intro_screen(void) - 3\n");
	odx_video_flip();
printf("odx_intro_screen(void) - 4\n");
	frontend_joystick_press();
printf("odx_intro_screen(void) - 5\n");
	
	sprintf(name,"skins/menu.bmp");
	f=fopen(name,"rb");
	if (f) {
		fread(menu_bmp,1,BMP_SIZE,f);
		fclose(f);
printf("odx_intro_screen(void) - read menu image ok\n");  
	}
    else {
printf("odx_intro_screen(void) - failed to read menu image\n");          
    }
}

static void game_list_init_nocache(void)
{
	char text[512];
	int i;
	FILE *f;
	if (strlen(romdir))
		strcpy(text,romdir);
	else
		sprintf(text,"%s/roms",mamedir);
	DIR *d=opendir(text);
	char game[32];
	if (d)
	{
		struct dirent *actual=readdir(d);
		while(actual)
		{
			for (i=0;i<NUMGAMES;i++)
			{
				if (odx_drivers[i].available==0)
				{
					sprintf(game,"%s.zip",odx_drivers[i].name);
					
					if (strcmp(actual->d_name,game)==0)
					{
						odx_drivers[i].available=1;
						game_num_avail++;
						break;
					}
				}
			}
			actual=readdir(d);
		}
		closedir(d);
	}
	
	if (game_num_avail)
	{
		sprintf(text,"%s/frontend/mame.lst",mamedir);
		remove(text);
		/* sync(); */
		f=fopen(text,"w");
		if (f)
		{
			for (i=0;i<NUMGAMES;i++)
			{
				fputc(odx_drivers[i].available,f);
			}
			fclose(f);
			/* sync(); */
		}
	}
}

static void game_list_init_cache(void)
{
	char text[512];
	FILE *f;
	int i;
	sprintf(text,"%s/frontend/mame.lst",mamedir);
	f=fopen(text,"r");
	if (f)
	{
		for (i=0;i<NUMGAMES;i++)
		{
			odx_drivers[i].available=fgetc(f);
			if (odx_drivers[i].available)
				game_num_avail++;
		}
		fclose(f);
	}
	else
		game_list_init_nocache();
}

static void game_list_init(int argc)
{
	if (argc==1)
		game_list_init_nocache();
	else
		game_list_init_cache();
}

static void game_list_view(int *pos) {

	int i;
	int view_pos;
	int aux_pos=0;
	int screen_y = 45;
	int screen_x = 38;

	/* Draw background image */
	blit_bmp_8bpp(menu_bmp);

	/* draw text */
	odx_gamelist_text_out( 4, 30,"Select ROM");
	odx_gamelist_text_out( 4, 230,"1=Select Game/Start  2=Back");
	odx_gamelist_text_out( 268, 230,"  Q=Exit");
	odx_gamelist_text_out( 264,2,frontend_build_version);

	/* Check Limits */
	if (*pos<0)
		*pos=game_num_avail-1;
	if (*pos>(game_num_avail-1))
		*pos=0;
					   
	/* Set View Pos */
	if (*pos<11) { // ALEK 10
		view_pos=0;
	} else {
		if (*pos>game_num_avail-12) { // ALEK 11
			view_pos=game_num_avail-22; // ALEK 21
			view_pos=(view_pos<0?0:view_pos);
		} else {
			view_pos=*pos-11; // ALEK 10
		}
	}

	/* Show List */
	for (i=0;i<NUMGAMES;i++) {
		if (odx_drivers[i].available==1) {
			if (aux_pos>=view_pos && aux_pos<=view_pos+21) { // ALEK 20
				odx_gamelist_text_out( screen_x, screen_y, odx_drivers[i].description);
				if (aux_pos==*pos) {
					odx_gamelist_text_out( screen_x-10, screen_y,">" );
					odx_gamelist_text_out( screen_x-13, screen_y-1,"-" );
				}
				screen_y+=8;
			}
			aux_pos++;
		}
	}
}

static void game_list_select (int index, char *game, char *emu) {
	int i;
	int aux_pos=0;
	for (i=0;i<NUMGAMES;i++)
	{
		if (odx_drivers[i].available==1)
		{
			if(aux_pos==index)
			{
				strcpy(game, odx_drivers[i].name);
				strcpy(emu, odx_drivers[i].exe);
				break;
			}
			aux_pos++;
		}
	}
}

static char *game_list_description (int index)
{
	int i;
	int aux_pos=0;
	for (i=0;i<NUMGAMES;i++) {
		if (odx_drivers[i].available==1) {
			if(aux_pos==index) {
				return(odx_drivers[i].description);
			}
			aux_pos++;
		   }
	}
	return ((char *)0);
}

static int show_options(char *game)
{
	unsigned int ExKey=0;
	int selected_option=0;
	int x_Pos = 41;
	int y_PosTop = 58;
	int y_Pos = y_PosTop;
	int options_count = 9;
	char text[512];
	FILE *f;
	int i=0;

	/* Read game configuration */
	sprintf(text,"%s/frontend/%s.cfg",mamedir,game);
	f=fopen(text,"r");
	if (f) {
		fscanf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&odx_freq,&odx_video_depth,&odx_video_aspect,&odx_video_sync,
		&odx_frameskip,&odx_sound,&odx_clock_cpu,&odx_clock_sound,&odx_cpu_cores,&odx_ramtweaks,&i,&odx_cheat,&odx_gsensor);
		fclose(f);
	}

	while(1)
	{
		y_Pos = y_PosTop;
	
		/* Draw background image */
		blit_bmp_8bpp(menu_bmp);

		/* draw text */
		odx_gamelist_text_out( 4, 30,"Game Options");
		odx_gamelist_text_out( 4, 230,"1=Select Game/Start  2=Back");
		odx_gamelist_text_out( 268, 230,"  Q=Exit");
		odx_gamelist_text_out( 264,2,frontend_build_version);

		/* Draw the options */
		strncpy (text,game_list_description(last_game_selected),33);
		text[32]='\0';
		odx_gamelist_text_out(x_Pos,y_Pos-10,text);

		/* (0) Video Depth */
		y_Pos += 10;
		switch (odx_video_depth)
		{
			case -1: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Depth    Auto"); break;
			case 8:  odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Depth    8 bit"); break;
			case 16: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Depth    16 bit"); break;
		}
		
		/* (1) Video Aspect */
		y_Pos += 10;
		switch (odx_video_aspect)
		{
			case 0: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Normal"); break;
			case 1: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Scale Horizontal"); break;
			case 2: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Scale Best"); break;
			case 3: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Scale Fast"); break;
			case 4: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Scale Halfsize"); break;
			case 5: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Rotate Normal"); break;
			case 6: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Rotate Scale Horiz"); break;
			case 7: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Rotate Best"); break;
			case 8: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Rotate Fast"); break;
			case 9: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Video Aspect   Rotate Halfsize"); break;
		}
		
		/* (2) Video Sync */
		y_Pos += 10;
		switch (odx_video_sync)
		{
			case 1: odx_gamelist_text_out(x_Pos,y_Pos, "Video Sync     VSync"); break;
			case 0: odx_gamelist_text_out(x_Pos,y_Pos, "Video Sync     Normal"); break;
			case 2: odx_gamelist_text_out(x_Pos,y_Pos, "Video Sync     DblBuf"); break;
			case -1: odx_gamelist_text_out(x_Pos,y_Pos,"Video Sync     OFF"); break;
		}
		
		/* (3) Frame-Skip */
		y_Pos += 10;
		if ((odx_video_sync==-1) && (odx_frameskip==-1)) odx_frameskip=0;
		if(odx_frameskip==-1) {
			odx_gamelist_text_out_fmt(x_Pos,y_Pos, "Frame-Skip     Auto");
		}
		else{
			odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Frame-Skip     %d",odx_frameskip);
		}

		/* (4) Sound */
		y_Pos += 10;
		switch(odx_sound)
		{
			case 0: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","OFF"); break;
			case 1: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (15 KHz fast)"); break;
			case 2: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (22 KHz fast)"); break;
			case 3: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (33 KHz fast)"); break;
			case 4: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (44 KHz fast)"); break;
			case 5: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (11 KHz fast)"); break;
			case 6: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (15 KHz)"); break;
			case 7: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (22 KHz)"); break;
			case 8: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (33 KHz)"); break;
			case 9: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (44 KHz)"); break;
			case 10: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (11 KHz)"); break;
			case 11: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (15 KHz stereo)"); break;
			case 12: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (22 KHz stereo)"); break;
			case 13: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (33 KHz stereo)"); break;
			case 14: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (44 KHz stereo)"); break;
			case 15: odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Sound          %s","ON (11 KHz stereo)"); break;
		}

		/* (5) CPU Clock */
		y_Pos += 10;
		odx_gamelist_text_out_fmt(x_Pos,y_Pos,"CPU Clock      %d%%",odx_clock_cpu);

		/* (6) Audio Clock */
		y_Pos += 10;
		odx_gamelist_text_out_fmt(x_Pos,y_Pos,"Audio Clock    %d%%",odx_clock_sound);

		/* (7) CPU cores */
		y_Pos += 10;
		switch (odx_cpu_cores)
		{
			case 0: odx_gamelist_text_out(x_Pos,y_Pos, "CPU FAST cores None"); break;
			case 1: odx_gamelist_text_out(x_Pos,y_Pos, "CPU FAST cores Fame"); break;
			default:odx_gamelist_text_out(x_Pos,y_Pos, "CPU FAST cores None"); odx_cpu_cores = 0; break;
		}

		/* (8) Cheats */
		y_Pos += 10;
		if (odx_cheat)
			odx_gamelist_text_out(x_Pos,y_Pos,"Cheats         ON");
		else
			odx_gamelist_text_out(x_Pos,y_Pos,"Cheats         OFF");

		//y_Pos += 30;
		//odx_gamelist_text_out(x_Pos,y_Pos,"Press B to confirm, X to return\0");

		/* Show currently selected item */
		odx_gamelist_text_out(x_Pos-16,y_PosTop+(selected_option*10)+10," >");

		COL_UnlockTexture(colRenderer);
		COL_RenderCopyAndPresent(colRenderer);

		ExKey=frontend_joystick_press();
		if(isJoyPressed_DOWN(ExKey)){
			selected_option++;
			selected_option = selected_option % options_count;
		}
		else if(isJoyPressed_UP(ExKey)){
			selected_option--;
			if(selected_option<0)
				selected_option = options_count - 1;
		}
		else if(isJoyPressed_LEFT(ExKey) || isJoyPressed_RIGHT(ExKey))
		{
			switch(selected_option) {
			case 0:
				switch (odx_video_depth)
				{
					case -1: odx_video_depth=8; break;
					case 8: odx_video_depth=16; break;
					case 16: odx_video_depth=-1; break;
				}
				break;
			case 1:
				if(isJoyPressed_RIGHT(ExKey))
				{
					odx_video_aspect++;
					if (odx_video_aspect>9)
						odx_video_aspect=0;
				}
				else
				{
					odx_video_aspect--;
					if (odx_video_aspect<0)
						odx_video_aspect=9;
				}
				break;
			case 2:
				odx_video_sync=odx_video_sync+1;
				if (odx_video_sync>2)
					odx_video_sync=-1;
				break;
			case 3:
				if(isJoyPressed_RIGHT(ExKey))
				{
					odx_frameskip ++;
					if (odx_frameskip>11)
						odx_frameskip=-1;
				}
				else
				{
					odx_frameskip--;
					if (odx_frameskip<-1)
						odx_frameskip=11;
				}
				break;
			case 4:
				if(isJoyPressed_RIGHT(ExKey))
				{
					odx_sound ++;
					if (odx_sound>15)
						odx_sound=0;
				}
				else
				{
					odx_sound--;
					if (odx_sound<0)
						odx_sound=15;
				}
				break;
			case 5:
				/* "CPU Clock" */
				if(isJoyPressed_RIGHT(ExKey))
				{
					odx_clock_cpu += 10; /* Add 10% */
					if (odx_clock_cpu > 200) /* 200% is the max */
						odx_clock_cpu = 200;
				}
				else
				{
					odx_clock_cpu -= 10; /* Subtract 10% */
					if (odx_clock_cpu < 10) /* 10% is the min */
						odx_clock_cpu = 10;
				}
				break;
			case 6:
				/* "Audio Clock" */
				if(isJoyPressed_RIGHT(ExKey))
				{
					odx_clock_sound += 10; /* Add 10% */
					if (odx_clock_sound > 200) /* 200% is the max */
						odx_clock_sound = 200;
				}
				else{
					odx_clock_sound -= 10; /* Subtract 10% */
					if (odx_clock_sound < 10) /* 10% is the min */
						odx_clock_sound = 10;
				}
				break;
			case 7: /* change for fast cpu core */
				odx_cpu_cores=(odx_cpu_cores+1)%2;
				break;
			case 8: /* Are we using cheats */
				odx_cheat=!odx_cheat;
				break;
			}
		}

		if ((isJoyPressed_A(ExKey)) || (isJoyPressed_START(ExKey)) || (isJoyPressed_SELECT(ExKey))) 
		{
			/* Write game configuration */
			sprintf(text,"%s/frontend/%s.cfg",mamedir,game);
			f=fopen(text,"w");
			if (f) {
				fprintf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",odx_freq,odx_video_depth,odx_video_aspect,odx_video_sync,
				odx_frameskip,odx_sound,odx_clock_cpu,odx_clock_sound,odx_cpu_cores,odx_ramtweaks,i,odx_cheat,odx_gsensor);
				fclose(f);
				/* sync(); */
			}

			/* Selected game will be run */
			return 1;
		}
		else if ((isJoyPressed_X(ExKey)) || (isJoyPressed_Y(ExKey)) || (isJoyPressed_B(ExKey)))
		{
			/* Return To Menu */
			return 0;
		}
	}
}

static void odx_exit(char *param)
{
	char text[512];
	
	sprintf(text,"%s/frontend/mame.lst",mamedir);
	remove(text);
	/* sync(); */
	odx_deinit();
	
	exit(0);
}

static void select_game(char *emu, char *game)
{

	unsigned int ExKey;

	/* No Selected game */
	strcpy(game,"builtinn");

	/* Clean screen */
	odx_video_flip();

	/* Wait until user selects a game */
	while(1)
	{
		game_list_view(&last_game_selected);
		COL_UnlockTexture(colRenderer);
		COL_RenderCopyAndPresent(colRenderer);

		ExKey=frontend_joystick_press();

		if ((isJoyPressed_L(ExKey)) && (isJoyPressed_R(ExKey)) ) { odx_exit(""); }
		if (isJoyPressed_UP(ExKey)) last_game_selected--;
		if (isJoyPressed_DOWN(ExKey)) last_game_selected++;
		if (isJoyPressed_LEFT(ExKey)) last_game_selected-=22; // ALEK 21
		if (isJoyPressed_RIGHT(ExKey)) last_game_selected+=22; // ALEK 21

		if ((isJoyPressed_A(ExKey)) || (isJoyPressed_START(ExKey)) )
		{
			/* Select the game */
			game_list_select(last_game_selected, game, emu);

			/* Emulation Options */
			if(show_options(game))
			{
				break;
			}
		}
	}
}

void execute_game (char *playemu, char *playgame, int *argc, char ***argv)
{
	static char *args[255];
	char str[8][64];
	int n=0;
	int i=0;
	
	/* executable */
	args[n]=playemu; n++;

	/* playgame */
	args[n]=playgame; n++;

	/* odx_freq */
	/*args[n]="-clock"; n++;
	sprintf(str[i],"%d",odx_freq);
	args[n]=str[i]; i++; n++;*/

    args[n]="-frontend"; n++;
    
    if(fullscreen) 
    {
 		args[n]="-fullscreen"; n++;
    }
	/* odx_video_depth */
	if (odx_video_depth==8)
	{
		args[n]="-depth"; n++;
		args[n]="8"; n++;
	}
	if (odx_video_depth==16)
	{
		args[n]="-depth"; n++;
		args[n]="16"; n++;
	}

	/* odx_video_aspect */
	if ((odx_video_aspect==1) || (odx_video_aspect==6))
	{
		args[n]="-horizscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((odx_video_aspect==2) || (odx_video_aspect==7))
	{
		args[n]="-bestscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((odx_video_aspect==3) || (odx_video_aspect==8))
	{
		args[n]="-fastscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((odx_video_aspect==4) || (odx_video_aspect==9))
	{
		args[n]="-halfscale"; n++;
		args[n]="-nodirty"; n++;
	}
	if ((odx_video_aspect>=5) && (odx_video_aspect<=9))
	{
		args[n]="-rotatecontrols"; n++;
		args[n]="-ror"; n++;
	}
	
	/* odx_video_sync */
	if (odx_video_sync==1)
	{
		args[n]="-nodirty"; n++;
		args[n]="-waitvsync"; n++;
	}
	else if ((odx_video_sync==2) || (odx_video_aspect==1) || (odx_video_aspect==9))
	{
		args[n]="-nodirty"; n++;
	}
	if (odx_video_sync==-1)
	{
		args[n]="-nothrottle"; n++;
	}
	
	/* odx_frameskip */
	if (odx_frameskip>=0)
	{
		args[n]="-frameskip"; n++;
		sprintf(str[i],"%d",odx_frameskip);
		args[n]=str[i]; i++; n++;
	}

	/* odx_sound */
	if (odx_sound==0)
	{
		args[n]="-soundcard"; n++;
		args[n]="0"; n++;
	}
	if ((odx_sound==1) || (odx_sound==6) || (odx_sound==11))
	{
		args[n]="-samplerate"; n++;
		args[n]="15000"; n++;
	}
	if ((odx_sound==2) || (odx_sound==7) || (odx_sound==12))
	{
		args[n]="-samplerate"; n++;
		args[n]="22050"; n++;
	}
	if ((odx_sound==3) || (odx_sound==8) || (odx_sound==13))
	{
		args[n]="-samplerate"; n++;
		args[n]="32000"; n++;
	}
	if ((odx_sound==4) || (odx_sound==9) || (odx_sound==14))
	{
		args[n]="-samplerate"; n++;
		args[n]="44100"; n++;
	}
	if ((odx_sound==5) || (odx_sound==10) || (odx_sound==15))
	{
		args[n]="-samplerate"; n++;
		args[n]="11025"; n++;
	}
	if ((odx_sound>=1) && (odx_sound<=5))
	{
		args[n]="-fastsound"; n++;
	}
	if (odx_sound>=11)
	{
		args[n]="-stereo"; n++;
	}

	/* odx_clock_cpu */
	if (odx_clock_cpu!=100)
	{
		args[n]="-uclock"; n++;
		sprintf(str[i],"%d",100-odx_clock_cpu);
		args[n]=str[i]; i++; n++;
	}

	/* odx_clock_sound */
	if (odx_clock_cpu!=100)
	{
		args[n]="-uclocks"; n++;
		sprintf(str[i],"%d",100-odx_clock_sound);
		args[n]=str[i]; i++; n++;
	}
	
	/* odx_cpu_cores */
	if ((odx_cpu_cores==1) )
	{
		args[n]="-fame"; n++;
	}
#if 0	
	if ((odx_cpu_cores==1) || (odx_cpu_cores==3) || (odx_cpu_cores==5))
	{
		args[n]="-cyclone"; n++;
	}
	if ((odx_cpu_cores==2) || (odx_cpu_cores==3))
	{
		args[n]="-drz80"; n++;
	}
	if ((odx_cpu_cores==4) || (odx_cpu_cores==5))
	{
		args[n]="-drz80_snd"; n++;
	}
#endif

	if (odx_ramtweaks)
	{
		args[n]="-ramtweaks"; n++;
	}
	
	if (odx_cheat)
	{
		args[n]="-cheat"; n++;
	}

	if (odx_video_aspect==24)
	{
		args[n]="-odx_rotated_video"; n++;
    	args[n]="-rol"; n++;
	}
    if (odx_video_aspect==25)
    {
		args[n]="-odx_rotated_video"; n++;
		args[n]="-rotatecontrols"; n++;
    }
	
	/* Add mame and rom directory */
    args[n]="-mamepath"; n++;
	args[n]=mamedir;n++;
    args[n]="-rompath"; n++;;
	args[n]=romdir;n++;
	
	args[n]=NULL;

#if 1
	for (i=0; i<n; i++)
	{
		fprintf(stderr,"%s ",args[i]);
	}
	fprintf(stderr,"\n");
	fflush(stderr);
#endif
	*argc = n;
	*argv = args;
}

 
#define FILE_LIST_ROWS 19
#define MAX_FILES 512
typedef struct  {
	char name[255];
	unsigned int type;
} filedirtype;
filedirtype filedir_list[MAX_FILES];

int sort_function(const void *src_str_ptr, const void *dest_str_ptr) {
  filedirtype *p1 = (filedirtype *) src_str_ptr;
  filedirtype *p2 = (filedirtype *) dest_str_ptr;
  
  return strcmp (p1->name, p2->name);
}

signed int get_romdir(char *result) {
	unsigned int ExKey;
	
	char current_dir_name[512];
	DIR *current_dir;
	struct dirent *current_file;
	struct stat file_info;
	char current_dir_short[81];
	unsigned int current_dir_length;
	unsigned int num_filedir;

	char *file_name;
	signed int return_value = 1;
	unsigned int repeat;
	unsigned int i;

	unsigned int current_filedir_scroll_value;
	unsigned int current_filedir_selection;
	unsigned int current_filedir_in_scroll;
	unsigned int current_filedir_number;
	
	// Init dir with saved one
	strcpy(current_dir_name,mamedir);

	while (return_value == 1) {
		current_filedir_in_scroll = 0;
		current_filedir_scroll_value  = 0;
		current_filedir_number = 0;
		current_filedir_selection = 0;
		num_filedir = 0;
		
		getcwd(current_dir_name, 512);
		current_dir = opendir(current_dir_name);
		
		do {
			if(current_dir) current_file = readdir(current_dir); else current_file = NULL;

			if(current_file) {
				file_name = current_file->d_name;

				if((stat(file_name, &file_info) >= 0) && ((file_name[0] != '.') || (file_name[1] == '.'))) {
					if(S_ISDIR(file_info.st_mode)) {
						filedir_list[num_filedir].type = 1; // 1 -> directory
						strcpy(filedir_list[num_filedir].name, file_name);
						num_filedir++;
						
					}
				}
			}
		} while ((current_file) && (num_filedir<MAX_FILES));

		if (num_filedir)
			qsort((void *)filedir_list, num_filedir, sizeof(filedirtype), sort_function);

		closedir(current_dir);

		current_dir_length = strlen(current_dir_name);
		if(current_dir_length > 39) {
			memcpy(current_dir_short, "...", 3);
			memcpy(current_dir_short + 3, current_dir_name + current_dir_length - (39-3), (39-3));
			current_dir_short[39] = 0;
		} else {
			memcpy(current_dir_short, current_dir_name, current_dir_length + 1);
		}
 
		repeat = 1;

		char print_buffer[81];

		while(repeat) {
			// TODO Lock
			blit_bmp_8bpp(menu_bmp);
			
			odx_gamelist_text_out( 182, 30,"Select a ROM directory");
			odx_gamelist_text_out( 4, 215,current_dir_short );
			odx_gamelist_text_out( 4, 230,"A=Enter dir START=Select dir");
			odx_gamelist_text_out( 280, 230,"B=Quit");
			odx_gamelist_text_out( 264,2,frontend_build_version);
			for(i = 0, current_filedir_number = i + current_filedir_scroll_value; i < FILE_LIST_ROWS; i++, current_filedir_number++) {
#define CHARLEN ((ODX_SCREEN_WIDTH/6)-2)
				if(current_filedir_number < num_filedir) {
					strncpy(print_buffer+1,filedir_list[current_filedir_number].name, CHARLEN-1);
					print_buffer[0] = ' ';
					if((current_filedir_number == current_filedir_selection))
						print_buffer[0] = '>';
					print_buffer[CHARLEN] = 0;
					odx_gamelist_text_out(4, 31+((i + 2) * 8), print_buffer );
				}
			}
			COL_UnlockTexture(colRenderer);
			COL_RenderCopyAndPresent(colRenderer);

			// Catch input
			ExKey=frontend_joystick_press();
		printf("Got key %d\n", ExKey);	
			/* L + R = Exit */
			if ((isJoyPressed_L(ExKey)) && (isJoyPressed_R(ExKey)) ) { odx_exit(""); }
		
			// START - choose directory
			if (isJoyPressed_START(ExKey)) { 
printf("Start\n");				
				repeat = 0;
				return_value = 0;
				strcpy(result,current_dir_name);
			}
			
			// A - choose file or enter directory
			if (isJoyPressed_A(ExKey)) { 
printf("Choose\n");				
				if (filedir_list[current_filedir_selection].type == 1)  { // so it's a directory
					repeat = 0;
printf("Folder %s\n", filedir_list[current_filedir_selection].name);				
					
					chdir(filedir_list[current_filedir_selection].name);
				}
			}

			// B - exit or back to previous menu
			if (isJoyPressed_B(ExKey)) { 
printf("Back\n");				
				return_value = -1;
				repeat = 0;
			}

			// UP - arrow up
			if (isJoyPressed_UP(ExKey)) { 
				if(current_filedir_selection) {
					current_filedir_selection--;
					if(current_filedir_in_scroll == 0) {
						current_filedir_scroll_value--;
					} else {
						current_filedir_in_scroll--;
					}
				}
				else {
					current_filedir_selection = (num_filedir - 1);
					current_filedir_in_scroll = num_filedir> FILE_LIST_ROWS ? (FILE_LIST_ROWS - 1) : num_filedir-1;
					current_filedir_scroll_value = num_filedir> FILE_LIST_ROWS ? (num_filedir - 1)-FILE_LIST_ROWS+1 : 0;
				}
			}

			//DOWN - arrow down
			if (isJoyPressed_DOWN(ExKey)) { 
				if(current_filedir_selection < (num_filedir - 1)) {
					current_filedir_selection++;
					if(current_filedir_in_scroll == (FILE_LIST_ROWS - 1)) {
						current_filedir_scroll_value++;
					} else {
						current_filedir_in_scroll++;
					}
				}
				else {
					current_filedir_selection = 0;
					current_filedir_in_scroll =0;
					current_filedir_scroll_value = 0;
				}
			}
		}
	}
	
	return return_value;
}

void gethomedir(char *dir, char* name) {
	char text[512];
#ifdef _GCW0_
	strcpy(dir, getenv("HOME"));
	if (strlen(dir) == 0) {
		getcwd(dir, 256);
	}
#else
	getcwd(dir, 256);
#endif
	if (strlen(name)) {
		sprintf(dir,"%s/.%s/",dir, name);
#ifdef _GCW0_
		mkdir(dir,S_IRWXU | S_IRWXG | S_IRWXO); // create $HOME/.program if it doesn't exist
		sprintf(text,"%s/frontend/",dir); mkdir(text,S_IRWXU | S_IRWXG | S_IRWXO); 
		sprintf(text,"%s/nvram/",dir); mkdir(text,S_IRWXU | S_IRWXG | S_IRWXO); 
		sprintf(text,"%s/hi/",dir); mkdir(text,S_IRWXU | S_IRWXG | S_IRWXO); 
		sprintf(text,"%s/cfg/",dir); mkdir(text,S_IRWXU | S_IRWXG | S_IRWXO); 
		sprintf(text,"%s/memcard/",dir); mkdir(text,S_IRWXU | S_IRWXG | S_IRWXO); 
		sprintf(text,"%s/snap/",dir); mkdir(text,S_IRWXU | S_IRWXG | S_IRWXO); 
#else
		mkdir(dir); // create /.program if it doesn't exist
		sprintf(text,"%s/frontend/",dir); mkdir(text); 
		sprintf(text,"%s/nvram/",dir); mkdir(text); 
		sprintf(text,"%s/hi/",dir);	mkdir(text); 
		sprintf(text,"%s/cfg/",dir); mkdir(text); 
		sprintf(text,"%s/memcard/",dir); mkdir(text); 
		sprintf(text,"%s/snap/",dir); mkdir(text); 
#endif
	}
}

int frontend_main (int argc, char **argv, int *front_argc, char ***front_argv, bool show_intro)
{
	char text[512], curDir[512];
	FILE *f;

	/* get initial home directory */
	gethomedir(mamedir,"mame4all");
	strcpy(romdir,"");
	
	odx_set_video_mode(8,ODX_SCREEN_WIDTH,ODX_SCREEN_HEIGHT);

	odx_video_color8(0,0,0,0);
	odx_video_color8(255,255,255,255);
	odx_video_setpalette();
	
	odx_clear_video();
	
	/* Show intro screen */
	if(show_intro) {
		odx_intro_screen();
	}

	/* Read default configuration */
	sprintf(text,"%s/frontend/mame.cfg",mamedir);
	f=fopen(text,"r");
	if (f) {
		fscanf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s",&odx_freq,&odx_video_depth,&odx_video_aspect,&odx_video_sync,
		&odx_frameskip,&odx_sound,&odx_clock_cpu,&odx_clock_sound,&odx_cpu_cores,&odx_ramtweaks,&last_game_selected,&odx_cheat,romdir);
		fclose(f);
	}

	/* Initialize list of available games */
	game_list_init(argc);
	if (game_num_avail==0)
	{
		/* save current dir */
		getcwd(curDir, 256);
		
		/* Check for rom dir */
		while (game_num_avail == 0) {
			odx_gamelist_text_out(10, 20, "Error: No available games found !");
			odx_gamelist_text_out(10, 40, "Press a key to select a rom directory");
			odx_video_flip();
			frontend_joystick_press();
			if (get_romdir(romdir) == -1)
				odx_exit("");
			else
				game_list_init(argc);
		}
		/* go back to default dir to avoid issue when launching mame after */
		chdir(curDir);
	}

	/* Select Game */
	select_game(playemu,playgame); 

	/* Write default configuration */
	f=fopen(text,"w");
	if (f) {
		fprintf(f,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s",odx_freq,odx_video_depth,odx_video_aspect,odx_video_sync,
		odx_frameskip,odx_sound,odx_clock_cpu,odx_clock_sound,odx_cpu_cores,odx_ramtweaks,last_game_selected,odx_cheat,romdir);
		fclose(f);
		/* sync(); */
	}

	/* Execute Game */
	execute_game (playemu,playgame, front_argc, front_argv);
	
	return 0;
}
