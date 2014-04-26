/****************************************
 * Written by Scott Vincent
 * 26th February 2014
 *
 * Copyright (c) 2014 S.L. Vincent
 ****************************************
 *
 * A Gpio object is the internal representation of
 * a single gpio pin. The gpio object intialises the
 * pin with the desired state (input or output) when
 * it is created. The engine makes calls to the gpio 
 * object at execution time to query/change its state.
 *
 * There is a monitor that, once started, runs continuously
 * in the background (on a separate thread) to read inputs
 * and write PWM outputs. It buffers the inputs to ensure
 * that none get missed.
 *
 * This class also performs some global initialisation
 * of the gpio sub-system.
 */

#ifndef __GPIO_H__
#define __GPIO_H__

#include <list>
#include <wiringPi.h>


class Gpio {

public:
	enum PIBRELLA {
		PIBRELLA_BUTTON = 14,
		PIBRELLA_INPUTA = 13,
		PIBRELLA_INPUTB = 11,
		PIBRELLA_INPUTC = 10,
		PIBRELLA_INPUTD = 12,
		PIBRELLA_RED = 2,
		PIBRELLA_AMBER = 0,
		PIBRELLA_GREEN = 7,
		PIBRELLA_OUTPUTE = 3,
		PIBRELLA_OUTPUTF = 4,
		PIBRELLA_OUTPUTG = 5,
		PIBRELLA_OUTPUTH = 6
	};

	// For type Switch we enable the internal pull-up/pull-down resistor
	enum Type { IN_PIN, SWITCH, OUT_PIN, PWM };
	enum State { PIN_HIGH, PIN_LOW };
	enum MonitorType { INACTIVE, MON_IN, PWM_RANDOM, PWM_LINEAR, PWM_SINE };

	// wiringPi pins are numbered from 0 to 20
	static const int MAX_PINS = 21;
	static const int MAX_PWM_PINS = 17;
	static const int MAX_PWM_VALUE = 100;
	static const int HW_PWM_PIN = 1;

private:
	static const char SB_DEVICE[];
	static const char SB_COMMAND[];
	static const int SB_CYCLE_TIME;
	static const char *SB_PARAMS[];
	static bool mHwPwmInit;

public:
	static bool mInit;
	static bool mUsePibrella;
	static pthread_t mMonitorId;
	static pid_t mServoBlasterPid;
	static int mServoBlasterOut;
	static bool mMonitorRunning;
	static std::list<Gpio*> mOutputList;
	static int mBuzzerClock;

	// These statics are used by the monitor when reading
	// inputs and writing PWM outputs.
	static pthread_mutex_t mMonMutex[MAX_PWM_PINS];
	static std::list<Gpio*> mMonitorList;
	static MonitorType mMonType[MAX_PINS];
	static int mValue[MAX_PINS];
	static int mLastValue[MAX_PINS];
	static bool mRecentChange[MAX_PINS];
	static double mStartTime[MAX_PWM_PINS];
	static double mChangeTime[MAX_PINS];
	static int mIntParam[MAX_PWM_PINS][6];
	static bool mLoop[MAX_PWM_PINS];
	static int mRangeMapMin[MAX_PWM_PINS];
	static int mRangeMapMax[MAX_PWM_PINS];

	int mPin;
	Type mType;

public:
	static bool init();
	static void cleanup();
	static bool startMonitor();
	static void stopMonitor();
	static bool setStateRange(int state, int startPin, int endPin);
	static bool setPwmRange(int milliVal, int startPin, int endPin);
	static bool startPwmRange(MonitorType monType, int startPin, int endPin,
		int param1, int param2, int param3, int param4, int param5, bool loop);
	static bool stopPwmRange(int startPin, int endPin);
	static bool usePibrella();
	static bool usingPibrella() { return mUsePibrella; };
	static int getPinFromName(const char *name);
	static void startBuzzer(int note);
	static void stopBuzzer();

	Gpio(int pin, Type type);
	~Gpio();
	int getPin() { return mPin; };
	Gpio::Type getType() { return mType; };
	const char *getPinName();
	void outputAndPwm();
	void inputAndSwitch();
	bool isState(int state);
	bool setState(int state);
	bool readInput(Gpio *gpioOut, int &value);
	bool setPwm(int milliVal);
	bool startPwm(MonitorType monType, int param1, int param2, int param3,
		int param4, int param5, bool loop);
	bool stopPwm();
	bool setRangeMap(int min, int max);

private:
	static void usePibrellaPwm();
	static bool startServoBlaster();
	static void stopServoBlaster();
	static void *monitor(void *arg);
	static void initMonitorInput(int pin);
	static void monitorInput(int pin, double now);
	static void monitorPwmRandom(int pin, double now);
	static void monitorPwmLinear(int pin, double now);
	static void monitorPwmSine(int pin, double now);
	static void setPwm(int pin, int milliPercent);
	static void writePwm(int pin, int value);
	static void resetPwm(int pin, int value);
};

#endif
