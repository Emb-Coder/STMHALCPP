/*
 * Gpio.cpp
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#include <Gpio.h>

GPIO_TypeDef* getHalPortInstance(GpioPortsTypeDef port);
void enableGpioClock(GpioPortsTypeDef port);
void disableGpioClock(GpioPortsTypeDef port);


static void (*interruptRoutines[16]) ();

// default constructor
Gpio::Gpio()
{

}

// constructor
Gpio::Gpio(GpioPortsTypeDef port, uint16_t pinNumber,
		GpioModeTypeDef mode, GpioPullTypeDef pull,
		GpioOutModeTypeDef outMode, GpioOutSpeedTypeDef outSpeed) :
					_port(port), _pinNumber(pinNumber), _mode(mode), _pull(pull), _outMode(outMode), _outSpeed(outSpeed)
{

}

// initialize the peripheral
DriversExecStatus Gpio::begin()
{
	// check attributes
	if (_port < GpioPortsTypeDef::PA || _port > GpioPortsTypeDef::PH
			|| _pinNumber < 0 || _pinNumber > 15
			|| _mode < GpioModeTypeDef::INPUT || _mode > GpioModeTypeDef::LOW_POWER
			|| _pull < GpioPullTypeDef::NOPULL || _pull > GpioPullTypeDef::PULL_UP
			|| _outMode < GpioOutModeTypeDef::OUT_PUSH_PULL || _outMode > GpioOutModeTypeDef::OUT_OPEN_DRAIN)
		return (STATUS_ERROR);

	// set initialization flag
	_initialized = true;

	// get GPIO_TypeDef instance
	_portInstance = getHalPortInstance(_port);

	// initialization structure
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// initialization structure
	enableGpioClock(_port);

	GPIO_InitStruct.Pin = _pinNumber == 0 ? GPIO_PIN_0:
						  _pinNumber == 1 ? GPIO_PIN_1:
						  _pinNumber == 2 ? GPIO_PIN_2:
						  _pinNumber == 3 ? GPIO_PIN_3:
						  _pinNumber == 4 ? GPIO_PIN_4:
						  _pinNumber == 5 ? GPIO_PIN_5:
						  _pinNumber == 6 ? GPIO_PIN_6:
						  _pinNumber == 7 ? GPIO_PIN_7:
						  _pinNumber == 8 ? GPIO_PIN_8:
						  _pinNumber == 9 ? GPIO_PIN_9:
						  _pinNumber == 10 ? GPIO_PIN_10:
					      _pinNumber == 11 ? GPIO_PIN_11:
						  _pinNumber == 12 ? GPIO_PIN_12:
						  _pinNumber == 13 ? GPIO_PIN_13:
						  _pinNumber == 14 ? GPIO_PIN_14:
						  GPIO_PIN_15;

	// get pin instance
	_pinInstance = GPIO_InitStruct.Pin;

	// set the gpio mode
	GPIO_InitStruct.Mode = _mode == GpioModeTypeDef::LOW_POWER || _mode == GpioModeTypeDef::ANALOG ? GPIO_MODE_ANALOG:
						   _mode == GpioModeTypeDef::INPUT ? GPIO_MODE_INPUT:
						   _outMode == GpioOutModeTypeDef::OUT_PUSH_PULL ? GPIO_MODE_OUTPUT_PP:
						   GPIO_MODE_OUTPUT_OD;

	// set Pull mode
	GPIO_InitStruct.Pull = _pull == GpioPullTypeDef::NOPULL ? GPIO_NOPULL:
						   _pull == GpioPullTypeDef::PULL_DOWN ? GPIO_PULLDOWN:
						   GPIO_PULLUP;

	// set output speed
	GPIO_InitStruct.Speed = _mode == GpioModeTypeDef::LOW_POWER ? GPIO_SPEED_FREQ_LOW:
						    _outSpeed == GpioOutSpeedTypeDef::OUT_FREQ_LOW ? GPIO_SPEED_FREQ_LOW :
						    GPIO_SPEED_FREQ_HIGH;

	// initialize the GPIO
	HAL_GPIO_Init(_portInstance, &GPIO_InitStruct);

	return (STATUS_OK);
}

// write digital output
DriversExecStatus Gpio::digitalWrite(DigitalStateTypeDef state)
{
	// check initalization
	if (! _initialized || _mode != GpioModeTypeDef::OUTPUT)
		return (STATUS_ERROR);

	if (HIGH == state)
		HAL_GPIO_WritePin(_portInstance, _pinInstance, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(_portInstance, _pinInstance, GPIO_PIN_RESET);

	return(STATUS_OK);
}

// read digital state
DigitalStateTypeDef Gpio::digitalRead(void)
{
	if (GPIO_PIN_SET == HAL_GPIO_ReadPin(_portInstance, _pinInstance))
		return ( HIGH );
	else
		return (LOW);
}

DriversExecStatus Gpio::digitalToggle(void)
{
	// check initalization
	if (! _initialized || _mode != GpioModeTypeDef::OUTPUT)
		return (STATUS_ERROR);

	HAL_GPIO_TogglePin(_portInstance, _pinInstance);

	return (STATUS_OK);
}

DriversExecStatus Gpio::setLowPower(std::set< std::pair<GpioPortsTypeDef, uint16_t> > exceptedPins)
{
	// data
	const uint8_t gpioPortSize = 4;
	GpioPortsTypeDef gpioPortArr[] = {GpioPortsTypeDef::PA, GpioPortsTypeDef::PB, GpioPortsTypeDef::PC, GpioPortsTypeDef::PD};

	for (uint8_t i = 0; i < gpioPortSize; ++i)
	{
		for (uint16_t k = 0; k < 16; k++)
		{
			bool matchedGpio = false;

			for (auto itr : exceptedPins)
			{
				if (itr.first == gpioPortArr[i] && itr.second == k)
					matchedGpio = true;
			}

			if (matchedGpio)
				continue;

			// initialization structure
			GPIO_InitTypeDef GPIO_InitStruct = {0};

			// initialization structure
			enableGpioClock(gpioPortArr[i]);

			GPIO_InitStruct.Pin = k == 0 ? GPIO_PIN_0:
								  k == 1 ? GPIO_PIN_1:
								  k == 2 ? GPIO_PIN_2:
								  k == 3 ? GPIO_PIN_3:
								  k == 4 ? GPIO_PIN_4:
								  k == 5 ? GPIO_PIN_5:
								  k == 6 ? GPIO_PIN_6:
								  k == 7 ? GPIO_PIN_7:
								  k == 8 ? GPIO_PIN_8:
								  k == 9 ? GPIO_PIN_9:
								  k == 10 ? GPIO_PIN_10:
								  k == 11 ? GPIO_PIN_11:
								  k == 12 ? GPIO_PIN_12:
								  k == 13 ? GPIO_PIN_13:
								  k == 14 ? GPIO_PIN_14:
								  GPIO_PIN_15;

			// set the gpio mode
			GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;

			GPIO_InitStruct.Pull = GPIO_NOPULL;

			// set output speed
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

			// initialize the GPIO
			HAL_GPIO_Init(getHalPortInstance(gpioPortArr[i]), &GPIO_InitStruct);
		}
	}

	return (STATUS_OK);
}

// attache external interrupt
void Gpio::attachInterrupt(void (*interruptRoutine)(), GpioTriggerModeTypeDef triggerMode, GpioPullTypeDef pullMode)
{
	// attach interrupt function
	if (interruptRoutines[_pinNumber] != nullptr)
		interruptRoutines[_pinNumber] = nullptr;

	interruptRoutines[_pinNumber] = interruptRoutine;

	//HAL_GPIO_DeInit(_portInstance, _pinInstance);

	// initialization structure
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// set pin number
	GPIO_InitStruct.Pin = _pinInstance;
	// set interrupt trigger mode
	GPIO_InitStruct.Mode = triggerMode == GpioTriggerModeTypeDef::RISING_EDGE ? GPIO_MODE_IT_RISING:
						   triggerMode == GpioTriggerModeTypeDef::FALLING_EDGE ? GPIO_MODE_IT_FALLING:
						   GPIO_MODE_IT_RISING_FALLING;
	// set pull mode
	GPIO_InitStruct.Pull = _pull == GpioPullTypeDef::NOPULL ? GPIO_NOPULL:
						   _pull == GpioPullTypeDef::PULL_DOWN ? GPIO_PULLDOWN:
						   GPIO_PULLUP;

	// initialize external interrupt GPIO
	HAL_GPIO_Init(_portInstance, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	IRQn_Type IRQnInstance = _pinNumber == 0 ? EXTI0_IRQn :
							 _pinNumber == 1 ? EXTI1_IRQn :
					         _pinNumber == 2 ? EXTI2_IRQn :
							 _pinNumber == 3 ? EXTI3_IRQn :
							 _pinNumber == 4 ? EXTI4_IRQn :
							 _pinNumber > 4 && _pinNumber < 10 ? EXTI9_5_IRQn:
							 EXTI15_10_IRQn;

	_iRQnInstance = IRQnInstance;

	// IRQ init
	HAL_NVIC_SetPriority(IRQnInstance, 3, 3);
	HAL_NVIC_EnableIRQ(IRQnInstance);
}

// destructor
Gpio::~Gpio()
{
	// deinitialize the GPIO
	HAL_GPIO_DeInit(_portInstance, _pinInstance);
	_initialized = false;

	if (_iRQnInstance >= 6 && _iRQnInstance <= 40)
	{
		interruptRoutines[_pinNumber] = nullptr;
		HAL_NVIC_DisableIRQ(_iRQnInstance);
	}
}

/* ------------------------------------------------------------------------------- */

