/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <list>
#include <algorithm>
#include "gpio.h"
#include "engine.h"
#include "variable.h"


// ServoBlaster Settings
const char Gpio::SB_DEVICE[] = "/dev/servoblaster";
const char Gpio::SB_COMMAND[] = "servod";
const int Gpio::SB_CYCLE_TIME = 10000; // Microseconds (= 10ms)
const char *Gpio::SB_PARAMS[] = {
	"--pcm",
//	"--idle-timeout=2000ms",	// Removed idle_timeout. Good for LEDs but
								// not so good for servos!
	"--min=0%",
	"--max=100%",
	"--debug",					// Run in foreground so we can kill it
	NULL
};

bool Gpio::mInit = false;
bool Gpio::mHwPwmInit = false;
bool Gpio::mUsePibrella = false;
pthread_t Gpio::mMonitorId;
pid_t Gpio::mServoBlasterPid = -1;
int Gpio::mServoBlasterOut = -1;
bool Gpio::mMonitorRunning = false;
int Gpio::mBuzzerClock = 0;
std::list<Gpio*> Gpio::mOutputList;
pthread_mutex_t Gpio::mMonMutex[Gpio::MAX_PWM_PINS];
std::list<Gpio*> Gpio::mMonitorList;
Gpio::MonitorType Gpio::mMonType[Gpio::MAX_PINS];
int Gpio::mValue[Gpio::MAX_PINS];
int Gpio::mLastValue[Gpio::MAX_PINS];
bool Gpio::mRecentChange[Gpio::MAX_PINS];
double Gpio::mStartTime[Gpio::MAX_PWM_PINS];
double Gpio::mChangeTime[Gpio::MAX_PINS];
int Gpio::mIntParam[Gpio::MAX_PWM_PINS][6];
bool Gpio::mLoop[Gpio::MAX_PWM_PINS];
int Gpio::mRangeMapMin[Gpio::MAX_PWM_PINS];
int Gpio::mRangeMapMax[Gpio::MAX_PWM_PINS];

 
/**
 * Global init to setup wiringPi library
 */
bool Gpio::init()
{
    if (wiringPiSetup() == -1){
        fprintf(stderr, "** Failed to initialize wiringPi\n");
        return 1;
    }

	srand(time(NULL));
	mInit = true;
	return true;
}


/**
 * Global cleanup to shutdown wiringPi library
 */
void Gpio::cleanup()
{
	if (mInit){
		// No cleanup needed for wiringPi
	}
}


/**
 * Start a thread to constantly monitor all the input pins we are
 * interested in. When a state change is detected we store the
 * time of the change.  This creates a buffer so we can still see
 * changes that happened a short time ago (so we don't miss any).
 *
 * It also writes PWM outputs and is responsible for writing random
 * data, sine waves etc. to these outputs. 
 */
bool Gpio::startMonitor()
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	// Do we have anything to monitor?
	if (mMonitorList.size() == 0)
		return true;

	bool usingPwm = false;
	for (std::list<Gpio*>::iterator iter =
		mMonitorList.begin(); iter != mMonitorList.end(); iter++)
	{
		Gpio *obj = *iter;
		int pin = obj->getPin();

		if (obj->getType() == PWM){
			// Don't start outputting PWM until we execute a relevant command
			mMonType[pin] = INACTIVE;
			mMonMutex[pin] = PTHREAD_MUTEX_INITIALIZER;

			// Set to full range initially (range map is in millis)
			mRangeMapMin[pin] = 0;
			mRangeMapMax[pin] = 100000;
			usingPwm = true;
		}
		else {
			// Input pins are always monitored
			initMonitorInput(pin);
		}
	}

	if (usingPwm){
		// Start ServoBlaster. We use this instead of hardware
		// PWM as there is only one hardware PWM pin on the Pi
		// and hardware PWM interferes with analog audio.
		if (!startServoBlaster())
			return false;
	}

	// Start monitor thread
	if (pthread_create(&mMonitorId, NULL, monitor, NULL) != 0){
		fprintf(stderr, "** Failed to start gpio monitor\n");
		return false;
	}

	// Wait for monitor to start
	for (int retries = 0; retries < 15; retries++){
		if (mMonitorRunning)
			return true;

		Engine::sleep(200);
	}

	printf("** Failed to start gpio monitor\n");
	return false;
}


