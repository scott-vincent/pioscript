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
#include <dirent.h>
#include <sys/types.h>
#include <sys/time.h>
#include "engine.h"
#include "method.h"
#include "gpio.h"

// Global sync timer
double Engine::syncTimer = 0;
bool Engine::displayOn = true;
Record *Engine::recorder = NULL;


/**
 *
 */
Engine::Engine()
{
}


/**
 *
 */
Engine::~Engine()
{
	while (true){
		auto iter = activities.begin();
		if (iter == activities.end())
			break;

		Activity *activity = *iter;
		delete activity;
		activities.erase(iter);
	}

	if (recorder)
		delete recorder;
}


/**
 *
 */
bool Engine::newActivity(Method *method, Activity *waitingActivity)
{
	if (activities.size() == MAX_ACTIVITIES){
		fprintf(stderr, "** Maximum number of activities exceeded (%d)\n",
			 MAX_ACTIVITIES);
		return false;
	}

	Activity *activity = new Activity(method);
	activity->setWaiting(waitingActivity);
	activities.push_back(activity);

	if (displayOn){
		printf("Start activity %s (%d active)\n",
			 activity->getName(), activities.size());
	}

	return true;
}


/**
 *
 */
bool Engine::execute(Method *main, bool usesRecording)
{
	if (usesRecording){
		recorder = new Record();
		if (!recorder->initialised())
			return false;
	}

	if (!Gpio::startMonitor())
		return false;

	newActivity(main, NULL);

	while (activities.size() > 0){
		// Get current time in millisecs
		double now = timeNow();

		// Advance each activity (execute one command)
		bool idle = true;
		auto iter = activities.begin();
		while (iter != activities.end()){
			Activity *activity = *iter;

			// Has this activity been ended by another activity?
			if (activity->isFinished()){
				// If another activity is waiting for this
				// one to complete then signal it.
				Activity *waitingActivity = activity->getWaiting();
				if (waitingActivity)
					waitingActivity->signal();

				iter = activities.erase(iter);
				if (displayOn){
					printf("End activity %s (%d active)\n",
						 activity->getName(), activities.size());
				}
				delete activity;
				continue;
			}

			// Are we ready to execute the next command?
			if (!activity->isReady(now)){
				// Advance to next activity
				iter++;
				continue;
			}

			idle = false;

			// Execute the next command
			if (!activity->advance(now)){
				// Activity either exited or failed
				Gpio::stopMonitor();
				return (activity->isExit());
			}

			// Should we start a new activity?
			Method *method = activity->startNew();
			if (method){
				// Is the current activity waiting
				// for the new activity to complete?
				Activity *waitingActivity = NULL;
				if (activity->isWaiting())
					waitingActivity = activity;

				if (!newActivity(method, waitingActivity)){
					Gpio::stopMonitor();
					return false;
				}
			}

			// Should we stop another activity?
			method = activity->stopOther();
			if (method){
				// Search activities in reverse order so, if there are
				// multiple, last one started will be first one stopped.
				for (auto it = activities.end(); it != activities.begin();)
				{
					it--;
					Activity *act = *it;
					if (act->getMethod() == method){
						act->setFinished();
						break;
					}
				}
				activity->clearStop();
			}

			// Should we end this activity?
			if (activity->isFinished()){
				// If another activity is waiting for this
				// one to complete then signal it.
				Activity *waitingActivity = activity->getWaiting();
				if (waitingActivity)
					waitingActivity->signal();

				// Delete activity and advance to next one
				iter = activities.erase(iter);
				if (displayOn){
					printf("End activity %s (%d active)\n",
						 activity->getName(), activities.size());
				}
				delete activity;
			}
			else {
				// Make sure activity is not too active!
				if (activity->isLooping()){
					fprintf(stderr, "Activity %s is consuming too much CPU. Please add a wait command.\n", activity->getName());
					fprintf(stderr, "** Script stopped\n");
					Gpio::stopMonitor();
					return false;
				}

				// Advance to next activity
				iter++;
			}
		}

		// If there is no activity we sleep for 10ms
		if (idle){
			Engine::sleep(10);
		}
	}

	Gpio::stopMonitor();
	return true;
}


/**
 *
 */
void Engine::sleep(int millisecs)
{
    struct timespec ts;

    ts.tv_sec = millisecs / 1000;
    ts.tv_nsec = (millisecs % 1000) * 1000000.0;
    nanosleep(&ts, NULL);
}


/**
 *
 */
double Engine::timeNow()
{
	// Get current time in millisecs
	timeval tv;
	gettimeofday(&tv, NULL);
	double now = (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
	return now;
}


/**
 *
 */
pid_t Engine::findProcess(const char *name)
{
	DIR* dir;
	if (!(dir = opendir("/proc"))){
		fprintf(stderr, "Can't read /proc\n");
		return -1;
	}

	dirent* ent;
	while((ent = readdir(dir)) != NULL){
		char* endptr;
		long pid = strtol(ent->d_name, &endptr, 10);
		if (*endptr != '\0')
			continue;

		char buf[512];
		sprintf(buf, "/proc/%ld/cmdline", pid);
		FILE* fp = fopen(buf, "r");
		if (fp){
			if (fgets(buf, sizeof(buf), fp) != NULL){
				char *cmd = strtok(buf, " ");
				char *sep = strrchr(cmd, '/');
				if (sep)
					cmd = sep + 1;
				if (strcmp(cmd, name) == 0) {
					fclose(fp);
					closedir(dir);
					return pid;
				}
			}
			fclose(fp);
		}
	}
	closedir(dir);
	return -1;
}
