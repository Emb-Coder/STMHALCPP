/*
 * Sim800l.h
 *
 *  Created on: 6 juin 2022
 *      Author: larbi
 */

#ifndef NETWORKS_INC_SIM800L_H_
#define NETWORKS_INC_SIM800L_H_

#include "Serial.h"
#include "Gpio.h"

#define SIM800L_DEFAULT_TIMEOUT		10000
#define SIM800L_SMS_DEFAULT_TIMEOUT	30000

class Sim800l {
public:
	Sim800l(SerialInstancesTypeDef serialInstance, uint32_t timeout = SIM800L_DEFAULT_TIMEOUT);

	Sim800l(SerialInstancesTypeDef serialInstance, std::pair<GpioPortsTypeDef, uint16_t> dtrPin, std::pair<GpioPortsTypeDef, uint16_t> rstPin, uint32_t timeout = SIM800L_DEFAULT_TIMEOUT);

	DriversExecStatus begin(void);

	void reset(void);

	// Methods for calls
 	bool answerCall();
 	void callNumber(const char* number);
 	bool hangoffCall();
 	uint8_t getCallStatus();

 	// Methods for sms
	bool sendSms(const char* number,char* text);
	std::string readSms(uint32_t timeout = SIM800L_SMS_DEFAULT_TIMEOUT);
	bool delAllSms();     // return :  OK or ERROR ..

	void sleep(void);
	void wakeup(void);

	void signalQuality();
	void setPhoneFunctionality();
	void activateBearerProfile();
	void deactivateBearerProfile();

	//get time with the variables by reference
	void RTCtime(int *day,int *month, int *year,int *hour,int *minute, int *second);
	std::string dateNet(); //return date,time, of the network
	bool updateRtc(int utc);  //Update the RTC Clock with de Time AND Date of red-.

	virtual ~Sim800l();

	void setDebugger(Serial *debugger) {
		this->_debugger = debugger;
	}

private:
	uint32_t _timeout;
	char _stringBuffer[255];

	SerialInstancesTypeDef _serialInstance;

	std::pair<GpioPortsTypeDef, uint16_t> _dtrPin;
	std::pair<GpioPortsTypeDef, uint16_t> _rstPin;
	bool _useRstPin = false;
	bool _useDtrPin = false;

	Serial* _debugger = nullptr;

	uint16_t readResponse();
	DriversExecStatus readResponseUntil(const char* delim);
	DriversExecStatus readResponseUntilOk();

	Serial* sim800lSerial = nullptr;

	Gpio* sim800lRstPin = nullptr;
	Gpio* sim800lDtrPin = nullptr;
};

#endif /* NETWORKS_INC_SIM800L_H_ */
