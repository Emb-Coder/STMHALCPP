/*
 * Gpio.h
 *
 *  Created on: Mar 9, 2022
 *      Author: larbi
 */

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

#include "ChipsUtil.h"

enum class GpioPortsTypeDef : uint8_t
{
	PA,
	PB,
	PC,
	PD,
	PH
};

enum class GpioModeTypeDef : uint8_t
{
	INPUT,
	OUTPUT,
	ANALOG,
	LOW_POWER
};

enum class GpioPullTypeDef : uint8_t
{
	NOPULL,
	PULL_DOWN,
	PULL_UP
};

enum class GpioOutModeTypeDef : uint8_t
{
	OUT_PUSH_PULL,
	OUT_OPEN_DRAIN
};

enum class GpioOutSpeedTypeDef : uint8_t
{
	OUT_FREQ_LOW,
	OUT_FREQ_HIGH
};

enum class GpioTriggerModeTypeDef : uint8_t
{
	FALLING_EDGE,
	RISING_EDGE,
	CHANGE
};

class Gpio {

protected:
	GpioPortsTypeDef _port;
	uint16_t _pinNumber;
	GpioModeTypeDef _mode;
	GpioPullTypeDef _pull;
	GpioOutModeTypeDef _outMode;
	GpioOutSpeedTypeDef _outSpeed;

	GPIO_TypeDef* _portInstance = nullptr;
	uint16_t _pinInstance = 0;
	IRQn_Type _iRQnInstance = (IRQn_Type) 99;

	bool _initialized = false;
public:
	Gpio();

	Gpio(GpioPortsTypeDef port, uint16_t pinNumber,
		GpioModeTypeDef mode, GpioPullTypeDef pull = GpioPullTypeDef::NOPULL,
		GpioOutModeTypeDef outMode = GpioOutModeTypeDef::OUT_PUSH_PULL, GpioOutSpeedTypeDef outSpeed = GpioOutSpeedTypeDef::OUT_FREQ_HIGH);

	DriversExecStatus begin();
	DriversExecStatus digitalWrite(DigitalStateTypeDef state);
	DigitalStateTypeDef digitalRead(void);
	DriversExecStatus digitalToggle(void);

	static DriversExecStatus setLowPower(std::set< std::pair<GpioPortsTypeDef, uint16_t> > exceptedPins);

	void attachInterrupt(void (*interruptRoutine)(), GpioTriggerModeTypeDef triggerMode, GpioPullTypeDef pullMode = GpioPullTypeDef::NOPULL);

	void startInterrupt(void);
	void stopInterrupt(void);

	virtual ~Gpio();

	bool isInitialized() const { return _initialized; }

	GpioModeTypeDef getMode() const { return _mode;}

	IRQn_Type getIRQnInstance() const {
		return _iRQnInstance;
	}

	GpioOutModeTypeDef getOutMode() const {
		return _outMode;
	}

	GpioOutSpeedTypeDef getOutSpeed() const {
		return _outSpeed;
	}

	uint16_t getPinInstance() const {
		return _pinInstance;
	}

	uint16_t getPinNumber() const {
		return _pinNumber;
	}

	GpioPortsTypeDef getPort() const {
		return _port;
	}

	const GPIO_TypeDef* getPortInstance() const {
		return _portInstance;
	}

	GpioPullTypeDef getPull() const {
		return _pull;
	}
};

#endif /* INC_GPIO_H_ */
