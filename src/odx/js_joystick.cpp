#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <errno.h>
class JsJoystick {
    
private:   
 
    const char *_dev_name;
    int _joy_fd;
    int* _axis;
    char* _button;
    int _num_of_axis;
    int _num_of_buttons;
    char _name_of_joystick[80];
    
    void _details() {
        ioctl( _joy_fd, JSIOCGAXES, &_num_of_axis );
        ioctl( _joy_fd, JSIOCGBUTTONS, &_num_of_buttons );
        ioctl( _joy_fd, JSIOCGNAME(80), &_name_of_joystick );
        
        fcntl( _joy_fd, F_SETFL, O_NONBLOCK );

        _axis = new int[_num_of_axis];
        _button = new char[_num_of_buttons];
    }
    
    inline bool _is_open() {
        return _joy_fd != -1;
    }
    
public:

    JsJoystick(const char *dev_name) : 
        _dev_name(dev_name), 
        _joy_fd(-1),
        _axis(NULL),
        _num_of_axis(0),
        _num_of_buttons(0),
        _button(NULL)
    {
    }
    
    void close() {
        if(_is_open()) {
            ::close(_joy_fd);
            delete _axis;
            delete _button;
            _joy_fd = -1;
        }
    } 
    
    ~JsJoystick() {
        close();
    }
    
    bool open() {
        if(!_is_open()) {
            _joy_fd = ::open(_dev_name , O_RDONLY);
            if(_is_open()) {
                _details();
                
                printf("Joystick detected: %s\n\t%d axis\n\t%d buttons\n\n"
                    , _name_of_joystick
                    , _num_of_axis
                    , _num_of_buttons );            
            }
        }
        return _is_open();
    }
      
    void poll() {
        
        if(!_is_open()) {
            if(!open()) {
                return;
            }
        }
        
       	struct js_event js;

        int r = 0;
		while((r = read(_joy_fd, &js, sizeof(struct js_event))) > 0) {
            switch (js.type & ~JS_EVENT_INIT)
            {
                case JS_EVENT_AXIS:
                    _axis   [ js.number ] = js.value;
                    break;
                case JS_EVENT_BUTTON:
                    _button [ js.number ] = js.value;
                    break;
            }  
        }
        if(r < 0 && errno != EAGAIN) {
            close();
            return;
        }
    }
    
    bool is_button_pressed(int button) {
        if(button >= _num_of_buttons) return false;
        return _button[button];
    }
    
    bool is_axis_left(int i) {
        if(i >= _num_of_axis) return false;
        int v = _axis[i];
        return v < -10;
    }
    
    bool is_axis_right(int i) {
        if(i >= _num_of_axis) return false;
        int v = _axis[i];
        return v > +10;
    }
    
    int axis(int i) {
        if(i >= _num_of_axis) return 0;
        return _axis[i];
    }
        
};

#define NUM_JOYSTICKS 2

JsJoystick js_joysticks[] = {
    JsJoystick("/dev/input/js0"),
    JsJoystick("/dev/input/js1")
};

void odx_poll_joysticks() {
    for(int i = 0; i < NUM_JOYSTICKS; ++i) {
        js_joysticks[i].poll();  
    }
}

// For front end only
unsigned int odx_joystick_read(unsigned int index) {
    if(index >= NUM_JOYSTICKS) return 0;
    return 0; // TODO
}

bool odx_is_joy_button_pressed(int index, int button) {
    if(index >= NUM_JOYSTICKS) return false;
    return js_joysticks[index].is_button_pressed(button);
}

bool odx_is_joy_axis_pressed (int index, int axis, int dir) {
    if(index >= NUM_JOYSTICKS) return false;  
    return dir == 1 ? 
        js_joysticks[index].is_axis_left(axis) :
        js_joysticks[index].is_axis_right(axis);
}

/* return a value in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player, int *analog_x, int *analog_y)
{
    if(player >= NUM_JOYSTICKS) return;  
	*analog_x = js_joysticks[player].axis(0) / 256;
	*analog_y = js_joysticks[player].axis(0) / 256;
}
