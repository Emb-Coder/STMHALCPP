/*
 * Serial.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#include <Serial.h>

static uint8_t RXBuffer[3][RX_BFR_SIZE];

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

Serial *Serial::serial_channel[3] = {nullptr, nullptr, nullptr};

Serial *Serial::getInstance(SerialInstancesTypeDef channel)
{
    if(channel > SerialInstancesTypeDef::SERIAL_6 || (channel < SerialInstancesTypeDef::SERIAL_1)) return nullptr;
    if(serial_channel[(uint8_t)channel] == nullptr) {
        serial_channel[(uint8_t)channel] = new Serial(channel);
    }

    return serial_channel[(uint8_t)channel];
}

void Serial::deleteInstance(SerialInstancesTypeDef channel)
{
    if(channel > SerialInstancesTypeDef::SERIAL_6 || (channel < SerialInstancesTypeDef::SERIAL_1)) return;
    if(serial_channel[(uint8_t)channel] == nullptr) {
        return;
    }
    if(serial_channel[(uint8_t)channel]->valid()) {
        HAL_UART_AbortReceive((UART_HandleTypeDef*)serial_channel[(uint8_t)channel]->_huart);
    }
    delete serial_channel[(uint8_t)channel];
    serial_channel[(uint8_t)channel] = nullptr;
}

Serial::Serial(SerialInstancesTypeDef channel)
{
    _valid = false;
    _channel = channel;
}

Serial::~Serial()
{
    if(_valid) {
        HAL_UART_DeInit((UART_HandleTypeDef*)_huart);
        free(_huart);

        // TODO : delete buffer or clear
    }
}

DriversExecStatus Serial::begin(settings_t setting)
{
	if (_channel < SerialInstancesTypeDef::SERIAL_1 || _channel > SerialInstancesTypeDef::SERIAL_6 ||
		setting.wordLength < SerialWordLengthTypeDef::EIGHT_BITS_LENGTH || setting.wordLength > SerialWordLengthTypeDef::NINE_BITS_LENGTH ||
		setting.stopBits < SerialStopBitTypeDef::ONE_STOP_BIT || setting.stopBits > SerialStopBitTypeDef::TWO_STOP_BITS ||
		setting.parity < SerialParityTypeDef::NONE || setting.parity > SerialParityTypeDef::ODD )
	{
		return (STATUS_ERROR);
	}

	_settings = setting;
	/* PINS INITIALIZATION */

    UART_HandleTypeDef UartHandle = {0};

	UartHandle.Instance = _channel == SerialInstancesTypeDef::SERIAL_1 ? USART1:
							_channel == SerialInstancesTypeDef::SERIAL_2 ? USART2:
							USART6;
	UartHandle.Init.BaudRate = setting.baudrate;
	UartHandle.Init.WordLength = setting.wordLength == SerialWordLengthTypeDef::EIGHT_BITS_LENGTH ? UART_WORDLENGTH_8B:
								   UART_WORDLENGTH_9B;
	UartHandle.Init.StopBits = setting.stopBits == SerialStopBitTypeDef::ONE_STOP_BIT ? UART_STOPBITS_1:
								 UART_STOPBITS_2;

	UartHandle.Init.Parity = setting.parity == SerialParityTypeDef::NONE ? UART_PARITY_NONE:
							   setting.parity == SerialParityTypeDef::ODD ? UART_PARITY_ODD:
							   UART_PARITY_EVEN;

	UartHandle.Init.Mode = UART_MODE_TX_RX;
	UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;

	if (HAL_UART_Init(&UartHandle) != HAL_OK)
	{
		return (STATUS_ERROR);
	}

    _huart = malloc(sizeof(UART_HandleTypeDef));
    memcpy(_huart, &UartHandle, sizeof(UART_HandleTypeDef));
    _valid = true;

	if ( HAL_OK != HAL_UART_Receive_DMA((UART_HandleTypeDef*)_huart, RXBuffer[(uint8_t)_channel], RX_BFR_SIZE))
	{
		free(_huart);
        _huart= nullptr;
        return (STATUS_ERROR);
	}

	return (STATUS_OK);
}


