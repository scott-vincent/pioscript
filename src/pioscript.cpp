/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pioscript.h"
#include "script.h"
#include "engine.h"
#include "gpio.h"


/**
 *
 */
bool Pioscript::init()
{
	srand (time(NULL));

	if (!Gpio::init()) {
		fprintf(stderr, "Failed to initialise gpio\n");
		return false;
	}

	// Audio is initialised on demand (only when it's first used)

	return true;
}


/**
 *
 */
void Pioscript::cleanup()
{
	Gpio::cleanup();
	Audio::cleanup();
}


/**
 *
 */
void Pioscript::run(char *scriptfile)
{
	if (!init()){
		fprintf(stderr, "PioScript init failed\n");
		cleanup();
		return;
	}

	printf("\n");
	printf("---------------------------------------\n");
	printf("  Parsing script %s\n", scriptfile);
	printf("---------------------------------------\n");

	Script script(scriptfile);
	if (!script.load()){
		fprintf(stderr, "** Failed to load script\n");
		cleanup();
		return;
	}
	script.report();

	printf("\n");
	printf("---------------------------------------\n");
	printf("  Executing script %s\n", scriptfile);
	printf("---------------------------------------\n");

	Engine engine;
	if (!engine.execute(script.getMain())){
		fprintf(stderr, "** Failed to execute script\n");
		cleanup();
		return;
	}

	printf("\n");
	printf("---------------------------------------\n");
	printf("  Completed script %s\n", scriptfile);
	printf("---------------------------------------\n");
	cleanup();
}
