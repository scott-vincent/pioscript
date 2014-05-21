/****************************************
 * Written by Scott Vincent
 * 4th May 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * This class is used to record from an attached USB
 * microphone (or USB webcam with built-in mic). It
 * uses the portaudio library to do the actual recording
 * then it post-processes the sound to remove background
 * noise and identify silence at the start and end of
 * a recording. It can also write the recording to a
 * WAV file.
 */

#ifndef __RECORD_H__
#define __RECORD_H__

#include <portaudio.h>
#include "audio.h"


class Record {
public:
	static const int ALTERNATE_RATE = 48000;
	static const int MAX_DURATION = 100;
	typedef short SAMPLE;
	static const PaSampleFormat SAMPLE_TYPE = paInt16;
	typedef struct {
		int recordingLevel;
		int actualRate;
		int framesPerBuffer;
		int maxIndex;
		SAMPLE *samples;
		int frameIndex;
		bool soundTriggered;
		int soundWanted;	// Stop recording after this many frames
		int silenceWanted;	// Stop recording after this many frames of silence
		int silenceStart;
		int chunkFrameCount;
		int chunkNoiseCount;
	} RecordData;

private:
	bool mInit;
	PaStream *mStream;
	RecordData mRecordData;
	bool mSaving;
	bool mConverted;

public:
	Record();
	~Record();
	bool initialised() { return mInit; };
	bool setLevel(int level);
	bool startStream(int soundWanted, int silenceWanted);
	bool stopStream();
	bool isActive() { return Pa_IsStreamActive(mStream); };
	bool isTriggered() { return mRecordData.soundTriggered; };
	bool saveWAV(const char *filename, Audio *audio);
	bool playWAV(Audio *audio, double now);
	bool convertData();
};

#endif
