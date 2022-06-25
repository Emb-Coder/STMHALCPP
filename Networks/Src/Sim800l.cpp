/*
 * Sim800l.cpp
 *
 *  Created on: 6 juin 2022
 *      Author: larbi
 */

#include <Sim800l.h>


Sim800l::Sim800l(SerialInstancesTypeDef serialInstance, uint32_t timeout)
{
	_timeout = timeout;
	_serialInstance = serialInstance;
}

Sim800l::Sim800l(SerialInstancesTypeDef serialInstance, std::pair<GpioPortsTypeDef, uint16_t> dtrPin, std::pair<GpioPortsTypeDef, uint16_t> rstPin, uint32_t timeout)
{
	_timeout = timeout;
	_serialInstance = serialInstance;
	_dtrPin = dtrPin;
	_useDtrPin = true;
	_rstPin = rstPin;
	_useRstPin = true;
}

uint16_t Sim800l::readResponse(void)
{
	memset(_stringBuffer, 0, 255);

	uint16_t ret = sim800lSerial->read(_stringBuffer, _timeout);

	if (ret == -1)
	{
		if (_debugger != nullptr)
			_debugger->println("no response");
	}
	else
	{
		if (_debugger != nullptr)
		{
			_debugger->print("response");
			_debugger->println("Successes");

			_debugger->println("the response is : ");

			_debugger->println(_stringBuffer);
		}
	}

	return (ret);
}

DriversExecStatus Sim800l::readResponseUntil(const char* delim)
{
	memset(_stringBuffer, 0, 255);

	if (STATUS_OK != sim800lSerial->readStringUntil(_stringBuffer, delim))
	{
		if (_debugger != nullptr)
			_debugger->println("response not OK");

		return (STATUS_ERROR);
	}

	if (_debugger != nullptr)
	{
		_debugger->print("response until");
		_debugger->print(delim);
		_debugger->println("Successes");

		_debugger->println("the response is : ");

		_debugger->println(_stringBuffer);
	}


	return (STATUS_OK);
}

DriversExecStatus Sim800l::readResponseUntilOk()
{
	return (this->readResponseUntil("OK"));
}

DriversExecStatus Sim800l::begin(void)
{
	sim800lSerial = Serial::getInstance(_serialInstance);

    Serial::settings_t settings = {
        .baudrate= 9600,
        .wordLength= SerialWordLengthTypeDef::EIGHT_BITS_LENGTH,
        .stopBits = SerialStopBitTypeDef::ONE_STOP_BIT,
        .parity= SerialParityTypeDef::NONE
    };

	if (STATUS_OK != sim800lSerial->begin(settings))
		return (STATUS_ERROR);


	if (_useDtrPin)
	{
		sim800lDtrPin = new Gpio(_dtrPin.first, _dtrPin.second, GpioModeTypeDef::OUTPUT);
		if (STATUS_OK != sim800lDtrPin->begin())
			return (STATUS_ERROR);

		sim800lDtrPin->digitalWrite(DigitalStateTypeDef::LOW);
	}

	if (_useRstPin)
	{
		sim800lRstPin = new Gpio(_rstPin.first, _rstPin.second, GpioModeTypeDef::OUTPUT);
		if (STATUS_OK != sim800lRstPin->begin())
			return (STATUS_ERROR);

		sim800lRstPin->digitalWrite(DigitalStateTypeDef::HIGH);
	}

	return (STATUS_OK);
}

void Sim800l::reset(void)
{
	if (_useRstPin)
	{
		sim800lRstPin->digitalWrite(DigitalStateTypeDef::HIGH);
		HAL_Delay(1000);
		sim800lRstPin->digitalWrite(DigitalStateTypeDef::LOW);
		HAL_Delay(1000);
	}

	if (_debugger != nullptr)
		_debugger->println("AT");

	sim800lSerial->println("AT");

	uint32_t time_now = HAL_GetTick();

	while (STATUS_OK != this->readResponseUntilOk() )
	{
		if (HAL_GetTick() - time_now >= _timeout)
		{
			if (_debugger != nullptr)
				_debugger->println("timeout///");

			return;
		}

		if (_debugger != nullptr)
			_debugger->println("AT");

		sim800lSerial->println("AT");
	}

}

bool Sim800l::answerCall()
{
	if (_debugger != nullptr)
		_debugger->println("ATA");

	sim800lSerial->println("ATA");

	if (STATUS_OK == this->readResponseUntilOk()) return (true);
	else return (false);
}



void Sim800l::callNumber(const char *number)
{
	if (_debugger != nullptr)
	{
		_debugger->print("ATD");
		_debugger->print(number);
		_debugger->println(";");
	}

	sim800lSerial->print("ATD");
	sim800lSerial->print(number);
	sim800lSerial->println(";");
}

bool Sim800l::hangoffCall()
{
	if (_debugger != nullptr)
		_debugger->println("ATH");

	sim800lSerial->println("ATH");
	if (STATUS_OK == this->readResponseUntilOk()) return (true);
	else return (false);
}

