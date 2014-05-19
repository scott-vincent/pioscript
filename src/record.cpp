/****************************************
 * Written by Scott Vincent
 * 4th May 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "record.h"
#include "engine.h"
#include "sample.h"


/**
 * This is the PortAudio callback function. It needs to
 * process the buffer as quickly as possible and save
 * the data to our own buffer. Note that the created
 * buffer is in mono.
 */
static int recordCallback(const void *inputBuffer, void *outputBuffer,
						  unsigned long framesPerBuffer,
						  const PaStreamCallbackTimeInfo* timeInfo,
						  PaStreamCallbackFlags statusFlags,
						  void *userData)
{
	// Put one frame in the buffer so we know we know we have
	// started recording.
	Record::RecordData *data = (Record::RecordData*)userData;
	if (data->frameIndex == 0){
		data->frameIndex = 1;
		data->samples[0] = 0;
		data->silenceStart = 1;
		data->soundStart = -1;
		data->lastSound = -1;
		data->soundCount = 0;
		data->loudSoundCount = 0;
	}

	const Record::SAMPLE *src = (const Record::SAMPLE*)inputBuffer;
	Record::SAMPLE *dest = &data->samples[data->frameIndex];

	// Save the frame buffer (will be in mono).
	// Any sound must be longer than 500 frames otherwise it is
	// deemed to be a crackle and will be replaced with silence.
	// Any sound below the threshold will be deemed to be
	// background noise and will be replaced with silence but
	// only if it lasts for more than 8000 frames as we don't
	// want to replace short snippets of background noise, e.g.
	// inbetween spoken words. 
	for (int i = 0; i < framesPerBuffer; i++){
		*dest++ = *src;
		if (*src < -(data->recordingLevel) || *src > data->recordingLevel)
		{
			if (data->soundStart == -1){
				data->soundStart = data->frameIndex;
				data->soundCount = 0;
				data->loudSoundCount = 0;
			}
			data->lastSound = data->frameIndex;
			data->soundCount++;
			if (*src < -10000 || *src > 10000)
				data->loudSoundCount++;
		}

		if (data->soundStart == -1){
			if (data->soundTriggered && (data->frameIndex - data->silenceStart)
						>= data->silenceWanted)
			{
				data->frameIndex = data->silenceStart;
				return paComplete;
			}
		}
		else if (data->frameIndex - data->soundStart > 500){
			// Is this the start of a new sound?
			if (data->soundStart > data->silenceStart){
				if (data->soundCount > 300 && data->loudSoundCount > 100){
					// Found genuine sound
					if (data->soundStart - data->silenceStart > 8000){
						// Found silence before this sound
						if (data->silenceStart == 1){
							// Remove silence from start
							int frames = data->frameIndex - data->soundStart;
							int byteLen = frames * sizeof(short);
							memcpy(&data->samples[0],
								&data->samples[data->soundStart], byteLen);
							dest = &data->samples[frames];
							data->frameIndex = frames - 1;
							data->soundStart = 0;
							data->lastSound = data->frameIndex;
						}
						else {
							// Remove background noise
							/*
							int frames = data->soundStart - data->silenceStart;
							int byteLen = frames * sizeof(short);
							memset(&data->samples[data->silenceStart],
								0, byteLen);
							*/
						}
					}
					data->silenceStart = data->lastSound + 1;
					data->soundTriggered = true;
				}
				else {
					// Ignore noise
					data->soundStart = -1;
				}
			}
			else if (data->frameIndex - data->lastSound > 500){
				// Sound has ended. Include the last 500 frames
				// so we don't chop the sound off.
				data->silenceStart = data->frameIndex;
				data->soundStart = -1;
			}
		}

		src++;
		data->frameIndex++;

		if (data->soundWanted == 0 || !data->soundTriggered){
			// Listening mode or not triggered yet so if
			// buffer is full just start again.
			if (data->frameIndex >= data->maxFrames){
				data->frameIndex = 1;
				data->samples[0] = 0;
				data->silenceStart = 1;
				data->soundStart = -1;
				data->lastSound = -1;
				data->soundCount = 0;
				data->loudSoundCount = 0;
				dest = &data->samples[1];
			}
		}
		else if (data->frameIndex >= data->soundWanted){
			// Final sound length reached
			return paComplete;
		}
	}

	return paContinue;
}


/**
 *
 */