// static functions
GPIO_TypeDef* getHalPortInstance(GpioPortsTypeDef port)
{
	if (GpioPortsTypeDef::PA == port)
		return (GPIOA);
	else if (GpioPortsTypeDef::PB == port)
		return (GPIOB);
	else if (GpioPortsTypeDef::PC == port)
		return (GPIOC);
	else if (GpioPortsTypeDef::PD == port)
		return (GPIOD);
	else if (GpioPortsTypeDef::PH == port)
		return (GPIOH);
	else
		return (nullptr);
}

void enableGpioClock(GpioPortsTypeDef port)
{
	if (GpioPortsTypeDef::PA == port && __HAL_RCC_GPIOA_IS_CLK_DISABLED())
		__HAL_RCC_GPIOA_CLK_ENABLE();
	else if (GpioPortsTypeDef::PB == port && __HAL_RCC_GPIOB_IS_CLK_DISABLED())
		__HAL_RCC_GPIOB_CLK_ENABLE();
	else if (GpioPortsTypeDef::PC == port && __HAL_RCC_GPIOC_IS_CLK_DISABLED())
		__HAL_RCC_GPIOC_CLK_ENABLE();
	else if (GpioPortsTypeDef::PD == port && __HAL_RCC_GPIOD_IS_CLK_DISABLED())
		__HAL_RCC_GPIOD_CLK_ENABLE();
	else if (GpioPortsTypeDef::PH == port && __HAL_RCC_GPIOH_IS_CLK_DISABLED())
		__HAL_RCC_GPIOH_CLK_ENABLE();
}

