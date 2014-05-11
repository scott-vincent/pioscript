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
bool Pioscript::run(char *scriptfile)
{
	if (!init()){
		fprintf(stderr, "PioScript init failed\n");
		cleanup();
		return false;
	}

	printf("\n");
	printf("Checking script %s\n", scriptfile);
	Script script(scriptfile);
	if (!script.load()){
		fprintf(stderr, "** Failed to load script\n");
		cleanup();
		return false;
	}
	if (Gpio::usingPibrella())
		printf("No errors\n");
	else
		script.report();

	printf("\n");
	printf("---------------------------------------\n");
	printf("  Executing script %s\n", scriptfile);
	printf("---------------------------------------\n");

	Engine engine;
	if (Gpio::usingPibrella())
		engine.displayOff();

	if (!engine.execute(script.getMain(), script.usesRecording())){
		fprintf(stderr, "** Failed to execute script\n");
		cleanup();
		return false;
	}

	printf("\n");
	printf("---------------------------------------\n");
	printf("  Completed script %s\n", scriptfile);
	printf("---------------------------------------\n");
	cleanup();
	return true;
}
