#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "minimal.h"

extern int master_volume;

static snd_pcm_t*           playback_handle = NULL;
static snd_pcm_uframes_t    sound_buffer_size_in_frames;
static unsigned int			odx_vol = 100;

unsigned int			odx_sound_rate=44100;
unsigned int			odx_sound_bits=8;
int						odx_sound_stereo=1;
int                     alsa_sound_stereo=0;

static snd_mixer_elem_t* odx_get_named_mixer(
    snd_mixer_t *handle,
    snd_mixer_selem_id_t *sid, 
    const char *selem_name)
{
    snd_mixer_selem_id_set_name(sid, selem_name);
    return snd_mixer_find_selem(handle, sid);  
}

// volume 0-100
void odx_set_alsa_volume(long volume)
{
    static long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    static bool initialised = false;

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_elem_t* elem = odx_get_named_mixer(handle, sid, "PCM");
    if(!elem) elem = odx_get_named_mixer(handle, sid, "Digital");
    if(elem) {  
        
        if(!initialised) {
            initialised = true;
            long l, r;
            snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_FRONT_LEFT, &l);
            snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_FRONT_RIGHT, &r);
            max = (l + r) / 2L;
            min = -8000;
        }    

		long range = max - min;
		long scaled = (volume * range) / 100;
		long offset = min + scaled;
	  printf("mixer setting min %ld, max %ld, offset %ld\n", min, max, offset);
		snd_mixer_selem_set_playback_dB_all(elem, offset, 1);
	}
    else {
        printf("mixer not found\n");
    }
    snd_mixer_close(handle);
}

void odx_sound_volume(int vol)
{
 	if( vol < 0 ) vol = 0;
 	if( vol > 100 ) vol = 100;
 	odx_set_alsa_volume(vol);

 	if( vol > 0 ) {
 		master_volume = vol;
 		if( odx_vol == 0 ) {
			odx_sound_thread_start();
 		}
 		
 	}
 	else {
		odx_sound_thread_stop();
 	}
 	
 	odx_vol = vol;	
}

void odx_sound_play_direct(
  void *buff, // The buffer
  int number_of_bytes     // Length of the buffer in bytes
)
{
	if(playback_handle == NULL) return;
	const unsigned int target_frames = sound_buffer_size_in_frames - 8192;

	int frames_to_write = number_of_bytes >> (alsa_sound_stereo ? 2 : 1);
	short *p = (short *)buff;
	int frames_free_in_alsa;
	while(true) {
		frames_free_in_alsa = snd_pcm_avail_update(playback_handle);
		if(frames_free_in_alsa > target_frames) {
			frames_free_in_alsa -= target_frames;
		}
		else if(frames_free_in_alsa > 0) {
			frames_free_in_alsa = 0;
		}
		if(frames_free_in_alsa > 4096) {
			odx_video_regulator += 2;
		}

		int err;
		int f;
		if((frames_free_in_alsa < 0) || (frames_free_in_alsa >= frames_to_write)){
			f = frames_to_write;
		}
		else {
			f = frames_free_in_alsa;
		}
		
		if ((err = snd_pcm_writei(playback_handle, p, f)) == -EPIPE) {
			printf("XRUN.\n");
			snd_pcm_prepare(playback_handle);
		} else if (err < 0) {
			printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(err));
		}
		
		frames_to_write -= f;
		
		if(frames_to_write == 0) break;
		
		usleep(1000);
		
		p += f;
		if(alsa_sound_stereo) p += f;
	}	
}

void odx_sound_play_double(
  void *buff, // The buffer
  int number_of_bytes     // Length of the buffer in bytes
)
{
    static short buffer[2048];
	int frames_to_write = number_of_bytes >> 1;
	short *p = (short *)buff;    
    while(frames_to_write > 0) {
        short *q = buffer;
        int i = 0;
        for(; i < frames_to_write && i < 1024; ++i) {
            *q++ = *p;
            *q++ = *p++;   
        }
        odx_sound_play_direct(buffer, i << 2);
        frames_to_write -= i;
    }
}

