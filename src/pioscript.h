/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * This class intialises all the sub-systems (audio and gpio),
 * loads and parses the script and then creates an engine to
 * execute the script.
 */

#ifndef __PIOSCRIPT_H__
#define __PIOSCRIPT_H__

class Pioscript {

public:
	bool init();
	void cleanup();
	bool run(char *scriptfile);
};

#endif
