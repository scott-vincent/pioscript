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
		fprintf(stderr, "Variable %s is undefined. For a list use $var[i].\n",
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

	while (true){
		const char *startPos = strchr(srcPos, '$');
		if (!startPos){
			strcpy(destPos, srcPos);
			return;
		}

		if (startPos > srcPos){
			int len = startPos - srcPos;
			strncpy(destPos, srcPos, len);
			srcPos = startPos;
			destPos += len;
		}

		startPos++;
		const char *endPos;
		char varName[256];
		if (*startPos == '{'){
			startPos++;
			endPos = strchr(startPos, '}');
			if (!endPos){
				strcpy(destPos, srcPos);
				return;
			}
			int len = endPos - startPos;
			strncpy(varName, startPos, len);
			varName[len] = '\0';
			endPos++;
		}
		else {
			endPos = startPos;
			bool inSub = false;
			while (*endPos != '\0'){
				if (*endPos == '[')
					inSub = true;

				if (!inSub && !isalnum(*endPos))
					break;

				if (*endPos == ']')
					inSub = false;

				endPos++;
			}

			int len = endPos - startPos;
			endPos = startPos + len;

			strncpy(varName, startPos, len);
			varName[len] = '\0';
		}

		if (*varName != '\0'){
			char strVal[256];

			// Is it a single variable?
			if (Variable::get(varName, NULL, strVal)){
				strcpy(destPos, strVal);
				destPos += strlen(strVal);
				srcPos = endPos;
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

			if (index > 0){
				srcPos = endPos;
				continue;
			}
		}

		// Variable not defined
		int len = endPos - srcPos;
		strncpy(destPos, srcPos, len);
		destPos += len;
		srcPos = endPos;
	}
}
