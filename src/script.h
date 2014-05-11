/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * A Script is an internal representation of the script
 * after it has been parsed. It stores all the objects
 * required to run the script including commands, methods,
 * sound resources and gpio resources.
 */

#ifndef __SCRIPT_H__
#define __SCRIPT_H__

#include <string>
#include <map>
#include "method.h"
#include "audio.h"
#include "gpio.h"

class Script {
private:
	char mFilename[256];
	Method *mCurrentMethod;
	Method *mMain;
	std::map<std::string, Method*> mMethods;
    std::map<std::string, Audio*> mSounds;
    std::map<int, Gpio*> mPins;
    std::list<Command*> nestedCommands;
	bool mUsesRecording;

public:
	Script(char *filename);
	~Script();
	bool load();
	Method* getMain() { return mMain; };
	bool usesRecording() { return mUsesRecording; };
	void report();

private:
	bool defineMethod(const char *name, int lineNum);
	bool orphanNestingCheck();
	Method* addMethod(const char *name, int lineNum);
	Audio* addAudio(const char *name, int lineNum);
	Gpio* addGpio(int pin, Gpio::Type type);
	bool addFuncObjects(Command *command, int paramNum);
	bool addCommand(char *name, char *params, int lineNum, char *lineStr);
};

#endif
