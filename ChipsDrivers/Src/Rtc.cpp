/*
 * Rtc.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#include <Rtc.h>

#define EPOCH_TIME_OFF      946684800  // This is 1st January 2000, 00:00:00 in epoch time
#define EPOCH_TIME_YEAR_OFF 100        // years since 1900

Rtc *Rtc::rtcInstance =  nullptr;

static void (*alarmAInterruptRoutines) () = nullptr;
static void (*alarmBInterruptRoutines) () = nullptr;
static void (*wakeupInterruptRoutines) () = nullptr;

static void defaultInterruptHandler();

static int get_date_of_year(int day, int mon, int year);

Rtc::Rtc()
{
}

Rtc* Rtc::getInstance(void)
{
	if (rtcInstance == nullptr)
		rtcInstance = new Rtc();

	return (rtcInstance);
}

void Rtc::deleteInstance(void)
{
	if (rtcInstance == nullptr)
		return;

    delete rtcInstance;
    rtcInstance = nullptr;
}

DriversExecStatus Rtc::begin(RtcClockSourceTypeDef clockSource, rtcTimeDate setting)
{
	if (setting.hour >= 24 || setting.minute >= 60 || setting.seconds >= 60 ||
			setting.day == 0 || setting.day >= 32 || setting.month == 0 || setting.month >= 13 )
	{
		return (STATUS_ERROR);
	}

	_clockSource = clockSource;
	_currentConfig = setting;

	RTC_HandleTypeDef rtcHandle = {0};

	/** Initialize RTC Only
	 */
	rtcHandle.Instance = RTC;
	rtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
	rtcHandle.Init.AsynchPrediv = 127;
	rtcHandle.Init.SynchPrediv = 255;
	rtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
	rtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	rtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInitStruct.RTCClockSelection = clockSource == RtcClockSourceTypeDef::LSE ? RCC_RTCCLKSOURCE_LSE : RCC_RTCCLKSOURCE_LSI;

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	/* Peripheral clock enable */
	__HAL_RCC_RTC_ENABLE();


	if (HAL_RTC_Init(&rtcHandle) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

    _hrtc = malloc(sizeof(RTC_HandleTypeDef));
    memcpy(_hrtc, &rtcHandle, sizeof(RTC_HandleTypeDef));
    _valid = true;

    if (readBackupReg(_initBckpRegister) != _initFlagValue)
    {
    	writeBackupReg(_initBckpRegister, _initFlagValue);

    	if (STATUS_OK != setTimeDate(setting))
    	{
    		return (STATUS_ERROR);
    	}
    }

	return (STATUS_OK);
}

DriversExecStatus Rtc::end(void)
{
	if ( HAL_OK != HAL_RTC_DeInit((RTC_HandleTypeDef*)_hrtc) )
		return (STATUS_ERROR);

	return (STATUS_OK);
}

