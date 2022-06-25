/*
 * Timer.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#include <Timer.h>

static DriversExecStatus getPrescalerAndArr(uint16_t* PrescArrValues, TimerTicksPeriodTypeDef ticksPeriod);

extern "C" void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

TimerModeTypeDef Timer::timerChannelsMode[8][4];

Timer *Timer::timer_instance[8] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

static void (*TimerCallbackRoutines[8]) ();


/* ---------------- Methods definition ------------------ */

Timer *Timer::getInstance(TimerInstancesTypeDef instance)
{
    if(instance >= TimerInstancesTypeDef::TIMER_11) return nullptr;
    if(timer_instance[(uint8_t)instance] == nullptr) {
        timer_instance[(uint8_t)instance] = new Timer(instance);
    }

    return timer_instance[(uint8_t)instance];
}

void Timer::deleteInstance(TimerInstancesTypeDef instance)
{
    if(instance >= TimerInstancesTypeDef::TIMER_11) return;
    if(timer_instance[(uint8_t)instance] == nullptr) {
        return;
    }
    if(timer_instance[(uint8_t)instance]->valid()) {
    	HAL_TIM_Base_DeInit((TIM_HandleTypeDef*)timer_instance[(uint8_t)instance]->_htim);

    	for (uint8_t j = 0; j < 4; j++)
    		timerChannelsMode[(uint8_t)instance][j] = TimerModeTypeDef::BASE;
    }
    delete timer_instance[(uint8_t)instance];
    timer_instance[(uint8_t)instance] = nullptr;
}


void Timer::start(void)
{
	HAL_TIM_Base_Start((TIM_HandleTypeDef*)_htim);
}

void Timer::triggerIt( void (*timerCallback)())
{
	if (TimerCallbackRoutines[(uint8_t)_instance] != nullptr)
	{
		TimerCallbackRoutines[(uint8_t)_instance] = nullptr;
	}

	TimerCallbackRoutines[(uint8_t)_instance] = timerCallback;

	if (_settings.useInterrupt)
		HAL_TIM_Base_Start_IT((TIM_HandleTypeDef*)_htim);
}

void Timer::disableIt(void)
{
	HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef*)_htim);
}

void Timer::stop(void)
{
	HAL_TIM_Base_Stop((TIM_HandleTypeDef*)_htim);
}

void Timer::setCount(uint16_t counts)
{
	__HAL_TIM_SET_COUNTER((TIM_HandleTypeDef*)_htim, counts);
}

uint16_t Timer::getCount(void)
{
	return (__HAL_TIM_GET_COUNTER((TIM_HandleTypeDef*)_htim));
}

DriversExecStatus Timer::delayUs(uint16_t DelayUs)
{
	if (_settings.ticksPeriod != TimerTicksPeriodTypeDef::MICROSECOND_TICK)
		return (STATUS_ERROR);

	this->setCount(0);

	uint16_t now = this->getCount();
	while (this->getCount() - now < DelayUs) {}

	return (STATUS_OK);
}

// call callback function each Delay microseconds
DriversExecStatus Timer::setIntervalUs(uint16_t DelayUs, void (*timerCallback)())
{
	if (_settings.ticksPeriod != TimerTicksPeriodTypeDef::MICROSECOND_TICK)
		return (STATUS_ERROR);

	__HAL_TIM_SET_AUTORELOAD((TIM_HandleTypeDef*)_htim, DelayUs);
	this->setCount(0);

	TimerCallbackRoutines[(uint8_t)_instance] = timerCallback;

	return (STATUS_OK);
}

