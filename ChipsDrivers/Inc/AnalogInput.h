/*
 * AnalogInput.h
 *
 *  Created on: Jun 23, 2022
 *      Author: larbi
 */

#ifndef CHIPSDRIVERS_INC_ANALOGINPUT_H_
#define CHIPSDRIVERS_INC_ANALOGINPUT_H_

#include "ChipsUtil.h"

#define ANALOG_IN_DEFAULT_TIMEOUT 	10000
#define ANALOG_IN_DMA_BFR_SIZE		16


enum class analogInputModeTypeDef: uint8_t
{
	INPUT,
	OUTPUT
};

enum class analogInputClkPrescTypeDef: uint8_t
{
	DIV2,
	DIV4,
	DIV6,
	DIV8
};

enum class analogInputChannelsTypeDef: uint8_t
{
	CH0 = 0x00,
	CH1,
	CH2,
	CH3,
	CH4,
	CH5,
	CH6,
	CH7,
	CH8,
	CH9,
	CH10,
	CH11,
	CH12,
	CH13,
	CH14,
	CH15,
};

class AnalogInput {
public:
	static AnalogInput *getInstance();
	static void deleteInstance();

	DriversExecStatus begin(std::set<analogInputChannelsTypeDef>& channels);
	DriversExecStatus end();

	DriversExecStatus startRead();
	DriversExecStatus stopRead();

	uint16_t read(analogInputChannelsTypeDef channel, uint32_t timeout = ANALOG_IN_DEFAULT_TIMEOUT);
	DriversExecStatus read(uint16_t* readData, uint32_t timeout = ANALOG_IN_DEFAULT_TIMEOUT);

	void* get_hadc() { return _hadc; }

	double convertRaw(uint16_t rawData, double Vmax = 3.3, double Vmin = 0);

	void setConversionComplete(volatile uint8_t conversionComplete = 0) {
		_conversionComplete = conversionComplete;
	}

	void setDmaItPreempPrio(uint8_t dmaItPreempPrio = 0) {
		_dmaItPreempPrio = dmaItPreempPrio;
	}

	void setDmaItSubPrio(uint8_t dmaItSubPrio = 0) {
		_dmaItSubPrio = dmaItSubPrio;
	}

	void setClkPrescaler(analogInputClkPrescTypeDef clkPrescaler =
			analogInputClkPrescTypeDef::DIV4) {
		_clkPrescaler = clkPrescaler;
	}

	const std::set<analogInputChannelsTypeDef>& getChannels() const {
		return _channels;
	}

	static AnalogInput* analogInInstance;

private:
	AnalogInput();
	virtual ~AnalogInput();

	analogInputModeTypeDef _mode;

	void *_hadc{nullptr};
	bool _valid{false};

	std::set<analogInputChannelsTypeDef> _channels;

	uint8_t _nbrOfChannels;
	uint8_t _dmaItSubPrio = 0;
	uint8_t _dmaItPreempPrio = 0;
	analogInputClkPrescTypeDef _clkPrescaler = analogInputClkPrescTypeDef::DIV4;

	uint16_t _dmaMainBuf[ANALOG_IN_DMA_BFR_SIZE];
	bool _started = false;

	volatile uint8_t _conversionComplete = 0;
};

#endif /* CHIPSDRIVERS_INC_ANALOGINPUT_H_ */
