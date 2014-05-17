/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include "parameter.h"
#include "variable.h"


/**
 * Constructor
 */
Parameter::Parameter()
{
	mIsVariable = false;
	mValue = 0;
}


/**
 *
 */
void Parameter::setVariable(char *name)
{
	mIsVariable = true;

	if (*name == '$')
		strcpy(mName, name+1);
	else
		strcpy(mName, name);
}


/**
 *
 */
bool Parameter::getVariable()
{
	if (!mIsVariable)
		return false;

	bool success = Variable::get(mName, NULL, mValue);
	if (!success){
		fprintf(stderr, "Variable %s is undefined.\n",
				Variable::errName);
	}
	return success;
}


/**
 * Get the string value after evaluating any variables
 */
void Parameter::getFullStrValue(char *strValue)
{
	const char *srcPos = mStrValue.c_str();
	char *destPos = strValue;
	const char *endPos;
	int len;

	while (true){
		while (*srcPos == ' ')
			*srcPos++;

		if (*srcPos == '\0'){
			*destPos = '\0';
			return;
		}

		// Is it a literal?
		if (*srcPos == '"'){
			srcPos++;

			// If no matching quote just take whole string
			endPos = strchr(srcPos, '"');
			if (!endPos){
				strcpy(destPos, srcPos);
				return;
			}

			len = endPos - srcPos;
			strncpy(destPos, srcPos, len);
			destPos += len;
			srcPos = endPos + 1;
			continue;
		}

		// Found a variable
		char varName[256];
		endPos = srcPos;
		int subCount = 0;
		while (*endPos != '\0'){
			if (*endPos == ' ' && subCount == 0)
				break;

			if (*endPos == '[')
				subCount++;
			else if (*endPos == ']' && subCount > 0)
				subCount--;

			endPos++;
		}

		int len = endPos - srcPos;
		strncpy(varName, srcPos, len);
		varName[len] = '\0';
		srcPos = endPos;

		// Is it a single variable?
		char strVal[256];
		if (Variable::get(varName, NULL, strVal)){
			strcpy(destPos, strVal);
			destPos += strlen(strVal);
			continue;
		}

		// Is it a list variable?
		int index = 0;
		char nextVarName[256];
		sprintf(nextVarName, "%s[%d]", varName, index);
		while (Variable::get(nextVarName, NULL, strVal)){
			if (index > 0){
				*destPos = ',';
				destPos++;
			}

			strcpy(destPos, strVal);
			destPos += strlen(strVal);
			index++;
			sprintf(nextVarName, "%s[%d]", varName, index);
		}

		if (index > 0)
			continue;

		// Variable not defined
		strcpy(destPos, varName);
		destPos += strlen(varName);
		strcpy(destPos, "{undefined variable} ");
		destPos += 21;
	}
}
