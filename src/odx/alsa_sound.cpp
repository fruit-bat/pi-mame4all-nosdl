#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <SDL2/SDL.h>

#include "minimal.h"

extern int master_volume;


unsigned int			odx_audio_buffer_len=0;
unsigned int			odx_sndlen=0;
unsigned int			odx_vol = 100;
unsigned int			odx_sound_rate=44100;
int						odx_sound_stereo=1;
SDL_AudioSpec 			odx_audio_spec;
pthread_mutex_t 		sndlock;

void odx_sound_volume(int vol)
{
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
}

static SDL_AudioSpec actual_audio_spec;

void odx_sound_play(
  void *buff, // The buffer
  int len     // Length of the buffer in bytes
)
{
	pthread_mutex_lock(&sndlock);
	int i = 0;
	const bool monoToStereo = odx_audio_spec.channels == 1 && actual_audio_spec.channels == 2;
	const int actualLen = monoToStereo ? len<<1 : len;
	while( odx_sndlen+actualLen > odx_audio_buffer_len ) {
        if(i == 5 && odx_video_regulator > 900) odx_video_regulator -= 5;
//printf("AUDIO Overrun %d %d\n", i, odx_video_regulator);        
		if(i++ > 100) {
			// Overrun 
		  odx_sndlen = 0;
		  pthread_mutex_unlock(&sndlock);
		  return;
		}
		pthread_mutex_unlock(&sndlock);
		usleep(2000);
		pthread_mutex_lock(&sndlock);
	}

	if( odx_audio_spec.userdata ) {
		if(monoToStereo) {
			short* p = (short*)((char*)odx_audio_spec.userdata + odx_sndlen);
			short* q = (short*)buff;
			for(int k=0; k < len>>1; ++k) {
				const short s = q[k];
				*p++ = s;
				*p++ = s;
			}
		}
		else {
			memcpy( (char*)odx_audio_spec.userdata + odx_sndlen, buff, len );
		}
		odx_sndlen += actualLen;
	}

	pthread_mutex_unlock(&sndlock);
}

static void odx_sound_callback(void *data, Uint8 *stream, int len)
{
	pthread_mutex_lock(&sndlock);
	
	if( odx_sndlen < len ) {
        if(odx_video_regulator < 2000) odx_video_regulator += 20;
//printf("AUDIO Underrun %d\n", odx_video_regulator);        
		memcpy( stream, data, odx_sndlen );
		memset( stream+odx_sndlen, 0, len-odx_sndlen);
		odx_sndlen = 0;
		pthread_mutex_unlock(&sndlock);
		return;
	}
	memcpy( stream, data, len );
	odx_sndlen -= len;
	memcpy( data, (Uint8*)data + len, odx_sndlen );

	pthread_mutex_unlock(&sndlock);
}

void odx_sound_init(int rate, int bits, int stereo) {

    odx_audio_spec.freq = rate;
	if( bits == 16 )
    	odx_audio_spec.format = AUDIO_S16SYS;
    else
    	odx_audio_spec.format = AUDIO_S8;
    odx_audio_spec.channels = stereo ? 2: 1;
    odx_audio_spec.samples = 1024;
    odx_audio_spec.callback = odx_sound_callback;
    odx_audio_spec.userdata = NULL;
}

void odx_sound_thread_start(void)
{  
	odx_sndlen=0;
printf("Starting audio...\n");
    odx_audio_spec.freq = odx_sound_rate;
    odx_audio_spec.channels = odx_sound_stereo ? 2: 1;

	odx_audio_buffer_len = 16384; // odx_audio_spec.samples * odx_audio_spec.channels * 2 * 64;
	void *audiobuf = malloc( odx_audio_buffer_len );
	memset( audiobuf, 0 , odx_audio_buffer_len );
	odx_audio_spec.userdata=audiobuf;


  
  printf("Requested audio format %d, freq %d, channels %d, samples %d\n",
    odx_audio_spec.format,
    odx_audio_spec.freq,
    odx_audio_spec.channels,
    odx_audio_spec.samples);
    
	if ( SDL_OpenAudio(&odx_audio_spec, &actual_audio_spec) < 0 ) {
		fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
		exit(1);
	}
  
  printf("Audio format %d, freq %d, channels %d, samples %d\n",
    actual_audio_spec.format,
    actual_audio_spec.freq,
    actual_audio_spec.channels,
    actual_audio_spec.samples);
  
  
  	if (pthread_mutex_init(&sndlock, NULL) != 0)
    {
		fprintf(stderr, "Unable to create audio mutex lock\n");
		SDL_CloseAudio();
        exit(1);
    }

	SDL_PauseAudio(0);

  printf("Audio started.\n");
}

void odx_sound_thread_stop(void)
{
	if( odx_audio_spec.userdata ) {
printf("odx_sound_thread_stop(void)\n");
		SDL_PauseAudio(1);
printf("Audio stopped.\n");
		pthread_mutex_destroy(&sndlock);
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);

		free( odx_audio_spec.userdata );
		odx_audio_spec.userdata = NULL;
	}
}

