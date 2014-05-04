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
#include <strings.h>
#include <math.h>
#include <ctype.h>
#include "command.h"
#include "audio.h"
#include "gpio.h"


/**
 *
 */
Command::Command(char *name, int _lineNum, char *_lineStr)
{
	lineNum = _lineNum;
	if (strlen(_lineStr) > 79){
		strncpy(lineStr, _lineStr, 75);
		lineStr[75] = '\0';
		strcat(lineStr, "...");
	}
	else {
		strcpy(lineStr, _lineStr);
	}
	object = NULL;
	nameParam.setStrValue("");
	mIsValid = false;
	nextFuncObject = 0;

	if (strcasecmp(name, "play_sound") == 0)
		type = Play_Sound;
	else if (strcasecmp(name, "play_loop") == 0)
		type = Play_Loop;
	else if (strcasecmp(name, "play_wait") == 0)
		type = Play_Wait;
	else if (strcasecmp(name, "play_from") == 0)
		type = Play_From;
	else if (strcasecmp(name, "pause") == 0)
		type = Pause;
	else if (strcasecmp(name, "resume") == 0)
		type = Resume;
	else if (strcasecmp(name, "stop_sound") == 0)
		type = Stop_Sound;
	else if (strcasecmp(name, "volume") == 0)
		type = Volume;
	else if (strcasecmp(name, "fadeout") == 0)
		type = FadeOut;
	else if (strcasecmp(name, "use_addon") == 0)
		type = Use_Addon;
	else if (strcasecmp(name, "play_note") == 0)
		type = Play_Note;
	else if (strcasecmp(name, "loop_note") == 0)
		type = Loop_Note;
	else if (strcasecmp(name, "stop_note") == 0)
		type = Stop_Note;
	else if (strcasecmp(name, "play_song") == 0)
		type = Play_Song;
	else if (strcasecmp(name, "sound_buzzer") == 0)
		type = Sound_Buzzer;
	else if (strcasecmp(name, "wait_press") == 0)
		type = Wait_Press;
	else if (strcasecmp(name, "wait_release") == 0)
		type = Wait_Release;
	else if (strcasecmp(name, "wait_high") == 0)
		type = Wait_High;
	else if (strcasecmp(name, "wait_low") == 0)
		type = Wait_Low;
	else if (strcasecmp(name, "read_input") == 0)
		type = Read_Input;
	else if (strcasecmp(name, "output_high") == 0)
		type = Output_High;
	else if (strcasecmp(name, "output_low") == 0)
		type = Output_Low;
	else if (strcasecmp(name, "output_pwm") == 0)
		type = Output_Pwm;
	else if (strcasecmp(name, "turn_on") == 0)
		type = Output_High;
	else if (strcasecmp(name, "turn_off") == 0)
		type = Output_Low;
	else if (strcasecmp(name, "set_power") == 0)
		type = Output_Pwm;
	else if (strcasecmp(name, "set_brightness") == 0)
		type = Output_Pwm;
	else if (strcasecmp(name, "set_position") == 0)
		type = Output_Pwm;
	else if (strcasecmp(name, "rangemap_pwm") == 0)
		type = Rangemap_Pwm;
	else if (strcasecmp(name, "random_pwm") == 0)
		type = Random_Pwm;
	else if (strcasecmp(name, "random_pwm_loop") == 0)
		type = Random_Pwm_Loop;
	else if (strcasecmp(name, "linear_pwm") == 0)
		type = Linear_Pwm;
	else if (strcasecmp(name, "linear_pwm_wait") == 0)
		type = Linear_Pwm_Wait;
	else if (strcasecmp(name, "linear_pwm_loop") == 0)
		type = Linear_Pwm_Loop;
	else if (strcasecmp(name, "sine_pwm") == 0)
		type = Sine_Pwm;
	else if (strcasecmp(name, "sine_pwm_wait") == 0)
		type = Sine_Pwm_Wait;
	else if (strcasecmp(name, "sine_pwm_loop") == 0)
		type = Sine_Pwm_Loop;
	else if (strcasecmp(name, "stop_pwm") == 0)
		type = Stop_Pwm;
	else if (strcasecmp(name, "flash") == 0)
		type = Flash;
	else if (strcasecmp(name, "start") == 0)
		type = Start;
	else if (strcasecmp(name, "start_wait") == 0)
		type = Start_Wait;
	else if (strcasecmp(name, "stop") == 0)
		type = Stop;
	else if (strcasecmp(name, "wait") == 0)
		type = Wait;
	else if (strcasecmp(name, "repeat") == 0)
		type = Repeat;
	else if (strcasecmp(name, "print") == 0)
		type = Print;
	else if (strcasecmp(name, "start_sync") == 0)
		type = Start_Sync;
	else if (strcasecmp(name, "set_sync") == 0)
		type = Set_Sync;
	else if (strcasecmp(name, "wait_sync") == 0)
		type = Wait_Sync;
	else if (strcasecmp(name, "set") == 0)
		type = Set;
	else if (strcasecmp(name, "if") == 0)
		type = If;
	else if (strcasecmp(name, "else") == 0)
		type = Else;
	else if (strcasecmp(name, "end_if") == 0)
		type = End_If;
	else if (strcasecmp(name, "while") == 0)
		type = While;
	else if (strcasecmp(name, "end_while") == 0)
		type = End_While;
	else if (strcasecmp(name, "for") == 0)
		type = For;
	else if (strcasecmp(name, "end_for") == 0)
		type = End_For;
	else if (strcasecmp(name, "clear_screen") == 0)
		type = Clear_Screen;
	else if (strcasecmp(name, "exit") == 0)
		type = Exit;
	else {
		fprintf(stderr, "Unknown command: %s\n", name);
		fprintf(stderr, "Type 'pioscript --help' for a list of valid commands.\n", name);
		return;
	}

	mIsValid = true;
}


/**
 *
 */
bool Command::isAudio()
{
	return (type == Play_Sound || type == Play_Loop || type == Play_Wait
		|| type == Play_From || type == Pause || type == Resume
		|| type == Stop_Sound || type == Volume || type == FadeOut);
}


/**
 *
 */
bool Command::isGpio()
{
	return (isGpioSwitch() || isGpioInput() || isGpioOutput() || isGpioPwm());
}


/**
 *
 */
bool Command::isGpioSwitch()
{
	return (type == Wait_Press || type == Wait_Release);
}


/**
 *
 */
bool Command::isGpioInput()
{
	return (type == Wait_High || type == Wait_Low);
}


/**
 *
 */
bool Command::isGpioOutput()
{
	return (type == Output_High || type == Output_Low);
}


