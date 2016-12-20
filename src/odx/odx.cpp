/***************************************************************************

  osdepend.c

  OS dependant stuff (display handling, keyboard scan...)
  This is the only file which should me modified in order to port the
  emulator to a different system.

***************************************************************************/

#include "driver.h"
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include "stdarg.h"
#include "string.h"
#include "minimal.h"

int  msdos_init_sound(void);
void msdos_init_input(void);
void msdos_shutdown_sound(void);
void msdos_shutdown_input(void);
int  frontend_help (int argc, char **argv);
void parse_cmdline (int argc, char **argv, int game);
void init_inpdir(void);

static FILE *errorlog;

/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(void)
{
	if (msdos_init_sound())
		return 1;
	msdos_init_input();
	return 0;
}


/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
	msdos_shutdown_sound();
	msdos_shutdown_input();
}

/* fuzzy string compare, compare short string against long string        */
/* e.g. astdel == "Asteroids Deluxe". The return code is the fuzz index, */
/* we simply count the gaps between maching chars.                       */
int fuzzycmp (const char *s, const char *l)
{
	int gaps = 0;
	int match = 0;
	int last = 1;

	for (; *s && *l; l++)
	{
		if (*s == *l)
			match = 1;
		else if (*s >= 'a' && *s <= 'z' && (*s - 'a') == (*l - 'A'))
			match = 1;
		else if (*s >= 'A' && *s <= 'Z' && (*s - 'A') == (*l - 'a'))
			match = 1;
		else
			match = 0;

		if (match)
			s++;

		if (match != last)
		{
			last = match;
			if (!match)
				gaps++;
		}
	}

	/* penalty if short string does not completely fit in */
	for (; *s; s++)
		gaps++;

	return gaps;
}

