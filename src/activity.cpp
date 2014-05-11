/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <time.h>
#include <list>
#include "activity.h"
#include "engine.h"
#include "method.h"
#include "command.h"
#include "parameter.h"
#include "variable.h"
#include "audio.h"
#include "gpio.h"


/**
 *
 */
Activity::Activity(Method *method)
{
	mMethod = method;
	mIter = method->commands.begin();
	mNextAdvance = 0;
	mNewActivity = NULL;
	mFinished = false;
	mExit = false;
	mWaiting = false;
	mWaitingActivity = NULL;
	mActive = 0;
}


/**
 *
 */
bool Activity::isReady(double now)
{
	// If an activity is always active it may be looping.
	// We keep a count of how active it is. Whenever the
	// activity is waiting we can reset the active count.
	if (now < mNextAdvance){
		mActive = 0;
		return false;
	}

	mActive++;
	return true;
}


/**
 *
 */
bool Activity::advance(double now)
{
	mNewActivity = NULL;

	// Have we finished?
	if (mIter == mMethod->commands.end()){
		mFinished = true;
		return true;
	}

	Command *command = *mIter;

	// Are we waiting for something?
	if (mWaiting){
		// See if wait condition is satisfied
		if (!wait(command, now)){
			fprintf(stderr, "** Command failed at line %d <%s>\n",
					command->lineNum, command->lineStr);
			return false;
		}

		if (!mWaiting)
			mIter++;

		return true;
	}

	mIter++;

	// Check for exit (stop all activity)
	if (command->type == Command::Exit){
		mExit = true;
		return false;
	}

	// Execute and check for failure
	if (!exec(command, now)){
		fprintf(stderr, "** Command failed at line %d <%s>\n",
				command->lineNum, command->lineStr);
		return false;
	}

	return true;
}


/**
 *
 */