DriversExecStatus Timer::begin(tim_settings settings)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};
	TIM_IC_InitTypeDef sConfigIC = {0};
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

	TIM_HandleTypeDef TimerHandle = {0};

	_settings = settings;


	for (uint8_t j = 0; j < 4; j++)
		timerChannelsMode[(uint8_t)_instance][j] = settings.mode[j];

	uint16_t PrescAndArr[2];

	PrescAndArr[0] = settings.prescValue;
	PrescAndArr[1] = settings.arrValue;

	if (settings.ticksPeriod != TimerTicksPeriodTypeDef::CUSTOM_TICK)
		if (STATUS_OK != getPrescalerAndArr(PrescAndArr, settings.ticksPeriod))
			return (STATUS_ERROR);

	TimerHandle.Instance = _instance == TimerInstancesTypeDef::TIMER_1 ? TIM1:
						   _instance == TimerInstancesTypeDef::TIMER_2 ? TIM2:
						   _instance == TimerInstancesTypeDef::TIMER_3 ? TIM3:
						   _instance == TimerInstancesTypeDef::TIMER_4 ? TIM4:
						   _instance == TimerInstancesTypeDef::TIMER_5 ? TIM5:
						   _instance == TimerInstancesTypeDef::TIMER_9 ? TIM9:
						   _instance == TimerInstancesTypeDef::TIMER_10 ? TIM10:
						   TIM11;

	TimerHandle.Init.Prescaler = PrescAndArr[0] ;
	TimerHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
	TimerHandle.Init.Period = PrescAndArr[1];
	TimerHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	TimerHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&TimerHandle) != HAL_OK)
	{
		return (STATUS_ERROR);
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

	if (HAL_TIM_ConfigClockSource(&TimerHandle, &sClockSourceConfig) != HAL_OK)
	{
		return (STATUS_ERROR);
	}


	if ( (settings.mode[0] == TimerModeTypeDef::PWM) || (settings.mode[1] == TimerModeTypeDef::PWM) ||
			(settings.mode[2] == TimerModeTypeDef::PWM) || (settings.mode[3] == TimerModeTypeDef::PWM))
	{
		if (HAL_TIM_PWM_Init(&TimerHandle) != HAL_OK)
		{
			return (STATUS_ERROR);
		}
	}
	else if ( (settings.mode[0] == TimerModeTypeDef::OUTPUT_COMPARE) || (settings.mode[1] == TimerModeTypeDef::OUTPUT_COMPARE) ||
			(settings.mode[2] == TimerModeTypeDef::OUTPUT_COMPARE) || (settings.mode[3] == TimerModeTypeDef::OUTPUT_COMPARE))
	{
		if (HAL_TIM_OC_Init(&TimerHandle) != HAL_OK)
		{
			return (STATUS_ERROR);
		}
	}
	else if ( (settings.mode[0] == TimerModeTypeDef::INPUT_COMPARE) || (settings.mode[1] == TimerModeTypeDef::INPUT_COMPARE) ||
			(settings.mode[2] == TimerModeTypeDef::INPUT_COMPARE) || (settings.mode[3] == TimerModeTypeDef::INPUT_COMPARE))
	{
		if (HAL_TIM_IC_Init(&TimerHandle) != HAL_OK)
		{
			return (STATUS_ERROR);
		}
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&TimerHandle, &sMasterConfig) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	if ( (settings.mode[0] == TimerModeTypeDef::PWM) || (settings.mode[1] == TimerModeTypeDef::PWM) ||
			(settings.mode[2] == TimerModeTypeDef::PWM) || (settings.mode[3] == TimerModeTypeDef::PWM))
	{
		sConfigOC.OCMode = TIM_OCMODE_PWM1;
		sConfigOC.Pulse = 1;
		sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
		sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
		sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
		sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

		if (settings.mode[0] == TimerModeTypeDef::PWM)
		{
			if (HAL_TIM_PWM_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[1] == TimerModeTypeDef::PWM)
		{
			if (HAL_TIM_PWM_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[2] == TimerModeTypeDef::PWM)
		{
			if (HAL_TIM_PWM_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[3] == TimerModeTypeDef::PWM)
		{
			if (HAL_TIM_PWM_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}

		sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
		sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
		sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
		sBreakDeadTimeConfig.DeadTime = 0;
		sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
		sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
		sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
		if (HAL_TIMEx_ConfigBreakDeadTime(&TimerHandle, &sBreakDeadTimeConfig) != HAL_OK)
		{
			return (STATUS_ERROR);
		}

	}
	else if ( (settings.mode[0] == TimerModeTypeDef::OUTPUT_COMPARE) || (settings.mode[1] == TimerModeTypeDef::OUTPUT_COMPARE) ||
			(settings.mode[2] == TimerModeTypeDef::OUTPUT_COMPARE) || (settings.mode[3] == TimerModeTypeDef::OUTPUT_COMPARE))
	{
		sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
		sConfigOC.Pulse = 1;
		sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;


		if (settings.mode[0] == TimerModeTypeDef::OUTPUT_COMPARE)
		{
			if (HAL_TIM_OC_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[1] == TimerModeTypeDef::OUTPUT_COMPARE)
		{
			if (HAL_TIM_OC_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[2] == TimerModeTypeDef::OUTPUT_COMPARE)
		{
			if (HAL_TIM_OC_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[3] == TimerModeTypeDef::OUTPUT_COMPARE)
		{
			if (HAL_TIM_OC_ConfigChannel(&TimerHandle, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
	}
	else if ( (settings.mode[0] == TimerModeTypeDef::INPUT_COMPARE) || (settings.mode[1] == TimerModeTypeDef::INPUT_COMPARE) ||
			(settings.mode[2] == TimerModeTypeDef::INPUT_COMPARE) || (settings.mode[3] == TimerModeTypeDef::INPUT_COMPARE))
	{
		sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
		sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
		sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
		sConfigIC.ICFilter = 0;

		if (settings.mode[0] == TimerModeTypeDef::INPUT_COMPARE)
		{
			if (HAL_TIM_IC_ConfigChannel(&TimerHandle, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[1] == TimerModeTypeDef::INPUT_COMPARE)
		{
			if (HAL_TIM_IC_ConfigChannel(&TimerHandle, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[2] == TimerModeTypeDef::INPUT_COMPARE)
		{
			if (HAL_TIM_IC_ConfigChannel(&TimerHandle, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
		else if (settings.mode[3] == TimerModeTypeDef::INPUT_COMPARE)
		{
			if (HAL_TIM_IC_ConfigChannel(&TimerHandle, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
			{
				return (STATUS_ERROR);
			}
		}
	}

	HAL_TIM_MspPostInit(&TimerHandle);

    _htim = malloc(sizeof(TIM_HandleTypeDef));
    memcpy(_htim, &TimerHandle, sizeof(TIM_HandleTypeDef));
    _valid = true;

	return (STATUS_OK);
}

Timer::Timer(TimerInstancesTypeDef instance) {
    _valid = false;
    _instance = instance;
}

Timer::~Timer() {
    if(_valid) {
    	HAL_TIM_Base_DeInit((TIM_HandleTypeDef*)_htim);
        free(_htim);

        // TODO : delete buffer or clear
    }

}

/* ---------------- Static functions ------------------ */

static DriversExecStatus getPrescalerAndArr(uint16_t* PrescArrValues, TimerTicksPeriodTypeDef ticksPeriod)
{
	uint16_t arr = 0;
	uint16_t prescaler = 0;

	if (TimerTicksPeriodTypeDef::MICROSECOND_TICK == ticksPeriod)
	{
		if (HAL_RCC_GetHCLKFreq() < 1000000)
			return (STATUS_ERROR);

		prescaler = (HAL_RCC_GetHCLKFreq() / 1000000) - 1;
		arr = UINT16_MAX;
	}
	else if (TimerTicksPeriodTypeDef::MILLISECOND_TICK == ticksPeriod)
	{
		if (HAL_RCC_GetHCLKFreq() > 65000000)
			return (STATUS_ERROR);

		prescaler = (HAL_RCC_GetHCLKFreq() / 1000) - 1;
		arr = UINT16_MAX;

	}
	else if (TimerTicksPeriodTypeDef::SECOND_TICK == ticksPeriod)
	{
		if (HAL_RCC_GetHCLKFreq() > 65000)
			return (STATUS_ERROR);

		prescaler = HAL_RCC_GetHCLKFreq() - 1;
		arr = UINT16_MAX;
	}

	*PrescArrValues = prescaler;
	*(++PrescArrValues) = arr;

	return (STATUS_OK);
}

/* ---------------- CALLBACKS ------------------ */

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM1)
		(*TimerCallbackRoutines[0])();
	else if (htim->Instance == TIM2)
		(*TimerCallbackRoutines[1])();
	else if (htim->Instance == TIM3)
		(*TimerCallbackRoutines[2])();
	else if (htim->Instance == TIM4)
		(*TimerCallbackRoutines[3])();
	else if (htim->Instance == TIM5)
		(*TimerCallbackRoutines[4])();
	else if (htim->Instance == TIM9)
		(*TimerCallbackRoutines[5])();
	else if (htim->Instance == TIM10)
		(*TimerCallbackRoutines[6])();
	else if (htim->Instance == TIM11)
		(*TimerCallbackRoutines[7])();
}

extern "C" void TIM1_BRK_TIM9_IRQHandler(void)
{
  HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[0]->get_htim());
  HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[5]->get_htim());
}

/**
  * @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
  */
extern "C" void TIM1_UP_TIM10_IRQHandler(void)
{
	HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[0]->get_htim());
	HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[6]->get_htim());
}

/**
  * @brief This function handles TIM1 trigger and commutation interrupts and TIM11 global interrupt.
  */
extern "C" void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
	 HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[0]->get_htim());
	  HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[7]->get_htim());
}

/**
  * @brief This function handles TIM1 capture compare interrupt.
  */
extern "C" void TIM1_CC_IRQHandler(void)
{
	 HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[0]->get_htim());
}

/**
  * @brief This function handles TIM2 global interrupt.
  */
extern "C" void TIM2_IRQHandler(void)
{
	 HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[1]->get_htim());
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
extern "C" void TIM3_IRQHandler(void)
{
	 HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[2]->get_htim());
}

/**
  * @brief This function handles TIM4 global interrupt.
  */
extern "C" void TIM4_IRQHandler(void)
{
	 HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[3]->get_htim());
}

/**
  * @brief This function handles TIM5 global interrupt.
  */
extern "C" void TIM5_IRQHandler(void)
{
	 HAL_TIM_IRQHandler((TIM_HandleTypeDef*)Timer::timer_instance[4]->get_htim());

}
/* ---------------- Msp initialization ------------------ */

extern "C" void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if(htim_base->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();

    if (Timer::timerChannelsMode[0][0] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs

    }

    if (Timer::timerChannelsMode[0][1] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs
    }

    if (Timer::timerChannelsMode[0][2] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs
    }

    if (Timer::timerChannelsMode[0][3] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs
    }

    if (Timer::timer_instance[0]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
    }
  }
  else if(htim_base->Instance==TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();

    if (Timer::timer_instance[1]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }
  }
  else if(htim_base->Instance==TIM3)
  {
	  /**TIM3 GPIO Configuration
	    PA6     ------> TIM3_CH1
	    PA7     ------> TIM3_CH2
	    PB0     ------> TIM3_CH3
	    PB1     ------> TIM3_CH4
	   */

    __HAL_RCC_TIM3_CLK_ENABLE();

    if (Timer::timerChannelsMode[2][0] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs
    	if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
    	    __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if (Timer::timerChannelsMode[2][1] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs
    	if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
    	    __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if (Timer::timerChannelsMode[2][2] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs
    	if (__HAL_RCC_GPIOB_IS_CLK_DISABLED())
    	    __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    }

    if (Timer::timerChannelsMode[2][3] == TimerModeTypeDef::INPUT_COMPARE)
    {
    	// init GPIOs
    	if (__HAL_RCC_GPIOB_IS_CLK_DISABLED())
    	    __HAL_RCC_GPIOB_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }


    if (Timer::timer_instance[2]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
    }

  }
  else if(htim_base->Instance==TIM4)
  {
    __HAL_RCC_TIM4_CLK_ENABLE();

    if (Timer::timer_instance[3]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
  }
  else if(htim_base->Instance==TIM5)
  {
    __HAL_RCC_TIM5_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

    if (Timer::timer_instance[4]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM5_IRQn);
    }
  }
  else if(htim_base->Instance==TIM9)
  {
    __HAL_RCC_TIM9_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

    if (Timer::timer_instance[5]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
    }
  }
  else if(htim_base->Instance==TIM10)
  {
    __HAL_RCC_TIM10_CLK_ENABLE();

    if (Timer::timer_instance[6]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
    }
  }
  else if(htim_base->Instance==TIM11)
  {
    __HAL_RCC_TIM11_CLK_ENABLE();
    if (Timer::timer_instance[7]->get_settings().useInterrupt)
    {
        HAL_NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
    }
  }
}

/**
* @brief TIM_IC MSP Initialization
* This function configures the hardware resources used in this example
* @param htim_ic: TIM_IC handle pointer
* @retval None
*/
extern "C" void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /**TIM1 GPIO Configuration
	  PA8     ------> TIM1_CH1
	  PA9     ------> TIM1_CH2
	  PA10     ------> TIM1_CH3
	  PA11     ------> TIM1_CH4
  */

  if (htim->Instance==TIM1)
  {
	  // init GPIOs
	  if (Timer::timerChannelsMode[0][0] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs

	  }
	  else if (Timer::timerChannelsMode[0][0] == TimerModeTypeDef::PWM)
	  {
		  // init GPIOs
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();

		  /**TIM1 GPIO Configuration
	  	  	  PA8     ------> TIM1_CH1
		   */

		  GPIO_InitStruct.Pin = GPIO_PIN_8;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;

		  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  }

	  if (Timer::timerChannelsMode[0][1] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs

	  }
	  else if (Timer::timerChannelsMode[0][1] == TimerModeTypeDef::PWM)
	  {
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();

		  /**TIM1 GPIO Configuration
	    	PA9     ------> TIM1_CH2
		   */
		  GPIO_InitStruct.Pin = GPIO_PIN_9;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;

		  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


	  }

	  if (Timer::timerChannelsMode[0][2] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs

	  }
	  else if (Timer::timerChannelsMode[0][2] == TimerModeTypeDef::PWM)
	  {
		  // init GPIOs

		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();

		  /**TIM1 GPIO Configuration
	    	PA10     ------> TIM1_CH3
		   */
		  GPIO_InitStruct.Pin = GPIO_PIN_10;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;

		  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  }

	  if (Timer::timerChannelsMode[0][3] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs

	  }
	  else if (Timer::timerChannelsMode[0][3] == TimerModeTypeDef::PWM)
	  {
		  // init GPIOs

		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();

		  /**TIM1 GPIO Configuration
	    	PA11     ------> TIM1_CH4
		   */
		  GPIO_InitStruct.Pin = GPIO_PIN_11;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;

		  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  }
  }
  else if(htim->Instance==TIM2)
  {
	  /**TIM2 GPIO Configuration
	    PA5     ------> TIM2_CH1
	    PA1     ------> TIM2_CH2
	    PB10     ------> TIM2_CH3
	    PA3     ------> TIM2_CH4
	   */

	  if (Timer::timerChannelsMode[1][0] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();

		  GPIO_InitStruct.Pin = GPIO_PIN_5;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
		  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  }
	  else if (Timer::timerChannelsMode[1][0] == TimerModeTypeDef::PWM)
	  {
	  }

	  if (Timer::timerChannelsMode[1][1] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();

		  GPIO_InitStruct.Pin = GPIO_PIN_1;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
		  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	  }
	  else if (Timer::timerChannelsMode[1][1] == TimerModeTypeDef::PWM)
	  {
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();


	  }

	  if (Timer::timerChannelsMode[1][2] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs
		  if (__HAL_RCC_GPIOB_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOB_CLK_ENABLE();

		  GPIO_InitStruct.Pin = GPIO_PIN_10;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
		  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	  }
	  else if (Timer::timerChannelsMode[1][2] == TimerModeTypeDef::PWM)
	  {
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();


	  }

	  if (Timer::timerChannelsMode[1][3] == TimerModeTypeDef::OUTPUT_COMPARE)
	  {
		  // init GPIOs
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();

		  GPIO_InitStruct.Pin = GPIO_PIN_3;
		  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		  GPIO_InitStruct.Pull = GPIO_NOPULL;
		  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
		  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  }
	  else if (Timer::timerChannelsMode[1][3] == TimerModeTypeDef::PWM)
	  {
		  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			  __HAL_RCC_GPIOA_CLK_ENABLE();
	  }
  }
  else if(htim->Instance==TIM3)
  {
  }
  else if(htim->Instance==TIM4)
  {
  }
  else if(htim->Instance==TIM5)
  {
  }
  else if(htim->Instance==TIM9)
  {
  }
  else if(htim->Instance==TIM10)
  {
  }
  else if(htim->Instance==TIM11)
  {
  }

}
/**
* @brief TIM_Base MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param htim_base: TIM_Base handle pointer
* @retval None
*/
extern "C" void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
  if(htim_base->Instance==TIM1)
  {
    __HAL_RCC_TIM1_CLK_DISABLE();
  }
  else if(htim_base->Instance==TIM2)
  {
    __HAL_RCC_TIM2_CLK_DISABLE();

  }
  else if(htim_base->Instance==TIM3)
  {
    __HAL_RCC_TIM3_CLK_DISABLE();
  }
  else if(htim_base->Instance==TIM4)
  {
    __HAL_RCC_TIM4_CLK_DISABLE();
  }
  else if(htim_base->Instance==TIM5)
  {
    __HAL_RCC_TIM5_CLK_DISABLE();
  }
  else if(htim_base->Instance==TIM9)
  {
    __HAL_RCC_TIM9_CLK_DISABLE();
  }
  else if(htim_base->Instance==TIM10)
  {
    __HAL_RCC_TIM10_CLK_DISABLE();
  }
  else if(htim_base->Instance==TIM11)
  {
    __HAL_RCC_TIM11_CLK_DISABLE();
  }

}
