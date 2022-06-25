/*
 * Pwm.h
 *
 *  Created on: 28 mars 2022
 *      Author: larbi
 */

#ifndef CHIPSDRIVERS_INC_PWM_H_
#define CHIPSDRIVERS_INC_PWM_H_

#include "Timer.h"

class Pwm : public Timer {

private:
	TimerChannelTypeDef _pwmChannel;
	TimerInstancesTypeDef _PwmTimerInstance;

	Timer* _timInstance = nullptr;

	uint16_t _arrValue = 0;
	uint32_t _cpuFrequency = 0;
	uint16_t _frequency = 0;

	uint8_t dutyCycle = 1;
	uint32_t _halTimChannel;

	bool _initialized = false;
	bool _pwmStarted = false;

public:
	Pwm(TimerInstancesTypeDef timerInstance, TimerChannelTypeDef channel);
	DriversExecStatus begin(uint16_t frequencyPrescaler, uint16_t arrValue);

	DriversExecStatus generatePwm(void);

	virtual ~Pwm();

	uint32_t getCpuFrequency() const { return _cpuFrequency; }

	DriversExecStatus setDutyCycle(uint8_t dutyCycle);
};

#endif /* CHIPSDRIVERS_INC_PWM_H_ */
