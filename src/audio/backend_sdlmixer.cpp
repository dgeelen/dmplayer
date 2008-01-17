#include "backend_sdlmixer.h"
#include "SDL.h"
#include "SDL_mixer.h"

SDLMixerBackend::SDLMixerBackend(IDecoder* dec)	: IBackend(dec) {
	/* We're going to be requesting certain things from our audio
	 * device, so we set them up beforehand
	 */

	//FIXME: Need to be configurable
	int audio_rate = 44100;
	Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
	int audio_channels = 2;
	int audio_buffers = 4096;

	SDL_Init(SDL_INIT_AUDIO);

	/* This is where we open up our audio device.  Mix_OpenAudio takes
	 * as its parameters the audio format we'd /like/ to have.
	 */
	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		throw "SDLMixerBackend: Mix_OpenAudio() failed!"
	}

	/* TODO: Check to see if we got what we asked for */
// 	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);


}

SDLMixerBackend::~SDLMixerBackend()	: IBackend(dec) {
	Mix_CloseAudio();
	SDL_Quit();
}

void SDLMixerBackend::test_playback(const char* filename) {
	Mix_Music music = Mix_LoadMUS( filename );
	Mix_PlayMusic(music, 0);
}