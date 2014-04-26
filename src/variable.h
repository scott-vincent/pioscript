/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * A Variable object is the internal representation
 * of a variable used in a script.
 */

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include <string>
#include <map>
#include "command.h"


class Variable {
public:
	static char errName[256];

private:
	char mName[256];
	int mValue;
	static std::map<std::string, Variable*> mVariables;

public:
	static void cleanup();

	static bool set(std::string name, std::string expression, void **funcObjects);
	static bool simpleSet(const char *name, int value);
	static bool get(char *name, void **funcObjects, int &value);
	static bool get(char *name, void **funcObjects, char *strValue);
	static bool getFullName(const char *name, void **funcObjects, char *fullName);
	static bool evaluate(std::string expression, void **funcObjects, int &value);

private:
	Variable(const char *name, int value);
	void setValue(int value) { mValue = value; };
	int getValue() { return mValue; };
	static bool setOne(std::string fullName, std::string expression, void **funcObjects);
	static const char *getToken(const char *pos, void **funcObjects, bool expectingOper, char &op, int &value);
	static char *numToStr(int value);
};
#endif