Record::Record()
{
	mInit = false;
	mStream = NULL;
	mSaving = false;
	mConverted = false;

	// Default recording level is 8
	setLevel(8000);

	PaError err;
	PaStreamParameters inputParams;

	// Crappy ALSA dumps errors to stderr so let's sink them
	int fd = dup(STDERR_FILENO);
	freopen("/dev/null", "w", stderr);
	err = Pa_Initialize();
	if (err != paNoError) {
		// Stop sinking stderr
		fclose(stderr);
		stderr = fdopen(fd, "w");
		fprintf(stderr, "Unable to initialise PortAudio: %s\n",
				Pa_GetErrorText(err));
		return;
	}

	inputParams.device = Pa_GetDefaultInputDevice();
	if (inputParams.device == paNoDevice){
		// Stop sinking stderr
		fclose(stderr);
		stderr = fdopen(fd, "w");
		fprintf(stderr, "No recording device found. Make sure you've connected a USB microphone or USB webcam with built-in mic.\n");
		Pa_Terminate();
		return;
	}

	inputParams.channelCount = 1;
	inputParams.sampleFormat = SAMPLE_TYPE;
	inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
	inputParams.hostApiSpecificStreamInfo = NULL;

	mRecordData.actualRate = Sample::INTERNAL_RATE;
	// Use 50ms buffer
	int framesPerBuffer = mRecordData.actualRate / 20;
    err = Pa_OpenStream(
              &mStream,
              &inputParams,
              NULL,
              mRecordData.actualRate,
              framesPerBuffer,
              paClipOff,
              recordCallback,
              &mRecordData);

	// If recording device doesn't like our preferred sample
	// rate use the alternative one.
	if (err == paInvalidSampleRate){
		mRecordData.actualRate = ALTERNATE_RATE;
		// Use 50ms buffer
		int framesPerBuffer = mRecordData.actualRate / 20;
	    err = Pa_OpenStream(
              	&mStream,
              	&inputParams,
              	NULL,
              	mRecordData.actualRate,
              	framesPerBuffer,
              	paClipOff,
              	recordCallback,
              	&mRecordData);
	}

	// Stop sinking stderr
	fclose(stderr);
	stderr = fdopen(fd, "w");

	if (err != paNoError) {
		fprintf(stderr, "Unable to open PortAudio recording stream: %s\n",
				Pa_GetErrorText(err));
		Pa_Terminate();
		return;
	}

	// Initialise the data buffer so we can store up
	// to 100 seconds of recorded audio in stereo.
	mRecordData.maxFrames = MAX_DURATION * mRecordData.actualRate;
	int maxBytes = mRecordData.maxFrames * sizeof(SAMPLE) * 2;
	mRecordData.samples = (SAMPLE*)malloc(maxBytes);
	if (!mRecordData.samples){
		fprintf(stderr, "Failed to allocate memory for record buffer\n");
		Pa_Terminate();
		return;
	}

	mInit = true;
}


/**
 *
 */
Record::~Record()
{
	if (mInit){
		Pa_CloseStream(mStream);
		free(mRecordData.samples);
		Pa_Terminate();
	}
}


/**
 *
 */
bool Record::setLevel(int level)
{
	// Level is in thousandths. Default is 5 and max is 100.
	// Convert to value between 0 and 10,000.
	mRecordData.recordingLevel = (level * 3) / 10;
}


/**
 * Start audio input. Only one stream can be active so
 * if there is already one running we need to stop it.
 * Every time the frame buffer fills the callback will
 * be called.
 */
bool Record::startStream(int soundWanted, int silenceWanted)
{
	if (!mInit){
		fprintf(stderr, "PortAudio not initialised\n");
		return false;
	}

	if (mSaving){
		fprintf(stderr, "Cannot start a recording when save_recording is still in progress");
		return false;
	}

	stopStream();

	// SDL_Mixer doesn't play nicely with portaudio so we
	// have to disable it temporarily during a recording.
	Audio::disable();

	// We are either doing a fixed length recording or a
	// recording that starts when there is no silence and
	// stops when there is silence.
	mRecordData.soundTriggered = false;
	if (soundWanted == -1)
		mRecordData.soundWanted = mRecordData.maxFrames;
	else
		mRecordData.soundWanted =
					(soundWanted * (mRecordData.actualRate / 100)) / 10;

	if (silenceWanted == -1){
		// Want recordeding to start immediately and don't
		// want recording to stop when silence is heard.
		mRecordData.silenceWanted = mRecordData.maxFrames;
		mRecordData.soundTriggered = true;
	}
	else
		mRecordData.silenceWanted =
					(silenceWanted * (mRecordData.actualRate / 100)) / 10;

	mConverted = false;

	// Start recording
	mRecordData.frameIndex = 0;
	PaError err = Pa_StartStream(mStream);
	if (err != paNoError){
		fprintf(stderr, "** Failed to start PortAudio recording: %s\n",
				Pa_GetErrorText(err));
		return false;
	}

	// Wait for first callback to start
	while (mRecordData.frameIndex == 0)
		Engine::sleep(20);

	return true;
}