/**
 *
 */
bool Command::isGpioPwm()
{
	return (type == Output_Pwm || type == Rangemap_Pwm
		|| type == Random_Pwm || type == Random_Pwm_Loop
		|| type == Linear_Pwm || type == Linear_Pwm_Wait
		|| type == Linear_Pwm_Loop || type == Sine_Pwm
		|| type == Sine_Pwm_Wait || type == Sine_Pwm_Loop
		|| type == Stop_Pwm || type == Flash);
}


/**
 *
 */
bool Command::isMethod()
{
	return (type == Start || type == Start_Wait || type == Stop);
}


/**
 *
 */
bool Command::validateParams(char *params)
{
	// Parse params
	char *pos = params;

	// param 7 is used for range end pin
	param[7].setValue(-1);

	switch (type) {

		case Wait_Press:
		case Wait_Release:
		case Wait_High:
		case Wait_Low:
			// Expect pin number
			if (!(pos = addPinParam(0, pos)))
				return false;

			break;

		case Read_Input:
			// Expect 2 pin numbers (1 in, 1 out)
			if (!(pos = addPinParam(0, pos)))
				return false;

			if (!(pos = addPinParam(1, pos)))
				return false;

			break;

		case Output_High:
		case Output_Low:
			// Expect pin number (or range)
			if (!(pos = addPinRangeParam(0, pos, false)))
				return false;

			break;

		case Stop_Pwm:
			// Expect pin number (or range)
			if (!(pos = addPinRangeParam(0, pos, true)))
				return false;

			break;

		case Output_Pwm:
			// Expect pin number (or range) and 1 decimal param
			if (!(pos = addPinRangeParam(0, pos, true)))
				return false;

			if (!(pos = addParam(1, pos)))
				return false;

			break;

		case Flash:
			// Expect pin number (or range) and 1 decimal param
			if (!(pos = addPinRangeParam(0, pos, true)))
				return false;
			
			if (!(pos = addParam(8, pos)))
				return false;

			// 4 params are hard-coded
			param[1].setValue(0);
			param[2].setValue(100000);
			param[3].setValue(-90);
			param[4].setValue(270);
			break;

		case Play_Note:
			// Expect 1 note name/number and 1 decimal param.
			if (!(pos = addNoteParam(0, pos, false)))
				return false;

			if (!(pos = addParam(1, pos)))
				return false;

			break;

		case Loop_Note:
			// Expect 1 note name/number
			if (!(pos = addNoteParam(0, pos, false)))
				return false;

			break;

		case Sound_Buzzer:
			// Expect 1 decimal param.
			param[0].setValue(1);
			if (!(pos = addParam(8, pos)))
				return false;

			break;

		case Play_Song:
			// Expect 1 decimal and 1 string.
			if (!(pos = addParam(8, pos)))
				return false;

			if (!(pos = addStrParam(1, pos)))
				return false;

			break;

		case Rangemap_Pwm:
			// Expect pin number and 2 decimal params
			if (!(pos = addPinParam(0, pos)))
				return false;

			if (!(pos = addParam(1, pos)))
				return false;

			if (!(pos = addParam(2, pos)))
				return false;

			break;

		case Random_Pwm:
			// Expect pin number (or range) and 4 decimal params
			if (!(pos = addPinRangeParam(0, pos, true)))
				return false;

			if (!(pos = addParam(1, pos)))
				return false;

			if (!(pos = addParam(2, pos)))
				return false;

			if (!(pos = addParam(3, pos)))
				return false;

			if (!(pos = addParam(4, pos)))
				return false;

			break;

		case Random_Pwm_Loop:
		case Linear_Pwm:
		case Linear_Pwm_Wait:
		case Linear_Pwm_Loop:
			// Expect pin number (or range) and 3 decimal params
			if (!(pos = addPinRangeParam(0, pos, true)))
				return false;

			if (!(pos = addParam(1, pos)))
				return false;

			if (!(pos = addParam(2, pos)))
				return false;

			if (!(pos = addParam(3, pos)))
				return false;

			break;

		case Sine_Pwm:
		case Sine_Pwm_Wait:
		case Sine_Pwm_Loop:
			// Expect pin number (or range), 2 decimal, 2 pos/neg
			// and 1 decimal params.
			if (!(pos = addPinRangeParam(0, pos, true)))
				return false;

			if (!(pos = addParam(1, pos)))
				return false;

			if (!(pos = addParam(2, pos)))
				return false;

			if (!(pos = addParam(3, pos)))
				return false;

			if (!(pos = addParam(4, pos)))
				return false;

			if (!(pos = addParam(5, pos)))
				return false;

			break;

		case Wait:
		case Set_Sync:
		case Wait_Sync:
			// Expect 1 decimal param
			if (!(pos = addParam(0, pos)))
				return false;

			break;
		
		case Play_From:
		case Volume:
		case FadeOut:
			// Expect 1 decimal and 1 string param
			if (!(pos = addParam(0, pos)))
				return false;

			if (!(pos = addStrParam(1, pos)))
				return false;

			break;

		case Play_Sound:
		case Play_Loop:
		case Play_Wait:
		case Pause:
		case Resume:
		case Stop_Sound:
		case Start:
		case Stop:
		case Start_Wait:
		case Print:
		case Use_Addon:
			// Expect 1 string param
			if (!(pos = addStrParam(0, pos)))
				return false;

			break;

		case Set:
			// Expect params in the form 'variable = expression'
			if (!(pos = addSetParams(0, pos)))
				return false;

			break;

		case If:
		case While:
			// Expect 1 string param
			if (!(pos = addStrParam(0, pos)))
				return false;

			break;

		case For:
			// Expect params in the form 'var=expr condition var=expr'
			if (!(pos = addForParams(pos)))
				return false;

			break;

		// Expect no params
		case Repeat:
		case Start_Sync:
		case Else:
		case End_If:
		case End_While:
		case End_For:
		case Clear_Screen:
		case Stop_Note:
		case Exit:
			break;

		default:
			fprintf(stderr, "Unknown command params\n");
			return false;
	}

	// make sure no extra parameters have been supplied
	while (*pos == ' ')
		pos++;

	if (*pos != '\0'){
		fprintf(stderr, "Unexpected parameter\n");
		return false;
	}

	if (type == Use_Addon){
		if (strcasecmp(nameParam.getStrValue().c_str(), "Pibrella") != 0){
			fprintf(stderr, "Only the Pibrella addon is currently supported.\n");
			return false;
		}
	}

	if (type == Play_Note || type == Loop_Note
			|| type == Stop_Note || type == Play_Song)
	{
		if (!Gpio::usingPibrella()){
			fprintf(stderr, "This command can only be used with the Pibrella add-on board. The use_addon command must appear in the script before this command.\n");
			return false;
		}
	}

	if (isGpio() && param[7].getValue() != -1
			&& param[0].getValue() >= param[7].getValue())
	{
		fprintf(stderr, "GPIO pin number range end must be greater than range start\n");
		return false;
	}

	if (isGpio() && param[7].getValue() >= Gpio::MAX_PINS){
		fprintf(stderr, "PWM pin number must be in the range 0 to %d\n",
				Gpio::MAX_PINS - 1);
		return false;
	}

	if (isGpioPwm()){
		if (param[7].getValue() >= Gpio::MAX_PWM_PINS){
			fprintf(stderr, "PWM pin number must be in the range 0 to %d\n",
					Gpio::MAX_PWM_PINS - 1);
			return false;
		}

		if (!validPwmParams())
			return false;
	}

	return true;
}


