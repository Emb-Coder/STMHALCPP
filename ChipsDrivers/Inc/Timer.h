/*
 * Timer.h
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

#include "ChipsUtil.h"

enum class TimerTicksPeriodTypeDef: uint8_t
{
	MICROSECOND_TICK,
	MILLISECOND_TICK,
	SECOND_TICK,
	CUSTOM_TICK  // user defined tick
};

enum class TimerInstancesTypeDef : uint8_t
{
	TIMER_1,
	TIMER_2,
	TIMER_3,
	TIMER_4,
	TIMER_5,
	TIMER_9,
	TIMER_10,
	TIMER_11,
};

enum class TimerModeTypeDef : uint8_t
{
	BASE, // NC
	OUTPUT_COMPARE,
	INPUT_COMPARE,
	PWM
};

enum class TimerChannelTypeDef : uint8_t
{
	CHANNEL_1,
	CHANNEL_2,
	CHANNEL_3,
	CHANNEL_4
};

struct __packed tim_settings {
	TimerModeTypeDef mode[4]; // mode of 4 channels
	TimerTicksPeriodTypeDef  ticksPeriod;
	uint16_t  arrValue;
	uint16_t  prescValue;
	bool useInterrupt;
};


class Timer {

public:
	static Timer *getInstance(TimerInstancesTypeDef instance);
	static void deleteInstance(TimerInstancesTypeDef instance);


	DriversExecStatus begin(tim_settings settings);

	void start(void);

	void triggerIt( void (*timerCallback)());
	void disableIt(void);

	void stop(void);

	uint16_t getCount(void);
	void setCount(uint16_t counts);

	// input capture
	float getSignalFrequency(void);

	DriversExecStatus delayUs(uint16_t DelayUs);

	// call callback function each Delay microseconds
	DriversExecStatus setIntervalUs(uint16_t DelayUs, void (*timerCallback)());

	// call callback function each Delay milliseconds
	void setIntervalMs(uint16_t DelaMs, void (*timerCallback)());

	void* get_htim() { return _htim; }

	tim_settings get_settings() { return _settings; }

	bool valid(){ return _valid;}

	const tim_settings& getSettings() const {
		return _settings;
	}

	static Timer* timer_instance[8];

	static TimerModeTypeDef timerChannelsMode[8][4];

protected:

	tim_settings _settings;

	Timer(TimerInstancesTypeDef instance);
	virtual ~Timer();

	void *_htim{nullptr};

	TimerInstancesTypeDef _instance;

	bool _valid{false};

};

#endif /* INC_Timer_H_ */
