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
bool Audio::mUsingAudio = false;
int Audio::mUniqueChannel = 0;
bool Audio::mHWFail = false;
bool Audio::mAllPaused = false;


/**
 * Global init to setup SDL library
 */
bool Audio::init()
{
	if (mInit)
		return true;

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
		if (!mHWFail){
			Mix_CloseAudio();
			SDL_Quit();
		}
		mInit = false;
	}
}


/**
 * SDL_Mixer doesn't play nicely with portaudio so we need
 * to disable it before starting a recording.
 */
void Audio::disable()
{
	if (mUsingAudio){
		// Closedown audio but don't free any chunks
		cleanup();
	}
}


/**
 * This method re-enables SDL_Mixer after it it has been
 * disabled.
 */
void Audio::enable()
{
	if (mUsingAudio){
		// Startup audio so existing chunks are now playable again
		init();
	}
}


/**
 * Constructor loads a new sound sample
 */
Audio::Audio(const char *wavfile, int lineNum)
{
	strcpy(mWavfile, wavfile);
	mLineNum = lineNum;
	mSound = NULL;
	mChannel = -1;

	if (!mInit){
		mUsingAudio = true;
		init();
	}

	if (access(wavfile, 0) == 0){
		mMissing = false;
		mSound = Mix_LoadWAV(wavfile);
		if (!mSound){
			fprintf(stderr, "Failed to load sound %s\n", wavfile);
			fprintf(stderr, "Error: %s\n", Mix_GetError());
			return;
		}
	}
	else {
		// May get replaced by a recorded sample so just
		// create silence for now.
		mMissing = true;
		short silence[2];
		silence[0] = 0;
		silence[1] = 0;
		mSound = Mix_QuickLoad_RAW((Uint8*)silence, 4);
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
	mMilliLen = (mSaved.alen * 10) / (441 * 4);
	mStartTime = 0;
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


/*
 * Replace the loaded WAV with a new one. This is called
 * when the recorder saves a WAV file that is being
 * used for playback. The WAV data comes straight from
 * the recorder's memory so it doesn't need to be loaded.
 */
bool Audio::replaceWAV(void *data, int dataSize)
{
	if (!mInit){
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
		return false;
	}

	if (Mix_Playing(mChannel) != 0)
		Mix_HaltChannel(mChannel);

	Mix_FreeChunk(mSound);
	mSound = Mix_QuickLoad_RAW((Uint8*)data, dataSize);
	mSaved.abuf = mSound->abuf;
	mSaved.alen = mSound->alen;
	mMilliLen = (mSaved.alen * 10) / (441 * 4);
	return true;
}


/**
 * loops = 0 to play sound once, -1 to loop indefinitely
 */
bool Audio::play(int loops, double now)
{
	if (!mInit){
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	if (mHWFail){
		fprintf(stderr, "Failed to play sound %s\n", mWavfile);
		fprintf(stderr, "Error: Reboot required\n");
		return false;
	}

	if (Mix_Playing(mChannel) != 0)
		Mix_HaltChannel(mChannel);

	int channel = Mix_PlayChannel(mChannel, mSound, loops);
	if (channel == -1) {
		fprintf(stderr, "Failed to play sound %s\n", mWavfile);
		fprintf(stderr, "Error: %s\n", Mix_GetError());
		return false;
	}

	if (loops == 0)
		mStartTime = now;

	return true;
}


/**
 *
 */
bool Audio::isPlaying(double now)
{
	static bool msgDisplayed = false;

	if (!mInit){
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
		return false;
	}

	bool playing = (Mix_Playing(mChannel) != 0);

	// Detect sound failure due to hardware PWM. When this happens
	// the sound thinks it is playing forever and never completes.
	if (playing && mStartTime > 0 && !mAllPaused){
		// If actual wait is at least 10 seconds more than expected
		// the sound has failed to stop.
		if ((now - mStartTime) > (mMilliLen + 10000)){
			if (!msgDisplayed){
				fprintf(stderr, "===============================================================================\n");
				fprintf(stderr, "  WARNING: Cannot play sound through the audio jack until the Pi is rebooted.\n");
				fprintf(stderr, "  This is because you have used either the sound_buzzer, play_note or play_song\n");
				fprintf(stderr, "  commands which interfere with analog audio. To avoid this problem in the\n");
				fprintf(stderr, "  future please use HDMI audio instead.\n");
				fprintf(stderr, "===============================================================================\n");
				fprintf(stderr, "ERROR: Reboot required\n");
				msgDisplayed = true;
			}
			mHWFail = true;
			playing = false;
		}
	}
	else {
		mStartTime = 0;
	}

	return playing;
}


/**
 *
 */
bool Audio::pause()
{
	if (!mInit){
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
		return false;
	}

	if (!mSound){
		fprintf(stderr, "Sound not loaded\n");
		return false;
	}

	Mix_Pause(mChannel);
	mStartTime = 0;
	return true;
}


/**
 *
 */
bool Audio::resume()
{
	if (!mInit){
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
	mStartTime = 0;
}


/**
 *
 */
bool Audio::pauseAll()
{
	if (!mInit){
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
		return false;
	}

	Mix_Pause(-1);
	mAllPaused = true;
	return true;
}


/**
 *
 */
bool Audio::resumeAll()
{
	if (!mInit){
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
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
		fprintf(stderr, "Cannot play sound when a recording is in progres\n");
		return false;
	}

	Mix_FadeOutChannel(-1, millis);
	return true;
}