/**
 *
 */
char *Command::addParam(int num, char *pos)
{
	// Remove leading spaces
	while (*pos == ' ')
		pos++;

	if (*pos == '\0'){
		fprintf(stderr, "Expected more parameters\n");
		return NULL;
	}

	bool isVar = (*pos == '$');
	if (isVar){
		pos++;
		char *startPos = pos;
		while (*pos != ' ' && *pos != '\0')
			pos++;

		// Set param to use variable value
		char var[256];
		int len = pos - startPos;
		strncpy(var, startPos, len);
		var[len] = '\0';

		param[num].setVariable(var);
		return pos;
	}

	int value;
	if (!(pos = parseDecimal(pos, num, value)))
		return NULL;

	param[num].setValue(value);
	if (!validParam(num))
		return NULL;

	return pos;
}


/**
 *
 */
char *Command::addStrParam(int num, char *params)
{
	char *pos = params;

	// Remove leading spaces
	while (*pos == ' ')
		pos++;

	if (*pos == '"'){
		// Find matching quote
		pos++;
		char *endPos = strrchr(pos, '"');
		if (!endPos){
			fprintf(stderr, "String has missing end quote\n");
			return NULL;
		}
		char str[512];
		int len = endPos - pos;
		strncpy(str, pos, len);
		str[len] = '\0';
		nameParam.setStrValue(str);
		pos = endPos + 1;
		return pos;
	}

	// Allow print to have no param to print a blank line
	if (*pos == '\0' && type != Print){
		fprintf(stderr, "Expected more parameters\n");
		return NULL;
	}

	nameParam.setStrValue(pos);
	pos += strlen(pos);
	return pos;
}


/**
 * Params is in form 'var=expression' so we split it
 * into 2 params and store var and expression separately.
 */
char *Command::addSetParams(int num, char *params)
{
	char *sepPos = strchr(params, '=');
	if (!sepPos){
		fprintf(stderr, "Set command has missing '='.\n");
		return NULL;
	}

	char *startPos = params;

	// Remove leading spaces
	while (*startPos == ' ')
		startPos++;

	char *endPos = startPos;
	while (*endPos != ' ' && endPos != sepPos)
		endPos++;

	if (startPos == sepPos){
		fprintf(stderr, "Set command has missing variable name.\n");
		return NULL;
	}

	char varName[256];
	int len = endPos - startPos;
	strncpy(varName, startPos, len);
	varName[len] = '\0';
	param[num].setStrValue(varName);

	char *pos = endPos;
	while (*pos = ' ' && pos < sepPos)
		pos++;

	if (pos < sepPos){
		fprintf(stderr, "Unexpected characters before '=': %s\n", pos);
		return NULL;
	}

	startPos = sepPos + 1;

	// Remove leading spaces
	while (*startPos == ' ')
		startPos++;

	if (*startPos == '\0'){
		fprintf(stderr, "Set command has missing expression.\n");
		return NULL;
	}

	param[num+1].setStrValue(startPos);
	return &startPos[strlen(startPos)];
}


/**
 * Need to find 3 space separated strings (brackets are honoured)
 * but 1st and 3rd are in form 'var=expression' which get split
 * so we end up with 5 params.
 */
char *Command::addForParams(char *params)
{
	int strNum = 0;
	char *startPos = params;
	while (true){
		// Remove leading spaces
		while (*startPos == ' ')
			startPos++;

		if (*startPos == '\0'){
			if (strNum == 3)
				return startPos;

			fprintf(stderr, "Missing parameter. Expecting 'For var=(expression) (condition) var=(expression)'.\n");
			return NULL;
		}

		int bracketCount = 0;
		char *pos = startPos;
		while (*pos != '\0'){
			if (*pos == '(')
				bracketCount++;

			if (bracketCount == 0 && *pos == ' ')
				break;

			if (*pos == ')')
				bracketCount--;

			pos++;
		}

		if (bracketCount > 0){
			fprintf(stderr, "Missing ')' while evaluating %s\n", startPos);
			return NULL;
		}

		char str[256];
		int len = pos - startPos;
		strncpy(str, startPos, len);
		str[len] = '\0';

		if (strNum == 0){
			if (!addSetParams(0, str))
				return false;
		}
		else if (strNum == 1){
			nameParam.setStrValue(str);
		}
		else if (strNum == 2){
			if (!addSetParams(2, str))
				return false;
		}
		else {
			fprintf(stderr, "Too many parameters. Try adding brackets. Expecting 'For var=(expression) (condition) var=(expression)'.\n");
			return NULL;
		}

		startPos = pos;
		strNum++;
	}
}


/**
 * Param must be a single pin number.
 * It may also be a pin name if using an add-on board.
 */
char *Command::addPinParam(int num, char *params)
{
	char *pos = params;
	int value = 0;

	// Remove leading spaces
	while (*pos == ' ')
		pos++;

	if (*pos == '\0'){
		if (num < 7)
			fprintf(stderr, "Parameter %d is missing\n", num+1);
		else
			fprintf(stderr, "Parameter is missing\n");

		return NULL;
	}

	if (Gpio::usingPibrella() && !isdigit(*pos)){
		// Get the pin name
		char *namePos = pos;
		while (*pos != ' ' && *pos != '\0')
			pos++;

		int len = pos - namePos;
		char name[256];
		strncpy(name, namePos, len);
		name[len] = '\0';
		value = Gpio::getPinFromName(name);
		if (value == -1)
			pos = namePos;
	}
	else {
		// Get the pin number
		if (!(pos = parseDecimal(pos, num, value)))
			return NULL;

		param[num].setValue(value);
		if (!validParam(num))
			return NULL;

		return pos;
	}

	// No other chars allowed
	if (*pos != ' ' && *pos != '\0'){
		if (num < 7)
			fprintf(stderr, "Parameter %d contains unexpected characters\n", num+1);
		else
			fprintf(stderr, "Parameter contains unexpected characters\n");
		return NULL;
	}

	param[num].setValue(value);
	return pos;
}


