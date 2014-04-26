/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * A parameter object is the internal representation
 * of a parameter used in a script. This can be either
 * a constant or a variable.
 */

#ifndef __PARAMETER_H__
#define __PARAMETER_H__

#include <string.h>
#include <string>


class Parameter {

private:
	bool mIsVariable;
	int mValue;
	std::string mStrValue;
	char mName[256];

public:
	Parameter();
	bool isVariable() { return mIsVariable; };
	void setValue(int value) { mValue = value; };
	int getValue() { return mValue; };
	void setVariable(char *name);
	bool getVariable();
	void setStrValue(const char *strValue) { mStrValue = strValue; };
	std::string getStrValue() { return mStrValue; };
	void getFullStrValue(char *strValue);
};
#endif