/**
 *
 */
void Gpio::stopMonitor()
{
	if (mMonitorRunning){
		mMonitorRunning = false;
		void *status;
		if (pthread_join(mMonitorId, &status) != 0)
			fprintf(stderr, "** Failed to stop gpio monitor\n");
	}

	if (mServoBlasterPid != -1)
		stopServoBlaster();
}


/**
 *
 */
bool Gpio::startServoBlaster()
{
	// Stop ServoBlaster if it's already running
	mServoBlasterPid = Engine::findProcess(SB_COMMAND);
	if (mServoBlasterPid != -1)
		stopServoBlaster();

	printf("Starting ServoBlaster (%s)\n", SB_COMMAND);
	mServoBlasterPid = fork();
	if (mServoBlasterPid == -1){
		fprintf(stderr, "** Failed to create child process\n");
		return false;
	}

	if (mServoBlasterPid != 0){
		// This is the parent process

		// Make sure ServoBlaster started
		int status;
		for (int retries = 0; retries < 15; retries++){
			if (waitpid(mServoBlasterPid, &status, WNOHANG) != -1){
				sleep(1);
				mServoBlasterOut = open(SB_DEVICE, O_WRONLY|O_SYNC);
				if (mServoBlasterOut == -1){
					fprintf(stderr, "** Failed to open %s\n", SB_DEVICE);
					return false;
				}
				return true;
			}

			Engine::sleep(200);
		}
		fprintf(stderr, "ServoBlaster failed to start\n");
		return false;
	}

	// Make ServoBlaster use same pin mapping as wiringPi.
	// We are mapping wiringPi pin number to physical pin number.
	const int pinMap[] = {11,12,13,15,16,18,22,7,3,5,24,26,19,21,23,8,10};
	int servoBlasterPin[MAX_PWM_PINS];

	// ServoBlaster needs to know which pins we are using for PWM
	for (int i = 0; i < MAX_PWM_PINS; i++)
		servoBlasterPin[i] = 0;

	for (std::list<Gpio*>::iterator iter =
		mMonitorList.begin(); iter != mMonitorList.end(); iter++)
	{
		Gpio *obj = *iter;
		if (obj->getType() == PWM){
			int pin = obj->getPin();
			servoBlasterPin[pin] = pinMap[pin];
		}
	}

	// First arg is the pin list
	char argv1[256];
	sprintf(argv1, "--p1pins=%d", servoBlasterPin[0]);
	char addStr[64];
	for (int i = 1; i < MAX_PWM_PINS; i++){
		sprintf(addStr, ",%d", servoBlasterPin[i]);
		strcat(argv1, addStr);
	}

	// Second arg is the cycle time
	char argv2[256];
	sprintf(argv2, "--cycle-time=%dus", SB_CYCLE_TIME);

	const char *argv[16];
	argv[0] = SB_COMMAND;
	argv[1] = argv1;
	argv[2] = argv2;
	for (int i = 0;; i++){
		argv[i+3] = SB_PARAMS[i];
		if (SB_PARAMS[i] == NULL)
			break;
	}

	// redirect stdout and stderr
	freopen("/tmp/servoblaster.log", "w", stdout);
	freopen("/tmp/servoblaster.log", "w", stderr);

	// Try from current dir first
	char command[256];
	sprintf(command, "./%s", SB_COMMAND);
	execv(command, (char* const*)argv);
	// execv only returns if an error occurs

	// Now try from PATH
	execvp(SB_COMMAND, (char* const*)argv);
	// execv only returns if an error occurs

	fprintf(stderr, "** Failed to start ServoBlaster using command:\n");
	fprintf(stderr, "%s", SB_COMMAND);
	for (int i = 0; argv[i] != NULL; i++)
		fprintf(stderr, " %s", argv[i]);

	fprintf(stderr, "\n");
	return true;
}


/**
 *
 */
void Gpio::stopServoBlaster()
{
	if (Engine::displayOn)
		printf("Stopping ServoBlaster (%s)\n", SB_COMMAND);

	// Close device
	if (mServoBlasterOut != -1)
		close(mServoBlasterOut);

	// Kill process
	int rc = kill(mServoBlasterPid, SIGTERM);
	if (rc != 0)
		return;

	// Wait for process to stop
	int status;
	for (int retries = 0; retries < 15; retries++){
		if (waitpid(mServoBlasterPid, &status, WNOHANG) != 0)
			return;

		Engine::sleep(200);
	}

	fprintf(stderr, "ServoBlaster failed to stop\n");
}