bool Activity::exec(Command *command, double now)
{
	Audio *audio;
	Gpio *gpio;
	int wantedState;
	int value;
	bool condition;

	switch (command->type){

        case Command::Play_Sound:
			audio = (Audio *)command->object;
			return audio->play(0);

        case Command::Play_Loop:
			audio = (Audio *)command->object;
			return audio->play(-1);

        case Command::Play_Wait:
			audio = (Audio *)command->object;
			if (!audio->play(0))
				return false;

			mIter--;
			mWaiting = true;
			// Wait for 40ms before checking wait condition
			mNextAdvance = now + 40;
			return true;

        case Command::Play_From:
			if (!command->setParamValues(1))
				return false;

			audio = (Audio *)command->object;
			return audio->playFrom(command->param[0].getValue());

        case Command::Pause:
			audio = (Audio *)command->object;
			if (audio)
				return audio->pause();
			else
				return Audio::pauseAll();

        case Command::Resume:
			audio = (Audio *)command->object;
			if (audio)
				return audio->resume();
			else
				return Audio::resumeAll();

        case Command::Stop_Sound:
			audio = (Audio *)command->object;
			if (audio)
				return audio->halt();
			else
				return Audio::haltAll();

        case Command::Volume:
			if (!command->setParamValues(1))
				return false;

			audio = (Audio *)command->object;
			if (audio)
				return audio->volume(command->param[0].getValue());
			else
				return Audio::volumeAll(command->param[0].getValue());

        case Command::FadeOut:
			if (!command->setParamValues(1))
				return false;

			audio = (Audio *)command->object;
			if (audio)
				return audio->fadeout(command->param[0].getValue());
			else
				return Audio::fadeoutAll(command->param[0].getValue());

		case Command::Start_Recording:
			if (!Engine::recorder->startStream())
				return false;

			mIter--;
			mWaiting = true;
			// Wait for 30ms before checking wait condition
			mNextAdvance = now + 30;
			return true;

		case Command::Stop_Recording:
			return Engine::recorder->stopStream();

		case Command::Play_Recording:
			audio = (Audio *)command->object;
			return Engine::recorder->playWAV(audio);

		case Command::Play_Recording_Wait:
			audio = (Audio *)command->object;
			if (!Engine::recorder->playWAV(audio))
				return false;

			mIter--;
			mWaiting = true;
			// Wait for 40ms before checking wait condition
			mNextAdvance = now + 40;
			return true;

		case Command::Save_Recording:
			audio = (Audio *)command->object;
			return Engine::recorder->saveWAV(command->nameParam.getStrValue().c_str(), audio);

        case Command::Wait_Low:
			gpio = (Gpio *)command->object;
			if (gpio->isState(0))
				return true;

			mIter--;
			mWaiting = true;
			mNextAdvance = now + 40;
			return true;

        case Command::Wait_High:
			gpio = (Gpio *)command->object;
			if (gpio->isState(1))
				return true;

			mIter--;
			mWaiting = true;
			mNextAdvance = now + 40;
			return true;

        case Command::Wait_Press:
			gpio = (Gpio *)command->object;
			if (Gpio::usingPibrella())
				wantedState = 1;
			else
				wantedState = 0;

			if (gpio->isState(wantedState))
				return true;

			mIter--;
			mWaiting = true;
			mNextAdvance = now + 40;
			return true;

        case Command::Wait_Release:
			gpio = (Gpio *)command->object;
			if (Gpio::usingPibrella())
				wantedState = 0;
			else
				wantedState = 1;

			if (gpio->isState(wantedState))
				return true;

			mIter--;
			mWaiting = true;
			mNextAdvance = now + 40;
			return true;

        case Command::Read_Input:
			gpio = (Gpio *)command->object;
			int value;
			if (!gpio->readInput((Gpio *)command->funcObject[0], value))
				return false;

			Variable::simpleSet("input", value * 1000);
			return true;

        case Command::Output_High:
			gpio = (Gpio *)command->object;
			if (gpio)
				return gpio->setState(1);
			else
				return Gpio::setStateRange(1, command->param[0].getValue(),
						command->param[7].getValue());

        case Command::Output_Low:
			gpio = (Gpio *)command->object;
			if (gpio)
				return gpio->setState(0);
			else
				return Gpio::setStateRange(0, command->param[0].getValue(),
						command->param[7].getValue());

        case Command::Output_Pwm:
			if (!command->setParamValues(2))
				return false;

			if (!command->validPwmParams())
				return false;

			gpio = (Gpio *)command->object;
			if (gpio)
				return gpio->setPwm(command->param[1].getValue());
			else
				return Gpio::setPwmRange(command->param[1].getValue(),
										 command->param[0].getValue(),
										 command->param[7].getValue());

        case Command::Rangemap_Pwm:
			if (!command->setParamValues(3))
				return false;

			if (!command->validPwmParams())
				return false;

			gpio = (Gpio *)command->object;
			return gpio->setRangeMap(command->param[1].getValue(),
					command->param[2].getValue());

        case Command::Random_Pwm:
        case Command::Random_Pwm_Loop:
			if (!command->setParamValues(6))
				return false;

			if (!command->validPwmParams())
				return false;

			gpio = (Gpio *)command->object;
			if (gpio)
				return gpio->startPwm(Gpio::PWM_RANDOM,
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), command->isGpioLoop());
			else
				return Gpio::startPwmRange(Gpio::PWM_RANDOM,
					command->param[0].getValue(), command->param[7].getValue(),
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), command->isGpioLoop());

        case Command::Linear_Pwm:
        case Command::Linear_Pwm_Wait:
        case Command::Linear_Pwm_Loop:
			if (!command->setParamValues(6))
				return false;

			if (!command->validPwmParams())
				return false;

			gpio = (Gpio *)command->object;
			if (command->isGpioWait())
				mNextAdvance = now + command->param[3].getValue();

			if (gpio)
				return gpio->startPwm(Gpio::PWM_LINEAR,
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), command->isGpioLoop());
			else
				return Gpio::startPwmRange(Gpio::PWM_LINEAR,
					command->param[0].getValue(), command->param[7].getValue(),
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), command->isGpioLoop());

        case Command::Sine_Pwm:
        case Command::Sine_Pwm_Wait:
        case Command::Sine_Pwm_Loop:
			if (!command->setParamValues(6))
				return false;

			if (!command->validPwmParams())
				return false;

			gpio = (Gpio *)command->object;
			if (command->isGpioWait())
				mNextAdvance = now + command->param[3].getValue();

			if (gpio)
				return gpio->startPwm(Gpio::PWM_SINE,
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), command->isGpioLoop());
			else
				return Gpio::startPwmRange(Gpio::PWM_SINE,
					command->param[0].getValue(), command->param[7].getValue(),
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), command->isGpioLoop());

        case Command::Flash:
			if (!command->setParamValues(9))
				return false;

			if (!command->validPwmParams())
				return false;

			gpio = (Gpio *)command->object;
			command->param[5].setValue(command->param[8].getValue());
			if (gpio)
				return gpio->startPwm(Gpio::PWM_SINE,
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), true);
			else
				return Gpio::startPwmRange(Gpio::PWM_SINE,
					command->param[0].getValue(), command->param[7].getValue(),
					command->param[1].getValue(), command->param[2].getValue(),
					command->param[3].getValue(), command->param[4].getValue(),
					command->param[5].getValue(), true);

        case Command::Stop_Pwm:
			gpio = (Gpio *)command->object;
			if (gpio)
				return gpio->stopPwm();
			else
				return Gpio::stopPwmRange(command->param[0].getValue(),
							command->param[7].getValue());

        case Command::Play_Note:
			if (!command->setParamValues(2))
				return false;

			playNote(command, now);
			mIter--;
			mWaiting = true;
			return true;

        case Command::Loop_Note:
			if (!command->setParamValues(1))
				return false;

			Gpio::startBuzzer(command->param[0].getValue());
			return true;

        case Command::Stop_Note:
			Gpio::stopBuzzer();
			return true;

        case Command::Sound_Buzzer:
			if (!command->setParamValues(9))
				return false;

			command->param[1].setValue(command->param[8].getValue());
			playNote(command, now);
			mIter--;
			mWaiting = true;
			return true;

        case Command::Play_Song:
			if (!command->setParamValues(9))
				return false;

			// Move to start of song
			command->param[3].setValue(0);
			playNextNote(command, now);
			mIter--;
			mWaiting = true;
			return true;

		case Command::Start:
			mNewActivity = (Method *)command->object;
			return true;

		case Command::Start_Wait:
			mNewActivity = (Method *)command->object;
			mIter--;
			mWaiting = true;
			mSignalled = false;
			// Wait for 40ms before checking wait condition
			mNextAdvance = now + 40;
			return true;

		case Command::Stop:
			mStopActivity = (Method *)command->object;
			return true;

		case Command::Wait:
			if (!command->setParamValues(1))
				return false;

			mNextAdvance = now + command->param[0].getValue();
			return true;

		case Command::Repeat:
			mIter = mMethod->commands.begin();
			return true;

		case Command::Print:
			char strValue[512];
			command->nameParam.getFullStrValue(strValue);
			printf("%s\n", strValue);
			return true;

		case Command::Start_Sync:
			Engine::syncTimer = now;
			return true;

		case Command::Set_Sync:
			if (!command->setParamValues(1))
				return false;

			Engine::syncTimer = now - command->param[0].getValue();
			return true;

		case Command::Wait_Sync:
			if (!command->setParamValues(1))
				return false;

			mNextAdvance = Engine::syncTimer + command->param[0].getValue();
			return true;

		case Command::Set:
			return Variable::set(command->param[0].getStrValue(),
								 command->param[1].getStrValue(),
								 command->funcObject);
	
		case Command::If:
			if (!Variable::evaluate(command->nameParam.getStrValue(),
									command->funcObject, value))
				return false;

			condition = (value != 0);
			if (!condition){
				// Jump to line after Else
				jumpForwards(command->nestEndLine);
			}
			return true;

		case Command::Else:
			// Jump to line after End_If
			jumpForwards(command->nestEndLine);
			return true;

		case Command::End_If:
			return true;

		case Command::While:
			if (!Variable::evaluate(command->nameParam.getStrValue(),
									command->funcObject, value))
				return false;

			condition = (value != 0);
			if (!condition){
				// Jump to line after End_While
				jumpForwards(command->nestEndLine);
			}
			return true;

		case Command::End_While:
			// Jump to While
			jumpBackwards(command->nestStartLine);
			return true;

		case Command::For:
			if (command->inFor){
				if (!Variable::set(command->param[2].getStrValue(),
								   command->param[3].getStrValue(),
								   command->funcObject))
					return false;
			}
			else {
				command->inFor = true;
				if (!Variable::set(command->param[0].getStrValue(),
								   command->param[1].getStrValue(),
								   command->funcObject))
					return false;
			}

			if (!Variable::evaluate(command->nameParam.getStrValue(),
									command->funcObject, value))
				return false;

			condition = (value != 0);
			if (!condition){
				command->inFor = false;
				// Jump to line after End_For
				jumpForwards(command->nestEndLine);
			}
			return true;

		case Command::End_For:
			// Jump to For
			jumpBackwards(command->nestStartLine);
			return true;

		case Command::Clear_Screen:
			Engine::displayOn = false;
			printf("\033c");
			return true;

		default:
			// Not implemented yet!
			printf("No-op at line %d\n", command->lineNum);
			return true;
	}
}


