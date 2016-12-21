#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "minimal.h"

extern int master_volume;

snd_pcm_t *playback_handle = NULL;

snd_pcm_uframes_t       sound_buffer_size_in_frames;
unsigned int			odx_vol = 100;
unsigned int			odx_sound_rate=44100;
unsigned int			odx_sound_bits=8;
int						odx_sound_stereo=1;

void odx_sound_volume(int vol)
{
	/*
printf("odx_sound_volume(void)\n");
 	if( vol < 0 ) vol = 0;
 	if( vol > 100 ) vol = 100;

 	if( vol > 0 ) {
 		master_volume = vol;
 		if( odx_vol == 0 ) {
printf("Audio started.\n");
 			SDL_PauseAudio(0);
 			if( odx_audio_spec.userdata )
	 			memset( odx_audio_spec.userdata, 0 , odx_audio_buffer_len );
 			odx_sndlen = 0;
 		}
 	}
 	else {
printf("Audio stopped.\n");
		SDL_PauseAudio(1);
 	}
 	
 	odx_vol = vol;
 	*/
}

void odx_sound_play(
  void *buff, // The buffer
  int number_of_bytes     // Length of the buffer in bytes
)
{
	int frames_free_in_alsa = snd_pcm_avail_update(playback_handle);
	int frames_to_write = number_of_bytes >> (odx_sound_stereo ? 2 : 1);
//printf("ALSA Wants %d and we have %d\n", frames_free_in_alsa, frames_to_write);	
	int err;
	if (err = snd_pcm_writei(playback_handle, buff, frames_to_write) == -EPIPE) {
		printf("XRUN.\n");
		snd_pcm_prepare(playback_handle);
	} else if (err < 0) {
		printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(err));
	}	
	
	if(frames_free_in_alsa >= 0) {
		const unsigned int target_frames = sound_buffer_size_in_frames - 4096;
		const int diff = frames_free_in_alsa - target_frames;
		const int f = diff / 10;
		int r = 1000 + f;
		if(r < 1) r = 1;
		odx_video_regulator = r;
		//printf("diff=%d, f=%d, reg=%d\n", diff, f, odx_video_regulator);
	}	
}

void odx_sound_init(int rate, int bits, int stereo) {
}

bool odx_sound_thread_start(void)
{  	
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

	int actual_rate = odx_sound_rate;
	if ((err = snd_pcm_hw_params_set_rate_near (playback_handle, hw_params, &actual_rate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		return false;
	}

	if ((err = snd_pcm_hw_params_set_channels (playback_handle, hw_params, odx_sound_stereo ? 2: 1)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
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
	printf("ALSA Buffer size %d\n", sound_buffer_size_in_frames);

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
	if(playback_handle != NULL) {
		snd_pcm_close (playback_handle);
		playback_handle = NULL;
	}
}