static DMA_HandleTypeDef hdma_rx[3];

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if(huart->Instance == USART1)
	{
		if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			__HAL_RCC_GPIOA_CLK_ENABLE();

		/**USART1 GPIO Configuration
		PA9     ------> USART1_TX
		PA10     ------> USART1_RX
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* DMA controller clock enable */
		if (__HAL_RCC_DMA2_IS_CLK_DISABLED())
			__HAL_RCC_DMA2_CLK_ENABLE();

		__HAL_RCC_USART1_CLK_ENABLE();

		/* USART1 DMA Init */
		/* USART1_RX Init */
		hdma_rx[0].Instance = DMA2_Stream2;
		hdma_rx[0].Init.Channel = DMA_CHANNEL_4;
		hdma_rx[0].Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_rx[0].Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_rx[0].Init.MemInc = DMA_MINC_ENABLE;
		hdma_rx[0].Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_rx[0].Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_rx[0].Init.Mode = DMA_CIRCULAR;
		hdma_rx[0].Init.Priority = DMA_PRIORITY_LOW;
		hdma_rx[0].Init.FIFOMode = DMA_FIFOMODE_DISABLE;

		HAL_DMA_Init(&hdma_rx[0]);
		__HAL_LINKDMA(huart,hdmarx,hdma_rx[0]);

		/* DMA2_Stream2_IRQn interrupt configuration */
		HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

		/* USART1 interrupt Init */
		HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(USART1_IRQn);

	}
	else if(huart->Instance == USART2)
	{
		if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
			__HAL_RCC_GPIOA_CLK_ENABLE();

		/**USART2 GPIO Configuration
		PA2     ------> USART2_TX
		PA3     ------> USART2_RX
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


		if (__HAL_RCC_DMA1_IS_CLK_DISABLED())
			__HAL_RCC_DMA1_CLK_ENABLE();

		/* Peripheral clock enable */
		__HAL_RCC_USART2_CLK_ENABLE();

		/* USART2 DMA Init */
		/* USART2_RX Init */
		hdma_rx[1].Instance = DMA1_Stream5;
		hdma_rx[1].Init.Channel = DMA_CHANNEL_4;
		hdma_rx[1].Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_rx[1].Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_rx[1].Init.MemInc = DMA_MINC_ENABLE;
		hdma_rx[1].Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_rx[1].Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_rx[1].Init.Mode = DMA_CIRCULAR;
		hdma_rx[1].Init.Priority = DMA_PRIORITY_LOW;
		hdma_rx[1].Init.FIFOMode = DMA_FIFOMODE_DISABLE;

		HAL_DMA_Init(&hdma_rx[1]);
		__HAL_LINKDMA(huart,hdmarx,hdma_rx[1]);

		/* DMA1_Stream5_IRQn interrupt configuration */
		HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

		/* USART2 interrupt Init */
		HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(USART2_IRQn);
	}
	else if(huart->Instance == USART6)
	{
		if (__HAL_RCC_GPIOC_IS_CLK_DISABLED())
			__HAL_RCC_GPIOC_CLK_ENABLE();

		/**USART6 GPIO Configuration
		PC6     ------> USART6_TX
		PC7     ------> USART6_RX
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		/* DMA controller clock enable */
		if (__HAL_RCC_DMA2_IS_CLK_DISABLED())
			__HAL_RCC_DMA2_CLK_ENABLE();


		/* Peripheral clock enable */
		__HAL_RCC_USART6_CLK_ENABLE();

		/* USART6 DMA Init */
		/* USART6_RX Init */
		hdma_rx[2].Instance = DMA2_Stream1;
		hdma_rx[2].Init.Channel = DMA_CHANNEL_5;
		hdma_rx[2].Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_rx[2].Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_rx[2].Init.MemInc = DMA_MINC_ENABLE;
		hdma_rx[2].Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_rx[2].Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_rx[2].Init.Mode = DMA_CIRCULAR;
		hdma_rx[2].Init.Priority = DMA_PRIORITY_LOW;
		hdma_rx[2].Init.FIFOMode = DMA_FIFOMODE_DISABLE;
		HAL_DMA_Init(&hdma_rx[2]);
		__HAL_LINKDMA(huart,hdmarx,hdma_rx[2]);


		/* DMA2_Stream1_IRQn interrupt configuration */
		HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

		/* USART6 interrupt Init */
		HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(USART6_IRQn);
	}
}

DriversExecStatus Serial::end(void)
{
    HAL_UART_DeInit((UART_HandleTypeDef*)_huart);

	return (STATUS_OK);
}

