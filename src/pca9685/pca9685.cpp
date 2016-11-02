#include "pca9685.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>


#include "../i2c/i2c.h"


/****************************
*                           *
*        DEFINITIONS        *
*                           *
*****************************/

//! True/False definitions
#define TRUE        1
#define FALSE       0


/************************************
*                                   *
*   INTERNAL FUNCTION DEFINITIONS   *
*                                   *
*************************************/



/************************************
*                                   *
*   REGISTER ADDRESS DEFINITIONS    *
*                                   *
*************************************/


enum ControlRegisters
{
		REG_MODE1 = 0x00,
		REG_MODE2 = 0x01,
		REG_SUBADR1 = 0x02,
		REG_SUBADR2 = 0x03,
		REG_SUBADR3 = 0x04,
		REG_ALLCALLADR = 0x05,
		REG_ALL_LED_ON = 0xFA,
		REG_ALL_LED_OFF = 0xFC,
		REG_PRESCALER = 0xFE,
};

char LedOnRegisters[16] =
{
	0x06, // LED0_ON
	0x0A, // LED1_ON
	0x0E, // LED2_ON
	0x12, // LED3_ON
	0x16, // LED4_ON
	0x1A, // LED5_ON
	0x1E, // LED6_ON
	0x22, // LED7_ON
	0x26, // LED8_ON
	0x2A, // LED9_ON
	0x2E, // LED10_ON
	0x32, // LED11_ON
	0x36, // LED12_ON
	0x3A, // LED13_ON
	0x3E, // LED14_ON
	0x41, // LED15_ON
};

char LedOffRegisters[16] =
{
	0x08, // LED0_OFF
	0x0C, // LED1_OFF
	0x10, // LED2_OFF
	0x14, // LED3_OFF
	0x18, // LED4_OFF
	0x1C, // LED5_OFF
	0x20, // LED6_OFF
	0x24, // LED7_OFF
	0x28, // LED8_OFF
	0x2C, // LED9_OFF
	0x30, // LED10_OFF
	0x34, // LED11_OFF
	0x38, // LED12_OFF
	0x3C, // LED13_OFF
	0x40, // LED14_OFF
	0x44, // LED15_OFF
};