/**
 * Param may be a single pin number, a range (1-5) or all (*).
 * It may also be a pin name if using an add-on board.
 */
char *Command::addPinRangeParam(int num, char *params, bool isPwm)
{
	char *pos = params;
	int value1 = 0;
	int value2 = 0;

	// Remove leading spaces
	while (*pos == ' ')
		pos++;

	if (*pos == '\0'){
		fprintf(stderr, "Parameter %d is missing\n", num+1);
		return NULL;
	}

	if (*pos == '*'){
		// All pins
		nameParam.setStrValue("*");
		param[num].setValue(value1);
		if (isPwm)
			value2 = Gpio::MAX_PWM_PINS - 1;
		else
			value2 = Gpio::MAX_PINS - 1;
		pos++;
	}
	else if (Gpio::usingPibrella() && !isdigit(*pos)){
		// Get the pin name
		char *namePos = pos;
		while (*pos != ' ' && *pos != '\0')
			pos++;

		int len = pos - namePos;
		char name[256];
		strncpy(name, namePos, len);
		name[len] = '\0';
		value1 = Gpio::getPinFromName(name);
		param[num].setValue(value1);
		value2 = -1;
		if (value1 == -1)
			pos = namePos;
	}
	else {
		// Get the first pin
		char *startPos = pos;
		while (*pos != '-' && *pos != ' ' && *pos != '\0'){
			pos++;
		}
		char firstPin[256];
		int len = pos - startPos;
		strncpy(firstPin, startPos, len);
		firstPin[len] = '\0';

		if (!parseDecimal(firstPin, num, value1))
			return NULL;

		param[num].setValue(value1);
		if (!validParam(num))
			return NULL;

		if (*pos == '-'){
			pos++;
			// Get the second pin
			while (isdigit(*pos)){
				value2 = 10 * value2 + (*pos - '0');
				pos++;
			}
		}
		else {
			// Just a single pin, not a range
			value2 = -1;
		}
	}

	// No other chars allowed
	if (*pos != ' ' && *pos != '\0'){
		fprintf(stderr, "Parameter %d contains unexpected characters\n", num+1);
		return NULL;
	}

	// Always use param 7 for range end pin
	param[7].setValue(value2);
	return pos;
}


/**
 * Param must be a MIDI note number or note name.
 */
char *Command::addNoteParam(int num, char *params, bool isSong)
{
	char *pos = params;
	int value = 0;

	// Remove leading spaces
	while (*pos == ' ')
		pos++;

	if (*pos == '\0'){
		if (isSong){
			param[num].setValue(-1);
			return pos;
		}
		fprintf(stderr, "Parameter %d is missing\n", num+1);
		return NULL;
	}

	if (!isdigit(*pos) && *pos != '$'){
		// Get the note name
		char *namePos = pos;
		while (*pos != ' ' && *pos != '\0'){
			if (isSong && *pos == ',')
				break;

			pos++;
		}

		int len = pos - namePos;
		char name[256];
		strncpy(name, namePos, len);
		name[len] = '\0';
		value = noteToNum(name);
		if (value == -1)
			return NULL;

		param[num].setValue(value);
	}
	else {
		// Get the note number
		char *startPos = pos;
		while (*pos != ',' && *pos != ' ' && *pos != '\0'){
			pos++;
		}
		char note[256];
		int len = pos - startPos;
		strncpy(note, startPos, len);
		note[len] = '\0';
		addParam(num, note);
	}

	if (!isSong){
		// No other chars allowed
		if (*pos != ' ' && *pos != '\0'){
			fprintf(stderr, "Parameter %d contains unexpected characters\n", num+1);
			return NULL;
		}
	}

	return pos;
}


/**
 * This is called when parsing a song to determine the note duration.
 * If not specified the default duration is 1 second.
 */
char *Command::addNoteLen(int num, char *params, int speed)
{
	char *pos = params;
	int value = 0;
	int millis = 0;

	if (*pos == ',')
		pos++;

	if (*pos == ' ' || *pos == '\0'){
		// Default is 1000 millisecs (speed is in thousandths)
		param[num].setValue(1000000 / speed);
		return pos;
	}

	// Get the integer part
	while (isdigit(*pos)){
		value = 10 * value + (*pos - '0');
		pos++;
	}

	// Allow our European friends to use a comma instead of a dot
	if (*pos == '.' || *pos == ','){
		pos++;
		// Get the fraction part (up to 3 d.p.)
		int multiplier = 100;
		while (isdigit(*pos)){
			millis = millis + multiplier * (*pos - '0');
			multiplier /= 10;
			pos++;
		}
	}

	// No other chars allowed
	if (*pos != ' ' && *pos != '\0'){
		fprintf(stderr, "Song note contains unexpected characters at: %s\n", params);
		return NULL;
	}

	// Speed is in thousandths
	param[num].setValue(((value * 1000 + millis) * 1000) / speed);
	return pos;
}


/**
 * Do some complex maths to work out new min and max values for PWM sine
 * so that the min and max are both achieved when the range is limited
 * between the start and end angles.
 */
void Command::adjustSine()
{
	double minValue = param[1].getValue();
	double maxValue = param[2].getValue();
	int startAngle = param[3].getValue();
	int endAngle = param[4].getValue();

	if (endAngle < startAngle){
		int tempAngle = startAngle;
		startAngle = endAngle;
		endAngle = tempAngle;
	}

	// min angle is closest to 270
	// max angle is closest to 90
	int minAngle;
	int maxAngle;

	if (endAngle - startAngle >= 360){
		minAngle = 270;
		maxAngle = 90;
	}
	else {
		while (startAngle < 0){
			startAngle += 360;
			endAngle += 360;
		}

		while (startAngle >= 360){
			startAngle -= 360;
			endAngle -= 360;
		}

		// Find max angle (closest to 90 or 450)
		if ((startAngle <= 90 && endAngle >= 90)
			|| (startAngle <= 450 && endAngle >= 450))
		{
			maxAngle = 90;
		}
		else {
			int diff1 = angleMinDiff(startAngle, 90);
			int diff2 = angleMinDiff(endAngle, 90);
			if (diff1 < diff2)
				maxAngle = 90 + diff1;
			else
				maxAngle = 90 + diff2;
		}

		// Find min angle (closest to 270 or 630)
		if ((startAngle <= 270 && endAngle >= 270)
			|| (startAngle <= 630 && endAngle >= 630))
		{
			minAngle = 270;
		}
		else {
			int diff1 = angleMinDiff(startAngle, 270);
			int diff2 = angleMinDiff(endAngle, 270);
			if (diff1 < diff2)
				minAngle = 270 + diff1;
			else
				minAngle = 270 + diff2;
		}
	}

	double minSine = sin(((double)minAngle * M_PI) / 180.0);
	double maxSine = sin(((double)maxAngle * M_PI) / 180.0);
	double amplitude = (maxValue - minValue) / (maxSine - minSine);
	double midPos = maxValue - maxSine * amplitude;

	// Store new min and max values
	param[1].setValue((int)((midPos - amplitude) + 0.5));
	param[2].setValue((int)((midPos + amplitude) + 0.5));
}


