/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "variable.h"
#include "command.h"
#include "engine.h"
#include "gpio.h"

std::map<std::string, Variable*> Variable::mVariables;
char Variable::errName[256];


/**
 * Global cleanup
 */
void Variable::cleanup()
{
    while (true){
        auto iter = mVariables.begin();
        if (iter == mVariables.end())
            break;

        Variable *var = iter->second;
        delete var;
        mVariables.erase(iter);
    }
}


/**
 * Constructor
 */
Variable::Variable(const char *name, int value)
{
	strcpy(mName, name);
	mValue = value;
}


/**
 * If expression contains commas then we set an array
 * of variables otherwise we just set a single variable.
 */
bool Variable::set(std::string name, std::string expression, void **funcObjects)
{
	char fullName[256];
	if (!getFullName(name.c_str(), funcObjects, fullName))
		return false;

	const char *pos = strchr(expression.c_str(), ',');
	if (!pos)
		return setOne(fullName, expression, funcObjects);

	// We create an array by appending subscripts to the variable name
	int index = 0;
	pos = expression.c_str();
	while (*pos != '\0'){
		while (*pos == ' ')
			pos++;

		if (*pos == ',' || *pos == '\0'){
			fprintf(stderr, "List has missing value\n");
			return false;
		}

		char varName[256];
		sprintf(varName, "%s[%d]", fullName, index);

		char varExpression[256];
		const char *endPos = strchr(pos, ',');
		if (endPos){
			int len = endPos - pos;
			strncpy(varExpression, pos, len);
			varExpression[len] = '\0';
			pos = endPos + 1;
		}
		else {
			strcpy(varExpression, pos);
			pos += strlen(pos);
		}

		if (!setOne(varName, varExpression, funcObjects))
			return false;

		index++;
	}
}


/**
 *
 */
bool Variable::simpleSet(const char *name, int value)
{
	// Create/update the variable
	auto iter = mVariables.find(name);
	if (iter == mVariables.end()){
		Variable *variable = new Variable(name, value);
		mVariables.insert(std::make_pair(name, variable));
	}
	else {
		Variable *variable = iter->second;
		variable->setValue(value);
	}

	return true;
}


/**
 *
 */
bool Variable::setOne(std::string fullName, std::string expression, void **funcObjects)
{
	int value;
	if (!evaluate(expression, funcObjects, value))
		return false;

	// Create/update the variable
	auto iter = mVariables.find(fullName);
	if (iter == mVariables.end()){
		Variable *variable = new Variable(fullName.c_str(), value);
		mVariables.insert(std::make_pair(fullName, variable));
	}
	else {
		Variable *variable = iter->second;
		variable->setValue(value);
	}

	return true;
}


/**
 *
 */
bool Variable::get(char *name, void **funcObjects, int &value)
{
	char fullName[256];
	if (!getFullName(name, funcObjects, fullName)){
		strcpy(errName, name);
		return false;
	}

	auto iter = mVariables.find(fullName);
	if (iter == mVariables.end()){
		strcpy(errName, fullName);
		return false;
	}

	Variable *variable = iter->second;
	value = variable->getValue();
	return true;
}


/**
 *
 */
bool Variable::get(char *name, void **funcObjects, char *strValue)
{
	int value;
	if (!get(name, funcObjects, value))
		return false;

	if (value % 1000 == 0){
		sprintf(strValue, "%d", value / 1000);
	}
	else {
		int decimal;
		if (value < 0)
			decimal = -value % 1000;
		else
			decimal = value % 1000;

		sprintf(strValue, "%d.%03d", value / 1000, decimal);

		// Remove trailing zeroes
		while (strValue[strlen(strValue)-1] == '0')
			strValue[strlen(strValue)-1] = '\0';
	}

	return true;
}


/**
 * If the name includes a subscript we need to evaluate
 * the subscript expression to get the full name,
 * e.g. if b = 1, then name a[$b+1] will become a[2].
 */