extern "C" void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
	if(huart->Instance==USART1)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USART1_CLK_DISABLE();

		/**USART1 GPIO Configuration
	    PA9     ------> USART1_TX
	    PA10     ------> USART1_RX
		 */
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
	}
	else if(huart->Instance==USART2)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USART2_CLK_DISABLE();

		/**USART2 GPIO Configuration
	    PA2     ------> USART2_TX
	    PA3     ------> USART2_RX
		 */
		HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
	}
	else if(huart->Instance==USART6)
	{
		/* Peripheral clock disable */
		__HAL_RCC_USART6_CLK_DISABLE();

		/**USART6 GPIO Configuration
	    PC6     ------> USART6_TX
	    PC7     ------> USART6_RX
		 */
		HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);
	}
}


DriversExecStatus Serial::write(uint8_t val, uint32_t timeout)
{
	if (HAL_OK == HAL_UART_Transmit((UART_HandleTypeDef*)_huart, &val, 1, timeout))
		return (STATUS_OK);

	return (STATUS_ERROR);
}

DriversExecStatus Serial::write(uint8_t* dataBuffer, uint16_t length, uint32_t timeout)
{
	if (HAL_OK == HAL_UART_Transmit((UART_HandleTypeDef*)_huart, dataBuffer, length, timeout))
		return (STATUS_OK);

	return (STATUS_ERROR);
}

uint16_t Serial::read(uint8_t* dataBuffer, uint32_t timeout)
{
    __HAL_UART_ENABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);

    _rxDataLen = 0;
    uint16_t i = 0;

	uint32_t timeNow = HAL_GetTick();

    while(!_rxDataFlag)
    {
    	if (HAL_GetTick() - timeNow >= timeout)
    	{
    		__HAL_UART_DISABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);
    		return (0);
    	}
    }

    _rxDataFlag = false;

	for (i = 0; i < _rxDataLen; ++i)
    {
		dataBuffer[i] = _mainBuffer[i];
    }

    __HAL_UART_DISABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);

	return (_rxDataLen);
}


uint16_t Serial::read(char* dataBuffer, uint32_t timeout)
{
	return (this->read((uint8_t*)(dataBuffer), timeout));
}

// read data through dma in blocking mode
// TODO : read data with non blocking mode
DriversExecStatus Serial::readByte(uint8_t* val, uint32_t timeout)
{
	return (this->readData(val, 1, timeout));
}

// read data through dma in blocking mode
// TODO : read data with non blocking mode
DriversExecStatus Serial::readData(uint8_t* dataBuffer, uint16_t size, uint32_t timeout)
{
    __HAL_UART_ENABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);

    uint16_t dataBufPos = 0;
    uint16_t i = 0;
    _rxDataLen = 0;

    while ((_rxDataLen+dataBufPos) < size)
    {
    	uint32_t timeNow = HAL_GetTick();
        while(!_rxDataFlag)
        {
        	if (HAL_GetTick() - timeNow >= timeout)
        	{
        		__HAL_UART_DISABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);
        		return (STATUS_ERROR);
        	}
        }
        _rxDataFlag = false;

    	for (i = dataBufPos; i < _rxDataLen+dataBufPos; ++i)
        {
    		dataBuffer[i] = _mainBuffer[i-dataBufPos];
        }

       	dataBufPos = i;
    }

    __HAL_UART_DISABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);

	return (STATUS_OK);
}