/**
 *
 */
int Command::angleMinDiff(int angle1, int angle2)
{
	int diff;
	while ((diff = abs(angle2 - angle1)) > 180){
		if (angle1 > angle2)
			angle1 -= 360;
		else
			angle2 -= 360;
	}
	return diff;
}


/**
 * Convert note name, e.g. A#0 to equivalent MIDI note number.
 * Sharp (#) or flat (b) is optional.
 * Octave number is optional (default is 1).
 *
 * Returns -1 if note name is invalid.
 */
int Command::noteToNum(const char *name)
{
	int note = 60;
	bool rest = false;

	char letter = toupper(name[0]);
	switch (letter){
		case 'C': note += 0; break;
		case 'D': note += 2; break;
		case 'E': note += 4; break;
		case 'F': note += 5; break;
		case 'G': note += 7; break;
		case 'A': note += 9; break;
		case 'B': note += 11; break;
		case '-': rest = true; break;
		default:
			fprintf(stderr, "Bad note: %s\n", name);
			return -1;
	}

	int pos = 1;
	if (name[pos] == '#'){
		note += 1;
		pos++;
	}
	else if (name[pos] == 'b'){
		note -= 1;
		pos++;
	}

	if (name[pos] == '\0')
		return note;

	switch (name[pos]){
		case '0': note -= 12; break;
		case '1': note += 0; break;
		case '2': note += 12; break;
		case '3': note += 24; break;
		default:
			fprintf(stderr, "Bad note: %s\n", name);
			return -1;
	}

	pos++;
	if (name[pos] != '\0'){
		fprintf(stderr, "Bad note: %s\n", name);
		return -1;
	}

	if (rest)
		note = 0;

	if (note > 1 && note < 48){
		fprintf(stderr, "Note too low: %s\n", name);
		return -1;
	}

	if (note > 84){
		fprintf(stderr, "Note too high: %s\n", name);
		return -1;
	}

	return note;
}


/**
 * Returns remaining str or points to terminating '\0' if no more params.
 * Decimal (up to 3 d.p.) is converted to thousandths.
 * Returns NULL on error.
 */
char *Command::parseDecimal(char *pos, int param, int &value)
{
	value = 0;
	int millis = 0;
	bool isNegative = false;

	if (*pos == '-'){
		pos++;
		isNegative = true;
	}

	// Get the integer part
	while (isdigit(*pos)){
		value = 10 * value + (*pos - '0');
		pos++;
	}

	// Allow our European friends to use a comma instead of a dot
	if (*pos == '.' || *pos == ','){
		pos++;
		// Get the fraction part (up to 3 d.p.)
		int multiplier = 100;
		while (isdigit(*pos)){
			millis = millis + multiplier * (*pos - '0');
			multiplier /= 10;
			pos++;
		}
	}

	// No other chars allowed
	if (*pos != ' ' && *pos != '\0'){
		if (param == -1)
			fprintf(stderr, "Expression contains unexpected characters. Variable should start with $.\n");
		else if (param < 7)
			fprintf(stderr, "Parameter %d contains unexpected characters. Variable should start with $.\n", param+1);
		else
			fprintf(stderr, "Parameter contains unexpected characters\n");
		return NULL;
	}

	value = value * 1000 + millis;

	if (isNegative)
		value = -value;

	return pos;
}


/**
 *
 */
bool Command::setParamValues(int paramCount)
{
	for (int i = 0; i < paramCount; i++){
		if (param[i].isVariable()){
			if (!param[i].getVariable())
				return false;

			if (!validParam(i))
				return false;
		}
	}
	return true;
}


/**
 * Validate the param. If num is -1 then this is a variable
 * embedded within a string so ignore the command type and
 * make sure the param is a non-negative integer.
 */