const char * Pca9685Registers8[256] =
{
	"MODE1",
	"MODE2",
	"SUBADR1",
	"SUBADR2",
	"SUBADR3",
	"ALLCALLADR",
	"LED0_ON_L",	"LED0_ON_H",	"LED0_OFF_L",	"LED0_OFF_H",
	"LED1_ON_L",	"LED1_ON_H",	"LED1_OFF_L",	"LED1_OFF_H",
	"LED2_ON_L",	"LED2_ON_H",	"LED2_OFF_L",	"LED2_OFF_H",
	"LED3_ON_L",	"LED3_ON_H",	"LED3_OFF_L",	"LED3_OFF_H",
	"LED4_ON_L",	"LED4_ON_H",	"LED4_OFF_L",	"LED4_OFF_H",
	"LED5_ON_L",	"LED5_ON_H",	"LED5_OFF_L",	"LED5_OFF_H",
	"LED6_ON_L",	"LED6_ON_H",	"LED6_OFF_L",	"LED6_OFF_H",
	"LED7_ON_L",	"LED7_ON_H",	"LED7_OFF_L",	"LED7_OFF_H",
	"LED8_ON_L",	"LED8_ON_H",	"LED8_OFF_L",	"LED8_OFF_H",
	"LED9_ON_L",	"LED9_ON_H",	"LED9_OFF_L",	"LED9_OFF_H",
	"LED10_ON_L",	"LED10_ON_H",	"LED10_OFF_L",	"LED10_OFF_H",
	"LED11_ON_L",	"LED11_ON_H",	"LED11_OFF_L",	"LED11_OFF_H",
	"LED12_ON_L",	"LED12_ON_H",	"LED12_OFF_L",	"LED12_OFF_H",
	"LED13_ON_L",	"LED13_ON_H",	"LED13_OFF_L",	"LED13_OFF_H",
	"LED14_ON_L",	"LED14_ON_H",	"LED14_OFF_L",	"LED14_OFF_H",
	"LED15_ON_L",	"LED15_ON_H",	"LED15_OFF_L",	"LED15_OFF_H",

	"46", "47", "48", "49",	"4A", "4B", "4C", "4D",	"4E", "4F", "50", "51", "52", "53", "54", "55",
	"56", "57", "58", "59", "5A", "5B", "5C", "5D",	"5E", "5F", "60", "61",	"62", "63", "64", "65",
	"66", "67", "68", "69",	"6A", "6B", "6C", "6D",	"6E", "6F", "70", "71",	"72", "73", "74", "75",
	"76", "77", "78", "79",	"7A", "7B", "7C", "7D",	"7E", "7F", "80", "81",	"82", "83", "84", "85",
	"86", "87", "88", "89",	"8A", "8B", "8C", "8D",	"8E", "8F", "90", "91",	"92", "93", "94", "95",
	"96", "97", "98", "99",	"9A", "9B", "9C", "9D",	"9E", "9F", "A0", "A1",	"A2", "A3", "A4", "A5",
	"A6", "A7", "A8", "A9",	"AA", "AB", "AC", "AD",	"AE", "AF", "B0", "B1",	"B2", "B3", "B4", "B5",
	"B6", "B7", "B8", "B9",	"BA", "BB", "BC", "BD",	"BE", "BF", "C0", "C1",	"C2", "C3", "C4", "C5",
	"C6", "C7", "C8", "C9",	"CA", "CB", "CC", "CD",	"CE", "CF", "D0", "D1",	"D2", "D3", "D4", "D5",
	"D6", "D7", "D8", "D9",	"DA", "DB", "DC", "DD",	"DE", "DF", "E0", "E1",	"E2", "E3", "E4", "E5",
	"E6", "E7", "E8", "E9",	"EA", "EB", "EC", "ED",	"EE", "EF", "F0", "F1",	"F2", "F3", "F4", "F5",
	"F6", "F7", "F8", "F9",

	"ALL_LED_ON_L", "ALL_LED_ON_H", "ALL_LED_OFF_L", "ALL_LED_OFF_H",
	"PRE_SCALE", "TestMode"
};



/************************************
*                                   *
*     STRUCT RELATED FUNCTIONS      *
*                                   *
*************************************/


Pca9685::Pca9685Config::Pca9685Config()
{
	this->Invert = false ;
	this->OutputChangeOnAck = false;
	this->OutputDrive = TotemPole;
	this->OutNotEnabled = OutputLow;

	// Mode1
	this->ExtClk = false;

	// Prescale
	this->OscillatorClock = 25000000;
	this->Frequency = 200;

	this->Restart = false;
	this->AutoIncrement = true;
	this->Sleep = true;

	// Mode 2
	this->Sub1 = false;
	this->Sub2 = false;
	this->Sub3 = false;
	this->AllCall = true;

}
double Pca9685::Pca9685Config::round(double number)
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}

uint8_t Pca9685::Pca9685Config::getMode1()
{
	uint8_t mode1 = 0x00;
    if(this->Restart)
    	mode1 |= 0x80;
    if(this->ExtClk)
    	mode1 |= 0x40;
    if(this->AutoIncrement)
    	mode1 |= 0x20;
    if(this->Sleep)
    	mode1 |= 0x10;
    if(this->Sub1)
    	mode1 |= 0x08;
    if(this->Sub2)
    	mode1 |= 0x04;
    if(this->Sub3)
    	mode1 |= 0x02;
    if(this->AllCall)
    	mode1 |= 0x01;

    return mode1;
}

uint8_t Pca9685::Pca9685Config::getMode2()
{
	uint8_t mode2 = 0x00;
    if(this->Invert)
    	mode2 |= 0x10;
    if(this->OutputChangeOnAck)
    	mode2 |= 0x08;
    if(this->OutputDrive == TotemPole)
    	mode2 |= 0x04;
    if(this->OutNotEnabled == OutputHighZ)
    	mode2 |= 0x02;
    else if(this->OutNotEnabled == OutputHigh)
    	mode2 |= 0x01;

    return mode2;
}

