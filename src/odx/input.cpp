#include "allegro.h"
#include "driver.h"
#include "minimal.h"

int use_mouse;
// Delivered from the command line!
int joystick;
int num_joysticks=4;

/* return a list of all available keys */
const struct KeyboardInfo *osd_get_key_list(void)
{
	return odx_window_get_keyboard_info();
}

int osd_is_key_pressed(int keycode)
{
	return odx_key_pressed(keycode);
}

int osd_wait_keypress(void)
{
	return 0;
}

int osd_readkey_unicode(int flush)
{
	return 0;
}

/*
  limits:
  - 7 joysticks
  - 15 sticks on each joystick
  - 63 buttons on each joystick

  - 256 total inputs
*/
#define JOYCODE(joy,stick,axis_or_button,dir) \
		((((dir)&0x03)<<14)|(((axis_or_button)&0x3f)<<8)|(((stick)&0x1f)<<3)|(((joy)&0x07)<<0))

#define GET_JOYCODE_JOY(code) (((code)>>0)&0x07)
#define GET_JOYCODE_STICK(code) (((code)>>3)&0x1f)
#define GET_JOYCODE_AXIS(code) (((code)>>8)&0x3f)
#define GET_JOYCODE_BUTTON(code) GET_JOYCODE_AXIS(code)
#define GET_JOYCODE_DIR(code) (((code)>>14)&0x03)

/* use otherwise unused joystick codes for the three mouse buttons */
#define MOUSE_BUTTON(button) JOYCODE(1,0,button,1)

#define MAX_JOY 256
#define MAX_JOY_NAME_LEN 40

static struct JoystickInfo joylist[MAX_JOY] =
{
	/* will be filled later */
	{ 0, 0, 0 }	/* end of table */
};

static char joynames[MAX_JOY][MAX_JOY_NAME_LEN+1];	/* will be used to store names for the above */


static int joyequiv[][2] =
{
	{ JOYCODE(1,1,1,1),	JOYCODE_1_LEFT },
	{ JOYCODE(1,1,1,2),	JOYCODE_1_RIGHT },
	{ JOYCODE(1,1,2,1),	JOYCODE_1_UP },
	{ JOYCODE(1,1,2,2),	JOYCODE_1_DOWN },
	{ JOYCODE(1,0,1,0),	JOYCODE_1_BUTTON1 },
	{ JOYCODE(1,0,2,0),	JOYCODE_1_BUTTON2 },
	{ JOYCODE(1,0,3,0),	JOYCODE_1_BUTTON3 },
	{ JOYCODE(1,0,4,0),	JOYCODE_1_BUTTON4 },
	{ JOYCODE(1,0,5,0),	JOYCODE_1_BUTTON5 },
	{ JOYCODE(1,0,6,0),	JOYCODE_1_BUTTON6 },
	{ JOYCODE(1,0,7,0),	JOYCODE_1_BUTTON7 },
	{ JOYCODE(1,0,8,0),	JOYCODE_1_BUTTON8 },
	{ JOYCODE(2,1,1,1),	JOYCODE_2_LEFT },
	{ JOYCODE(2,1,1,2),	JOYCODE_2_RIGHT },
	{ JOYCODE(2,1,2,1),	JOYCODE_2_UP },
	{ JOYCODE(2,1,2,2),	JOYCODE_2_DOWN },
	{ JOYCODE(2,0,1,0),	JOYCODE_2_BUTTON1 },
	{ JOYCODE(2,0,2,0),	JOYCODE_2_BUTTON2 },
	{ JOYCODE(2,0,3,0),	JOYCODE_2_BUTTON3 },
	{ JOYCODE(2,0,4,0),	JOYCODE_2_BUTTON4 },
	{ JOYCODE(2,0,5,0),	JOYCODE_2_BUTTON5 },
	{ JOYCODE(2,0,6,0),	JOYCODE_2_BUTTON6 },
	{ JOYCODE(2,0,7,0),	JOYCODE_2_BUTTON7 },
	{ JOYCODE(2,0,8,0),	JOYCODE_2_BUTTON8 },
	{ JOYCODE(3,1,1,1),	JOYCODE_3_LEFT },
	{ JOYCODE(3,1,1,2),	JOYCODE_3_RIGHT },
	{ JOYCODE(3,1,2,1),	JOYCODE_3_UP },
	{ JOYCODE(3,1,2,2),	JOYCODE_3_DOWN },
	{ JOYCODE(3,0,1,0),	JOYCODE_3_BUTTON1 },
	{ JOYCODE(3,0,2,0),	JOYCODE_3_BUTTON2 },
	{ JOYCODE(3,0,3,0),	JOYCODE_3_BUTTON3 },
	{ JOYCODE(3,0,4,0),	JOYCODE_3_BUTTON4 },
	{ JOYCODE(3,0,5,0),	JOYCODE_3_BUTTON5 },
	{ JOYCODE(3,0,6,0),	JOYCODE_3_BUTTON6 },
	{ JOYCODE(3,0,7,0),	JOYCODE_3_BUTTON7 },
	{ JOYCODE(3,0,8,0),	JOYCODE_3_BUTTON8 },
	{ JOYCODE(4,1,1,1),	JOYCODE_4_LEFT },
	{ JOYCODE(4,1,1,2),	JOYCODE_4_RIGHT },
	{ JOYCODE(4,1,2,1),	JOYCODE_4_UP },
	{ JOYCODE(4,1,2,2),	JOYCODE_4_DOWN },
	{ JOYCODE(4,0,1,0),	JOYCODE_4_BUTTON1 },
	{ JOYCODE(4,0,2,0),	JOYCODE_4_BUTTON2 },
	{ JOYCODE(4,0,3,0),	JOYCODE_4_BUTTON3 },
	{ JOYCODE(4,0,4,0),	JOYCODE_4_BUTTON4 },
	{ JOYCODE(4,0,5,0),	JOYCODE_4_BUTTON5 },
	{ JOYCODE(4,0,6,0),	JOYCODE_4_BUTTON6 },
	{ JOYCODE(4,0,7,0),	JOYCODE_4_BUTTON7 },
	{ JOYCODE(4,0,8,0),	JOYCODE_4_BUTTON8 },
	{ 0,0 }
};