void disableGpioClock(GpioPortsTypeDef port)
{
	if (GpioPortsTypeDef::PA == port && __HAL_RCC_GPIOA_IS_CLK_DISABLED())
		__HAL_RCC_GPIOA_CLK_DISABLE();
	else if (GpioPortsTypeDef::PB == port && __HAL_RCC_GPIOB_IS_CLK_DISABLED())
		__HAL_RCC_GPIOB_CLK_DISABLE();
	else if (GpioPortsTypeDef::PC == port && __HAL_RCC_GPIOC_IS_CLK_DISABLED())
		__HAL_RCC_GPIOC_CLK_DISABLE();
	else if (GpioPortsTypeDef::PD == port && __HAL_RCC_GPIOD_IS_CLK_DISABLED())
		__HAL_RCC_GPIOD_CLK_DISABLE();
	else if (GpioPortsTypeDef::PH == port && __HAL_RCC_GPIOH_IS_CLK_DISABLED())
		__HAL_RCC_GPIOH_CLK_DISABLE();
}


/* ------------------------------------------------------------------------------- */

// IRQ Handler and callbacks
extern "C"{

void EXTI0_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI2_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void EXTI3_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void EXTI4_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

void EXTI15_10_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_0)
		(*interruptRoutines[0])();
	else if (GPIO_Pin == GPIO_PIN_1)
		(*interruptRoutines[1])();
	else if (GPIO_Pin == GPIO_PIN_2)
		(*interruptRoutines[2])();
	else if (GPIO_Pin == GPIO_PIN_3)
		(*interruptRoutines[3])();
	else if (GPIO_Pin == GPIO_PIN_4)
		(*interruptRoutines[4])();
	else if (GPIO_Pin == GPIO_PIN_5)
		(*interruptRoutines[5])();
	else if (GPIO_Pin == GPIO_PIN_6)
		(*interruptRoutines[6])();
	else if (GPIO_Pin == GPIO_PIN_7)
		(*interruptRoutines[7])();
	else if (GPIO_Pin == GPIO_PIN_8)
		(*interruptRoutines[8])();
	else if (GPIO_Pin == GPIO_PIN_9)
		(*interruptRoutines[9])();
	else if (GPIO_Pin == GPIO_PIN_10)
		(*interruptRoutines[10])();
	else if (GPIO_Pin == GPIO_PIN_11)
		(*interruptRoutines[11])();
	else if (GPIO_Pin == GPIO_PIN_12)
		(*interruptRoutines[12])();
	else if (GPIO_Pin == GPIO_PIN_13)
		(*interruptRoutines[13])();
	else if (GPIO_Pin == GPIO_PIN_14)
		(*interruptRoutines[14])();
	else if (GPIO_Pin == GPIO_PIN_15)
		(*interruptRoutines[15])();
}
}