uint8_t Pca9685::Pca9685Config::getPrescaler()
{
	if(this->Frequency > 1526)
		this->Frequency = 1526;
	else if(this->Frequency < 24)
		this->Frequency = 24;

	// Calculate the prescaler value
	uint32_t prescaler = (uint32_t)(this->round(((double)this->OscillatorClock) / (4096.0 * (double)this->Frequency))) - 1;

	// Respect the limits of the prescaler
	if(prescaler > 0xFF)
		prescaler = 0xFF;
	else if (prescaler < 0x03)
		prescaler = 0x03;

	return (uint8_t)prescaler;
}

uint16_t Pca9685::Pca9685Config::getActualFrequency()
{
	uint8_t prescaler = this->getPrescaler();
	// recalculate the frequency from the prescaler, to reflect the actual frequency
	return (uint16_t)(((double)this->OscillatorClock)/(4096.0 * (prescaler + 1)));
}

void Pca9685::Pca9685Config::setMode1(uint8_t mode)
{
	this->Restart = ((mode & 0x80) != 0);
	this->ExtClk = ((mode & 0x40) != 0);;
    this->AutoIncrement = ((mode & 0x20) != 0);
    this->Sleep = ((mode & 0x10) != 0);
    this->Sub1 = ((mode & 0x08) != 0);
    this->Sub2 = ((mode & 0x04) != 0);
    this->Sub3 = ((mode & 0x02) != 0);
    this->AllCall = ((mode & 0x01) != 0);
}

void Pca9685::Pca9685Config::setMode2(uint8_t mode)
{
	this->Invert = ((mode & 0x10) != 0);
	this->OutputChangeOnAck = ((mode & 0x08) != 0);
	this->OutputDrive = ((mode & 0x04) != 0)?(TotemPole):(OpenDrain);

	if((mode & 0x02) != 0)
		this->OutNotEnabled = OutputHighZ;
	else if((mode & 0x01) != 0)
		this->OutNotEnabled = OutputHigh;
	else
		this->OutNotEnabled = OutputLow;
}





//! Internal helper function that translates an IOConfig structure to an unsigned int denoting the proper configuration for an MCP23017

/************************************
*                                   *
*       CONNECTION SETUP            *
*                                   *
*************************************/

//! Open a new connection to the MCP23017 device, and initialize it.
Pca9685::Pca9685( uint8_t adr, Pca9685Config cfg)
{
    int i;
    // Try opening the port for the specific address
    fp = i2cInit(adr);
    if(fp <= 0) // error
    {
        if(fp == 0)
            throw OperationFailedException("Got NULL pointer to I2C File");
        else if (fp == -1)
            throw OperationFailedException("Could not open I2C device for reading and writing");
        else if (fp == -2)
            throw OperationFailedException("Could not set I2C destination address");
        else
            throw OperationFailedException("Got unspecified error [%d] opening I2C device",fp);
    }
    
    // Initialize objcect variables
    this->adr = adr;                         // set address

    cfg.AutoIncrement = true; // We want to use autoincrement
    cfg.Sleep = false; // we write this last, and want to disable sleep after that
    
    // Copy HWConfig to key
    this->config = cfg;

    try
    {
    	// Write the configuration
    	tryI2CWrite8(REG_PRESCALER,cfg.getPrescaler());
    	tryI2CWrite8(REG_MODE2,cfg.getMode2());
    	tryI2CWrite8(REG_MODE1,cfg.getMode1());
    }
    catch(OperationFailedException x)
    {
        i2cClose(fp);
        throw x;
    }
}


//! Destructor 
Pca9685::~Pca9685()
{
    i2cClose(fp);
}


/************************************
*                                   *
*     REGULAR I/O FUNCTIONS         *
*                                   *
************************************/

//! Set new output value of the PWM
void Pca9685::setValue(uint8_t pin, uint16_t off, uint16_t on)
{
	if(pin < 16)
	{
		tryI2CWrite16(LedOffRegisters[pin], off);
		tryI2CWrite16(LedOnRegisters[pin], on);
	}
}

