/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * A Command is a single command line as read from
 * the script. It contains the parameters specified
 * for that command and, if it acts on an object such
 * as another method, audio resource or gpio resource
 * it contains a pointer to that other object.
 */

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <string.h>
#include "parameter.h"


class Command {

public:
	enum Type {
		Play_Sound,
		Play_Loop,
		Play_Wait,
		Play_From,
		Pause,
		Resume,
		Stop_Sound,
		Volume,
		FadeOut,
		Record_Sound,
		Start_Recording,
		Stop_Recording,
		Start_Recording_Wait,
		Start_Listening_Wait,
		Recording_Level,
		Play_Recording,
		Play_Recording_Wait,
		Save_Recording,
		Use_Addon,
		Play_Note,
		Loop_Note,
		Stop_Note,
		Play_Song,
		Sound_Buzzer,
		Wait_Press,
		Wait_Release,
		Wait_High,
		Wait_Low,
		Read_Input,
		Output_High,
		Output_Low,
		Output_Pwm,
		Output_Pwm_Loop,
		Rangemap_Pwm,
		Random_Pwm,
		Random_Pwm_Loop,
		Linear_Pwm,
		Linear_Pwm_Wait,
		Linear_Pwm_Loop,
		Sine_Pwm,
		Sine_Pwm_Wait,
		Sine_Pwm_Loop,
		Stop_Pwm,
		Flash,
		Start,
		Start_Wait,
		Stop,
		Wait,
		Repeat,
		Print,
		Start_Sync,
		Set_Sync,
		Wait_Sync,
		Set,
		If,
		Else,
		End_If,
		While,
		End_While,
		For,
		End_For,
		Clear_Screen,
		Exit
	};

	Type type;
	int lineNum;
	char lineStr[80];
	void *object;
	Parameter nameParam;
	Parameter param[9];
	int nestStartLine;
	int nestEndLine;
	bool inFor;
	int nextFuncObject;
	void *funcObject[9];

private:
	bool mIsValid;

public:
	Command(char *name, int lineNum, char *lineStr);
	bool isValid() { return mIsValid; };
	bool validateParams(char *params);
	bool isMethod();
	bool isAudio();
	bool isRecording();
	bool isGpio();
	bool isGpioSwitch();
	bool isGpioInput();
	bool isGpioOutput();
	bool isGpioPwm();
	bool isGpioLinear();
	bool isGpioSine();
	bool isGpioLoop();
	bool isGpioWait();
	bool isStartNest();
	bool isEndNest();
	bool isGlobal() { return (nameParam.getStrValue() == "*"); };
	char *addPinParam(int num, char *params);
	char *addNoteParam(int num, char *params, bool isSong);
	char *addNoteLen(int num, char *params, int speed);
	static char *parseDecimal(char *str, int num, int &value);
	bool setParamValues(int paramCount);
	bool validParam(int num);
	bool validPwmParams();
	char *paramToStr(int num);
	static void reportAdvanced();
	static void reportPibrella();

private:
	char *addParam(int num, char *params);
	char *addStrParam(int num, char *params);
	char *addSetParams(int num, char *params);
	char *addForParams(char *params);
	char *addPinRangeParam(int num, char *params, bool isPwm);
	void adjustSine();
	int angleMinDiff(int angle1, int angle2);
	int noteToNum(const char *name);
};

#endif
