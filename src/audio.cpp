/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <unistd.h>
#include "audio.h"

bool Audio::mInit = false;
int Audio::mUniqueChannel = 0;

/**
 * Global init to setup SDL library
 */
bool Audio::init()
{
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		fprintf(stderr, "Unable to initialise SDL: %s\n", SDL_GetError());
		return false;
	}
 
	int audio_rate = 44100;
	Uint16 audio_format = AUDIO_S16SYS;
	int audio_channels = 2;
	int audio_buffers = 4096;
 
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
		fprintf(stderr, "Unable to initialise audio: %s\n", Mix_GetError());
		SDL_Quit();
		return false;
	}

	Mix_AllocateChannels(MAX_SOUNDS);

	mInit = true;
	return true;
}


/**
 * Global cleanup to shutdown SDL library
 */
void Audio::cleanup()
{
	if (mInit){
		Mix_CloseAudio();
		SDL_Quit();
		mInit = false;
	}
}


/**
 * Constructor loads a new sound sample
 */
Audio::Audio(const char *wavfile)
{
	strcpy(mWavfile, wavfile);
	mSound = NULL;
	mChannel = -1;

	if (!mInit)
		init();

	if (access(wavfile, 0) != 0){
		fprintf(stderr, "File %s not found\n", wavfile);
		return;
	}

	mSound = Mix_LoadWAV(wavfile);
	if (!mSound){
		fprintf(stderr, "Failed to load sound %s\n", wavfile);
		fprintf(stderr, "Error: %s\n", Mix_GetError());
		return;
	}

	// Allocate a unique channel for this sound
	if (mUniqueChannel == MAX_SOUNDS - 1){
		fprintf(stderr, "Failed to load sound %s\n", wavfile);
		fprintf(stderr, "Maximum number of sounds exceeded (%d)\n", MAX_SOUNDS);
		return;
	} 

	mChannel = ++mUniqueChannel;

	// Save the buffer pointer in case we re-position it
	mSaved.abuf = mSound->abuf;
	mSaved.alen = mSound->alen;
}


/**
 * Destructor unloads sound sample
 */
Audio::~Audio()
{
	if (mSound){
		// Restore the buffer pointer in case we re-positioned it
		mSound->abuf = mSaved.abuf;
		mSound->alen = mSaved.alen;
		Mix_FreeChunk(mSound);
	}
}


/**
 * loops = 0 to play sound once, -1 to loop indefinitely
 */
bool Audio::play(int loops)
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	int channel = Mix_PlayChannel(mChannel, mSound, loops);
	if (channel == -1) {
		fprintf(stderr, "Failed to play sound %s\n", mWavfile);
		fprintf(stderr, "Error: %s\n", Mix_GetError());
		return false;
	}

	return true;
}


/**
 *
 */
bool Audio::isPlaying()
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	return (Mix_Playing(mChannel) != 0);
}


/**
 *
 */
bool Audio::pause()
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	Mix_Pause(mChannel);
	return true;
}


/**
 *
 */
bool Audio::resume()
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	Mix_Resume(mChannel);
	return true;
}


/**
 *
 */
bool Audio::halt()
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	Mix_HaltChannel(mChannel);
	return true;
}


/**
 *
 */
bool Audio::volume(int millis)
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	// Convert volume range 0 to 10 into correct range
	int volume = (millis * MIX_MAX_VOLUME) / 10000;
	Mix_Volume(mChannel, volume);
	return true;
}


/**
 *
 */
bool Audio::fadeout(int millis)
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	Mix_FadeOutChannel(mChannel, millis);
	return true;
}



/**
 *
 */
bool Audio::playFrom(int millis)
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	// 44100 x 2 (16-bit) x 2 (stereo) samples per secound
	int newPos = 1764 * (millis / 10);
	if (newPos >= mSaved.alen){
		fprintf(stderr, "Failed to position sound %s\n", mWavfile);
		fprintf(stderr, "Error: Cannot play from point past end\n");
		return false;
	}

	mSound->abuf = mSaved.abuf + newPos;
	mSound->alen = mSaved.alen - newPos;

	int channel = Mix_PlayChannel(mChannel, mSound, 0);
	if (channel == -1) {
		fprintf(stderr, "Failed to play sound %s\n", mWavfile);
		fprintf(stderr, "Error: %s\n", Mix_GetError());
		return false;
	}
}


/**
 *
 */
bool Audio::pauseAll()
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	Mix_Pause(-1);
	return true;
}


/**
 *
 */
bool Audio::resumeAll()
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	Mix_Resume(-1);
	return true;
}


/**
 *
 */
bool Audio::haltAll()
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	Mix_HaltChannel(-1);
	return true;
}


/**
 *
 */
bool Audio::volumeAll(int millis)
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	// Convert volume range 0 to 10 into correct range
	int volume = (millis * MIX_MAX_VOLUME) / 10000;
	Mix_Volume(-1, volume);
	return true;
}


/**
 *
 */
bool Audio::fadeoutAll(int millis)
{
	if (!mInit){
		fprintf(stderr, "Audio not initialised\n");
		return false;
	}

	Mix_FadeOutChannel(-1, millis);
	return true;
}
