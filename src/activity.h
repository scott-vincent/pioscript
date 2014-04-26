/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * An Activity is an actively running Method.
 * A single activity that runs the :main method
 * is created when the engine starts. Other
 * activities are created when a Start command
 * is executed. An activity is destroyed when
 * the method reaches the end.
 */

#ifndef __ACTIVITY_H__
#define __ACTIVITY_H__

#include <time.h>
#include <list>
#include "method.h"
#include "command.h"


class Activity {

private:
	Method* mMethod;
	std::list<Command*>::iterator mIter;
	double mNextAdvance;
	Method *mNewActivity;
	Method *mStopActivity;
	bool mFinished;
	bool mExit;
	bool mWaiting;
	Activity *mWaitingActivity;
	bool mSignalled;
	int mActive;

public:
	Activity(Method *method);
	const char *getName() { return mMethod->getName(); };
	Method* getMethod() { return mMethod; };
	Method* startNew() { return mNewActivity; };
	Method* stopOther() { return mStopActivity; };
	void clearStop() { mStopActivity = NULL; };
	bool isFinished() { return mFinished; };
	void setFinished() { mFinished = true; };
	bool isExit() { return mExit; };
	bool isWaiting() { return mWaiting; };
	Activity* getWaiting() { return mWaitingActivity; };
	bool setWaiting(Activity* activity) { mWaitingActivity = activity; };
	bool signal() { mSignalled = true; };
	bool isReady(double now);
	bool isLooping() { return (mActive > 9999); };
	bool advance(double now);
	bool exec(Command *command, double now);
	void jumpForwards(int lineNum);
	void jumpBackwards(int lineNum);
	bool wait(Command *command, double now);

private:
	bool playNextNote(Command *command, double now);
	void playNote(Command *command, double now);
};

#endif