/**
 * Runs in a separate thread
 */
void *Gpio::monitor(void *arg)
{
	mMonitorRunning = true;
	while (mMonitorRunning){
		double now = Engine::timeNow();
		for (std::list<Gpio*>::iterator iter =
			mMonitorList.begin(); iter != mMonitorList.end(); iter++)
		{
			Gpio *obj = *iter;
			int pin = obj->getPin();

			switch (mMonType[pin]) {
				case INACTIVE:
					break;

				case MON_IN:
					monitorInput(pin, now);
					break;

				case PWM_RANDOM:
					monitorPwmRandom(pin, now);
					break;

				case PWM_LINEAR:
					monitorPwmLinear(pin, now);
					break;

				case PWM_SINE:
					monitorPwmSine(pin, now);
					break;

				default:
					printf("Unknown monitor pin type\n", mMonType[pin]);
					mMonitorRunning = false;
					pthread_exit(NULL);
			}
		}
		Engine::sleep(10);
	}
	pthread_exit(NULL);
}


/**
 *
 */
void Gpio::initMonitorInput(int pin)
{
	// Initialise values for monitoring an input pin
	mValue[pin] = digitalRead(pin);
	mRecentChange[pin] = false;

	// Start monitoring the input pin
	mMonType[pin] = MON_IN;
}


/**
 *
 */
void Gpio::monitorInput(int pin, double now)
{
	int newValue = digitalRead(pin);
	if (newValue != mValue[pin]){
		mValue[pin] = newValue;
		mRecentChange[pin] = true;
		mChangeTime[pin] = now;
	}
	else if (mRecentChange[pin]){
		// Expire buffer after 100ms
		if ((now - mChangeTime[pin]) > 100)
			mRecentChange[pin] = false;
	}
}


/**
 *
 */
void Gpio::monitorPwmRandom(int pin, double now)
{
	pthread_mutex_lock(&mMonMutex[pin]);
	if (mMonType[pin] == INACTIVE){
		pthread_mutex_unlock(&mMonMutex[pin]);
		return;
	}

	// Get params
	int minVal = mIntParam[pin][1];
	int maxVal = mIntParam[pin][2];
	int stepTime = mIntParam[pin][3];
	int duration = mIntParam[pin][4];

	// Are we at the start or end of the cycle?
	if (mLastValue[pin] == -1){
		mStartTime[pin] = now;
		mChangeTime[pin] = 0;
	}
	else if (!mLoop[pin] && (now - mStartTime[pin]) > duration){
		mMonType[pin] = INACTIVE;
		pthread_mutex_unlock(&mMonMutex[pin]);
		return;
	}

	// Have we reached the next step yet?
	if ((now - mChangeTime[pin]) < stepTime){
		pthread_mutex_unlock(&mMonMutex[pin]);
		return;
	}

	// Write a new value
	mChangeTime[pin] = now;
	int value = minVal + (rand() % (1 + maxVal - minVal));
	if (mLastValue[pin] != value)
		writePwm(pin, value);

	pthread_mutex_unlock(&mMonMutex[pin]);
}


/**
 *
 */
void Gpio::monitorPwmLinear(int pin, double now)
{
	pthread_mutex_lock(&mMonMutex[pin]);
	if (mMonType[pin] == INACTIVE){
		pthread_mutex_unlock(&mMonMutex[pin]);
		return;
	}

	// Get params
	int startVal = mIntParam[pin][1];
	int endVal = mIntParam[pin][2];
	int duration = mIntParam[pin][3];

	// Are we at the start or end of the cycle?
	if (mLastValue[pin] == -1){
		mStartTime[pin] = now;
		mChangeTime[pin] = 0;
	}
	else if ((now - mStartTime[pin]) > duration){
		if (mLoop[pin]){
			// Start a new cycle
			mStartTime[pin] = now;
			mChangeTime[pin] = 0;
		}
		else {
			mMonType[pin] = INACTIVE;
			writePwm(pin, endVal);
			pthread_mutex_unlock(&mMonMutex[pin]);
			return;
		}
	}

	// Use a step time of 40ms
	if ((now - mChangeTime[pin]) < 40){
		pthread_mutex_unlock(&mMonMutex[pin]);
		return;
	}

	// Write a new value
	mChangeTime[pin] = now;
	int cyclePos = (int)(now - mStartTime[pin]);
	int value = startVal + (((endVal - startVal) * cyclePos) / duration);
	if (mLastValue[pin] != value)
		writePwm(pin, value);

	pthread_mutex_unlock(&mMonMutex[pin]);
}