static int joy_standardcode_to_joycode(int standardcode) {
	for(int i = 0; joyequiv[i][1] != 0; ++i) {
		if(standardcode == joyequiv[i][1]) return joyequiv[i][0];
	}
	return 0;
}

bool odx_is_joy_pressed_by_standardcode(int standardcode) {
	int joycode = joy_standardcode_to_joycode(standardcode);
	return osd_is_joy_pressed(joycode);
}

static void init_joy_list(void)
{
	int tot,i,j,k;
	char buf[256];

	tot = 0;

	/* first of all, map mouse buttons */
	for (j = 0;j < 3;j++)
	{
		sprintf(buf,"MOUSE B%d",j+1);
		strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
		joynames[tot][MAX_JOY_NAME_LEN] = 0;
		joylist[tot].name = joynames[tot];
		joylist[tot].code = MOUSE_BUTTON(j+1);
		tot++;
	}

	for (i = 0;i < num_joysticks;i++)
	{
		for (j = 0;j < 1;j++)
		{
			for (k = 0;k < 2;k++)
			{
				sprintf(buf,"J%d %s %s -",i+1,"JoystickAxis","-");
				strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
				joynames[tot][MAX_JOY_NAME_LEN] = 0;
				joylist[tot].name = joynames[tot];
				joylist[tot].code = JOYCODE(i+1,j+1,k+1,1);
				tot++;

				sprintf(buf,"J%d %s %s +",i+1,"JoystickAxis","+");
				strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
				joynames[tot][MAX_JOY_NAME_LEN] = 0;
				joylist[tot].name = joynames[tot];
				joylist[tot].code = JOYCODE(i+1,j+1,k+1,2);
				tot++;
			}
		}
		for (j = 0;j < 8;j++)
		{
			sprintf(buf,"J%d %s",i+1,"JoystickButton");
			strncpy(joynames[tot],buf,MAX_JOY_NAME_LEN);
			joynames[tot][MAX_JOY_NAME_LEN] = 0;
			joylist[tot].name = joynames[tot];
			joylist[tot].code = JOYCODE(i+1,0,j+1,0);
			tot++;
		}
	}

	/* terminate array */
	joylist[tot].name = 0;
	joylist[tot].code = 0;
	joylist[tot].standardcode = 0;

	/* fill in equivalences */
	for (i = 0;i < tot;i++)
	{
		joylist[i].standardcode = JOYCODE_OTHER;

		j = 0;
		while (joyequiv[j][0] != 0)
		{
			if (joyequiv[j][0] == joylist[i].code)
			{
				joylist[i].standardcode = joyequiv[j][1];
				break;
			}
			j++;
		}
	}
}

