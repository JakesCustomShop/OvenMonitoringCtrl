#pragma once

#define ARDUINO_WAIT_TIME 2000
#define INPUI_DATA_BYTES 7	//Number of bytes for recieved string "60.00\r\n"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


class SerialPort
{
private:
	HANDLE handToCOM;
	bool connected;
	COMSTAT status;
	DWORD errors;
public:
	SerialPort(char* portName);
	~SerialPort();

	int ReadSerialPort(char* buffer, unsigned int buf_size);
	bool WriteSerialPort(char* buffer, unsigned int buf_siuze);
	bool isConnected();

};