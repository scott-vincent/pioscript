/****************************************
 * Written by Scott Vincent
 * 15th May 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <samplerate.h>
#include "sample.h"


/*
 * This method converts from any sample rate in mono or stereo
 * to our internal format which is 44.1 KHz stereo. Only 16-bit
 * sound is supported.
 *
 * If the sound is already in 44.1 KHz stereo then the method
 * just returns otherwise it replaces the input buffer with the
 * new sample. You must make sure that the input buffer is big
 * enough to hold the converted sample.
 *
 * @param data			Input/output buffer
 * @param channels		Number of input channels
 * @param frameCount	Number of input frames
 * @param sampleRate	Input sample rate
 * @return				The new frame count or -1 if error
 */
short *Sample::convert(short *data, int channels, int &frameCount, int sampleRate)
{
	if (channels != 1 && channels != 2){
		fprintf(stderr, "Only mono or stereo sound samples are currently supported. Sound sample contains %d channels.\n", channels);
		return NULL;
	}

	if (sampleRate != INTERNAL_RATE){
		// Convert to internal rate (44.1 KHz)

		// Allocate float input buffer
		int inBytes = frameCount * channels * sizeof(float);
		float *inBuffer = (float *)malloc(inBytes);
		if (!inBuffer){
			fprintf(stderr, "Failed to allocate memory when converting sound sample\n");
			return NULL;
		}

		// Allocate float output buffer
		double srcRatio = (double)INTERNAL_RATE / (double)sampleRate;
		int outFrameCount = (int)((double)frameCount * srcRatio);
		int outBytes = outFrameCount * channels * sizeof(float);
		float *outBuffer = (float *)malloc(outBytes);
		if (!outBuffer){
			fprintf(stderr, "Failed to allocate memory when converting sound sample\n");
			free(inBuffer);
			return NULL;
		}

		// Convert input from 16-bit (-32768 to 32767) to float (-1.0 to 1.0)
		short *pShort = data;
		float *pFloat = inBuffer;
		for (int i = 0; i < frameCount * channels; i++){
			*pFloat++ = (float)(*pShort++) / 32768;
		}

		// Convert sample rate
		SRC_DATA srcData;
		srcData.data_in = inBuffer;
		srcData.data_out = outBuffer;
		srcData.input_frames = frameCount;
		srcData.output_frames = outFrameCount;
		srcData.src_ratio = srcRatio;
		int err = src_simple(&srcData, SRC_SINC_FASTEST, channels);
		free(inBuffer);
		if (err != 0){
			fprintf(stderr, "Failed to convert sound sample. Error: %s\n",
					src_strerror(err));
			free(outBuffer);
			return NULL;
		}
		
		// Find clipped peak value (> 1.0)
		float peak = 1.0f;
		pFloat = outBuffer;
		for (int i = 0; i < outFrameCount * channels; i++){
			if (*pFloat > peak)
				peak = *pFloat;
			else if (*pFloat < -peak)
				peak = -(*pFloat);

			pFloat++;
		}
		
		// Convert output back to 16-bit and normalise if clipped.
		// Also convert from mono to stereo if required.
		outBytes = outFrameCount * sizeof(short) * 2;
		short *outData = (short *)malloc(outBytes);
		if (!outData){
			fprintf(stderr, "Failed to allocate memory when converting sound sample\n");
			free(outBuffer);
			return NULL;
		}

		float normaliser = 32767.0f / peak;
		pShort = outData;
		pFloat = outBuffer;
		for (int i = 0; i < outFrameCount * channels; i++){
			short val = (short)(*pFloat++ * normaliser);
			*pShort++ = val;
			if (channels == 1){
				// Convert to stereo
				*pShort++ = val;
			}
		}
		free(outBuffer);

		frameCount = outFrameCount;
		return outData;
	}

	if (channels == 1){
		// Convert from mono to stereo
		int outBytes = frameCount * sizeof(short) * 2;
		short *outData = (short *)malloc(outBytes);
		if (!outData){
			fprintf(stderr, "Failed to allocate memory when converting sound sample\n");
			return NULL;
		}

		short *src = data;	
		short *dest = outData;
		for (int i = 0; i < frameCount; i++){
			*dest++ = *src;
			*dest++ = *src++;
		}
		return outData;
	}

	return NULL;
}


/*
 * This method converts a sound from 8-bit or 32-bit to
 * 16-bit sound. The output buffer is malloc'ed and
 * returned. The output has the same number of channels
 * as the input.
 *
 * On success, the malloc'ed buffer is returned and
 * dataLen is updated. On error, NULL is returned.
 */
short *Sample::convert(void *data, int &dataLen, int bits)
{
	// Note that in a WAV file 8-bit data is unsigned but
	// 16-bit is signed!
	if (bits != 8 && bits != 32){
		fprintf(stderr, "Only 8-bit, 16-bit or 32-bit sound is supported. This sound is %d-bit.\n", bits);
		return NULL;
	}

	int sampleSize = bits / 8;
	int sampleCount = dataLen / sampleSize;
	int outLen = sampleCount * sizeof(short);
	short *outData = (short*)malloc(outLen);
	if (!outData){
		fprintf(stderr, "Failed to allocate memory when converting sound sample\n");
		return NULL;
	}

	short *dest = outData;
	if (bits == 8){
		uint8_t *src = (uint8_t*)data;
		for (int i = 0; i < sampleCount; i++){
			*dest++ = ((short)*src++ - 128) << 8;
		}
	}
	else if (bits == 32){
		int *src = (int*)data;
		for (int i = 0; i < sampleCount; i++){
			*dest++ = (short)(*src++ >> 16);
		}
	}

	dataLen = outLen;
	return outData;
}