/**
 *
 */
void Gpio::monitorPwmSine(int pin, double now)
{
	pthread_mutex_lock(&mMonMutex[pin]);
	if (mMonType[pin] == INACTIVE){
		pthread_mutex_unlock(&mMonMutex[pin]);
		return;
	}

	// Get params
	double minVal = mIntParam[pin][1];
	double maxVal = mIntParam[pin][2];
	double startAngle = ((double)mIntParam[pin][3] * M_PI) / 180.0;
	double endAngle = ((double)mIntParam[pin][4] * M_PI) / 180.0;
	int duration = mIntParam[pin][5];

	// Are we at the start or end of the cycle?
	if (mLastValue[pin] == -1){
		mStartTime[pin] = now;
		mChangeTime[pin] = 0;
	}
	else if ((now - mStartTime[pin]) > duration){
		if (mLoop[pin]){
			// Start a new cycle
			mStartTime[pin] = now;
			mChangeTime[pin] = 0;
		}
		else {
			// Finished
			double midPoint = (minVal + maxVal) / 2.0;
			double amplitude = (maxVal - minVal) / 2.0;
			int value = (int)(midPoint + amplitude * sin(endAngle) + 0.5);
			mMonType[pin] = INACTIVE;
			writePwm(pin, value);
			pthread_mutex_unlock(&mMonMutex[pin]);
			return;
		}
	}

	// Use a step time of 40ms
	if ((now - mChangeTime[pin]) < 40){
		pthread_mutex_unlock(&mMonMutex[pin]);
		return;
	}

	// Write a new value
	mChangeTime[pin] = now;
	double cyclePos = now - mStartTime[pin];
	double angle = startAngle + (((endAngle - startAngle) * cyclePos) / (double)duration);
	double midPoint = (minVal + maxVal) / 2.0;
	double amplitude = (maxVal - minVal) / 2.0;
	int value = (int)(midPoint + amplitude * sin(angle) + 0.5);
	if (mLastValue[pin] != value)
		writePwm(pin, value);

	pthread_mutex_unlock(&mMonMutex[pin]);
}


/**
 *
 */
void Gpio::writePwm(int pin, int milliPercent)
{
	// Convert value from percentage in millis to microseconds using range map
	int min = mRangeMapMin[pin];
	int max = mRangeMapMax[pin];
	double valueMillis = min + ((double)(max - min) * milliPercent) / 100000.0;
	int usecs = (int)((SB_CYCLE_TIME * valueMillis) / 100000.0);
	char outStr[64];
	sprintf(outStr, "%d=%dus\n", pin, usecs);
	write(mServoBlasterOut, outStr, strlen(outStr));
	mLastValue[pin] = milliPercent;
}


/**
 *
 */
void Gpio::resetPwm(int pin, int value)
{
	pthread_mutex_lock(&mMonMutex[pin]);
	mMonType[pin] = INACTIVE;

	if (value == -1){
		// Turn off PWM
		char outStr[64];
		sprintf(outStr, "%d=0us\n", pin);
		write(mServoBlasterOut, outStr, strlen(outStr));
		mLastValue[pin] = 0;
	}
	else
		writePwm(pin, value);

	pthread_mutex_unlock(&mMonMutex[pin]);
}


/**
 *
 */
bool Gpio::usePibrella()
{
	if (mOutputList.size() > 0 || mMonitorList.size() > 0){
		fprintf(stderr, "use_addon must appear in the script before any inputs or outputs are used.\n");
		return false;
	}

	mUsePibrella = true;
	return true;
}


/**
 *
 */