bool Variable::getFullName(const char *name, void **funcObjects, char *fullName)
{
	if (*name == '$')
		name++;

	// If subscript found make sure we have matching square brackets
	// and evaluate the contents to get the value.
	const char *startPos = strchr(name, '[');
	if (!startPos){
		strcpy(fullName, name);
		return true;
	}

	const char *endPos = strchr(name, ']');
	if (!endPos){
		fprintf(stderr, "subscript has missing ']'. Found value = %s\n",
				name);
		return false;
	}

	if (endPos < startPos){
		fprintf(stderr, "subscript has missing '['. Found value = %s\n",
				name);
		return false;
	}

	char sub[256];
	int len = endPos - (startPos + 1);
	strncpy(sub, startPos+1, len);
	sub[len] = '\0';
	int value;
	if (!evaluate(sub, funcObjects, value))
		return false;

	if (value < 0){
		fprintf(stderr, "subscript must not be negative. Found value = %s\n",
				numToStr(value));
		return false;
	}

	if (value % 1000 != 0){
		fprintf(stderr, "Subscript must not be a decimal. Found value = %s\n",
				numToStr(value));
		return false;
	}

	len = 1 + startPos - name;
	strncpy(fullName, name, len);
	fullName[len] = '\0';
	strcat(fullName, numToStr(value));
	strcat(fullName, endPos);
	return true;
}


/**
 * The supplied string can contain constants, arithmetic
 * operators +, -, *, /, =, <, > and other variables.
 *
 * True and False are converted to constansts 1 and 0.
 * And and Or are converted to operators * and +.
 */
bool Variable::evaluate(std::string expression, void **funcObjects, int &value)
{
	value = 0;

	const char *pos = expression.c_str();
	char lastOp = '+';
	while (*pos != '\0'){
		char op;
		int val;
		bool expectingOper = (lastOp == ' ');
		pos = getToken(pos, funcObjects, expectingOper, op, val);
		if (pos == NULL)
			return false;

		if (op == '\0')
			continue;

		if (expectingOper){
			// We are expecting an operator
			if (op == ' '){
				fprintf(stderr, "Expression has missing operator\n");
				return false;
			}
		}
		else {
			// We are expecting a value
			if (op != ' '){
				fprintf(stderr, "Expression has missing value\n");
				return false;
			}
			
			switch (lastOp){
				case '+': value += val; break;
				case '-': value -= val; break;
				case '*': value = (int)(((double)value * val) / 1000.0); break;
				case '/': value = (int)(((double)value * 1000.0) / val); break;
				case '=': if (value == val) value = 1000; else value = 0; break;
				case '<': if (value < val) value = 1000; else value = 0; break;
				case '>': if (value > val) value = 1000; else value = 0; break;
				case 'N': if (value != val) value = 1000; else value = 0; break;
				case 'L': if (value <= val) value = 1000; else value = 0; break;
				case 'G': if (value >= val) value = 1000; else value = 0; break;
			}
		}

		lastOp = op;
	}

	if (lastOp != ' '){
		fprintf(stderr, "Expression has missing value\n");
		return false;
	}

	return true;
}


/**
 * Returns the next token in the string. If the token is a
 * variable or function its value is returned. If the value
 * is a number it is returned in thousandths. The next
 * token may be one of the operators +, -, *, /.
 * If a value is returned the operator is set to ' '.
 * If there is no token (end of string) the operator
 * is set to '\0'. If the string is invalid, NULL is
 * returned.
 */
