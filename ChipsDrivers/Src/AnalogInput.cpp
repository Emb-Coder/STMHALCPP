/*
 * AnalogInput.cpp
 *
 *  Created on: Jun 23, 2022
 *      Author: larbi
 */

#include <AnalogInput.h>

AnalogInput *AnalogInput::analogInInstance =  nullptr;

static DMA_HandleTypeDef hdma_adc;

AnalogInput* AnalogInput::getInstance()
{
	if (analogInInstance == nullptr)
		analogInInstance = new AnalogInput();

	return (analogInInstance);
}

void AnalogInput::deleteInstance()
{
	if (analogInInstance == nullptr)
		return;

    delete analogInInstance;
    analogInInstance = nullptr;
}

DriversExecStatus AnalogInput::begin(std::set<analogInputChannelsTypeDef>& channels)
{
	std::pair<GPIO_TypeDef*, uint32_t> adc_channels_map[] = {
			{GPIOA, GPIO_PIN_0}, // ch0
			{GPIOA, GPIO_PIN_1}, // ch1
			{GPIOA, GPIO_PIN_2}, // ch2
			{GPIOA, GPIO_PIN_3}, // ch3
			{GPIOA, GPIO_PIN_4}, // ch4
			{GPIOA, GPIO_PIN_5}, // ch5
			{GPIOA, GPIO_PIN_6}, // ch6
			{GPIOA, GPIO_PIN_7}, // ch7
			{GPIOB, GPIO_PIN_0}, // ch8
			{GPIOB, GPIO_PIN_1}, // ch9
			{GPIOC, GPIO_PIN_0}, // ch10
			{GPIOC, GPIO_PIN_1}, // ch11
			{GPIOC, GPIO_PIN_2}, // ch12
			{GPIOC, GPIO_PIN_3}, // ch13
			{GPIOC, GPIO_PIN_4}, // ch14
			{GPIOC, GPIO_PIN_5}, // ch15
	};

	uint32_t adc_channelsTp[16] = {
			ADC_CHANNEL_0,
			ADC_CHANNEL_1,
			ADC_CHANNEL_2,
			ADC_CHANNEL_3,
			ADC_CHANNEL_4,
			ADC_CHANNEL_5,
			ADC_CHANNEL_6,
			ADC_CHANNEL_7,
			ADC_CHANNEL_8,
			ADC_CHANNEL_9,
			ADC_CHANNEL_10,
			ADC_CHANNEL_11,
			ADC_CHANNEL_12,
			ADC_CHANNEL_13,
			ADC_CHANNEL_14,
			ADC_CHANNEL_15,
	};

	ADC_ChannelConfTypeDef sConfig = {0};
	ADC_HandleTypeDef adcHandle = {0};

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* DMA controller clock enable */
	if (__HAL_RCC_DMA2_IS_CLK_DISABLED())
		__HAL_RCC_DMA2_CLK_ENABLE();

	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);


	_nbrOfChannels = channels.size();
	_channels = channels;

	adcHandle.Instance = ADC1;
	adcHandle.Init.ClockPrescaler = _clkPrescaler == analogInputClkPrescTypeDef::DIV2 ? ADC_CLOCK_SYNC_PCLK_DIV2:
								    _clkPrescaler == analogInputClkPrescTypeDef::DIV4 ? ADC_CLOCK_SYNC_PCLK_DIV4:
									_clkPrescaler == analogInputClkPrescTypeDef::DIV6 ? ADC_CLOCKPRESCALER_PCLK_DIV6:
									ADC_CLOCKPRESCALER_PCLK_DIV8;

	adcHandle.Init.Resolution = ADC_RESOLUTION_12B;
	adcHandle.Init.ScanConvMode = ENABLE;
	adcHandle.Init.ContinuousConvMode = DISABLE;
	adcHandle.Init.DiscontinuousConvMode = DISABLE;
	adcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	adcHandle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	adcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	adcHandle.Init.NbrOfConversion = _nbrOfChannels;
	adcHandle.Init.DMAContinuousRequests = DISABLE;
	adcHandle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    __HAL_RCC_ADC1_CLK_ENABLE();

    for( std::set<analogInputChannelsTypeDef>::iterator it = channels.begin(); it!=channels.end(); ++it)
    {
    	if (adc_channels_map[(uint8_t)*it].first == GPIOA && __HAL_RCC_GPIOA_IS_CLK_DISABLED())
    		__HAL_RCC_GPIOA_CLK_ENABLE();
    	else if (adc_channels_map[(uint8_t)*it].first == GPIOB && __HAL_RCC_GPIOB_IS_CLK_DISABLED())
    		__HAL_RCC_GPIOB_CLK_ENABLE();
    	else if (adc_channels_map[(uint8_t)*it].first == GPIOC && __HAL_RCC_GPIOC_IS_CLK_DISABLED())
    		__HAL_RCC_GPIOC_CLK_ENABLE();

    	GPIO_InitStruct.Pin = adc_channels_map[(uint8_t)*it].second;
    	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    	GPIO_InitStruct.Pull = GPIO_NOPULL;
    	HAL_GPIO_Init(adc_channels_map[(uint8_t)*it].first, &GPIO_InitStruct);
    }

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc.Instance = DMA2_Stream0;
    hdma_adc.Init.Channel = DMA_CHANNEL_0;
    hdma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc.Init.Mode = DMA_NORMAL;
    hdma_adc.Init.Priority = DMA_PRIORITY_LOW;
    hdma_adc.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_adc) != HAL_OK)
    {
		return (STATUS_ERROR);
    }


    /* ADC1 interrupt Init */
    HAL_NVIC_SetPriority(ADC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
  /* USER CODE BEGIN ADC1_MspInit 1 */

  /* USER CODE END ADC1_MspInit 1 */


	if (HAL_ADC_Init(&adcHandle) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

	_hadc = malloc(sizeof(ADC_HandleTypeDef));
	memcpy(_hadc, &adcHandle, sizeof(ADC_HandleTypeDef));
	_valid = true;

    __HAL_LINKDMA((ADC_HandleTypeDef*)_hadc,DMA_Handle,hdma_adc);

	uint8_t channel_rank = 1;

    for( std::set<analogInputChannelsTypeDef>::iterator it = channels.begin(); it!=channels.end(); ++it)
    {
    	sConfig.Channel = adc_channelsTp[(uint8_t) *it];
    	sConfig.Rank = channel_rank++;
		sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;

		if (HAL_ADC_ConfigChannel((ADC_HandleTypeDef*)_hadc, &sConfig) != HAL_OK)
		{
			free(_hadc);
	        _hadc= nullptr;

			return (STATUS_ERROR);
		}
    }

	return (STATUS_OK);
}

DriversExecStatus AnalogInput::end()
{
	if (HAL_OK != HAL_ADC_DeInit((ADC_HandleTypeDef*)_hadc))
		return (STATUS_ERROR);

	return (STATUS_OK);
}

DriversExecStatus AnalogInput::startRead()
{
	if (!_valid)
		return (STATUS_ERROR);

	if (HAL_OK != HAL_ADC_Start_DMA((ADC_HandleTypeDef*)_hadc, (uint32_t*)_dmaMainBuf, _nbrOfChannels))
		return (STATUS_ERROR);

	_started = true;

	return (STATUS_OK);
}

DriversExecStatus AnalogInput::stopRead()
{
	if (HAL_OK != HAL_ADC_Stop_DMA((ADC_HandleTypeDef*) _hadc))
		return (STATUS_ERROR);

	_started = false;

	return (STATUS_OK);
}

DriversExecStatus AnalogInput::read(uint16_t* readData, uint32_t timeout)
{
	if (!_started)
		return (STATUS_ERROR);

	uint32_t timeNow = HAL_GetTick();

	while (_conversionComplete == 0)
	{
		if (HAL_GetTick() - timeNow >= timeout)
			return (STATUS_ERROR);
	}

	_conversionComplete = 0;

	for (uint8_t i = 0; i < _nbrOfChannels; ++i)
		readData[i] = _dmaMainBuf[i];


	return (STATUS_OK);
}

uint16_t AnalogInput::read(analogInputChannelsTypeDef channel, uint32_t timeout)
{
    auto it = _channels.find(channel);

	if (!_started || it == _channels.end() )
		return (0);

	uint32_t timeNow = HAL_GetTick();

	while (_conversionComplete == 0)
	{
		if (HAL_GetTick() - timeNow >= timeout)
			return (0);
	}

	_conversionComplete = 0;

	int index = std_set_find_index<analogInputChannelsTypeDef>(_channels, *it);

	if (index == -1)
		return (0);

	return (_dmaMainBuf[index]);
}


double AnalogInput::convertRaw(uint16_t rawData, double Vmax, double Vmin)
{
	return ( ( (Vmax - Vmin) * rawData ) / 4095 + Vmin );
}

AnalogInput::AnalogInput() {

}
AnalogInput::~AnalogInput() {
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	AnalogInput::analogInInstance->setConversionComplete(1);
}

extern "C" void ADC_IRQHandler(void)
{
	HAL_ADC_IRQHandler((ADC_HandleTypeDef*)AnalogInput::analogInInstance->get_hadc());
}


extern "C" void DMA2_Stream0_IRQHandler(void)
{
	HAL_DMA_IRQHandler((DMA_HandleTypeDef*)((ADC_HandleTypeDef*)AnalogInput::analogInInstance->get_hadc())->DMA_Handle);
}
