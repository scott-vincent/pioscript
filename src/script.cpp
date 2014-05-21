/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include "script.h"
#include "command.h"
#include "audio.h"
#include "gpio.h"
#include "variable.h"


/**
 *
 */
Script::Script(char *filename)
{
	strcpy(mFilename, filename);
	mCurrentMethod = NULL;
	mMain = NULL;
	mUsesRecording = false;
}

/**
 *
 */
Script::~Script()
{
	// Cleanup methods and commands
	while (true){
		auto iter = mMethods.begin();
		if (iter == mMethods.end())
			break;

		Method *method = iter->second;
		while (true){
			auto iter2 = method->commands.begin();
			if (iter2 == method->commands.end())
				break;

			Command *command = *iter2;
			delete command;
			method->commands.erase(iter2);
		}
		
		delete method;
		mMethods.erase(iter);
	}

	// Cleanup sounds
	while (true){
		auto iter = mSounds.begin();
		if (iter == mSounds.end())
			break;

		Audio *audio = iter->second;
		delete audio;
		mSounds.erase(iter);
	}

	// Cleanup gpio pins
	while (true){
		auto iter = mPins.begin();
		if (iter == mPins.end())
			break;

		Gpio *gpio = iter->second;
		delete gpio;
		mPins.erase(iter);
	}

	// Cleanup variables
	Variable::cleanup();
}


/**
 *
 */
bool Script::load()
{
	FILE *inf = fopen(mFilename, "r");
	if (!inf){
		fprintf(stderr, "Cannot open file %s\n", mFilename);
		return false;
	}

	char buff[256];
	int lineNum = 0;
	while (fgets(buff, sizeof(buff), inf)){
		lineNum++;
		int endPos = strlen(buff);
		while (endPos > 0
				&& (buff[endPos-1] == '\r' || buff[endPos-1] == '\n' ||
				buff[endPos-1] == ' ' || buff[endPos-1] == '\t'))
		{
			endPos--;
		}
		buff[endPos] = '\0';

		int startPos = 0;
		while (startPos < strlen(buff)
				&& (buff[startPos] == ' ' || buff[startPos] == '\t'))
		{
			startPos++;
		}

		if (startPos > 0)
			strcpy(buff, &buff[startPos]);
		
		// Ignore comments and blank lines
		if (*buff == '\0' || buff[0] == '#')
			continue;

		// Is this a new method?
		if (strlen(buff) >= 8 && strncasecmp(buff, "activity", 8) == 0){
			if (!defineMethod(&buff[8], lineNum)){
				fprintf(stderr, "at line %d <%s>\n", lineNum, buff);
				fclose(inf);
				return false;
			}
			continue;
		}

		// Must be a new command
		char command[256];
		char params[256];
		char *sep = strchr(buff, ' ');
		if (!sep){
			strcpy(command, buff);
			*params = '\0';
		}
		else {
			*sep = '\0';
			strcpy(command, buff);
			*sep = ' ';
			sep++;
			while (*sep == ' ')
				sep++;

			strcpy(params, sep);
		}

		if (!addCommand(command, params, lineNum, buff)){
			fprintf(stderr, "at line %d <%s>\n", lineNum, buff);
			fclose(inf);
			return false;
		}
	}

	fclose(inf);

	if (!mMain){
		fprintf(stderr, "Activity Main is missing\n");
		return false;
	}

	// Check for empty methods
	for (auto iter = mMethods.begin(); iter != mMethods.end(); iter++)
	{
		Method *method = iter->second;
		if (method->commands.size() == 0){
			fprintf(stderr, "Activity %s has not been defined or is empty\n",
					method->getName());
			fprintf(stderr, "at line %d\n", method->getLineNum());
			return false;
		}
	}

	// Check for missing sounds
	for (auto iter = mSounds.begin(); iter != mSounds.end(); iter++)
	{
		Audio *audio = iter->second;
		if (audio->isMissing()){
			fprintf(stderr, "Sound file not found: %s\n",
					audio->getName());
			fprintf(stderr, "at line %d\n", audio->getLineNum());
			return false;
		}
	}

	if (!orphanNestingCheck())
		return false;

	return true;
}