/* return a list of all available joys */
const struct JoystickInfo *osd_get_joy_list(void)
{
	return joylist;
}

#define JOY_LEFT_PRESSED odx_is_joy_axis_pressed(0,0,1)
#define JOY_RIGHT_PRESSED odx_is_joy_axis_pressed(0,0,2)
#define JOY_UP_PRESSED odx_is_joy_axis_pressed(0,1,1)
#define JOY_DOWN_PRESSED odx_is_joy_axis_pressed(0,1,2)

int osd_is_joy_pressed(int joycode)
{
	int joy_num,stick;

	/* special case for mouse buttons */
	switch (joycode)
	{
// TODO PS Fix this when I work out what its for!
		case MOUSE_BUTTON(1):
		case MOUSE_BUTTON(2):
		case MOUSE_BUTTON(3):
			return false;
        default:
            break;
    }

	joy_num = GET_JOYCODE_JOY(joycode);

	/* do we have as many sticks? */
	if (joy_num == 0 || joy_num > num_joysticks)
		return 0;
	joy_num--;

	stick = GET_JOYCODE_STICK(joycode);

	if (stick == 0)
	{
		/* buttons */
		int button;

		button = GET_JOYCODE_BUTTON(joycode);
		if (button == 0 || button > 8)
			return 0;
		button--;

        return odx_is_joy_button_pressed (joy_num, button);
	}
	else
	{
		/* sticks */
		int axis,dir;

		if (stick > 1)
			return 0;
		stick--;

		axis = GET_JOYCODE_AXIS(joycode);
		dir = GET_JOYCODE_DIR(joycode);

		if (axis == 0 || axis > 2)
			return 0;
		axis--;

        return odx_is_joy_axis_pressed (joy_num, axis, dir);
	}

	return 0;
}

void osd_poll_joysticks(void)
{
  odx_window_process_events();
  odx_poll_joysticks();
}

int osd_joystick_needs_calibration (void)
{
	return 0;
}


void osd_joystick_start_calibration (void)
{
}

char *osd_joystick_calibrate_next (void)
{
	return 0;
}

void osd_joystick_calibrate (void)
{
}

void osd_joystick_end_calibration (void)
{
}

void osd_trak_read(int player,int *deltax,int *deltay)
{
	if (player != 0 || use_mouse == 0)
		*deltax = *deltay = 0;
	else
	{
		*deltax = *deltay = 0;
		if(JOY_LEFT_PRESSED) *deltax=-5;
	  	if(JOY_RIGHT_PRESSED) *deltax=5;
	  	if(JOY_UP_PRESSED) *deltay=5; 
	 	if(JOY_DOWN_PRESSED) *deltay=-5;
	}
}

#ifndef MESS
#ifndef TINY_COMPILE
extern int no_of_tiles;
extern struct GameDriver driver_neogeo;
#endif
#endif

void osd_customize_inputport_defaults(struct ipd *defaults)
{
}

void osd_led_w(int led,int on) {
}

void msdos_init_input (void)
{
	if (joystick == JOY_TYPE_NONE)
		logerror("Joystick not found\n");
	else
		logerror("Installed %s %s\n","Joystick", "GCW0");

	init_joy_list();

	if (use_mouse)
		use_mouse = 1;
	else
		use_mouse = 0;
}

void msdos_shutdown_input(void)
{
}
