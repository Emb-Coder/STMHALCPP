/*
 * Rtc.h
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#ifndef INC_RTC_H_
#define INC_RTC_H_

#include "ChipsUtil.h"

enum class RtcAlarmTypesTypeDef : uint8_t
{
	A,
	B
};

enum class RtcWeekDayTypeDef : uint8_t
{
	MONDAY = 0x01,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY,
	SUNDAY
};

enum class RtcClockSourceTypeDef : uint8_t
{
	LSI,
	LSE
};

enum class RtcBackupRegistersTypeDef : uint32_t
{
	DR0 = RTC_BKP_DR0,
	DR1 = RTC_BKP_DR1,
	DR2 = RTC_BKP_DR2,
	DR3 = RTC_BKP_DR3,
	DR4 = RTC_BKP_DR4,
	DR5 = RTC_BKP_DR5,
	DR6 = RTC_BKP_DR6,
	DR7 = RTC_BKP_DR7,
	DR8 = RTC_BKP_DR8,
	DR9 = RTC_BKP_DR9,
	DR10 = RTC_BKP_DR10,
	DR11 = RTC_BKP_DR11,
	DR12 = RTC_BKP_DR12,
	DR13 = RTC_BKP_DR13,
	DR14 = RTC_BKP_DR14,
	DR15 = RTC_BKP_DR15,
	DR16 = RTC_BKP_DR16,
	DR17 = RTC_BKP_DR17,
	DR18 = RTC_BKP_DR18,
	DR19 = RTC_BKP_DR19,
};



class Rtc {
public:

	static Rtc *getInstance(void);
	static void deleteInstance(void);

	using rtcTimeDate = struct {
		uint8_t hour:5;				// 0-23
		uint8_t minute:6;			// 0-59
		uint8_t seconds:6;			// 0-59

		RtcWeekDayTypeDef weekday:3;
		uint8_t day:5;				// 1-31
		uint8_t month:4;			// 1-12
		uint8_t year;				// 0-255

	};


	RtcClockSourceTypeDef getClockSource() const
	{
		return _clockSource;
	}

	const rtcTimeDate& getCurrentConfig() const
	{
		return _currentConfig;
	}

	RtcBackupRegistersTypeDef getInitBckpRegister() const
	{
		return _initBckpRegister;
	}

	void setInitBckpRegister(RtcBackupRegistersTypeDef initBckpRegister = RtcBackupRegistersTypeDef::DR0)
	{
		_initBckpRegister = initBckpRegister;
	}

	uint32_t getInitFlagValue() const
	{
		return _initFlagValue;
	}

	void setInitFlagValue(uint32_t initFlagValue = 0x12345)
	{
		_initFlagValue = initFlagValue;
	}

	void* get_hrtc() { return _hrtc; }

	bool valid() { return _valid;}

	DriversExecStatus begin(RtcClockSourceTypeDef clockSource, rtcTimeDate setting);
	DriversExecStatus end(void);

	time_t getTimeStamps(void);

	DriversExecStatus setTimeDate(time_t timestamps);
	DriversExecStatus setTimeDate(rtcTimeDate timeDate);

	DriversExecStatus setAlarm(RtcAlarmTypesTypeDef alarm, time_t timestamps, void (*interruptRoutine)(), uint32_t preemptPrio = 0, uint32_t subPrio = 0);
	DriversExecStatus setAlarm(RtcAlarmTypesTypeDef alarm, uint32_t timeFromNow, void (*interruptRoutine)(), uint32_t preemptPrio = 0, uint32_t subPrio = 0);
	DriversExecStatus setAlarm(RtcAlarmTypesTypeDef alarm, rtcTimeDate timeDate, void (*interruptRoutine)(), uint32_t preemptPrio = 0, uint32_t subPrio = 0);
	DriversExecStatus resetAlarm(RtcAlarmTypesTypeDef alarm);

	DriversExecStatus setWakeupTimer(uint16_t period, void (*interruptRoutine)(), uint32_t preemptPrio = 0, uint32_t subPrio = 0);
	DriversExecStatus resetWakeupTimer(void);

	void writeBackupReg(RtcBackupRegistersTypeDef address, uint32_t value);
	uint32_t readBackupReg(RtcBackupRegistersTypeDef address);

	static Rtc* rtcInstance;

private:

	Rtc();
	virtual ~Rtc();

	rtcTimeDate _currentConfig;

	void* _hrtc;
	bool _valid {false};
	RtcClockSourceTypeDef _clockSource;

	RtcBackupRegistersTypeDef _initBckpRegister = RtcBackupRegistersTypeDef::DR0;
	uint32_t _initFlagValue = 0x12345;

	void epoch_to_rtcCfg(rtcTimeDate& dateTime, time_t timestamps);
};

#endif /* INC_RTC_H_ */