int main (int main_argc, char **main_argv)
{
	int res, i, j = 0, game_index;
   	char *playbackname = NULL;
	int use_fame=0;
    bool fullscreen=false;
    bool frontend=false;
   	extern int video_scale;
	extern int video_border;
	extern int video_aspect;
	extern int throttle;

	memset(&options,0,sizeof(options));

	/* these two are not available in mame.cfg */
	errorlog = 0;


	for (i = 1;i < main_argc;i++) /* V.V_121997 */
	{
		if (strcasecmp(main_argv[i],"-log") == 0)
			errorlog = fopen("error.log","wa");
		if (strcasecmp(main_argv[i],"-fame") == 0)
			use_fame=1;
		if (strcasecmp(main_argv[i],"-horizscale") == 0)
			video_scale=1;
		if (strcasecmp(main_argv[i],"-halfscale") == 0)
			video_scale=2;
		if (strcasecmp(main_argv[i],"-bestscale") == 0)
			video_scale=3;
		if (strcasecmp(main_argv[i],"-fastscale") == 0)
			video_scale=4;
		if (strcasecmp(main_argv[i],"-fullscreen") == 0)
			fullscreen=true;
		if (strcasecmp(main_argv[i],"-frontend") == 0)
			frontend=true;
		if (strcasecmp(main_argv[i],"-border") == 0)
			video_border=1;
		if (strcasecmp(main_argv[i],"-aspect") == 0)
			video_aspect=1;
		if (strcasecmp(main_argv[i],"-nothrottle") == 0)
			throttle=0;
		if ((strcasecmp(main_argv[i],"-clock") == 0) && (i<main_argc-1))
			odx_clock=atoi(main_argv[i+1]);
        if (strcasecmp(main_argv[i],"-playback") == 0)
		{
			i++;
			if (i < main_argc)  /* point to inp file name */
				playbackname = main_argv[i];
        }
	}

	/* Initialization */
	odx_init(1000,16,44100,16,0,60,fullscreen);

	/* check for frontend options */
	res = frontend_help (main_argc, main_argv);
	
	int argc = main_argc;
	char **argv = main_argv;
	
	/* if frontend options were used, return to DOS with the error code */
	if (res != 1234)
	{
		printf("Trying frontend_main frontend=%d res = %d\n", frontend, res);
		if(frontend) {
			frontend_main(main_argc, main_argv, &argc, &argv, true);
		}
		else {
			odx_deinit();
			exit(res);
		}
	}

	while(frontend) {
		game_index = -1;
		
		/* handle playback which is not available in mame.cfg */
		init_inpdir(); /* Init input directory for opening .inp for playback */

		if (playbackname != NULL)
			options.playback = osd_fopen(playbackname,0,OSD_FILETYPE_INPUTLOG,0);

		/* check for game name embedded in .inp header */
		if (options.playback)
		{
			INP_HEADER inp_header;

			/* read playback header */
			osd_fread(options.playback, &inp_header, sizeof(INP_HEADER));

			if (!isalnum(inp_header.name[0])) /* If first byte is not alpha-numeric */
				osd_fseek(options.playback, 0, SEEK_SET); /* old .inp file - no header */
			else
			{
				for (i = 0; (drivers[i] != 0); i++) /* find game and play it */
				{
					if (strcmp(drivers[i]->name, inp_header.name) == 0)
					{
						game_index = i;
						printf("Playing back previously recorded game %s (%s) [press return]\n",
							drivers[game_index]->name,drivers[game_index]->description);
						getchar();
						break;
					}
				}
			}
		}

		/* If not playing back a new .inp file */
		if (game_index == -1)
		{
			/* take the first commandline argument without "-" as the game name */
			for (j = 1; j < argc; j++)
			{
				if (argv[j][0] != '-') break;
			}
		/* do we have a driver for this? */
			{
				for (i = 0; drivers[i] && (game_index == -1); i++)
				{
					//ALEK fprintf(stderr,"%s -> %s\n",argv[j],drivers[i]->name); fflush(stderr);
					if (strcasecmp(argv[j],drivers[i]->name) == 0)
					{
						game_index = i;
						break;
					}
				}

				/* educated guess on what the user wants to play */
				if (game_index == -1)
				{
					int fuzz = 9999; /* best fuzz factor so far */

					for (i = 0; (drivers[i] != 0); i++)
					{
						int tmp;
						tmp = fuzzycmp(argv[j], drivers[i]->description);
						/* continue if the fuzz index is worse */
						if (tmp > fuzz)
							continue;

						/* on equal fuzz index, we prefer working, original games */
						if (tmp == fuzz)
						{
							/* game is a clone */
							if (drivers[i]->clone_of != 0
									&& !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
							{
								/* if the game we already found works, why bother. */
								/* and broken clones aren't very helpful either */
								if ((!drivers[game_index]->flags & GAME_NOT_WORKING) ||
									(drivers[i]->flags & GAME_NOT_WORKING))
									continue;
							}
							else continue;
						}

						/* we found a better match */
						game_index = i;
						fuzz = tmp;
					}

					if (game_index != -1)
						printf("fuzzy name compare, running %s\n",drivers[game_index]->name);
				}
			}

			if (game_index == -1)
			{
				printf("Game \"%s\" not supported\n", argv[j]);
				return 1;
			}
		}

		/* parse generic (os-independent) options */
		parse_cmdline (argc, argv, game_index);

		{	/* Mish:  I need sample rate initialised _before_ rom loading for optional rom regions */
			extern int soundcard;

			if (soundcard == 0) {    /* silence, this would be -1 if unknown in which case all roms are loaded */
				Machine->sample_rate = 0; /* update the Machine structure to show that sound is disabled */
				options.samplerate=0;
			}
		}

		/* handle record which is not available in mame.cfg */
		for (i = 1; i < argc; i++)
		{
			if (strcasecmp(argv[i],"-record") == 0)
			{
				i++;
				if (i < argc)
					options.record = osd_fopen(argv[i],0,OSD_FILETYPE_INPUTLOG,1);
			}
		}

		if (options.record)
		{
			INP_HEADER inp_header;

			memset(&inp_header, '\0', sizeof(INP_HEADER));
			strcpy(inp_header.name, drivers[game_index]->name);
			/* MAME32 stores the MAME version numbers at bytes 9 - 11
			 * MAME DOS keeps this information in a string, the
			 * Windows code defines them in the Makefile.
			 */
			/*
			inp_header.version[0] = 0;
			inp_header.version[1] = VERSION;
			inp_header.version[2] = BETA_VERSION;
			*/
			osd_fwrite(options.record, &inp_header, sizeof(INP_HEADER));
		}

		/* Replace M68000 by FAME */
	#ifdef HAS_FAME
		if (use_fame) 
		{
			for (i=0;i<MAX_CPU;i++)
			{
				int *type=(int*)&(drivers[game_index]->drv->cpu[i].cpu_type);
				#ifdef NEOMAME
				if (((*type)&0xff)==CPU_M68000)
				#else
				if (((*type)&0xff)==CPU_M68000 || ((*type)&0xff)==CPU_M68010 )
				#endif
				{
					*type=((*type)&(~0xff))|CPU_FAME;
				}
			}
		}
	#endif

		/*
			for (i=0;i<MAX_CPU;i++)
			{
				int *type=(int*)&(drivers[game_index]->drv->cpu[i].cpu_type);
				if (((*type)&0xff)==CPU_V30)
				{
					*type=((*type)&(~0xff))|CPU_ARMV30;
				}
			}
			for (i=0;i<MAX_CPU;i++)
			{
				int *type=(int*)&(drivers[game_index]->drv->cpu[i].cpu_type);
				if (((*type)&0xff)==CPU_V33)
				{
					*type=((*type)&(~0xff))|CPU_ARMV33;
				}
			}
		*/
		
		// Remove the mouse usage for certain games
		if ( (strcasecmp(drivers[game_index]->name,"hbarrel")==0) || (strcasecmp(drivers[game_index]->name,"hbarrelw")==0) ||
			 (strcasecmp(drivers[game_index]->name,"midres")==0) || (strcasecmp(drivers[game_index]->name,"midresu")==0) ||
			 (strcasecmp(drivers[game_index]->name,"midresj")==0) || (strcasecmp(drivers[game_index]->name,"tnk3")==0) ||
			 (strcasecmp(drivers[game_index]->name,"tnk3j")==0) || (strcasecmp(drivers[game_index]->name,"ikari")==0) ||
			 (strcasecmp(drivers[game_index]->name,"ikarijp")==0) || (strcasecmp(drivers[game_index]->name,"ikarijpb")==0) ||
			 (strcasecmp(drivers[game_index]->name,"victroad")==0) || (strcasecmp(drivers[game_index]->name,"dogosoke")==0) ||
			 (strcasecmp(drivers[game_index]->name,"gwar")==0) || (strcasecmp(drivers[game_index]->name,"gwarj")==0) ||
			 (strcasecmp(drivers[game_index]->name,"gwara")==0) || (strcasecmp(drivers[game_index]->name,"gwarb")==0) ||
			 (strcasecmp(drivers[game_index]->name,"bermudat")==0) || (strcasecmp(drivers[game_index]->name,"bermudaj")==0) ||
			 (strcasecmp(drivers[game_index]->name,"bermudaa")==0) || (strcasecmp(drivers[game_index]->name,"mplanets")==0) ||
			 (strcasecmp(drivers[game_index]->name,"forgottn")==0) || (strcasecmp(drivers[game_index]->name,"lostwrld")==0) ||
			 (strcasecmp(drivers[game_index]->name,"gondo")==0) || (strcasecmp(drivers[game_index]->name,"makyosen")==0) ||
			 (strcasecmp(drivers[game_index]->name,"topgunr")==0) || (strcasecmp(drivers[game_index]->name,"topgunbl")==0) ||
			 (strcasecmp(drivers[game_index]->name,"tron")==0) || (strcasecmp(drivers[game_index]->name,"tron2")==0) ||
			 (strcasecmp(drivers[game_index]->name,"kroozr")==0) ||(strcasecmp(drivers[game_index]->name,"crater")==0) ||
			 (strcasecmp(drivers[game_index]->name,"dotron")==0) || (strcasecmp(drivers[game_index]->name,"dotrone")==0) ||
			 (strcasecmp(drivers[game_index]->name,"zwackery")==0) || (strcasecmp(drivers[game_index]->name,"ikari3")==0) ||
			 (strcasecmp(drivers[game_index]->name,"searchar")==0) || (strcasecmp(drivers[game_index]->name,"sercharu")==0) ||
			 (strcasecmp(drivers[game_index]->name,"timesold")==0) || (strcasecmp(drivers[game_index]->name,"timesol1")==0) ||
			 (strcasecmp(drivers[game_index]->name,"btlfield")==0) || (strcasecmp(drivers[game_index]->name,"aztarac")==0))
		{
			extern int use_mouse;
			use_mouse=0;
		}

		/* go for it */
		printf ("%s (%s)...\n",drivers[game_index]->description,drivers[game_index]->name);
		res = run_game (game_index);

		/* close open files */
		if (errorlog) fclose (errorlog);
		if (options.playback) osd_fclose (options.playback);
		if (options.record)   osd_fclose (options.record);
		if (options.language_file) osd_fclose (options.language_file);

		if (res!=0)
		{
			/* wait a key press */
			odx_video_flip_single();
			odx_joystick_press();
		}
		
		if(frontend) {
			frontend_main(main_argc, main_argv, &argc, &argv, false);			
		}
	}
   	odx_deinit();

	exit (res);
}

void CLIB_DECL logerror(const char *text,...)
{
	if (errorlog)
	{
		va_list arg;
		va_start(arg,text);
		vfprintf(errorlog,text,arg);
		va_end(arg);
	}
}
