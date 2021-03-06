/****************************************
 * Written by Scott Vincent
 * 15th May 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * This class is used to convert an audio sample
 * from one sample rate to another. It is needed
 * by the recorder when the input device does not
 * support the rate we require.
 */

#ifndef __SAMPLE_H__
#define __SAMPLE_H__


class Sample {
public:
	static const int INTERNAL_RATE = 44100;

	static short *convert(short *data, int channels, int &frameCount, int sampleRate);
	static short *convert(void *data, int &dataLen, int bits);
};

#endif
