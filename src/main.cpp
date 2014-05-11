/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <strings.h>
#include "pioscript.h"
#include "command.h"


/**
 *
 */
int main(int argc, char *argv[])
{
	if (argc < 2 || strcmp(argv[1], "/?") == 0 || strcmp(argv[1], "-?") == 0){
		printf("Usage: pioscript <script_file>\n");
		printf("Type 'pioscript -help' for help when using the Pibrella\n");
		printf("Type 'pioscript -morehelp' for advanced help\n");
		return 1;
	}

	if (strcasecmp(argv[1], "-help") == 0){
		Command::reportPibrella();
		return 1;
	}
	else if (strcasecmp(argv[1], "-morehelp") == 0){
		Command::reportAdvanced();
		return 1;
	}

	// Check we are root
	if (geteuid() != 0){
		printf("Please run this program as root\n");
		return 1;
	}

	Pioscript pioscript;
	if (!pioscript.run(argv[1]))
		return 1;

	return 0;
}