void Gpio::usePibrellaPwm()
{
	if (mHwPwmInit)
		return;

	// Setup hardware PWM for the buzzer (we need to control the frequency).
	// Note that the buzzer is on wiring pi pin 1 which is the only pin that
	// can be used for hardware PWM.

	pinMode(HW_PWM_PIN, PWM_OUTPUT);

	// Switch hardware PWM to mark-space mode
	pwmSetMode(PWM_MODE_MS);

	// Need a square wave so set range to 100 so value 50 = 50% duty cycle
	pwmSetRange(100);

	// Turn PWM off initially
	pwmWrite(HW_PWM_PIN, 0);

	mHwPwmInit = true;
}


/**
 * Constructor adds a new gpio pin we are interested in
 */
Gpio::Gpio(int pin, Type type)
{
	mPin = pin;
	mType = type;

	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return;
	}

	switch (type) {
		case IN_PIN:
			pinMode(pin, INPUT);
			// Disable internal pull-up/pull-down resistor
			pullUpDnControl(pin, PUD_OFF);
			mMonitorList.push_back(this);
			break;

		case SWITCH:
			pinMode(pin, INPUT);
			// Enable internal pull-up/pull-down resistor
			if (mUsePibrella)
				pullUpDnControl(pin, PUD_DOWN);
			else
				pullUpDnControl(pin, PUD_UP);

			mMonitorList.push_back(this);
			break;

		case OUT_PIN:
			pinMode(pin, OUTPUT);
			mOutputList.push_back(this);
			break;

		case PWM:
			pinMode(pin, OUTPUT);
			mMonitorList.push_back(this);
			break;

		default:
			fprintf(stderr, "Unknown pin type\n");
	}
}


/**
 * Destructor removes pin
 */
Gpio::~Gpio()
{
}


/**
 *
 */
const char *Gpio::getPinName()
{
	static char name[256];

	if (!mUsePibrella){
		sprintf(name , "%d", mPin);
		return name;
	}

	switch (mPin){
		case PIBRELLA_BUTTON:
			sprintf(name, "Button (%d)", mPin);
			return name;

		case PIBRELLA_INPUTA:
			sprintf(name, "InputA (%d)", mPin);
			return name;

		case PIBRELLA_INPUTB:
			sprintf(name, "InputB (%d)", mPin);
			return name;

		case PIBRELLA_INPUTC:
			sprintf(name, "InputC (%d)", mPin);
			return name;

		case PIBRELLA_INPUTD:
			sprintf(name, "InputD (%d)", mPin);
			return name;

		case PIBRELLA_RED:
			sprintf(name, "Red (%d)", mPin);
			return name;

		case PIBRELLA_AMBER:
			sprintf(name, "Amber (%d)", mPin);
			return name;

		case PIBRELLA_GREEN:
			sprintf(name, "Green (%d)", mPin);
			return name;

		case PIBRELLA_OUTPUTE:
			sprintf(name, "OutputE (%d)", mPin);
			return name;

		case PIBRELLA_OUTPUTF:
			sprintf(name, "OutputF (%d)", mPin);
			return name;

		case PIBRELLA_OUTPUTG:
			sprintf(name, "OutputG (%d)", mPin);
			return name;

		case PIBRELLA_OUTPUTH:
			sprintf(name, "OutputH (%d)", mPin);
			return name;
	}
}


/**
 *
 */
int Gpio::getPinFromName(const char *name)
{
	if (strcasecmp(name, "Button") == 0)
		return PIBRELLA_BUTTON;
	else if (strcasecmp(name, "InputA") == 0)
		return PIBRELLA_INPUTA;
	else if (strcasecmp(name, "InputB") == 0)
		return PIBRELLA_INPUTB;
	else if (strcasecmp(name, "InputC") == 0)
		return PIBRELLA_INPUTC;
	else if (strcasecmp(name, "InputD") == 0)
		return PIBRELLA_INPUTD;
	else if (strcasecmp(name, "Red") == 0)
		return PIBRELLA_RED;
	else if (strcasecmp(name, "Amber") == 0)
		return PIBRELLA_AMBER;
	else if (strcasecmp(name, "Green") == 0)
		return PIBRELLA_GREEN;
	else if (strcasecmp(name, "OutputE") == 0)
		return PIBRELLA_OUTPUTE;
	else if (strcasecmp(name, "OutputF") == 0)
		return PIBRELLA_OUTPUTF;
	else if (strcasecmp(name, "OutputG") == 0)
		return PIBRELLA_OUTPUTG;
	else if (strcasecmp(name, "OutputH") == 0)
		return PIBRELLA_OUTPUTH;

	return -1;
}


