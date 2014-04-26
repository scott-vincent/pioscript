/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * An Audio object is the internal representation of
 * a single audio resource, e.g. one WAV file. The
 * audio object pre-loads the sound into memory when it
 * is created. The engine makes calls to the audio
 * object at execution time to query/change its state.
 *
 * This class also performs some global initialisation
 * of the audio sub-system.
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>


class Audio {

public:
	static const int MAX_SOUNDS = 64;
	static const int MAX_VOLUME = 10;
	static bool mInit;
	static int mUniqueChannel;

private:
	char mWavfile[256];
	Mix_Chunk *mSound;
	int mChannel;
	int mVolume;
	Mix_Chunk mSaved;

public:
	static bool init();
	static void cleanup();

	Audio(const char *wavfile);
	~Audio();
	char *getName() { return mWavfile; };
	bool isValid() { return (mChannel != -1); };
	bool play(int loops);
	bool isPlaying();
	bool pause();
	bool resume();
	bool halt();
	bool volume(int millis);
	bool fadeout(int millis);
	bool playFrom(int millis);
	static bool pauseAll();
	static bool resumeAll();
	static bool haltAll();
	static bool volumeAll(int millis);
	static bool fadeoutAll(int millis);
};

#endif