// read data through dma in blocking mode
// TODO : read data in non blocking mode
DriversExecStatus Serial::readBytesUntil(uint8_t* dataBuffer, const uint8_t* delimiter, uint16_t sizeOfDelim, uint32_t timeout)
{
    __HAL_UART_ENABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);

    bool stopCpy = false;

    uint16_t dataBufPos = 0;
    uint16_t i = 0;
    _rxDataLen = 0;

    while (!stopCpy)
    {
    	uint32_t timeNow = HAL_GetTick();
        while(!_rxDataFlag)
        {
        	if (HAL_GetTick() - timeNow >= timeout)
        	{
        		__HAL_UART_DISABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);
        		return (STATUS_ERROR);
        	}
        }
        _rxDataFlag = false;

    	for (i = dataBufPos; i < (_rxDataLen+dataBufPos) && !stopCpy; ++i)
        {
            if (_mainBuffer[i-dataBufPos] == delimiter[0])
            {
                if (sizeOfDelim == 1)
                    stopCpy = true;
                else
                {
                    for (int j = 1; j < sizeOfDelim; ++j)
                    {
                        stopCpy = _mainBuffer[(i-dataBufPos)+j] == delimiter[j];
                    }
                }

                if (!stopCpy)
                {
                	dataBuffer[i] = _mainBuffer[i-dataBufPos];
                }
            }
            else
            {
                dataBuffer[i] = _mainBuffer[i-dataBufPos];
            }
        }

       	dataBufPos = i;
    }

    __HAL_UART_DISABLE_IT((UART_HandleTypeDef*)_huart, UART_IT_IDLE);

    return (STATUS_OK);
}

// read data through dma in blocking mode
// TODO : read data with non blocking mode
DriversExecStatus Serial::readStringUntil(char* RxData, const char* delimiter, uint32_t timeout)
{
	return (this->readBytesUntil(reinterpret_cast<uint8_t*>(RxData), reinterpret_cast<const uint8_t*>(delimiter), strlen(delimiter), timeout));
}


DriversExecStatus Serial::printf(const char *format, ...)
{
    char loc_buf[64];
    char * temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), format, copy);
    va_end(copy);
    if(len < 0) {
        va_end(arg);
        return (STATUS_ERROR);
    };

    int sizeOfLocBuf = sizeof(loc_buf);

    if(len >= sizeOfLocBuf){
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            va_end(arg);
            return (STATUS_ERROR);
        }
        len = vsnprintf(temp, len+1, format, arg);
    }
    va_end(arg);

    DriversExecStatus ErrSatus = this->write((uint8_t*)temp, len);

    if(temp != loc_buf){
        free(temp);
    }

    return ErrSatus;

}

DriversExecStatus Serial::print(const char* str, uint32_t timeout)
{
	return (this->write((uint8_t*) str, strlen(str), timeout));
}

DriversExecStatus Serial::print(const std::string &s, uint32_t timeout)
{
	return (this->print(s.c_str(), timeout));
}

DriversExecStatus Serial::print(char c, uint32_t timeout)
{
	return (this->write(static_cast<uint8_t> (c), timeout));
}

DriversExecStatus Serial::print(unsigned char uc, int base, uint32_t timeout)
{
    return this->print((unsigned long) uc, base, timeout);
}

DriversExecStatus Serial::print(int val, int base, uint32_t timeout)
{
    return this->print((long) val, base, timeout);
}

DriversExecStatus Serial::print(unsigned int val, int base, uint32_t timeout)
{
    return this->print((unsigned long) val, base, timeout);
}

DriversExecStatus Serial::print(long val, int base, uint32_t timeout)
{
    if (base == 10 && val < 0)
    {
        if (STATUS_OK != this->print('-', timeout))
        	return (STATUS_ERROR);

        val = -val;
    }

    return (this->print(static_cast<unsigned long>(val), base, timeout));
}

DriversExecStatus Serial::print(unsigned long val, int base, uint32_t timeout)
{
    char buf[8 * sizeof(val) + 1]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof(buf) - 1];

    *str = '\0';

    // prevent crash if called with base == 1
    if(base < 2) {
        base = 10;
    }

    do {
        char c = val % base;
        val /= base;

        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while (val);

    return (this->print(str, timeout));
}

DriversExecStatus Serial::print(long long val, int base, uint32_t timeout)
{
    if (base == 10 && val < 0)
    {
        if (STATUS_OK != this->print('-', timeout))
        	return (STATUS_ERROR);

        val = -val;
    }

    return (this->print(static_cast<unsigned long>(val), base, timeout));
}

DriversExecStatus Serial::print(unsigned long long val, int base,
		uint32_t timeout)
{
    if (base == 0) {
        return this->write(val, timeout);
    } else {
        return this->print(static_cast<unsigned long>(val), base, timeout);
    }

}

