/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * A single instance of the Engine is created to execute
 * the script. It creates and destroys activities as required
 * and execution finishes when there are no more activities
 * to execute.
 * 
 * Rather than running activities in concurrent threads it
 * uses a time-slicing approach and executes one command from
 * each activity in turn. This allows for better control of
 * cpu consumption so that only a small amount is consumed by
 * the execution management and the bulk is free to be used
 * by the important resources such as audio and gpio.
 */

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <list>
#include "method.h"
#include "activity.h"


class Engine {

private:
	static const int MAX_ACTIVITIES = 500;
	std::list<Activity*> activities;

public:
	static double syncTimer;
	static bool displayOn;

	Engine();
	~Engine();
	bool execute(Method *main);
	static void sleep(int millisecs);
	static double timeNow();
	static pid_t findProcess(const char *name);

private:
	bool newActivity(Method *method, Activity *waitingActivity);
};

#endif