const char *Variable::getToken(const char *pos, void **funcObjects, bool expectingOper, char &op, int &value)
{
	// Remove leading spaces
	while (*pos == ' ')
		pos++;

	if (*pos == '\0'){
		op = '\0';
		return pos;
	}

	bool negativeVal = false;
	if (*pos == '+'){
		op = '+';
		return pos+1;
	}
	else if (*pos == '-'){
		if (expectingOper){
			op = '-';
			return pos+1;
		}
		negativeVal = true;
		pos++;
	}
	else if (*pos == '*'){
		op = '*';
		return pos+1;
	}
	else if (*pos == '/'){
		op = '/';
		return pos+1;
	}
	else if (*pos == '='){
		op = '=';
		// Also allow '=='
		if (*(pos+1) == '=')
			pos++;
		return pos+1;
	}
	else if (*pos == '<'){
		if (*(pos+1) == '='){
			op = 'L';
			pos++;
		}
		else {
			op = '<';
		}
		return pos+1;
	}
	else if (*pos == '>'){
		if (*(pos+1) == '='){
			op = 'G';
			pos++;
		}
		else {
			op = '>';
		}
		return pos+1;
	}
	else if (strncasecmp(pos, "!=", 2) == 0){
		op = 'N';
		return pos+2;
	}
	else if (strncasecmp(pos, "and", 3) == 0){
		op = '*';
		return pos+3;
	}
	else if (strncasecmp(pos, "or", 2) == 0){
		op = '+';
		return pos+2;
	}
	else if (strncasecmp(pos, "true", 4) == 0){
		op = ' ';
		value = 1000;
		return pos+4;
	}
	else if (strncasecmp(pos, "false", 5) == 0){
		op = ' ';
		value = 0;
		return pos+5;
	}

	bool logicalNot = false;
	if (!negativeVal && strncasecmp(pos, "not ", 4) == 0){
		logicalNot = true;
		pos += 4;
		while (*pos == ' ')
			pos++;
	}

	// Found a variable or a number
	op = ' ';

	// If brackets we need to evaluate them
	if (*pos == '('){
		pos++;
		const char *startPos = pos;

		// Find matching bracket
		int bracketCount = 1;
		while (bracketCount > 0){
			if (*pos == '\0'){
				fprintf(stderr, "Missing bracket evaluating %s\n", startPos);
				return NULL;
			}
			if (*pos == '(')
				bracketCount++;
			else if (*pos == ')')
				bracketCount--;

			pos++;
		}

		char expression[256];
		int len = (pos - 1) - startPos;
		strncpy(expression, startPos, len);
		expression[len] = '\0';
		if (!evaluate(expression, funcObjects, value)){
			fprintf(stderr, "while expanding brackets %s\n", expression);
			return NULL;
		}
		if (negativeVal)
			value = -value;

		if (logicalNot){
			if (value == 0)
				value = 1000;
			else
				value = 0;
		}
		return pos;
	}

	// Must end with either a space or an operator.
	const char *startPos = pos;
	int subCount = 0;
	int bracketCount = 0;
	while (*pos != '\0'){
		if (*pos == '[')
			subCount++;

		if (*pos == '(')
			bracketCount++;

		if (subCount == 0 && bracketCount == 0 && (*pos == ' ' || *pos == '+' || *pos == '-' || *pos == '*' || *pos == '/') || *pos == '=' || *pos == '<' || *pos == '>' || (*pos == '!' && *(pos+1) == '='))
			break;

		if (*pos == ']')
			subCount--;

		if (*pos == ')')
			bracketCount--;

		pos++;
	}

	if (subCount > 0){
		fprintf(stderr, "Missing ']' while evaluating %s\n", startPos);
		return NULL;
	}

	if (bracketCount > 0){
		fprintf(stderr, "Missing ')' while evaluating %s\n", startPos);
		return NULL;
	}

	char operand[256];
	int len = pos - startPos;
	strncpy(operand, startPos, len);
	operand[len] = '\0';

	// Is it a variable?
	if (*operand == '$'){
		// Is it a valid variable?
		if (!Variable::get(operand, funcObjects, value)){
			fprintf(stderr, "Variable %s is undefined. For a list use $var[i].\n", Variable::errName);
			return NULL;
		}
		if (negativeVal)
			value = -value;

		if (logicalNot){
			if (value == 0)
				value = 1000;
			else
				value = 0;
		}
		return pos;
	}

	// Is it a function?
	char *funcExpression = strchr(operand, '(');
	if (funcExpression){
		char *func;
		if (*operand == '!'){
			func = operand + 1;
			logicalNot = !logicalNot;
		}
		else
			func = operand;

		*funcExpression = '\0';
		funcExpression++;
		char *endExpression = strrchr(funcExpression, ')');
		if (!endExpression){
			fprintf(stderr, "Function %s has missing ')'\n", func);
			return NULL;
		}

		*endExpression = '\0';
		if (*(endExpression+1) != '\0'){
			fprintf(stderr, "Function %s has unexpected characters after ')'\n", func);
			return NULL;
		}

		// Is it a valid function?
		if (strcasecmp(func, "sync") == 0){
			value = Engine::timeNow() - Engine::syncTimer;
		}
		else if (strcasecmp(func, "rand") == 0){
			value = rand() % 1000;
		}
		else if (strcasecmp(func, "int") == 0
				 || strcasecmp(func, "trunc") == 0)
		{
			int funcVal;
			if (!evaluate(funcExpression, funcObjects, funcVal)){
				fprintf(stderr, "while evaluating function %s\n", func);
				return NULL;
			}
			value = (funcVal / 1000) * 1000;
		}
		else if (strcasecmp(func, "round") == 0){
			int funcVal;
			if (!evaluate(funcExpression, funcObjects, funcVal)){
				fprintf(stderr, "while evaluating function %s\n", func);
				return NULL;
			}
			value = ((funcVal + 500) / 1000) * 1000;
		}
		else if (strcasecmp(func, "playing") == 0){
			int num = atoi(funcExpression);
			Audio *audio = (Audio *)funcObjects[num];
			if (audio->isPlaying(Engine::timeNow()))
				value = 1000;
			else
				value = 0;
		}
		else if (strcasecmp(func, "recording") == 0){
			if (Engine::recorder->isActive())
				value = 1000;
			else
				value = 0;
		}
		else if (strcasecmp(func, "pressed") == 0){
			int num = atoi(funcExpression);
			Gpio *gpio = (Gpio *)funcObjects[num];
			int wantedState;
			if (Gpio::usingPibrella())
				wantedState = 1;
			else
				wantedState = 0;

			if (gpio->isState(wantedState))
				value = 1000;
			else
				value = 0;
		}
		else if (strcasecmp(func, "released") == 0){
			int num = atoi(funcExpression);
			Gpio *gpio = (Gpio *)funcObjects[num];
			int wantedState;
			if (Gpio::usingPibrella())
				wantedState = 0;
			else
				wantedState = 1;

			if (gpio->isState(wantedState))
				value = 1000;
			else
				value = 0;
		}
		else {
			fprintf(stderr, "Unknown function %s\n", func);
			return NULL;
		}
		if (negativeVal)
			value = -value;

		if (logicalNot){
			if (value == 0)
				value = 1000;
			else
				value = 0;
		}
		return pos;
	}

	// Must be a constant
	if (Command::parseDecimal(operand, -1, value) == NULL)
		return NULL;

	if (negativeVal)
		value = -value;

	if (logicalNot){
		if (value == 0)
			value = 1000;
		else
			value = 0;
	}
	return pos;
}


/**
 *
 */
char *Variable::numToStr(int value)
{
    static char strValue[256];

    if (value % 1000 == 0){
        sprintf(strValue, "%d", value / 1000);
    }
    else {
		int decimal;
		if (value < 0)
			decimal = -value % 1000;
		else
			decimal = value % 1000;

        sprintf(strValue, "%d.%03d", value / 1000, decimal);

        // Remove trailing zeroes
        while (strValue[strlen(strValue)-1] == '0')
            strValue[strlen(strValue)-1] = '\0';
    }

    return strValue;
}