DriversExecStatus Serial::print(double number, int digits, uint32_t timeout)
{

    if(std::isnan(number)) {
        return this->print("nan", timeout);
    }
    if(std::isinf(number)) {
        return this->print("inf", timeout);
    }
    if(number > 4294967040.0) {
        return this->print("ovf", timeout);    // constant determined empirically
    }
    if(number < -4294967040.0) {
        return this->print("ovf", timeout);    // constant determined empirically
    }

    // Handle negative numbers
    if(number < 0.0) {
    	this->print('-');
        number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for(uint8_t i = 0; i < digits; ++i) {
        rounding /= 10.0;
    }

    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long) number;
    double remainder = number - (double) int_part;
    this->print(int_part);

    // Print the decimal point, but only if there are digits beyond
    if(digits > 0) {
    	this->print(".");
    }

    // Extract digits from the remainder one at a time
    while(digits-- > 0) {
        remainder *= 10.0;
        int toPrint = int(remainder);
        this->print(toPrint);
        remainder -= toPrint;
    }

    return (STATUS_OK);
}


DriversExecStatus Serial::print(struct tm *timeinfo, const char *format,
		uint32_t timeout)
{
    const char* f = format;

    if(!f){
        f = "%c";
    }

    char buf[64];

    size_t written = strftime(buf, 64, f, timeinfo);

    if(written == 0){
        return (STATUS_ERROR);
    }

    return (this->print(buf, timeout));
}