/**
 *
 */
bool Script::defineMethod(const char *name, int lineNum)
{
	while (*name == ' ')
		name++;

	if (*name == '\0'){
		fprintf(stderr, "Missing activity name\n");
		return false;
	}

	if (mCurrentMethod && mCurrentMethod->commands.size() == 0){
		fprintf(stderr, "Activity %s must contain at least one command\n",
				mCurrentMethod->getName());
		return false;
	}

	if (!orphanNestingCheck())
		return false;

	mCurrentMethod = addMethod(name, lineNum);

	// Make sure method has not already been defined
	if (mCurrentMethod->commands.size() > 0){
		fprintf(stderr, "Duplicate definition of activity %s\n", name);
		return false;
	}

	if (strcasecmp(name, "main") == 0){
		if (mMain){
			fprintf(stderr, "Only one main activity is allowed\n");
			return false;
		}
		mMain = mCurrentMethod;
	}

	return true;
}


/**
 *
 */
bool Script::orphanNestingCheck()
{
	if (mCurrentMethod && nestedCommands.size() != 0){
		Command *nestStart = nestedCommands.back();
		if (nestStart->type == Command::If){
			fprintf(stderr, "Activity %s has If at line %d with no matching End_If.\n",
					mCurrentMethod->getName(), nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::Else_If){
			fprintf(stderr, "Activity %s has Else If at line %d with no matching End_If.\n",
					mCurrentMethod->getName(), nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::Else){
			fprintf(stderr, "Activity %s has Else at line %d with no matching End_If.\n",
					mCurrentMethod->getName(), nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::While){
			fprintf(stderr, "Activity %s has While at line %d with no matching End_While.\n",
					mCurrentMethod->getName(), nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::For){
			fprintf(stderr, "Activity %s has For at line %d with no matching End_For.\n",
					mCurrentMethod->getName(), nestStart->lineNum);
			return false;
		}
	}
	return true;
}


/**
 * Find existing or create new if method does not already exist
 */
Method* Script::addMethod(const char *name, int lineNum)
{
	Method *method;
	auto iter = mMethods.find(name);
	if (iter == mMethods.end()){
		method = new Method(name, lineNum);
		mMethods.insert(std::make_pair(name, method));
	}
	else
		method = iter->second;

	return method;
}


/**
 * Find existing or create new if sound does not already exist
 */
Audio* Script::addAudio(const char *name, int lineNum)
{
	Audio *audio;
	auto iter = mSounds.find(name);
	if (iter == mSounds.end()){
		audio = new Audio(name, lineNum);
		if (!audio->isValid())
			return NULL;

		mSounds.insert(std::make_pair(name, audio));
	}
	else
		audio = iter->second;

	return audio;
}


/**
 * Find existing or create new if gpio pin does not already exist
 */
Gpio* Script::addGpio(int pin, Gpio::Type type)
{
	Gpio *gpio;
	auto iter = mPins.find(pin);
	if (iter == mPins.end()){
		if (Gpio::usingPibrella()){
			fprintf(stderr, "Pin %d is not a valid Pibrella pin.\n", pin);
			return NULL;
		}
		gpio = new Gpio(pin, type);
		mPins.insert(std::make_pair(pin, gpio));
	}
	else {
		gpio = iter->second;

		// Type must match for existing pin
		if (type != gpio->getType())
		{
			if ((type == Gpio::OUT_PIN && gpio->getType() == Gpio::PWM)
				|| (type == Gpio::PWM && gpio->getType() == Gpio::OUT_PIN)){
				// It is ok for a pin to be both Output and PWM
				gpio->outputAndPwm();
			}
			else if ((type == Gpio::IN_PIN && gpio->getType() == Gpio::SWITCH)
				|| (type == Gpio::SWITCH && gpio->getType() == Gpio::IN_PIN)){
				// It is ok for a pin to be both Input and Switch
				gpio->inputAndSwitch();
			}
			else {
				fprintf(stderr, "Pin %d cannot have multiple uses. Must be either input/switch or output/PWM.\n", pin);
				return NULL;
			}
		}
	}

	return gpio;
}


/**
 * If an expression contains any 'pressed' or 'released'
 * functions we need to add the gpio switches and if it
 * contains any 'playing' functions we need to add the
 * audio object. The function param is replaced by the
 * command's funcObject number.
 */
bool Script::addFuncObjects(Command *command, int paramNum, int lineNum)
{
	char oldStr[512];
	const char *src = oldStr;
	char newStr[512];
	char *dest = newStr;

	if (paramNum == -1)
		strcpy(oldStr, command->nameParam.getStrValue().c_str());
	else
		strcpy(oldStr, command->param[paramNum].getStrValue().c_str());

	// Convert to lower case
	char *lwr = oldStr;
	while (*lwr != '\0'){
		tolower(*lwr++);
	}
	*lwr = '\0';

	if (strstr(src, "recording(") || strstr(src, "listening("))
		mUsesRecording = true;

	while (true){
		bool isAudio = false;
		const char *func = strstr(src, "pressed(");
		int funcLen = 8;
		const char *nextFunc = strstr(src, "released(");
		if (nextFunc && (!func || nextFunc < func)){
			func = nextFunc;
			funcLen = 9;
		}
		nextFunc = strstr(src, "playing(");
		if (nextFunc && (!func || nextFunc < func)){
			func = nextFunc;
			funcLen = 8;
			isAudio = true;
		}

		if (!func)
			break;

		const char *startParam = func + funcLen;
		const char *endParam = strchr(startParam, ')');
		if (!endParam){
			fprintf(stderr, "Function has missing ')' while evaluating %s\n", func);
			return false;
		}

		while (*startParam == ' ')
			startParam++;

		char param[256];
		int len = endParam - startParam;
		strncpy(param, startParam, len);
		param[len] = '\0';

		if (isAudio){
			// Parse the value
			if (*param == '\0'){
				fprintf(stderr, "Expected a parameter\n");
				fprintf(stderr, "while evaluating function %s\n", func);
				return false;
			}

			// Add the audio object
			command->object = addAudio(param, lineNum);
			if (!command->object)
				return false;
		}
		else {
			// Use param 8 temporarily to parse the value
			if (!command->addPinParam(8, param)){
				fprintf(stderr, "while evaluating function %s\n", func);
				return false;
			}

			// Add the Gpio switch
			int pin = command->param[8].getValue();
			command->object = addGpio(pin, Gpio::SWITCH);
			if (!command->object)
				return false;
		}
	
		if (command->nextFuncObject > 8){
			fprintf(stderr, "Expression contains too many Playing/Pressed/Released functions (Max is 9)\n");
			return false;
		}

		// Replace name/number with funcObject number
		command->funcObject[command->nextFuncObject] = command->object;
		len = startParam - src;
		strncpy(dest, src, len);
		sprintf(dest + len, "%d", command->nextFuncObject);
		dest += strlen(dest);
		command->nextFuncObject++;
		src = endParam;
	}

	strcpy(dest, src);
	if (paramNum == -1)
		command->nameParam.setStrValue(newStr);
	else
		command->param[paramNum].setStrValue(newStr);

	return true;
}


/**
 *
 */
bool Script::addCommand(char *name, char *params, int lineNum, char *lineStr)
{
	if (!mCurrentMethod)
		defineMethod("Main", lineNum);

	Command *command = new Command(name, lineNum, lineStr);
	if (!command->isValid())
		return false;

	if (!command->validateParams(params))
		return false;

	if (command->type == Command::Use_Addon){
		// Initialise the Pibrella (only addon supported currently)
		if (!Gpio::usePibrella()){
			return false;
		}

		// Setup the inputs and outputs
		Gpio *gpio;
		gpio = new Gpio(Gpio::PIBRELLA_BUTTON, Gpio::SWITCH);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_BUTTON, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_INPUTA, Gpio::IN_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_INPUTA, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_INPUTB, Gpio::IN_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_INPUTB, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_INPUTC, Gpio::IN_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_INPUTC, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_INPUTD, Gpio::IN_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_INPUTD, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_RED, Gpio::OUT_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_RED, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_AMBER, Gpio::OUT_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_AMBER, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_GREEN, Gpio::OUT_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_GREEN, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_OUTPUTE, Gpio::OUT_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_OUTPUTE, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_OUTPUTF, Gpio::OUT_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_OUTPUTF, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_OUTPUTG, Gpio::OUT_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_OUTPUTG, gpio));
		gpio = new Gpio(Gpio::PIBRELLA_OUTPUTH, Gpio::OUT_PIN);
		mPins.insert(std::make_pair(Gpio::PIBRELLA_OUTPUTH, gpio));
	}
	else
		mCurrentMethod->commands.push_back(command);

	// Add object if required
	if (command->isMethod()){
		command->object = addMethod(command->nameParam.getStrValue().c_str(), lineNum);
		if (!command->object)
			return false;
	}
	else if (command->isAudio()){
		// Some audio commands are global rather than
		// for a single object.
		if (command->isGlobal())
			return true;

		command->object = addAudio(command->nameParam.getStrValue().c_str(), lineNum);
		if (!command->object)
			return false;
	}
	else if (command->type == Command::Save_Recording){
		Audio *audio = addAudio(command->nameParam.getStrValue().c_str(), lineNum);
		if (!audio)
			return false;

		audio->notMissing();
		command->object = audio;
	}
	else if (command->type == Command::Play_Recording)
	{
		Audio *audio = addAudio("{recorded_wav}", lineNum);
		if (!audio)
			return false;

		audio->notMissing();
		command->object = audio;
	}
	else if (command->isGpioSwitch()){
		command->object = addGpio(command->param[0].getValue(), Gpio::SWITCH);
		if (!command->object)
			return false;
	}
	else if (command->type == Command::Read_Input){
		command->object = addGpio(command->param[0].getValue(), Gpio::IN_PIN);
		if (!command->object)
			return false;

		command->funcObject[0] = addGpio(command->param[1].getValue(), Gpio::OUT_PIN);
		if (!command->funcObject[0])
			return false;
	}
	else if (command->isGpioPwm()){
		if (command->param[7].getValue() == -1){
			// Single pin
			command->object = addGpio(command->param[0].getValue(), Gpio::PWM);
			if (!command->object)
				return false;
		}
		else if (!command->isGlobal()){
			// Must be for a range of pins
			for (int pin = command->param[0].getValue();
				pin <= command->param[7].getValue(); pin++)
			{
				if (!addGpio(pin, Gpio::PWM))
					return false;
			}
		}
	}
	else if (command->isGpioInput()){
		command->object = addGpio(command->param[0].getValue(), Gpio::IN_PIN);
		if (!command->object)
			return false;
	}
	else if (command->isGpioOutput()){
		if (command->param[7].getValue() == -1){
			// Single pin
			command->object = addGpio(command->param[0].getValue(), Gpio::OUT_PIN);
			if (!command->object)
				return false;
		}
		else if (!command->isGlobal()){
			// Must be for a range of pins
			for (int pin = command->param[0].getValue();
				pin <= command->param[7].getValue(); pin++)
			{
				if (!addGpio(pin, Gpio::OUT_PIN))
					return false;
			}
		}
	}

	if (command->isRecording())
		mUsesRecording = true;

	if (command->type == Command::Set){
		if (!addFuncObjects(command, 1, lineNum))
			return false;
	}

	// Add nesting info
	if (command->isStartNest()){
		command->nestEndLine = -1;
		nestedCommands.push_back(command);
		if (!addFuncObjects(command, -1, lineNum))
			return false;

		if (command->type == Command::For){
			if (!addFuncObjects(command, 1, lineNum))
				return false;

			if (!addFuncObjects(command, 3, lineNum))
				return false;
		}
	}
	else if (command->isEndNest()){
		Command *nestStart = NULL;
		if (nestedCommands.size() > 0){
			nestStart = nestedCommands.back();
			nestedCommands.pop_back();
			if (command->type == Command::Else
				|| command->type == Command::Else_If)
			{
				nestedCommands.push_back(command);
			}
			if (command->type == Command::Else_If){
				if (!addFuncObjects(command, -1, lineNum))
					return false;
			}
		}
		else {
			// Found end type with no start type
			if (command->type == Command::Else_If)
				fprintf(stderr, "Found Else If without a matching If\n");
			else if (command->type == Command::Else)
				fprintf(stderr, "Found Else without a matching If\n");
			else if (command->type == Command::End_If)
				fprintf(stderr, "Found End_If without a matching If\n");
			else if (command->type == Command::End_While)
				fprintf(stderr, "Found End_While without a matching While\n");
			else if (command->type == Command::End_For)
				fprintf(stderr, "Found End_For without a matching For\n");

			return false;
		}

		// Make sure we have a matching end type
		if (nestStart->type == Command::If
				&& command->type != Command::Else_If
				&& command->type != Command::Else
				&& command->type != Command::End_If)
		{
			fprintf(stderr, "If at line %d has no matching End_If\n",
					nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::Else_If
				&& command->type != Command::Else_If
				&& command->type != Command::Else
				&& command->type != Command::End_If)
		{
			fprintf(stderr, "Else If at line %d has no matching End_If\n",
					nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::Else
				&& command->type != Command::End_If)
		{
			fprintf(stderr, "Else at line %d has no matching End_If\n",
					nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::While
				&& command->type != Command::End_While)
		{
			fprintf(stderr, "While at line %d has no matching End_While\n",
					nestStart->lineNum);
			return false;
		}
		else if (nestStart->type == Command::For
				&& command->type != Command::End_For)
		{
			fprintf(stderr, "For at line %d has no matching End_For\n",
					nestStart->lineNum);
			return false;
		}

		nestStart->nestEndLine = command->lineNum;
		command->nestStartLine = nestStart->lineNum;
	}

	return true;
}


/**
 *
 */
void Script::report()
{
	printf("\nActivities (%d)\n", mMethods.size());
	printf("----------\n");

	for (auto iter = mMethods.begin(); iter != mMethods.end(); iter++)
	{
		Method *method = iter->second;
		printf("%s\n", method->getName());
	}

	printf("\nSounds (%d)\n", mSounds.size());
	printf("------\n");

	for (auto iter = mSounds.begin(); iter != mSounds.end(); iter++)
	{
		Audio *audio = iter->second;
		printf("%s\n", audio->getName());
	}

	printf("\nRecording\n", mPins.size());
	printf("---------\n");
	if (mUsesRecording)
		printf("Active - requires a USB microphone or USB webcam with built-in microphone\n");
	else
		printf("Not active\n");

	printf("\nGPIO (%d)\n", mPins.size());
	printf("----\n");

	for (auto iter = mPins.begin(); iter != mPins.end(); iter++)
	{
		Gpio *gpio = iter->second;
		if (gpio->getType() == Gpio::SWITCH){
			if (Gpio::usingPibrella())
				printf("Switch pin %s\n", gpio->getPinName());
			else
				printf("Switch (internal pull-up) pin %s\n", gpio->getPinName());
		}
		else if (gpio->getType() == Gpio::IN_PIN)
			printf("Input pin %s\n", gpio->getPinName());
		else if (gpio->getType() == Gpio::OUT_PIN)
			printf("Output pin %s\n", gpio->getPinName());
		else
			printf("PWM pin %s\n", gpio->getPinName());
	}
}