/**
 * Stop audio input if it is currently active.
 */
bool Record::stopStream()
{
	if (Pa_IsStreamStopped(mStream) == 0)
		Pa_AbortStream(mStream);

	// Re-enable SDL_Mixer
	Audio::enable();
	return true;
}


/**
 * Save the current recording to a file. It will also
 * replace the audio buffer of a loaded sound if one
 * is passed in.
 */
bool Record::saveWAV(const char *filename, Audio *audio)
{
	if (!mInit){
		fprintf(stderr, "PortAudio not initialised\n");
		return false;
	}

	if (mSaving){
		fprintf(stderr, "Cannot save a recording when save_recording is already in progress");
		return false;
	}

	mSaving = true;
	stopStream();
	if (!convertData())
		return false;

	static const int headerSize = 44;
	char header[headerSize];
	int dataSize = mRecordData.frameIndex * sizeof(SAMPLE) * 2;

	memcpy(&header[0], "RIFF", 4);
	int val = headerSize + dataSize - 8;
	memcpy(&header[4], &val, 4);
	memcpy(&header[8], "WAVE", 4);
	memcpy(&header[12], "fmt ", 4);
	val = 16;
	memcpy(&header[16], &val, 4);
	short sval = 1;		// Type = PCM
	memcpy(&header[20], &sval, 2);
	sval = 2;			// Stereo (2 channels)
	memcpy(&header[22], &sval, 2);
	val = Sample::INTERNAL_RATE;		// Sample rate
	memcpy(&header[24], &val, 4);
	val = Sample::INTERNAL_RATE * 4;	// Sample rate * 2 (16-bit) * channels
	memcpy(&header[28], &val, 4);
	sval = 4;			// 2 (16-bit) * channels
	memcpy(&header[32], &sval, 2);
	sval = 16;			// Bits per sample
	memcpy(&header[34], &sval, 2);
	memcpy(&header[36], "data", 4);
	memcpy(&header[40], &dataSize, 4);

	FILE *outf = fopen(filename, "wb");
	if (outf){
		fwrite(header, 1, 44, outf);
		fwrite(mRecordData.samples, 1, dataSize, outf);
		fclose(outf);
	}
	else
		fprintf(stderr, "Failed to write WAV file %s\n", filename);

	if (audio){
		if (!audio->replaceWAV(mRecordData.samples, dataSize)){
			mSaving = false;
			return false;
		}
	}

	mSaving = false;
	return true;
}


/*
 *
 */
bool Record::playWAV(Audio *audio, double now)
{
	if (!mInit){
		fprintf(stderr, "PortAudio not initialised\n");
		return false;
	}

	if (mSaving){
		fprintf(stderr, "Cannot play a recording when save_recording is in progress");
		return false;
	}

	stopStream();
	if (!convertData())
		return false;

	int dataSize = mRecordData.frameIndex * sizeof(SAMPLE) * 2;
	if (!audio->replaceWAV(mRecordData.samples, dataSize))
		return false;

	return audio->play(0, now);
}


/*
 * If the data is not in the required format we convert it here.
 * It converts mono to stereo and also converts to our internal
 * sample rate. The frame index will be adjusted if the sample
 * rate is changed.
 */
bool Record::convertData()
{
	if (mConverted)
		return true;

	mConverted = true;

	// Add one frame of silence if buffer is empty
	if (mRecordData.frameIndex == 0){
		mRecordData.samples[0] = 0;
		mRecordData.frameIndex = 1;
	}

	// Convert from mono to stereo and also adjust
	// the sample rate if it is wrong.
	short *outData = Sample::convert(
			mRecordData.samples, 1, mRecordData.frameIndex,
			mRecordData.actualRate);

	if (!outData)
		return false;

	// Copy outData and then free it
	int dataLen = mRecordData.frameIndex * sizeof(short) * 2;
	memcpy(mRecordData.samples, outData, dataLen);
	free(outData);
	return true;
}
