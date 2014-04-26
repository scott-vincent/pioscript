/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "method.h"
#include "command.h"


/**
 *
 */
Method::Method(const char *name, int lineNum)
{
	strcpy(mName, name);
	mLineNum = lineNum;
}