time_t Rtc::getTimeStamps(void)
{
	/* ref : https://github.com/stm32duino/STM32RTC/blob/main/src/STM32RTC.cpp */

	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	HAL_RTC_GetTime((RTC_HandleTypeDef*)_hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate((RTC_HandleTypeDef*)_hrtc, &sDate, RTC_FORMAT_BIN);

	struct tm tm;

	tm.tm_isdst = -1;

	tm.tm_yday = get_date_of_year(sDate.Date, sDate.Month, sDate.Year);

	if (sDate.WeekDay == RTC_WEEKDAY_SUNDAY) {
		tm.tm_wday = 0;
	} else {
		tm.tm_wday = sDate.WeekDay;
	}

	tm.tm_year = sDate.Year + EPOCH_TIME_YEAR_OFF;
	tm.tm_mon = sDate.Month - 1;
	tm.tm_mday = sDate.Date;
	tm.tm_hour = sTime.Hours;
	tm.tm_min = sTime.Minutes;
	tm.tm_sec = sTime.Seconds;

	return ( mktime(&tm) );
}

DriversExecStatus Rtc::setTimeDate(time_t timestamps)
{
	/* ref : https://github.com/stm32duino/STM32RTC/blob/main/src/STM32RTC.cpp */

	if (timestamps < EPOCH_TIME_OFF) {
		timestamps = EPOCH_TIME_OFF;
	}

	epoch_to_rtcCfg(_currentConfig, timestamps);

	return (setTimeDate(_currentConfig));
}

DriversExecStatus Rtc::setTimeDate(rtcTimeDate timeDate)
{
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	/** Initialize RTC and set the Time and Date
	 */
	sTime.Hours = timeDate.hour;
	sTime.Minutes = timeDate.minute;
	sTime.Seconds = timeDate.seconds;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime((RTC_HandleTypeDef*)_hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	sDate.WeekDay = (uint8_t) timeDate.weekday;

	char buf[3] = "\0";
	sprintf(buf, "%d", timeDate.month);

	sDate.Month = str_to_hex(buf);

	sDate.Date = timeDate.day;
	sDate.Year = timeDate.year;

	if (HAL_RTC_SetDate((RTC_HandleTypeDef*)_hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);

}

DriversExecStatus Rtc::setAlarm(RtcAlarmTypesTypeDef alarm,
		rtcTimeDate timeDate, void (*interruptRoutine)(), uint32_t preemptPrio, uint32_t subPrio)
{
	RTC_AlarmTypeDef sAlarm = {0};

	if (alarm == RtcAlarmTypesTypeDef::A)
	{
		if (interruptRoutine == nullptr)
		{
			alarmAInterruptRoutines = defaultInterruptHandler;

		}
		else
		{
			if (alarmAInterruptRoutines != nullptr)
				alarmAInterruptRoutines = nullptr;

			alarmAInterruptRoutines = interruptRoutine;
		}
	}
	else
	{
		if (interruptRoutine == nullptr)
		{
			alarmBInterruptRoutines = defaultInterruptHandler;
		}
		else
		{
			if (alarmBInterruptRoutines != nullptr)
				alarmBInterruptRoutines = nullptr;

			alarmBInterruptRoutines = interruptRoutine;
		}
	}

	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, preemptPrio, subPrio);
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

	/** Enable the Alarm
	 */
	sAlarm.AlarmTime.Hours = timeDate.hour;
	sAlarm.AlarmTime.Minutes = timeDate.minute;
	sAlarm.AlarmTime.Seconds = timeDate.seconds;
	sAlarm.AlarmTime.SubSeconds = 0;
	sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
	sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
	sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
	sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	sAlarm.AlarmDateWeekDay = timeDate.day;
	sAlarm.Alarm = alarm == RtcAlarmTypesTypeDef::A ? RTC_ALARM_A :
				   RTC_ALARM_B;

	if (HAL_RTC_SetAlarm_IT((RTC_HandleTypeDef*)_hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}

DriversExecStatus Rtc::setAlarm(RtcAlarmTypesTypeDef alarm, time_t timestamps,
		void (*interruptRoutine)(), uint32_t preemptPrio, uint32_t subPrio)
{
	rtcTimeDate timeCfg;

	epoch_to_rtcCfg(timeCfg, timestamps);

	return (setAlarm(alarm, timeCfg, interruptRoutine, preemptPrio, subPrio));
}

DriversExecStatus Rtc::setAlarm(RtcAlarmTypesTypeDef alarm, uint32_t timeFromNow, void (*interruptRoutine)(), uint32_t preemptPrio, uint32_t subPrio)
{
	time_t epochNow = getTimeStamps();

	return (setAlarm(alarm, (time_t) (epochNow + timeFromNow), interruptRoutine, preemptPrio, subPrio));
}

DriversExecStatus Rtc::resetAlarm(RtcAlarmTypesTypeDef alarm)
{
	if (alarm == RtcAlarmTypesTypeDef::A)
		alarmAInterruptRoutines = nullptr;
	else
		alarmBInterruptRoutines = nullptr;

	if ( HAL_OK != HAL_RTC_DeactivateAlarm((RTC_HandleTypeDef*)_hrtc, alarm == RtcAlarmTypesTypeDef::A ? RTC_ALARM_A : RTC_ALARM_B) )
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}

DriversExecStatus Rtc::setWakeupTimer(uint16_t period, void (*interruptRoutine)(), uint32_t preemptPrio, uint32_t subPrio)
{
	// period in milliseconds
	// max period is 31000
	if (period > 31000)
		return (STATUS_ERROR);

	if (wakeupInterruptRoutines != nullptr)
		wakeupInterruptRoutines = nullptr;

	wakeupInterruptRoutines = interruptRoutine;

	uint32_t wakeupCounter = period * 2.04918;

	HAL_NVIC_SetPriority(RTC_WKUP_IRQn, preemptPrio, subPrio);
	HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

	if (HAL_RTCEx_SetWakeUpTimer_IT((RTC_HandleTypeDef*)_hrtc, wakeupCounter, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}

DriversExecStatus Rtc::resetWakeupTimer(void)
{
	wakeupInterruptRoutines = nullptr;

	if (HAL_RTCEx_DeactivateWakeUpTimer((RTC_HandleTypeDef*)_hrtc) != HAL_OK )
	{
		return (STATUS_ERROR);
	}

	return (STATUS_OK);
}

void Rtc::writeBackupReg(RtcBackupRegistersTypeDef address, uint32_t value)
{
	HAL_RTCEx_BKUPWrite((RTC_HandleTypeDef*)_hrtc, (uint32_t)address, value);
}

uint32_t Rtc::readBackupReg(RtcBackupRegistersTypeDef address)
{
	return (HAL_RTCEx_BKUPRead((RTC_HandleTypeDef*)_hrtc, (uint32_t) address));
}

Rtc::~Rtc() {
}

void Rtc::epoch_to_rtcCfg(rtcTimeDate& dateTime, time_t timestamps)
{
	struct tm *tmp = gmtime(&timestamps);

	dateTime.year =  tmp->tm_year - EPOCH_TIME_YEAR_OFF;
	dateTime.month =  tmp->tm_mon + 1;
	dateTime.day = tmp->tm_mday;

	if (tmp->tm_wday == 0) {
		dateTime.weekday = RtcWeekDayTypeDef::SUNDAY;
	} else {
		dateTime.weekday = (RtcWeekDayTypeDef) tmp->tm_wday;
	}

	dateTime.hour = tmp->tm_hour;
	dateTime.minute = tmp->tm_min;
	dateTime.seconds = tmp->tm_sec;
}

static int get_date_of_year(int day, int mon, int year)
{
	/* ref : https://overiq.com/c-examples/c-program-to-calculate-the-day-of-year-from-the-date */
	int days_in_feb = 28;
	int doy;    // day of year

    doy = day;

    // check for leap year
    if( (year % 4 == 0 && year % 100 != 0 ) || (year % 400 == 0) )
    	days_in_feb = 29;

    switch(mon)
    {
    case 2:
    	doy += 31;
    	break;
    case 3:
    	doy += 31+days_in_feb;
    	break;
    case 4:
    	doy += 31+days_in_feb+31;
    	break;
    case 5:
    	doy += 31+days_in_feb+31+30;
    	break;
    case 6:
    	doy += 31+days_in_feb+31+30+31;
    	break;
    case 7:
    	doy += 31+days_in_feb+31+30+31+30;
    	break;
    case 8:
    	doy += 31+days_in_feb+31+30+31+30+31;
    	break;
    case 9:
    	doy += 31+days_in_feb+31+30+31+30+31+31;
    	break;
    case 10:
    	doy += 31+days_in_feb+31+30+31+30+31+31+30;
    	break;
    case 11:
    	doy += 31+days_in_feb+31+30+31+30+31+31+30+31;
    	break;
    case 12:
    	doy += 31+days_in_feb+31+30+31+30+31+31+30+31+30;
    	break;
    }

    return (doy);
}

/* ------------------------------------------------------------------------------- */

// IRQ Handler and callbacks

static void defaultInterruptHandler()
{
	__NOP();
}

extern "C" void RTC_WKUP_IRQHandler(void)
{
	HAL_RTCEx_WakeUpTimerIRQHandler((RTC_HandleTypeDef*)Rtc::rtcInstance->get_hrtc());
}

extern "C" void RTC_Alarm_IRQHandler(void)
{
	HAL_RTC_AlarmIRQHandler((RTC_HandleTypeDef*)Rtc::rtcInstance->get_hrtc());
}

extern "C" void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	(*alarmAInterruptRoutines)();
}

extern "C" void HAL_RTC_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
	(*alarmBInterruptRoutines)();
}

extern "C" void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
	(*wakeupInterruptRoutines)();
}