/**
 * Move on to line after the one specified (may be past end)
 */
void Activity::jumpForwards(int lineNum)
{
	while (true){
		Command *command = *mIter;
		if (command->lineNum == lineNum)
			break;

		mIter++;
	}
	mIter++;
}


/**
 * Move back to specified line
 */
void Activity::jumpBackwards(int lineNum)
{
	mIter--;

	while (true){
		mIter--;
		Command *command = *mIter;
		if (command->lineNum == lineNum)
			break;
	}
}

/**
 * Checks to see if wait condition is satisfied yet
 */
bool Activity::wait(Command *command, double now)
{
	Audio *audio;
	Gpio *gpio;
	int wantedState;

	switch (command->type){

        case Command::Play_Wait:
        case Command::Play_Recording_Wait:
			audio = (Audio *)command->object;
			if (!audio->isPlaying())
			{
				mWaiting = false;
				return true;
			}
			// Check again in 40ms time
			mNextAdvance = now + 40;
			return true;

		case Command::Start_Recording:
			if (Engine::recorder->isStarted())
			{
				mWaiting = false;
				return true;
			}
			// Check again in 30ms time
			mNextAdvance = now + 30;
			return true;

        case Command::Wait_Low:
			gpio = (Gpio *)command->object;
			if (gpio->isState(0))
				mWaiting = false;
			else
				mNextAdvance = now + 40;

			return true;

        case Command::Wait_High:
			gpio = (Gpio *)command->object;
			if (gpio->isState(1))
				mWaiting = false;
			else
				mNextAdvance = now + 40;

			return true;

        case Command::Wait_Press:
			gpio = (Gpio *)command->object;
			if (Gpio::usingPibrella())
				wantedState = 1;
			else
				wantedState = 0;

			if (gpio->isState(wantedState))
				mWaiting = false;
			else
				mNextAdvance = now + 40;

			return true;

        case Command::Wait_Release:
			gpio = (Gpio *)command->object;
			if (Gpio::usingPibrella())
				wantedState = 0;
			else
				wantedState = 1;

			if (gpio->isState(wantedState))
				mWaiting = false;
			else
				mNextAdvance = now + 40;

			return true;

		case Command::Start_Wait:
			// Has the engine signalled us yet?
			if (mSignalled){
				mWaiting = false;
				return true;
			}
			// Check again in 40ms time
			mNextAdvance = now + 40;
			return true;

        case Command::Play_Note:
        case Command::Sound_Buzzer:
			if (command->param[2].getValue() == 0){
				// Silence done
				mWaiting = false;
			}
			else {
				// Stop the note and start the silence
				Gpio::stopBuzzer();
				mNextAdvance = now + command->param[2].getValue();
				command->param[2].setValue(0);
			}
			return true;

        case Command::Play_Song:
			if (command->param[2].getValue() == 0){
				// Silence done
				if (!playNextNote(command, now))
					mWaiting = false;
			}
			else {
				// Stop the note and start the silence
				Gpio::stopBuzzer();
				mNextAdvance = now + command->param[2].getValue();
				command->param[2].setValue(0);
			}
			return true;

		default:
			printf("No-op wait at line %d\n", command->lineNum);
			return true;
	}
}


/*
 *
 */
bool Activity::playNextNote(Command *command, double now)
{
	char strValue[512];
	command->nameParam.getFullStrValue(strValue);
	char *pos = &(strValue[command->param[3].getValue()]);
	pos = command->addNoteParam(0, pos, true);
	if (command->param[0].getValue() == -1)
		return false;
		
	pos = command->addNoteLen(1, pos, command->param[8].getValue());
	if (pos == NULL)
		return false;

	// Store position of next note
	command->param[3].setValue(pos - strValue);

	playNote(command, now);
	return true;
}


/*
 *
 */
void Activity::playNote(Command *command, double now)
{
	Gpio::startBuzzer(command->param[0].getValue());

	// Replace very end of note with silence so the
	// same note can be played in a row.
	int noteLen = command->param[1].getValue();
	int silence;
	if (noteLen > 25)
		silence = 25;
	else if (noteLen > 10)
		silence = 10;
	else
		silence = 1;

	mNextAdvance = now + noteLen - silence;
	// Use param 2 to store silence duration
	command->param[2].setValue(silence);
}