uint8_t Sim800l::getCallStatus()
{

	/*
	  values of return:

	 0 Ready (MT allows commands from TA/TE)
	 2 Unknown (MT is not guaranteed to respond to tructions)
	 3 Ringing (MT is ready for commands from TA/TE, but the ringer is active)
	 4 Call in progress
	*/
	if (_debugger != nullptr)
		_debugger->println("AT+CPAS");

	sim800lSerial->println("AT+CPAS");

	if (0 == this->readResponse())
		return (0);

	int indexOfCpas = findSubString(_stringBuffer, "+CPAS: ");

	if (indexOfCpas == -1)
		return (0);

	char callStatus[3];

	for (int i = 0; i < 3; i++)
		callStatus[i] = _stringBuffer[indexOfCpas+7+i];

	return (atoi(callStatus));
}

bool Sim800l::sendSms(const char *number, char *text)
{
	if (_debugger != nullptr)
		_debugger->println("AT+CMGF=1");

	sim800lSerial->println("AT+CMGF=1"); //set sms to text mode

	if (0 == this->readResponse())
		return (false);

	if (_debugger != nullptr)
	{
		_debugger->print("AT+CMGS=\"");
		_debugger->print(number);
		_debugger->print("\"\r");
	}

	sim800lSerial->print("AT+CMGS=\"");  // command to send sms
	sim800lSerial->print (number);
	sim800lSerial->print("\"\r");

	if (0 == this->readResponse())
		return (false);

	if (_debugger != nullptr)
	{
		_debugger->print(text);
		_debugger->print("\r");
	}

	sim800lSerial->print (text);
	sim800lSerial->print ("\r");

	uint32_t old_timeout = _timeout;
	_timeout = 100;

	//change delay 100 to readserial
	if (0 == this->readResponse())
		return (false);

	if (_debugger != nullptr)
		_debugger->print((char)26);

	sim800lSerial->print((char)26);

	_timeout = old_timeout;
	if (0 == this->readResponse())
		return (false);

	//expect CMGS:xxx   , where xxx is a number,for the sending sms.
	if (-1 != findSubString(_stringBuffer, "CMGS"))
		return (true);
	else
		return (false);
}

std::string Sim800l::readSms(uint32_t timeout)
{
	if (_debugger != nullptr)
		_debugger->print("AT+CMGF=1\r");

	sim800lSerial->print("AT+CMGF=1\r");

	if (0 == this->readResponse())
		return ("");

	if (_debugger != nullptr)
		_debugger->println("AT+CNMI=1,2,0,0,0");

	sim800lSerial->println("AT+CNMI=1,2,0,0,0");

	if (0 == this->readResponse())
		return ("");


	uint32_t oldTimeout = _timeout;

	_timeout = timeout;

	uint16_t rsplen = this->readResponse();
	if (0 == rsplen)
	{
		_timeout = oldTimeout;

		return ("");
	}

	_timeout = oldTimeout;

    bool startCpy = false;
    int PartsCounter = 0;
    std::string BrutMsg = "";

    for (int i = 0; i < rsplen; ++i)
    {
        if (startCpy)
          BrutMsg += _stringBuffer[i];

        if (_stringBuffer[i] == '"') // 34 DEC = " ASCII
          PartsCounter++;

        if (PartsCounter >= 6)
        {
            i += 2;
            PartsCounter = 0;
            startCpy = true;
        }
    }

	return (BrutMsg);
}

bool Sim800l::delAllSms()
{
	if (_debugger != nullptr)
		_debugger->println("at+cmgda=\"del all\"");

	sim800lSerial->println("at+cmgda=\"del all\"");

	if (STATUS_OK == this->readResponseUntilOk()) return (true);
	else return (false);
}

void Sim800l::sleep(void)
{
	sim800lSerial->println("AT+CSCLK=2");
}

void Sim800l::wakeup(void)
{
	sim800lSerial->println("AT");
	sim800lSerial->println("AT+CSCLK=0");
}

void Sim800l::signalQuality()
{
	/*Response
	+CSQ: <rssi>,<ber>Parameters
	<rssi>
	0 -115 dBm or less
	1 -111 dBm
	2...30 -110... -54 dBm
	31 -52 dBm or greater
	99 not known or not detectable
	<ber> (in percent):
	0...7 As RXQUAL values in the table in GSM 05.08 [20]
	subclause 7.2.4
	99 Not known or not detectable
	*/
	if (_debugger != nullptr)
		_debugger->println("AT+CSQ");

	sim800lSerial->println("AT+CSQ");

	this->readResponse();
}

void Sim800l::setPhoneFunctionality()
{
	 /*AT+CFUN=<fun>[,<rst>]
	  Parameters
	  <fun> 0 Minimum functionality
	  1 Full functionality (Default)
	  4 Disable phone both transmit and receive RF circuits.
	  <rst> 1 Reset the MT before setting it to <fun> power level.
	  */

	if (_debugger != nullptr)
		_debugger->println("AT+CFUN=1");

	sim800lSerial->println("AT+CFUN=1");
}

