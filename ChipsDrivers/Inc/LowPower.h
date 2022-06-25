/*
 * LowPower.h
 *
 *  Created on: 19 juin 2022
 *      Author: larbi
 */

#ifndef CHIPSDRIVERS_INC_LOWPOWER_H_
#define CHIPSDRIVERS_INC_LOWPOWER_H_

#include "ChipsUtil.h"

#include "Rtc.h"
#include "Gpio.h"

enum class LowPowerModesTypeDef : uint8_t
{
	sleep,
	stop,
	standby
};

class LowPower {
public:

	static void init();
	static DriversExecStatus enable();
	static DriversExecStatus disable();

	inline static std::set< std::pair<GpioPortsTypeDef, uint16_t> > exceptedGpioLowPower;

	static bool debugMode;
	static bool millisecondsSleep;
	static bool secondsSleep;
	static uint16_t sleepTime;
	static LowPowerModesTypeDef LPmode;

	static Rtc* lpRtc;

};

#endif /* CHIPSDRIVERS_INC_LOWPOWER_H_ */
