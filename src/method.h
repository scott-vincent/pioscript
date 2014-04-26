/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * A Method is the definition of an activity, i.e.
 * an activity runs a method. An activity can only
 * run one method but the same method may be run by
 * multiple activities.
 *
 * A method stores the list of commands in the script
 * from the method declaration (starts with :) to the
 * start of the next method declaration (or end of file).
 */

#ifndef __METHOD_H__
#define __METHOD_H__

#include <list>
#include "command.h"


class Method {

public:
	std::list<Command*> commands;

private:
	char mName[256];
	int mLineNum;

public:
	Method(const char *name, int lineNum);
	const char *getName() { return mName; };
	int getLineNum() { return mLineNum; };
};

#endif
