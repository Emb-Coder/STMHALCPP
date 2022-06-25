/*
 * LowPower.cpp
 *
 *  Created on: 19 juin 2022
 *      Author: larbi
 */

#include <LowPower.h>


Rtc* LowPower::lpRtc = nullptr;

bool LowPower::debugMode = false;
bool LowPower::millisecondsSleep = false;
bool LowPower::secondsSleep = false;
uint16_t LowPower::sleepTime = 0;
LowPowerModesTypeDef LowPower::LPmode = LowPowerModesTypeDef::stop;

void LowPower::init()
{
	exceptedGpioLowPower.insert(std::make_pair(GpioPortsTypeDef::PC, 14));
	exceptedGpioLowPower.insert(std::make_pair(GpioPortsTypeDef::PC, 15));

	exceptedGpioLowPower.insert(std::make_pair(GpioPortsTypeDef::PH, 0));
	exceptedGpioLowPower.insert(std::make_pair(GpioPortsTypeDef::PH, 1));
}

DriversExecStatus LowPower::enable()
{
	if (lpRtc == nullptr)
		return (STATUS_ERROR);

	if (!lpRtc->valid())
		return (STATUS_ERROR);

	if ( (millisecondsSleep && secondsSleep) || (!millisecondsSleep && !secondsSleep) )
		return (STATUS_ERROR);


	if (sleepTime != UINT16_MAX)
	{
		if (millisecondsSleep)
		{
			if (STATUS_OK != lpRtc->setWakeupTimer(sleepTime, nullptr))
				return (STATUS_ERROR);
		}
		else
		{
			if (STATUS_OK != lpRtc->setAlarm(RtcAlarmTypesTypeDef::A, (uint32_t) sleepTime, nullptr))
				return (STATUS_ERROR);
		}
	}


	if (debugMode)
	{
		exceptedGpioLowPower.insert(std::make_pair(GpioPortsTypeDef::PA, 13));
		exceptedGpioLowPower.insert(std::make_pair(GpioPortsTypeDef::PA, 14));
		exceptedGpioLowPower.insert(std::make_pair(GpioPortsTypeDef::PB, 3));
	}

	Gpio::setLowPower(exceptedGpioLowPower);

	HAL_SuspendTick();

	if (LPmode == LowPowerModesTypeDef::sleep)
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	else if (LPmode == LowPowerModesTypeDef::stop)
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
	else
		HAL_PWR_EnterSTANDBYMode();


	return (STATUS_OK);
}

DriversExecStatus LowPower::disable()
{
	HAL_ResumeTick();

	return (STATUS_OK);
}