DriversExecStatus Serial::println(const std::string &s, uint32_t timeout)
{
	if (STATUS_OK == this->print(s, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(const char* str, uint32_t timeout)
{
	if (STATUS_OK == this->print(str, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(char c, uint32_t timeout)
{
	if (STATUS_OK == this->print(c, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(unsigned char uc, int base,
		uint32_t timeout)
{
	if (STATUS_OK == this->print(uc, base, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(int val, int base, uint32_t timeout)
{
	if (STATUS_OK == this->print(val, base, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(unsigned int val, int base,
		uint32_t timeout)
{
	if (STATUS_OK == this->print(val, base, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(long val, int base, uint32_t timeout)
{
	if (STATUS_OK == this->print(val, base, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(unsigned long val, int base,
		uint32_t timeout)
{
	if (STATUS_OK == this->print(val, base, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(long long val, int base, uint32_t timeout)
{
	if (STATUS_OK == this->print(val, base, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(unsigned long long val, int base,
		uint32_t timeout)
{
	if (STATUS_OK == this->print(val, base, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(double val, int digitsAfterDecimalP,
		uint32_t timeout)
{
	if (STATUS_OK == this->print(val, digitsAfterDecimalP, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(struct tm *timeinfo, const char *format,
		uint32_t timeout)
{
	if (STATUS_OK == this->print(timeinfo, format, timeout))
		return (this->print("\r\n", timeout));

	return (STATUS_ERROR);
}

DriversExecStatus Serial::println(uint32_t timeout)
{
	return (this->print("\r\n", timeout));
}

void Serial::processCallback(bool state)
{
	if(state) {									// Check if it is an "Idle Interrupt"
		_rxDataFlag = true;
		_rxCounter++;
		uint16_t start = _rxBfrPos;														// Rx bytes start position (=last buffer position)
		_rxBfrPos = RX_BFR_SIZE - (uint16_t)static_cast<UART_HandleTypeDef*>(_huart)->hdmarx->Instance->NDTR;				// determine actual buffer position
		uint16_t len = RX_BFR_SIZE;														// init len with max. size

		if(_rxRollover < 2)  {
			if(_rxRollover) {															// rolled over once
				if(_rxBfrPos <= start) len = _rxBfrPos + RX_BFR_SIZE - start;				// no bytes overwritten
				else len = RX_BFR_SIZE + 1;												// bytes overwritten error
			} else {
				len = _rxBfrPos - start;													// no bytes overwritten
			}
		} else {
			len = RX_BFR_SIZE + 2;														// dual rollover error
		}

		if(len && (len <= RX_BFR_SIZE)) {
			_rxDataLen = len;
			// add received bytes to _mainBuffer
			uint16_t i;
			for(i = 0; i < len; i++) *(_mainBuffer + i) = *(RXBuffer[(uint8_t)_channel] + ((start + i) % RX_BFR_SIZE));
		} else {
			_rxDataLen = 0;
			// buffer overflow error:
		}
		_rxRollover = 0;																	// reset the Rollover variable
	} else {
		// no idle flag? --> DMA rollover occurred
		_rxRollover++;		// increment Rollover Counter
	}
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		if(__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {									// Check if it is an "Idle Interrupt"
			__HAL_UART_CLEAR_IDLEFLAG((UART_HandleTypeDef*)Serial::serial_channel[0]->get_huart());												// clear the interrupt
			Serial::serial_channel[0]->processCallback(true);
		}
		else
		{
			Serial::serial_channel[0]->processCallback(false);
		}
	}
	else if (huart->Instance == USART2)
	{
		if(__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {									// Check if it is an "Idle Interrupt"
			__HAL_UART_CLEAR_IDLEFLAG((UART_HandleTypeDef*)Serial::serial_channel[1]->get_huart());												// clear the interrupt
			Serial::serial_channel[1]->processCallback(true);
		}
		else
		{
			Serial::serial_channel[1]->processCallback(false);
		}
	}
    else if (huart->Instance == USART6)
	{
		if(__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {									// Check if it is an "Idle Interrupt"
			__HAL_UART_CLEAR_IDLEFLAG((UART_HandleTypeDef*)Serial::serial_channel[2]->get_huart());												// clear the interrupt
			Serial::serial_channel[2]->processCallback(true);
		}
		else
		{
			Serial::serial_channel[2]->processCallback(false);
		}
	}

}

extern "C" void DMA2_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */

  /* USER CODE END DMA1_Stream5_IRQn 0 */
    HAL_DMA_IRQHandler((DMA_HandleTypeDef*)((UART_HandleTypeDef*)Serial::serial_channel[0]->get_huart())->hdmarx);
  /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */

  /* USER CODE END DMA1_Stream5_IRQn 1 */
}

extern "C" void DMA1_Stream5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */

  /* USER CODE END DMA1_Stream5_IRQn 0 */
    HAL_DMA_IRQHandler((DMA_HandleTypeDef*)((UART_HandleTypeDef*)Serial::serial_channel[1]->get_huart())->hdmarx);
  /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */

  /* USER CODE END DMA1_Stream5_IRQn 1 */
}

extern "C" void DMA2_Stream1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */

  /* USER CODE END DMA1_Stream5_IRQn 0 */
    HAL_DMA_IRQHandler((DMA_HandleTypeDef*)((UART_HandleTypeDef*)Serial::serial_channel[2]->get_huart())->hdmarx);
  /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */

  /* USER CODE END DMA1_Stream5_IRQn 1 */
}
extern "C" void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
	/* USER CODE END USART2_IRQn 0 */
    HAL_UART_IRQHandler((UART_HandleTypeDef*)Serial::serial_channel[0]->get_huart());
	/* USER CODE BEGIN USART2_IRQn 1 */
	if(__HAL_UART_GET_FLAG((UART_HandleTypeDef*)Serial::serial_channel[0]->get_huart(), UART_FLAG_IDLE))
		HAL_UART_RxCpltCallback((UART_HandleTypeDef*)Serial::serial_channel[0]->get_huart());

  /* USER CODE END USART2_IRQn 1 */
}

extern "C" void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

	/* USER CODE END USART2_IRQn 0 */
    HAL_UART_IRQHandler((UART_HandleTypeDef*)Serial::serial_channel[1]->get_huart());
	/* USER CODE BEGIN USART2_IRQn 1 */
	if(__HAL_UART_GET_FLAG((UART_HandleTypeDef*)Serial::serial_channel[1]->get_huart(), UART_FLAG_IDLE))
		HAL_UART_RxCpltCallback((UART_HandleTypeDef*)Serial::serial_channel[1]->get_huart());


  /* USER CODE END USART2_IRQn 1 */
}

extern "C" void USART6_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

	/* USER CODE END USART2_IRQn 0 */
    HAL_UART_IRQHandler((UART_HandleTypeDef*)Serial::serial_channel[2]->get_huart());
	/* USER CODE BEGIN USART2_IRQn 1 */
	if(__HAL_UART_GET_FLAG((UART_HandleTypeDef*)Serial::serial_channel[2]->get_huart(), UART_FLAG_IDLE))
		HAL_UART_RxCpltCallback((UART_HandleTypeDef*)Serial::serial_channel[2]->get_huart());

  /* USER CODE END USART2_IRQn 1 */
}

/* ---------------STATIC FUNCTIONS---------------------------------- */
