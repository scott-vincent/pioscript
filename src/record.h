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
	static const int MAX_DURATION = 100;
	typedef short SAMPLE;
	static const PaSampleFormat SAMPLE_TYPE = paInt16;
	typedef struct {
		int recordingLevel;
		int maxFrames;
		SAMPLE *samples;
		int frameIndex;
		bool soundTriggered;
		int soundWanted;	// Stop recording after this many frames
		int silenceWanted;	// Stop recording after this many frames of silence
		int silenceStart;
		int soundStart;
		int soundCount;
		int silenceLen;
		int silenceEnd;
	} RecordData;

private:
	bool mInit;
	PaStream *mStream;
	RecordData mRecordData;
	bool mSaving;

public:
	Record();
	~Record();
	bool initialised() { return mInit; };
	bool setLevel(int level);
	bool startStream();
	bool stopStream();
	bool isStarted() { return (mRecordData.frameIndex > 0); };
	bool isActive() { Pa_IsStreamActive(mStream); };
	bool saveWAV(const char *filename, Audio *audio);
	bool playWAV(Audio *audio, double now);
};

#endif
