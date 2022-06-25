/*
 * Pwm.cpp
 *
 *  Created on: 28 mars 2022
 *      Author: larbi
 */

#include "Pwm.h"

Pwm::Pwm(TimerInstancesTypeDef timerInstance, TimerChannelTypeDef channel) : Timer(timerInstance)
{
	_pwmChannel = channel;

	_timInstance = Timer::getInstance(timerInstance);

	_cpuFrequency = HAL_RCC_GetHCLKFreq();

	_halTimChannel = _pwmChannel == TimerChannelTypeDef::CHANNEL_1 ? TIM_CHANNEL_1:
					 _pwmChannel == TimerChannelTypeDef::CHANNEL_2 ? TIM_CHANNEL_2:
					 _pwmChannel ==	TimerChannelTypeDef::CHANNEL_3 ? TIM_CHANNEL_3:
					 TIM_CHANNEL_4;

}

DriversExecStatus Pwm::begin(uint16_t frequencyPsc, uint16_t arrValue)
{
	tim_settings pwm_settings;

	_arrValue = arrValue;

	if (_pwmChannel == TimerChannelTypeDef::CHANNEL_1)
	{
		pwm_settings.mode[0] = TimerModeTypeDef::PWM;
	}
	else if (_pwmChannel == TimerChannelTypeDef::CHANNEL_2)
	{
		pwm_settings.mode[1] = TimerModeTypeDef::PWM;
	}
	else if (_pwmChannel == TimerChannelTypeDef::CHANNEL_3)
	{
		pwm_settings.mode[2] = TimerModeTypeDef::PWM;
	}
	else if (_pwmChannel == TimerChannelTypeDef::CHANNEL_4)
	{
		pwm_settings.mode[3] = TimerModeTypeDef::PWM;
	}

	pwm_settings.ticksPeriod = TimerTicksPeriodTypeDef::CUSTOM_TICK;
	pwm_settings.arrValue = arrValue;
	pwm_settings.prescValue = frequencyPsc;
	pwm_settings.useInterrupt = false;

	if ( STATUS_OK != Timer::begin(pwm_settings))
	{
		return (STATUS_ERROR);
	}

	_initialized = true;

	return (STATUS_OK);
}

DriversExecStatus Pwm::generatePwm()
{
	if (HAL_OK != HAL_TIM_PWM_Start((TIM_HandleTypeDef*)Timer::get_htim(), _halTimChannel))
	{
		return (STATUS_ERROR);
	}

	_pwmStarted = true;

	return (STATUS_OK);
}

DriversExecStatus Pwm::setDutyCycle(uint8_t dutyCycle)
{
	if (dutyCycle < 0 || dutyCycle > 100)
		return (STATUS_ERROR);

	if (!_pwmStarted)
		if (STATUS_OK != this->generatePwm())
			return (STATUS_ERROR);

	uint32_t ccr1Value = (_arrValue * dutyCycle)/100;

	__HAL_TIM_SET_COMPARE((TIM_HandleTypeDef*)Timer::get_htim(), _halTimChannel, ccr1Value);

	return (STATUS_OK);
}

Pwm::~Pwm()
{
}