void Sim800l::activateBearerProfile()
{
	if (_debugger != nullptr)
		_debugger->println(" AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");

	sim800lSerial->println(" AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
	this->readResponse();

	if (_debugger != nullptr)
		_debugger->println(" AT+SAPBR=3,1,\"APN\",\"internet\"");

	sim800lSerial->println(" AT+SAPBR=3,1,\"APN\",\"internet\"");
	this->readResponse();

	if (_debugger != nullptr)
		_debugger->println(" AT+SAPBR=1,1");

	sim800lSerial->println(" AT+SAPBR=1,1");
	HAL_Delay(1200);
	this->readResponse();

	if (_debugger != nullptr)
		_debugger->println(" AT+SAPBR=2,1");

	sim800lSerial->println(" AT+SAPBR=2,1");
	HAL_Delay(3000);
	this->readResponse();
}

void Sim800l::deactivateBearerProfile()
{
	if (_debugger != nullptr)
		_debugger->println("AT+SAPBR=0,1");

	sim800lSerial->println("AT+SAPBR=0,1");
	HAL_Delay(1500);
}

void Sim800l::RTCtime(int *day, int *month, int *year, int *hour, int *minute,
		int *second)
{
	if (_debugger != nullptr)
		_debugger->println("at+cclk?");

	sim800lSerial->println("at+cclk?");
	// if respond with ERROR try one more time.
	this->readResponse();

	if (findSubString(_stringBuffer, "ERR") != -1)
	{
		HAL_Delay(50);

		if (_debugger != nullptr)
			_debugger->println("at+cclk?");

		sim800lSerial->println("at+cclk?");
	}
	if (findSubString(_stringBuffer, "ERR") == -1)
	{
		std::string buffer = char_arr_to_string(_stringBuffer);
		buffer= buffer.substr(findSubString(buffer, "\"")+1, findSubString(buffer,"\"", true)-1);
		*year= stoi(buffer.substr(0,2));
		*month= stoi(buffer.substr(3,5));
		*day= stoi(buffer.substr(6,8));
		*hour= stoi(buffer.substr(9,11));
		*minute= stoi(buffer.substr(12,14));
		*second= stoi(buffer.substr(15,17));
	}
}

std::string Sim800l::dateNet()
{
	if (_debugger != nullptr)
		_debugger->println("AT+CIPGSMLOC=2,1");

	sim800lSerial->println("AT+CIPGSMLOC=2,1");

	if (STATUS_OK == this->readResponseUntilOk())
	{
		std::string buffer = char_arr_to_string(_stringBuffer);
		return (buffer.substr(findSubString(buffer, ":")+2, strlen(_stringBuffer)-6));
	}
	else
		return ("0");
}

bool Sim800l::updateRtc(int utc)
{
	this->activateBearerProfile();
	std::string buffer = dateNet();
	this->deactivateBearerProfile();

	buffer= buffer.substr(findSubString(buffer, ",")+1, buffer.length());
	std::string dt= buffer.substr(0, findSubString(buffer, ","));
	std::string tm= buffer.substr(findSubString(buffer, ",")+1, buffer.length());

	int hour = stoi(tm.substr(0,2));
	int day = stoi(dt.substr(8,10));

	hour=hour+utc;

	std::string tmp_hour;
	std::string tmp_day;
	//TODO : fix if the day is 0, this occur when day is 1 then decrement to 1,
	//       will need to check the last month what is the last day .
	if (hour<0){
		hour+=24;
		day-=1;
	}
	if (hour<10){

		tmp_hour="0"+std::to_string(hour);
	}else{
		tmp_hour=std::to_string(hour);
	}
	if (day<10){
		tmp_day="0"+std::to_string(day);
	}else{
		tmp_day=std::to_string(day);
	}
	//for debugging
	if (_debugger != nullptr)
		_debugger->print("at+cclk=\""+dt.substr(2,4)+"/"+dt.substr(5,7)+"/"+tmp_day+","+tmp_hour+":"+tm.substr(3,5)+":"+tm.substr(6,8)+"-03\"\r\n");

	//Serial.println("at+cclk=\""+dt.substring(2,4)+"/"+dt.substring(5,7)+"/"+tmp_day+","+tmp_hour+":"+tm.substring(3,5)+":"+tm.substring(6,8)+"-03\"\r\n");
	sim800lSerial->print("at+cclk=\""+dt.substr(2,4)+"/"+dt.substr(5,7)+"/"+tmp_day+","+tmp_hour+":"+tm.substr(3,5)+":"+tm.substr(6,8)+"-03\"\r\n");

	this->readResponse();

	if ( findSubString(_stringBuffer, "ER") !=-1 )
		return false;

	return true;
}

Sim800l::~Sim800l() {
}