//! Get current on time of the PWM
uint16_t Pca9685::getOnValue(uint8_t pin)
{
	if(pin < 16)
	{
		return tryI2CRead16(LedOnRegisters[pin]);
	}
	else
		return 0xFFFF;
}

//! Get current off time of the PWM
uint16_t Pca9685::getOffValue(uint8_t pin)
{
	if(pin < 16)
	{
		return tryI2CRead16(LedOffRegisters[pin]);
	}
	else
		return 0xFFFF;
}


/************************************
*                                   *
*     INTERNAL HELPER FUNCTIONS     *
*                                   *
*************************************/

/*! Try to read an 8 bit value from a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
uint8_t Pca9685::tryI2CRead8(uint8_t reg)
{
    int ret;
    // lock process
    MutexLock();

    // Now start reading
    ret = i2cReadReg8(this->fp,reg);

    // unlock process
    MutexUnlock();
    
    // And properly set any error messages
    if(ret == -1)
        throw OperationFailedException("Error writing register address, attempted to read 8bit value from register %s (0x%2x) ",Pca9685Registers8[reg], reg);
    else if(ret == -2)
        throw OperationFailedException("Error reading value, attempted to read 8bit value from register %s (0x%2x)",Pca9685Registers8[reg], reg);
    else if(ret < 0)
        throw OperationFailedException("Unknown error [%d], attempted to read 8bit value from register %s (0x%2x)",ret, Pca9685Registers8[reg], reg);

    return (uint8_t)ret;
}

/*! Try to write an 8 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Pca9685::tryI2CWrite8(uint8_t reg, uint8_t value)
{
    int ret;
    
    // lock process
    MutexLock();
    
    // Now start reading    
    ret = i2cWriteReg8(this->fp,reg,value);

    // unlock process
    MutexUnlock();
    
    // And properly set any error messages
    if(ret == -1)
       throw OperationFailedException("Error writing to register, attempted to write 8bit value 0x%2x to register %s (0x%2x)",value, Pca9685Registers8[reg], reg);
    else if(ret < 0)
       throw OperationFailedException("Unknown error [%d], attempted to write 8bit value 0x%2x to register %s (0x%2x)",ret, value, Pca9685Registers8[reg], reg);


}

/*! Try to write specific bits in an 8 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Pca9685::tryI2CMaskedWrite8(uint8_t reg, uint8_t value, uint8_t mask)
{
    int ret;
    uint8_t newval;
    
    // lock process
    MutexLock();

    // read current value;
    if( (ret = i2cReadReg8(this->fp,reg)) < 0)
    {
        // unlock process
        MutexUnlock();
        // And properly set any error messages
        if(ret == -1)
            throw OperationFailedException("Error writing register address, attempted to read 8bit value from register %s (0x%2x) for masked write", Pca9685Registers8[reg], reg);
        else if(ret == -2)
            throw OperationFailedException("Error reading value, attempted to read 8bit value from register %s (0x%2x) for masked write", Pca9685Registers8[reg], reg);
        else if(ret < 0)
            throw OperationFailedException("Unknown error [%d], attempted to read 8bit value from register %s (0x%2x) for masked write",ret, Pca9685Registers8[reg], reg);
    } 
    
    // copy result to new variable
    newval = (uint16_t)(ret);
    // keep only the non-masked bits
    newval &= ~mask;
    // overwrite the masked bits with the new value
    newval |= (value & mask);
    
    if( (ret = i2cWriteReg8(this->fp,reg,newval)) < 0)
    {
        // unlock process
        MutexUnlock();

        // And properly set any error messages
        if(ret == -1)
            throw OperationFailedException("Error writing to register, attempted to write 8bit value 0x%2x to register %s (0x%2x) for masked write",value, Pca9685Registers8[reg], reg);
        else if(ret < 0)
            throw OperationFailedException("Unknown error [%d], attempted to write 8bit value 0x%2x to register %s (0x%2x) for masked write",ret, value, Pca9685Registers8[reg], reg);
    } 

    // unlock process
    MutexUnlock();
}

/*! Try to read a 16 bit value from a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
uint16_t Pca9685::tryI2CRead16(uint8_t reg)
{
    int ret;

    // lock process
    MutexLock();


    // Now start reading
    ret = i2cReadReg16(this->fp,reg);
    
    // unlock process
    MutexUnlock();
    
    // And properly set any error messages
	if(ret == -1)
		throw OperationFailedException("Error writing register address, attempted to read 16bit value from register %s (0x%2x)", Pca9685Registers8[reg], reg);
	else if(ret == -2)
		throw OperationFailedException("Error reading value, attempted to read 16bit value from register %s (0x%2x)", Pca9685Registers8[reg], reg);
	else if(ret < 0)
		throw OperationFailedException("Unknown error [%d], attempted to read 16bit value from register %s (0x%2x)",ret, Pca9685Registers8[reg], reg);

    return (uint16_t)ret;
}

/*! Try to write a 16 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Pca9685::tryI2CWrite16(uint8_t reg,uint16_t value)
{
    int ret;

    // lock process
    MutexLock();

    // Now start writing
    ret = i2cWriteReg16(this->fp,reg, value);

    // unlock process
    MutexUnlock();

    // And properly set any error messages
	if(ret == -1)
		throw OperationFailedException("Error writing to register, attempted to write 16bit value 0x%4x to register %s (0x%2x)",value, Pca9685Registers8[reg], reg);
	else if(ret < 0)
		throw OperationFailedException("Unknown error [%d], attempted to write 16bit value 0x%4x to register %s (0x%2x)",ret, value, Pca9685Registers8[reg], reg);
    

}

/*! Try to write specific bits in a 16 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Pca9685::tryI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask)
{
    int ret;
    uint16_t newval;

    // lock process
    MutexLock();



    // read current value;
    if( (ret = i2cReadReg16(this->fp,reg)) < 0)
    {
        // unlock process
        MutexUnlock();
        
        // And properly set any error messages
		if(ret == -1)
			throw OperationFailedException("Error writing register address, attempted to read 16bit value from register %s (0x%2x) for masked write", Pca9685Registers8[reg], reg);
		else if(ret == -2)
			throw OperationFailedException("Error reading value, attempted to read 16bit value from register %s (0x%2x) for masked write", Pca9685Registers8[reg], reg);
		else if(ret < 0)
			throw OperationFailedException("Unknown error [%d], attempted to read 16bit value from register %s (0x%2x) for masked write",ret, Pca9685Registers8[reg], reg);
    } 
    
    // copy result to new variable
    newval = (uint16_t)(ret);
    // keep only the non-masked bits
    newval &= ~mask;
    // overwrite the masked bits with the new value
    newval |= (value & mask);
    
    if( (ret = i2cWriteReg16(this->fp,reg,newval)) < 0)
    {
        // unlock process
        MutexUnlock();


		if(ret == -1)
			throw OperationFailedException("Error writing to register, attempted to write 16bit value 0x%4x to register %s (0x%2x) for masked write",value, Pca9685Registers8[reg], reg);
		else if(ret < 0)
			throw OperationFailedException("Unknown error [%d], attempted to write 16bit value 0x%4x to register %s (0x%2x) for masked write",ret, value, Pca9685Registers8[reg], reg);
    } 
    
    // unlock process
    MutexUnlock();
}

/*! Attempt to write specific bits in a 16 bit value to a register
    In case of an error, the function will return prematurely, 
    without setting an error.
*/
void Pca9685::muteI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask)
{
    int result;
    uint16_t newval;

   
    // lock process
    MutexLock();


    // read current value;
    if( (result = i2cReadReg16(this->fp,reg)) < 0)
    {
//        fprintf(stderr,"WARNING: Got error code [%d] attempting to read\r\n",result);
        // unlock process
        MutexUnlock();
        return;
    } 
    
    // copy result to new variable
    newval = (uint16_t)(result);
    // keep only the non-masked bits
    newval &= ~mask;
    // overwrite the masked bits with the new value
    newval |= (value & mask);
    
    if( (result = i2cWriteReg16(this->fp,reg,newval)) < 0)
    {
//        fprintf(stderr,"WARNING: Got error code [%d] attempting to write\r\n",result);
        // unlock process
        MutexUnlock();
        return;
    } 
    
    // unlock process
    MutexUnlock();

}