bool Command::validParam(int num)
{
	bool validated = true;
	bool allowNegative = false;
	bool allowDecimal = false;
	bool isPin = false;
	bool isPwmPin = false;
	bool isVolume = false;
	bool isNote = false;
	int paramVal = param[num].getValue();

	if (num != -1){
		switch (type){
			case Wait_Press:
			case Wait_Release:
			case Wait_High:
			case Wait_Low:
			case Output_High:
			case Output_Low:
				switch (num){
					case 0: isPin = true; break;
					default: validated = false;
				}
				break;

			case Read_Input:
				switch (num){
					case 0: isPin = true; break;
					case 1: isPin = true; break;
					default: validated = false;
				}
				break;

			case Stop_Pwm:
				switch (num){
					case 0: isPwmPin = true; break;
					default: validated = false;
				}
				break;

			case Output_Pwm:
				switch (num){
					case 0: isPwmPin = true; break;
					case 1: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Flash:
				switch (num){
					case 0: isPwmPin = true; break;
					case 8: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Random_Pwm:
				switch (num){
					case 0: isPwmPin = true; break;
					case 1: allowDecimal = true; break;
					case 2: allowDecimal = true; break;
					case 3: allowDecimal = true; break;
					case 4: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Rangemap_Pwm:
				switch (num){
					case 0: isPwmPin = true; break;
					case 1: allowDecimal = true; break;
					case 2: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Random_Pwm_Loop:
			case Linear_Pwm:
			case Linear_Pwm_Wait:
			case Linear_Pwm_Loop:
				switch (num){
					case 0: isPwmPin = true; break;
					case 1: allowDecimal = true; break;
					case 2: allowDecimal = true; break;
					case 3: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Sine_Pwm:
			case Sine_Pwm_Wait:
			case Sine_Pwm_Loop:
				switch (num){
					case 0: isPwmPin = true; break;
					case 1: allowDecimal = true; break;
					case 2: allowDecimal = true; break;
					case 3: allowNegative = true; break;
					case 4: allowNegative = true; break;
					case 5: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Set:
			case If:
			case While:
			case For:
				switch (num){
					case 8: isPin = true; break;
					default: validated = false;
				}
				break;

			case Wait:
			case Set_Sync:
			case Wait_Sync:
				switch (num){
					case 0: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Play_From:
			case FadeOut:
				switch (num){
					case 0: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Sound_Buzzer:
				switch (num){
					case 8: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Volume:
				switch (num){
					case 0: isVolume = true; allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Play_Note:
				switch (num){
					case 0: isNote = true; break;
					case 1: allowDecimal = true; break;
					default: validated = false;
				}
				break;

			case Loop_Note:
				switch (num){
					case 0: isNote = true; break;
					default: validated = false;
				}
				break;

			case Play_Song:
				switch (num){
					case 0: isNote = true; break;
					case 1: allowDecimal = true; break;
					case 8:
						allowDecimal = true;
						if (paramVal == 0){
							fprintf(stderr, "Song speed must not be 0.\n");
							return false;
						}
						break;
					default: validated = false;
				}
				break;

			default: validated = false;
		}
	}

	if (!validated){
		fprintf(stderr, "Parameter %d was not validated\n", num+1);
		return false;
	}

	if (!allowNegative && paramVal < 0){
		char msgPrefix[256];
		if (num == -1)
			strcpy(msgPrefix, "Variables embedded in strings");
		else if (num < 7)
			sprintf(msgPrefix, "Parameter %d", num+1);
		else
			strcpy(msgPrefix, "Parameter");

		fprintf(stderr, "%s must not be negative. Found value = %s\n",
				msgPrefix, paramToStr(num));
		return false;
	}

	if (!allowDecimal){
		if (paramVal % 1000 != 0){
			char msgPrefix[256];
			if (num == -1)
				strcpy(msgPrefix, "Variables embedded in strings");
			else if (num < 7)
				sprintf(msgPrefix, "Parameter %d", num+1);
			else
				strcpy(msgPrefix, "Parameter");

			fprintf(stderr, "%s must not be a decimal. Found value = %s\n",
					msgPrefix, paramToStr(num));
			return false;
		}

		// Remove the decimal part of the param
		paramVal = paramVal / 1000;
		param[num].setValue(paramVal);
	}

	if (isPin && paramVal >= Gpio::MAX_PINS){
		fprintf(stderr, "GPIO pin number must be in the range 0 to %d. Found value = %d\n", Gpio::MAX_PINS - 1, paramVal);
		return false;
	}

	if (isPwmPin && paramVal >= Gpio::MAX_PWM_PINS){
		fprintf(stderr, "PWM pin number must be in the range 0 to %d. Found value = %d\n", Gpio::MAX_PWM_PINS - 1, paramVal);
		return false;
	}

	if (isVolume && paramVal > Audio::MAX_VOLUME * 1000){
		fprintf(stderr, "Volume must be in the range 0 to %d.0. Found value = %s\n", Audio::MAX_VOLUME, paramToStr(num));
		return false;
	}

	if (isNote && ((paramVal > 1 && paramVal < 48) || paramVal > 84))
	{
		fprintf(stderr, "Note must be in the range 48 (C0) to 84 (C3). Found value = %d\n", paramVal);
		return false;
	}

	return true;
}


/**
 *
 */
bool Command::validPwmParams()
{
	if (((type == Random_Pwm || isGpioSine())
			&& param[2].getValue() > Gpio::MAX_PWM_VALUE * 1000) ||
		(isGpioLinear() && (param[1].getValue() > Gpio::MAX_PWM_VALUE * 1000
			|| param[2].getValue() > Gpio::MAX_PWM_VALUE * 1000)))
	{
		fprintf(stderr, "PWM value must be in the range 0 to %d\n",
			Gpio::MAX_PWM_VALUE);
		return false;
	}

	if ((type == Output_Pwm && param[1].getValue() > Gpio::MAX_PWM_VALUE * 1000)
		|| (type == Rangemap_Pwm && param[2].getValue() > Gpio::MAX_PWM_VALUE * 1000))
	{
		fprintf(stderr, "PWM value must be in the range 0.0 to %d.0\n",
			Gpio::MAX_PWM_VALUE);
		return false;
	}

	if ((type == Random_Pwm || isGpioSine() || type == Rangemap_Pwm)
		&& param[1].getValue() >= param[2].getValue())
	{
		fprintf(stderr, "Min value must be less than max value\n");
		return false;
	}

	if (type == Random_Pwm && param[3].getValue() < 40){
		fprintf(stderr, "Minimum step value is 40ms (0.04).\n");
		return false;
	}

	if (type == Sine_Pwm || type == Sine_Pwm_Wait || type == Sine_Pwm_Loop){
		// Apply fiddle factor to min and max values
		adjustSine();
	}
	
	return true;
}


/**
 *
 */
char *Command::paramToStr(int num)
{
	static char strValue[256];
	int value = param[num].getValue();

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


/**
 *
 */
void Command::report()
{
	printf("\n");
	printf("Pioscript v2.0    Scott Vincent    April 2014    email: scottvincent@yahoo.com\n");
	printf("\n");
	printf("GPIO input and output pins should be specified using wiringPi numbering,\n");
	printf("i.e. pin 0 is physical pin 11 on the Raspberry Pi connector.\n");
	printf("\n");
	printf("PWM output uses ServoBlaster and only pins 0 to 16 may be used. Again,\n");
	printf("wiringPi numbering should be used.\n");
	printf("\n");
	printf("Mapping for WiringPi Pin to Physical Pin on Main Connector\n");
	printf("  0->11, 1->12, 2->13, 3->15, 4->16, 5->18, 6->22, 7->7, 8->3, 9->5, 10->24,\n");
	printf("  11->26, 12->19, 13->21, 14->23, 15->8, 16->10\n");
	printf("\n");
	printf("Mapping for WiringPi Pin to Physical Pin on Auxilliary Connector (Rev. 2 boards\n");
	printf("only)\n");
	printf("  17->3, 18->4, 19->5, 20->6\n");
	printf("\n");
	printf("For Reference (and special thanks):\n");
	printf("  Name: WiringPi\n");
	printf("  Author: Gordon Henderson\n");
	printf("  http://wiringpi.com\n");
	printf("\n");
	printf("  Name: ServoBlaster\n");
	printf("  Author: Richard Hirst\n");
	printf("  https://github.com/richardghirst/PiBits/tree/master/ServoBlaster\n");
	printf("\n");
	printf("When using PWM to drive LEDs a value of 0 sets the LED fully off (0%) and a\n");
	printf("value of 100 sets it fully on (100%).\n");
	printf("\n");
	printf("When using PWM to drive servos you should first use the output_pwm command to\n");
	printf("determine the minimum and maximum rotation values for your servo. You should\n");
	printf("then use the rangemap_pwm command to set the discovered range and then revert\n");
	printf("to using values between 0 and 100 for all PWM commands to this servo where a\n");
	printf("value of 0 will set the servo to its minimum rotation and a value of 100 will\n");
	printf("set the servo to its maximum rotation. Cycle time is 10ms so a good starting\n");
	printf("point is to set the minimum to 5 (=0.5ms) and maximum to 23 (=2.3ms).\n");
	printf("\n");
	printf("Important Note: There is no idle_timeout set on ServoBlaster so if you use\n");
	printf("the output_pwm command on a servo and leave it in the same position for long\n");
	printf("periods of time the servo may get warm. To protect your servo you should also\n");
	printf("use the stop_pwm command after a short delay.\n");
	printf("\n");
	printf("Important Note 2: If you are using the Pibrella add-on and use either the\n");
	printf("sound_buzzer, play_note or play_song commands you will no longer hear any sound\n");
	printf("samples play until you reboot your Pi. This is because the internal buzzer\n");
	printf("uses hardware PWM which interferes with the Pi's audio output.\n"); 
	printf("\n");
	printf("Command List\n");
	printf("============\n");
	printf("\n");
	printf("General\n");
	printf("-------\n");
	printf("  # Comment              This is a comment.\n");
	printf("  :main                  Define main activity. This activity is started when\n");
	printf("                         the script first runs.\n");
	printf("  :myjob                 Define a sub-activity.\n");
	printf("  start myjob            Start a sub-activity.\n");
	printf("  start_wait myjob       Start a sub-activity and wait for it to complete.\n");
	printf("  stop myjob             Interrupt and stop a sub-activity.\n");
	printf("  wait 1.5               Pause this activity for the specified number of\n");
	printf("                         seconds.\n");
	printf("  repeat                 Repeat this activity.\n");
	printf("  print My text          Print some text. Useful for debugging scripts.\n");
	printf("  print \"  My text\"      Use quotes to print leading spaces or blank lines.\n");
	printf("  print Value is $myvar  Print the value of a variable.\n");
	printf("  start_sync             Start or reset the global timer. This is useful for\n");
	printf("                         synchronising commands across concurrent activities.\n");
	printf("                         The current value of the global timer can be obtained\n");
	printf("                         using the sync() function.\n");
	printf("  set_sync 12.5          Set the global timer to the specified number of\n");
	printf("                         seconds. This is useful when testing a new script.\n");
	printf("  wait_sync 3.5          Wait for the global timer to reach the specified\n");
	printf("                         number of seconds. If the time has already passed the\n");
	printf("                         next command will be executed immediately.\n");
	printf("  exit                   Stop all activities and quit.\n");
	printf("  use_addon pibrella     Use the Pibrella add-on board. Instead of pin numbers\n");
	printf("                         you can use the following names: Button, Red, Amber,\n");
	printf("                         Green, InputA, InputB, InputC, InputD, OutputE,\n");
	printf("                         OutputF, OutputG, OutputH.\n");
	printf("\n");
	printf("Audio\n");
	printf("-----\n");
	printf("  play_sound mywavfile   Play a sound. Only 44.1 KHz 16-bit stereo WAV files\n");
	printf("                         are supported.\n");
	printf("  play_loop mywavfile    Play in a continuous loop.\n");
	printf("  play_wait mywavfile    Play a sound and wait for it to complete.\n");
	printf("  play_from 4.5 mywavfile\n");
	printf("                         Play a sound from the specified number of seconds\n");
	printf("                         after the start.\n");
	printf("  pause mywavfile        Pause a sound.\n");
	printf("  pause *                Pause all currently playing sounds.\n");
	printf("  resume mywavfile       Resume playing a sound.\n");
	printf("  resume *               Resume all currently paused sounds.\n");
	printf("  stop_sound mywavfile   Stop playing a sound.\n");
	printf("  stop_sound *           Stop all sounds.\n");
	printf("  volume 7.5 mywavfile   Set the volume of a sound in the range 0 to 10.\n");
	printf("  volume 10 *            Set the volume of all sounds.\n");
	printf("  fadeout 2.5 mywavfile  Fade a sound out over the specified number of seconds.\n");
	printf("  fadeout 5 *            Fade out all currently playing sounds over the\n");
	printf("                         specified number of seconds.\n");
	printf("  sound_buzzer 2         Pibrella only. Sound the buzzer for the specified\n");
	printf("                         number of seconds.\n");
	printf("  play_note 60 .5        Pibrella only. Play the specified MIDI note for the\n");
	printf("                         specified number of seconds. Note 60 is middle C. The\n");
	printf("                         buzzer is limited to 3 octaves so note must be between\n");
	printf("                         48 and 84.\n");
	printf("  play_note A#2 .5       You can use note names instead of MIDI note numbers.\n");
	printf("                         Specify A to G or - for a rest optionally followed by\n");
	printf("                         # (sharp) or b (flat) optionally followed by an octave\n");
	printf("                         number 0, 1, 2 or 3.\n");
	printf("  play_song 1 A#2,.5 C,.25 G -,2 Db0,4 etc.\n");
	printf("                         Plays the [note,length] sequence at speed 1. Use speed\n");
	printf("                         1 for normal length notes, 2 for double speed etc.\n");
	printf("\n");
	printf("GPIO\n");
	printf("----\n");
	printf("  wait_press 1           Wait for switch 1 to be pressed.\n");
	printf("                         Note: Internal pull-up resistor will be enabled or\n");
	printf("                         internal pull-down resistor if you are using a\n");
	printf("                         Pibrella add-on board.\n");
	printf("  wait_release 1         Wait for switch 1 to be released.\n");
	printf("  wait_high 1            Wait for input 1 to go high.\n");
	printf("  wait_low 1             Wait for input 1 to go low.\n");
	printf("  output_high 1          Set output 1 high.\n");
	printf("  output_low 1           Set output 1 low.\n");
	printf("  read_input 0 1         Uses a combination of input 0 and output 1 to read an\n");
	printf("                         analog input such as a photoresistor. The value is\n");
	printf("                         stored in the $input variable. See the 'photosensor'\n");
	printf("                         example script for more information.\n");
	printf("  output_pwm 1 50        Set output 1 to a PWM value in the range 0 to 100.\n");
	printf("  turn_on 1              Same as output_high 1. Use this for LEDs.\n");
	printf("  flash 1 1.5            Flash the LED every 1.5 seconds until it is turned off.\n");
	printf("  turn_off 1             Same as output_low 1.\n");
	printf("  set_power 1 50         Same as output_pwm 1 50. Use this for motors.\n");
	printf("  set_brightness 1 50    Same as output_pwm 1 50. Use this for LEDs.\n");
	printf("  set_position 1 50      Same as output_pwm 1 50. Use this for servos.\n");
	printf("  rangemap_pwm 1 2.5 12.5\n");
	printf("                         Setting output 1 to PWM value 0 will now equate to a\n");
	printf("                         real PWM value of 2.5 and setting it to PWM value 100\n");
	printf("                         wil now equate to a real PWM value of 12.5.\n");
	printf("  random_pwm 1 15 100 0.05 7.5\n");
	printf("                         Set output 1 to a random PWM value between 15 and 100\n");
	printf("                         every 50ms for a total duration of 7.5 seconds.\n");
	printf("  random_pwm_loop 1 15 100 0.05\n");
	printf("                         Set output 1 to a random PWM value between 15 and 100\n");
	printf("                         every 50ms continuously.\n");
	printf("  linear_pwm 1 5 95 7.5  Vary PWM linearly from 5 to 95 for a total duration\n");
	printf("                         of 7.5 seconds.\n");
	printf("  linear_pwm_wait 1 5 95 7.5\n");
	printf("                         Same as random_pwm but wait for the cycle to complete\n");
	printf("                         before executing the next command.\n");
	printf("  linear_pwm_loop 1 5 95 7.5\n");
	printf("                         Vary PWM linearly from 5 to 95 continuously with each\n");
	printf("                         cycle taking 7.5 seconds.\n");
	printf("  sine_pwm 1 15 85 0 360 7.5\n");
	printf("                         Vary PWM between 15 (at -90 degrees) and 85 (at 90\n");
	printf("                         degrees) using a sine wave that varies from 0 to 360\n");
	printf("                         degrees for a total duration of 7.5 seconds.\n");
	printf("  sine_pwm_wait 1 15 85 0 360 7.5\n");
	printf("                         Same as sine_pwm but wait for the cycle to complete\n");
	printf("                         before executing the next command.\n");
	printf("  sine_pwm_loop 1 15 85 0 360 7.5\n");
	printf("                         Vary PWM between 15 (at -90 degrees) and 85 (at 90\n");
	printf("                         degrees) using a sine wave that varies from 0 to 360\n");
	printf("                         degrees continuously with each cycle taking 7.5\n");
	printf("                         seconds.\n");
	printf("  stop_pwm 1             Stop PWM on output 1.\n");
	printf("\n");
	printf("VARIABLES\n");
	printf("---------\n");
	printf("  set a = 1              Creates a variable a and assigns value 1 to it.\n");
	printf("  set b = $a + 1         Creates a variable b and uses an expression to assign\n");
	printf("                         a value to it.\n");
	printf("  set a = $a + 1         Increments a variable by 1.\n");
	printf("  set $myvar = -2        A variable name can be any length. The $ is optional\n");
	printf("                         on the left of an assignment but must be used in an\n");
	printf("                         expression to distinguish variables from functions.\n");
	printf("  set a[1] = 2           Subscripts (arrays) are supported and don't have to be\n");
	printf("                         sequential.\n");
	printf("  set a[$b+1] = 2        An expression can be used for the subscript.\n");
	printf("\n");
	printf("SPECIAL VARIABLES\n");
	printf("-----------------\n");
	printf("  print $input           The $input variable stores the last value read by the\n");
	printf("                         read_input command.\n");
	printf("\n");
	printf("OPERATORS\n");
	printf("---------\n");
	printf("  set a = ($b - 1) * $c  Any of the mathematical operators +, -, *, / may be\n");
	printf("                         used in an expression.\n");
	printf("  set a = ($b > $c) and not $d\n");
	printf("                         Any of the logical operators =, <, >, !=, <=, >=, and,\n");
	printf("                         or, not may be used in an expression.\n");
	printf("  set a = ($b = true)    A logical expression evaluates to 1 (true) or 0\n");
	printf("                         false). You may also use the constants true and false.\n");
	printf("  set a = 3 + ($b >= $c) An expression can contain any mix of mathematical and\n");
	printf("                         logical operators.\n");
	printf("\n");
	printf("FUNCTIONS\n");
	printf("---------\n");
	printf("  sync()                 Returns the current value of the global timer.\n");
	printf("  int($a)                Rounds $a down to its integer value.\n");
	printf("  trunc($a)              Same as int function.\n");
	printf("  round($a)              Rounds $a up or down to its nearest integer value.\n");
	printf("  rand()                 Returns a random number in the range 0 to 0.999.\n");
	printf("  set a = 1 + int(10 * rand())\n");
	printf("                         Sets $a to a random integer between 1 and 10.\n");
	printf("  pressed(0)             Returns true if the specified button is currently\n");
	printf("                         being pressed. You must specify an actual pin number\n");
	printf("                         or Pibrella input name, not a variable or expression.\n");
	printf("  released(0)            Returns true if the specified button is not currently\n");
	printf("                         being pressed.\n");
	printf("\n");
	printf("CONDITIONALS\n");
	printf("---------\n");
	printf("  if $winner             Only executes the commands between if and else/end_if\n");
	printf("                         when the expression evaluates to true.\n");
	printf("  else                   Only executes the commands between else and end_if when\n");
	printf("                         the expression evaluates to false. The else command is\n");
	printf("                         optional within an if block.\n");
	printf("  end_if                 Marks the end of an if block.\n");
	printf("  while $a != 1          Continuously executes the commands between while and\n");
	printf("                         end_while until the expression evaluates to false.\n");
	printf("  end_while              Marks the end of a while block.\n");
	printf("  for $a=0 ($a < 10) $a=$a+1\n");
	printf("                         The for command expects exactly 3 parameters. If a\n");
	printf("                         parameter includes spaces you must enclose it in\n");
	printf("                         brackets. Param 1 is a set command to execute on intial\n");
	printf("                         entry to the for block. Param 2 is an expression to\n");
	printf("                         evaluate at the start of each iteration and param 3 is\n");
	printf("                         a set command to execute at the end of each iteration.\n");
	printf("  end_for                Marks the end of a for block.\n");
	printf("========================================\n");
	printf("\n");
	printf("Note: Any command that takes an output pin as its first parameter will also\n");
	printf("accept a range of pins or * for all pins, e.g.\n");
	printf("\n");
	printf("  linear_pwm 3-11 5 95 7.5\n");
	printf("                         Performs linear PWM output on all pins from 3 to 11.\n");
	printf("  output_high *          Sets all defined outputs high.\n");
	printf("\n");
}