void odx_sound_play(
  void *buff, // The buffer
  int number_of_bytes     // Length of the buffer in bytes
)
{
    if(odx_sound_stereo == alsa_sound_stereo) {
        odx_sound_play_direct(buff, number_of_bytes);
    }
    else if(odx_sound_stereo == 0) {
        odx_sound_play_double(buff, number_of_bytes);
    }
}

void odx_sound_init(int rate, int bits, int stereo) {
	printf("ALSA Audio init rate %d, bits %d, stereo %d\n", rate, bits, stereo);
}

bool odx_sound_thread_start(void)
{  	
	if(playback_handle != NULL) return true;
	
	printf("ALSA Audio start thread\n");
	const char *pcm_device = "default";
	
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_sframes_t frames_to_deliver;
	int nfds;
	int err;
	struct pollfd *pfds;

	if ((err = snd_pcm_open (&playback_handle, pcm_device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf (stderr, "cannot open audio device %s (%s)\n", 
			 pcm_device,
			 snd_strerror (err));
		return false;
	}
	   
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return false;
	}
			 
	if ((err = snd_pcm_hw_params_any (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return false;
	}

	if ((err = snd_pcm_hw_params_set_access (playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		return false;
	}

	if ((err = snd_pcm_hw_params_set_format (playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		return false;
	}

    alsa_sound_stereo = odx_sound_stereo;
	if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, odx_sound_stereo ? 2: 1)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)... trying 2 instead\n",
			 snd_strerror (err));
        alsa_sound_stereo = 1;
             
        if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, 2)) < 0) {
            fprintf (stderr, "cannot set channel count (%s)\n",
                 snd_strerror (err));
            return false;
        }
	}
    
	unsigned int actual_rate = odx_sound_rate;
	if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &actual_rate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		return false;
	}
    
	if ((err = snd_pcm_hw_params (playback_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		return false;
	}

	if ((err = snd_pcm_hw_params_get_buffer_size (hw_params, &sound_buffer_size_in_frames)) < 0) {
		fprintf (stderr, "cannot get buffer size (%s)\n",
			 snd_strerror (err));
		return false;
	}
	printf("ALSA Buffer size %d stero %d\n", sound_buffer_size_in_frames, odx_sound_stereo);

	snd_pcm_hw_params_free (hw_params);

	/* tell ALSA to wake us up whenever 4096 or more frames
	   of playback data can be delivered. Also, tell
	   ALSA that we'll start the device ourselves.
	*/

	if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
		fprintf (stderr, "cannot allocate software parameters structure (%s)\n",
			 snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_sw_params_current (playback_handle, sw_params)) < 0) {
		fprintf (stderr, "cannot initialize software parameters structure (%s)\n",
			 snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_sw_params_set_avail_min (playback_handle, sw_params, 512)) < 0) {
		fprintf (stderr, "cannot set minimum available count (%s)\n",
			 snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_sw_params_set_start_threshold (playback_handle, sw_params, 0U)) < 0) {
		fprintf (stderr, "cannot set start mode (%s)\n",
			 snd_strerror (err));
		return false;
	}
	if ((err = snd_pcm_sw_params (playback_handle, sw_params)) < 0) {
		fprintf (stderr, "cannot set software parameters (%s)\n",
			 snd_strerror (err));
		return false;
	}

	/* the interface will interrupt the kernel every 4096 frames, and ALSA
	   will wake up this program very soon after that.
	*/

	if ((err = snd_pcm_prepare (playback_handle)) < 0) {
		fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
			 snd_strerror (err));
		return false;
	}
	
	printf("ALSA Audio init OK\n"); 
}

void odx_sound_thread_stop(void)
{
	if(playback_handle == NULL) return;
	
	snd_pcm_close (playback_handle);
	playback_handle = NULL;
}

