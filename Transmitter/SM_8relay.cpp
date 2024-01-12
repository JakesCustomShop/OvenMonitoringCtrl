extern "C"
{
#include <inttypes.h>
}

#include "Arduino.h"
#include "Wire.h"
#include "SM_8relay.h"

#define RELAY8_INPORT_REG_ADD	0x00
#define RELAY8_OUTPORT_REG_ADD	0x01
#define RELAY8_POLINV_REG_ADD	0x02
#define RELAY8_CFG_REG_ADD		0x03
#define RELAY8_HW_I2C_BASE_ADD	0x38
#define RELAY8_HW_I2C_ALTERNATE_BASE_ADD 0x20

const uint8_t relaysRemap[8] =
{
	0x01,	//1
	0x04,	//2
	0x40,
	0x10,
	0x20,
	0x80,
	0x08,	//7	
	0x02	//8	
	};

SM_8relay::SM_8relay()
{
}


//.begin() returns 0 if card is detected. (This is backwards from other SM cards)
uint8_t SM_8relay::begin(uint8_t stack)
{
	if (stack < 0)
	{
		stack = 0;
	}
	if (stack > 7)
	{
		stack = 7;
	}

	_hwAdd = RELAY8_HW_I2C_BASE_ADD + stack ^ 0x07;
	
	Wire.begin();
	Wire.beginTransmission(_hwAdd);
	Wire.write(RELAY8_OUTPORT_REG_ADD);
	Wire.write(0x00); //make all High
	if(0 !=  Wire.endTransmission()){
		_hwAdd = RELAY8_HW_I2C_ALTERNATE_BASE_ADD + stack ^ 0x07; //old card type
		Wire.begin();
		Wire.beginTransmission(_hwAdd);
		Wire.write(RELAY8_OUTPORT_REG_ADD);
		Wire.write(0x00); //make all High
		if(0 !=  Wire.endTransmission()){
			return 1;
		}
	}

	Wire.begin();
	Wire.beginTransmission(_hwAdd);
	Wire.write(RELAY8_CFG_REG_ADD);
	Wire.write(0x00); //make all output
	return Wire.endTransmission();
}

uint8_t SM_8relay::begin()
{
	return begin(0);
}
uint8_t SM_8relay::rawRead(uint8_t *val)
{
	Wire.beginTransmission(_hwAdd);
	Wire.write(RELAY8_OUTPORT_REG_ADD);
	if (Wire.endTransmission() != 0)
	{
		return 1;
	}

	Wire.requestFrom(_hwAdd, (uint8_t)1);
	if (1 <= Wire.available())
	{
		*val = Wire.read();
	}
	return 0;
}

uint8_t SM_8relay::rawWrite(uint8_t val)
{

	Wire.beginTransmission(_hwAdd);
	Wire.write(RELAY8_OUTPORT_REG_ADD);
	Wire.write(val);
	return Wire.endTransmission();
}

uint8_t SM_8relay::writeChannel(uint8_t channel, uint8_t state)
{
	uint8_t val = 0;

	if (channel < 1)
	{
		channel = 1;
	}
	if (channel > 8)
	{
		channel = 8;
	}

	if (rawRead(&val) != 0)
	{
		return 1;
	}

	if (state != 0)//normal polarity
	{
		val |= relaysRemap[channel - 1];
	}
	else
	{
		val &= ~relaysRemap[channel - 1];
	}

	return rawWrite(val);
}

uint8_t SM_8relay::writeChannel(int channel, int state)
{
	return writeChannel((uint8_t)channel, (uint8_t)state);
}

uint8_t SM_8relay::readChannel(uint8_t channel)
{
	uint8_t val = 0;

	if (channel < 1)
	{
		channel = 1;
	}
	if (channel > 8)
	{
		channel = 8;
	}

	if (rawRead(&val) != 0)
	{
		return 2;
	}
	if ( (val & relaysRemap[channel - 1]) != 0)
	{
		return 1;
	}
	return 0;
}

uint8_t SM_8relay::writeAll(uint8_t val)
{
	uint8_t outVal = 0;
	uint8_t i = 0;

	for (i = 0; i < 8; i++)
	{
		if ( ( (1 << i) & val) != 0)
		{
			outVal += relaysRemap[i];
		}
	}
	return rawWrite(outVal);
}

uint8_t SM_8relay::readAll()
{
	uint8_t val = 0;
	uint8_t outVal = 0;
	uint8_t i = 0;
	if (rawRead(&val) != 0)
	{
		return 0xff; // todo: error passing
	}
	for (i = 0; i < 8; i++)
	{
		if ( (val & relaysRemap[i]) != 0)
		{
			outVal += 1 << i;
		}
	}
	return outVal;

}