/**
 *
 */
void Gpio::outputAndPwm()
{
	mType = PWM;

    std::list<Gpio*>::iterator outIter = std::find(mOutputList.begin(), mOutputList.end(), this);
    if (outIter == mOutputList.end())
		mOutputList.push_back(this);

    std::list<Gpio*>::iterator monIter = std::find(mMonitorList.begin(), mMonitorList.end(), this);
    if (monIter == mMonitorList.end())
		mMonitorList.push_back(this);
}


/**
 *
 */
void Gpio::inputAndSwitch()
{
	mType = SWITCH;

	// Enable internal pull-up/pull-down resistor
	if (mUsePibrella)
		pullUpDnControl(mPin, PUD_DOWN);
	else
		pullUpDnControl(mPin, PUD_UP);
}


/**
 *
 */
bool Gpio::isState(int state)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	if (mType != IN_PIN && mType != SWITCH){
		fprintf(stderr, "Pin %d is not an input pin\n", mPin);
		return false;
	}

	// Does current state match?
	if (mValue[mPin] == state){
		// Clear the buffer to preserve state order
		mRecentChange[mPin] = false;
		return true;
	}

	// Does buffered state match? (state is buffered for up to 100ms)
	if (mRecentChange[mPin])
		return true;

	return false;
}


/**
 *
 */
bool Gpio::setState(int state)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	if (mType == PWM){
		if (state == 0)
			resetPwm(mPin, 0);
		else
			resetPwm(mPin, 100000);
	}
	else if (mType == OUT_PIN){
		digitalWrite(mPin, state);
	}
	else {
		fprintf(stderr, "Pin %d is not an output/PWM pin\n", mPin);
		return false;
	}

	return true;
}


/**
 *
 */
bool Gpio::setStateRange(int state, int startPin, int endPin)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	int pwmVal;
	if (state == 0)
		pwmVal = 0;
	else
		pwmVal = 100000;

	for (std::list<Gpio*>::iterator iter =
		mOutputList.begin(); iter != mOutputList.end(); iter++)
	{
		Gpio *obj = *iter;
		int pin = obj->getPin();
		if (obj->getType() == OUT_PIN && pin >= startPin && pin <= endPin)
			digitalWrite(obj->getPin(), state);
	}

	for (std::list<Gpio*>::iterator iter =
		mMonitorList.begin(); iter != mMonitorList.end(); iter++)
	{
		Gpio *obj = *iter;
		int pin = obj->getPin();
		if (obj->getType() == PWM && pin >= startPin && pin <= endPin){
			if (mLastValue[pin] != pwmVal)
				resetPwm(pin, pwmVal);
		}
	}

	return true;
}


/**
 * This reads an analog input, e.g. a photosensor by
 * setting the output low for 20ms (to short the capacitor)
 * and then timing how long it takes for the input to
 * go high (resistance is proportional to length of time
 * capacitor takes to charge up).
 */
bool Gpio::readInput(Gpio *gpioOut, int &value)
{
	digitalWrite(gpioOut->mPin, 0);
	Engine::sleep(40);

	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 20;
	int maxWait = 500;

	double startTime = Engine::timeNow();
	digitalWrite(gpioOut->mPin, 1);

	// Wait for input to go high
	while (digitalRead(mPin) == 0 && maxWait > 0){
		nanosleep(&ts, NULL);
		maxWait--;
	}

	double endTime = Engine::timeNow();
	value = (int)(100 * (endTime - startTime));

	// Fiddle value so to 0 = completely dark and
	// 100 = completely light. Also, use a square
	// so it is more sensitive to lighter values.
	if (value < 10)
		value = 100;
	else if (value > 4000)
		value = 0;
	else
		value = (int)(25000000.0 / ((value + 490.0) * (value + 490.0)));

	return true;
}


/**
 *
 */
bool Gpio::setPwm(int milliVal)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	if (mType != PWM){
		fprintf(stderr, "Pin %d is not a PWM pin\n", mPin);
		return false;
	}

	resetPwm(mPin, milliVal);
	return true;
}


/**
 *
 */
bool Gpio::setPwmRange(int milliVal, int startPin, int endPin)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	for (std::list<Gpio*>::iterator iter =
		mMonitorList.begin(); iter != mMonitorList.end(); iter++)
	{
		Gpio *obj = *iter;
		int pin = obj->getPin();
		if (mMonType[pin] != MON_IN && pin >= startPin && pin <= endPin){
			resetPwm(pin, milliVal);
		}
	}
	return true;
}


/**
 *
 */
bool Gpio::startPwm(MonitorType pwmType,
	int param1, int param2, int param3, int param4, int param5, bool loop)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	if (mType != PWM){
		fprintf(stderr, "Pin %d is not a PWM pin\n", mPin);
		return false;
	}

	pthread_mutex_lock(&mMonMutex[mPin]);

	// Initialise values for writing to a pwm pin
	mLastValue[mPin] = -1;
	mIntParam[mPin][1] = param1;
	mIntParam[mPin][2] = param2;
	mIntParam[mPin][3] = param3;
	mIntParam[mPin][4] = param4;
	mIntParam[mPin][5] = param5;
	mLoop[mPin] = loop;

	// Start writing to the pin
	mMonType[mPin] = pwmType;
	pthread_mutex_unlock(&mMonMutex[mPin]);
	return true;
}


/**
 *
 */
bool Gpio::startPwmRange(MonitorType pwmType, int startPin, int endPin,
	int param1, int param2, int param3, int param4, int param5, bool loop)
{
	bool res = true;
	for (std::list<Gpio*>::iterator iter =
		mMonitorList.begin(); iter != mMonitorList.end(); iter++)
	{
		Gpio *obj = *iter;
		int pin = obj->getPin();
		if (mMonType[pin] != MON_IN && pin >= startPin && pin <= endPin){
			if (!obj->startPwm(pwmType, param1, param2, param3, param4,
					param5, loop))
			{
				res = false;
			}
		}
	}
	return res;
}


/**
 *
 */
bool Gpio::stopPwm()

{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	if (mType != PWM){
		fprintf(stderr, "Pin %d is not a PWM pin\n", mPin);
		return false;
	}

	resetPwm(mPin, -1);
	return true;
}


/**
 *
 */
bool Gpio::stopPwmRange(int startPin, int endPin)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	for (std::list<Gpio*>::iterator iter =
		mMonitorList.begin(); iter != mMonitorList.end(); iter++)
	{
		Gpio *obj = *iter;
		int pin = obj->getPin();
		if (mMonType[pin] != MON_IN && pin >= startPin && pin <= endPin)
			resetPwm(pin, -1);
	}
	return true;
}


/**
 *
 */
bool Gpio::setRangeMap(int min, int max)
{
	if (!mInit){
		fprintf(stderr, "GPIO not initialised\n");
		return false;
	}

	if (mType != PWM){
		fprintf(stderr, "Pin %d is not a PWM pin\n", mPin);
		return false;
	}

	mRangeMapMin[mPin] = min;
	mRangeMapMax[mPin] = max;
	return true;
}


/**
 * Plays the specified MIDI note number where 60 = Middle C, 69 = A (440 Hz).
 * Note 0 is a rest (silence) and note 1 is a buzz (low freq).
 */
void Gpio::startBuzzer(int note)
{
	if (!mHwPwmInit)
		usePibrellaPwm();

	if (note == 0){
		pwmWrite(HW_PWM_PIN, 0);
		mBuzzerClock = 0;
		return;
	}

	// Convert MIDI note number to frequency
	double freq;
	if (note == 1)
		freq = 20;
	else
		freq = 440.0 * pow(2.0, ((note - 69) / 12.0));

	// Fiddle the clock value so that it sounds good from 1 octave below
	// middle C to one octave above, i.e. 3 full octaves.
	int clock = (int)(60000.0 / freq);
	if (mBuzzerClock != clock){
		pwmSetClock(clock);
		pwmWrite(HW_PWM_PIN, 50);
		mBuzzerClock = clock;
	}
}


/**
 *
 */
void Gpio::stopBuzzer()
{
	pwmWrite(HW_PWM_PIN, 0);
	mBuzzerClock = 0;
}
