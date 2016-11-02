#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>



#include "mcp23017.hpp"
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

// This interface used only BANK=0 addresses, which are defined below
// The Init function ensures that the device is started in BANK=0 mode

#define IODIR    0x00
#define IODIRA   0x00
#define IODIRB   0x01

#define IPOL     0x02
#define IPOLA    0x02
#define IPOLB    0x03

#define GPINTEN  0x04
#define GPINTENA 0x04
#define GPINTENB 0x05

#define DEFVAL   0x06
#define DEFVALA  0x06
#define DEFVALB  0x07

#define INTCON   0x08
#define INTCONA  0x08
#define INTCONB  0x09

#define IOCON    0x0A
#define IOCONA   0x0A
#define IOCONB   0x0B

#define GPPU     0x0C
#define GPPUA    0x0C
#define GPPUB    0x0D

#define INTF     0x0E
#define INTFA    0x0E
#define INTFB    0x0F

#define INTCAP   0x10
#define INTCAPA  0x10
#define INTCAPB  0x11

#define GPIO     0x12
#define GPIOA    0x12
#define GPIOB    0x13

#define OLAT     0x14
#define OLATA    0x14
#define OLATB    0x15

/************************************
*                                   *
*      PSEUDO-GLOBAL VARIABLES      *
*                                   *
*************************************/

// Reverse address lookup tables for registers.
// Used mainly in error reporting

// 8 Bit names
const char * Mcp23017Registers8[22] = 
{
    "IODIRA",   // 00
    "IODIRB",   // 01
    "IPOLA",    // 02
    "IPOLB",    // 03
    "GPINTENA", // 04
    "GPINTENB", // 05
    "DEFVALA",  // 06
    "DEFVALB",  // 07
    "INTCONA",  // 08
    "INTCONB",  // 09
    "IOCONA",   // 0A
    "IOCONB",   // 0B
    "GPPUA",    // 0C
    "GPPUB",    // 0D
    "INTFA",    // 0E
    "INTFB",    // 0F
    "INTCAPA",  // 10
    "INTCAPB",  // 11
    "GPIOA",    // 12
    "GPIOB",    // 13
    "OLATA",    // 14
    "OLATB"     // 15
};

// 16 bit names
const char * Mcp23017Registers16[22] = 
{
    "IODIR",    // 00
    "IODIR",    // 01
    "IPOL",     // 02
    "IPOL",     // 03
    "GPINTEN",  // 04
    "GPINTEN",  // 05
    "DEFVAL",   // 06
    "DEFVAL",   // 07
    "INTCON",   // 08
    "INTCON",   // 09
    "IOCON",    // 0A
    "IOCON",    // 0B
    "GPPU",     // 0C
    "GPPU",     // 0D
    "INTF",     // 0E
    "INTF",     // 0F
    "INTCAP",   // 10
    "INTCAP",   // 11
    "GPIO",     // 12
    "GPIO",     // 13
    "OLAT",     // 14
    "OLAT"  	// 15
};




/************************************
*                                   *
*     STRUCT RELATED FUNCTIONS      *
*                                   *
*************************************/

HWConfig::HWConfig()
{
    DISSLEW = false;      // Leave slew rate control enabled
    INT_MIRROR = true;    // Mirror Interrupt pins
    INT_ODR = false;      // Interrupt is not an open drain
    INT_POL = false;      // Interrupt is Active-Low
}

uint8_t HWConfig::parse()
{
    uint8_t val = 0;
    if(DISSLEW)
        val |= 0x10;
    if(INT_MIRROR)
        val |= 0x40;
    if(INT_ODR)
        val |= 0x04;
    if(INT_POL)
        val |= 0x02;

    val |= 0x20; // Disable the address pointer

    return val;
}



//! Internal helper function that translates an IOConfig structure to an unsigned int denoting the proper configuration for an MCP23017

/************************************
*                                   *
*       CONNECTION SETUP            *
*                                   *
*************************************/

//! Open a new connection to the MCP23017 device, and initialize it.
Mcp23017::Mcp23017(    uint8_t adr, 
                        uint16_t iodir, 
                        uint16_t ipol,
                        uint16_t pullup, 
                        HWConfig hwcfg, 
                        bool swapAB)
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
    this->swapAB = swapAB;                   // set swapAB value;

    this->pwm_enabled = false;
    this->pwm_mask = 0x0000;
    this->pwm_prev_val = 0x0000;
    
    // Initialize the PWM values to null
    for(i=0; i< 16; i++)
    {
        this->pwm_values[i] = 0;
        this->pwm_v_values[i] = 0;
    }

    // Initialize to default pwm speed
    this->pwm_tick_delay_us = 800; // 800us * 16 steps would result in 78Hz
    this->pwm_ticks = 16;

    // Copy HWConfig to key
    this->hwConfig = hwcfg;

    // Use the following trick to assure we're talking to the MCP23017 in BANK=0 mode, so we can properly set the IOCON value

    try
    {
        // If we're in BANK1 mode, address 0x15 corresponds to the GPINTENB register, which should be inited to 0 anyway;
        tryI2CWrite8(0x05, 0x00);
        // Now write the proper IOCON value
        tryI2CWrite8(IOCON, hwcfg.parse());
        // Finally, initialize both GPINTENA and GPINTENB to 0x00
        tryI2CWrite8(GPINTENA, 0x00);
        tryI2CWrite8(GPINTENB, 0x00);
            
        // Further initialization
        
        // Set up the IO direction
        tryI2CWrite16(IODIR, iodir);
        // Set up the input polarity
        tryI2CWrite16(IPOL, ipol);
        // Set up the pullups
        tryI2CWrite16(GPPU, pullup);
    }
    catch(OperationFailedException x)
    {
        i2cClose(fp);
        throw x;
    }
}


//! Destructor 
Mcp23017::~Mcp23017()
{
    PwmStop();   // try to stop the PWM driver;
    i2cClose(fp);
}


/************************************
*                                   *
*     INTERRUPT SETTINGS            *
*                                   *
************************************/

//! Set interrupt config for the 
void Mcp23017::IntConfig( uint16_t intcon, uint16_t defval, uint16_t int_enable)
{
    tryI2CWrite16(INTCON,intcon);
    tryI2CWrite16(DEFVAL,defval);
    tryI2CWrite16(GPINTEN,int_enable);
}

//! Get Interrupt flags
uint16_t Mcp23017::getIntF()
{
    return tryI2CRead16(INTF);
}

//! Get state of input pins on latest interrupt 
uint16_t Mcp23017::getIntCap()
{
    return tryI2CRead16(INTCAP);
}

//! Set Default value for pins
void Mcp23017::setDefault( uint16_t value)
{
    tryI2CWrite16(DEFVAL, value);
}

//! Get Default value for pins
uint16_t Mcp23017::getDefault()
{
    return tryI2CRead16(DEFVAL);
}

//! Set Interrupt enable
void Mcp23017::setIntEnable( uint16_t value)
{
    tryI2CWrite16(GPINTEN, value);
}

//! Get Interrupt enable
uint16_t Mcp23017::getIntEnable()
{
    return tryI2CRead16(GPINTEN);
}

//! Set Interrupt control value
void Mcp23017::setIntControl(uint16_t value)
{
    tryI2CWrite16(INTCON, value);
}

//! Get Interrupt control value
uint16_t Mcp23017::getIntControl()
{
    return tryI2CRead16(INTCON);
}

/************************************
*                                   *
*     REGULAR I/O FUNCTIONS         *
*                                   *
************************************/

//! Set input polarity
void Mcp23017::setIPol(uint16_t value)
{
    tryI2CWrite16(IPOL, value);
}

//! Get input polarity
uint16_t Mcp23017::getIPol()
{
    return tryI2CRead16(IPOL);
}

//! Get output latch value
uint16_t Mcp23017::getOLat()
{
    return tryI2CRead16(OLAT);
}

//! Set IO Direction
void Mcp23017::setDirection(uint16_t value)
{
    tryI2CWrite16(IODIR, value);
}

//! Get IO Direction
uint16_t Mcp23017::getDirection()
{
    return tryI2CRead16(IODIR);
}

//! Set new output value of the I/O pins
void Mcp23017::setValue(uint16_t value)
{
    // if pwm is enabled, use a masked write with the inverse of the pwm_mask
    // to avoid overwriting pwm values
    if(this->pwm_enabled)
        tryI2CMaskedWrite16(GPIO,value,~this->pwm_mask);
    else
        tryI2CWrite16(GPIO, value);
}

//! Get current input value of the I/O pins
uint16_t Mcp23017::getValue()
{
    return tryI2CRead16(GPIO);
}

//! Set new output value for a subset of the I/O pins (masked by 'mask', where high bits indicat bits to set in the output)
void Mcp23017::setMaskedValue(uint16_t value, uint16_t mask)
{
    // if pwm is enabled, join the pwm_mask with this mask
    // to avoid overwriting pwm values
    if(this->pwm_enabled)
        mask &= ~this->pwm_mask;
        
    tryI2CMaskedWrite16(GPIO,value,mask);
}

//! Get the values of a specific pin
bool Mcp23017::getPin(uint8_t pin)
{
    uint16_t value;

    if(pin > 15)
        throw InvalidArgumentException("Pin number exceeds maximum pin id. Ignoring request to set pin value.");

    value = tryI2CRead16(GPIO);
    
    if(value & (1 << pin))
        return TRUE;
    else
        return FALSE;
}

//! Set the value of a specific pin
void Mcp23017::setPin(uint8_t pin, bool value)
{
    uint16_t newval, mask;

    if(pin > 15)
        throw InvalidArgumentException("Pin number exceeds maximum pin id. Ignoring request to set pin value.");

    // set mask
    mask = ( 1 << pin );

    // if pwm is enabled, join the pwm_mask with this mask
    // to avoid overwriting pwm values
    if(this->pwm_enabled)
        mask &= ~this->pwm_mask;
    
    if(mask == 0x00)
        throw InvalidArgumentException("Pin is active in PWM. Ignoring request to set pin value.");
    
    if(value)
    {
        newval = ( 1 << pin );
    }
    else
    {
        newval = 0;
    } 

    tryI2CMaskedWrite16(GPIO, newval, mask);
}

/************************************
*                                   *
*     PWM FUNCTIONS                 *
*                                   *
************************************/

//! Start the PWM routine for this I/O expander
void Mcp23017::PwmStart()
{

    if(!ThreadRunning())
    {
        ThreadStart();
    }
}

//! Stop the PWM routine for this I/O expander
void Mcp23017::PwmStop()
{
    if(ThreadRunning())
    {
        ThreadStop();
        // Set the PWM pins back to low
        tryI2CMaskedWrite16(OLAT, 0x0000, this->pwm_mask);
    }
}

//! Get the PWM value for a specific pin as 0-255 value (can be different from previously set value, due to rounding errors)
uint8_t Mcp23017::getPwmValue(uint8_t pin)
{

    if(pin > 15)
        throw InvalidArgumentException("Pin number exceeds maximum pin id. Ignoring request.");

    return this->pwm_v_values[pin]; // Return the cached 0-255 value of the pin
}

//! Set the PWM value for a specific pin as 0-255 value 
void Mcp23017::setPwmValue(uint8_t pin, uint8_t value)
{

    if(pin > 15)
        throw InvalidArgumentException("Pin number exceeds maximum pin id. Ignoring request.");

    this->pwm_v_values[pin] = value; // cache the provided value for returning and for updating pwm_value on change in pwm_ticks);
    this->pwm_values[pin] = value / (256/this->pwm_ticks);
}

//! Set the PWM value for a specific pin as 0-255 value and apply gamma correction for LED light levels
void Mcp23017::setPwmLedValue(uint8_t pin, uint8_t lightvalue)
{
    // Gamma correction table
    const uint8_t GammaToLinear[256] = 
    {
      0,   0,   0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0,   0,   1, 
      1,   1,   1,   1,   1,   1,   1,   1, 
      1,   2,   2,   2,   2,   2,   2,   2, 
      3,   3,   3,   3,   3,   4,   4,   4, 
      4,   5,   5,   5,   5,   6,   6,   6, 
      6,   7,   7,   7,   8,   8,   8,   9, 
      9,   9,  10,  10,  11,  11,  11,  12, 
     12,  13,  13,  14,  14,  14,  15,  15, 
     16,  16,  17,  17,  18,  18,  19,  19, 
     20,  21,  21,  22,  22,  23,  23,  24, 
     25,  25,  26,  27,  27,  28,  28,  29, 
     30,  31,  31,  32,  33,  33,  34,  35, 
     36,  36,  37,  38,  39,  39,  40,  41, 
     42,  43,  44,  44,  45,  46,  47,  48, 
     49,  50,  51,  51,  52,  53,  54,  55, 
     56,  57,  58,  59,  60,  61,  62,  63, 
     64,  65,  66,  67,  68,  70,  71,  72, 
     73,  74,  75,  76,  77,  78,  80,  81, 
     82,  83,  84,  86,  87,  88,  89,  91, 
     92,  93,  94,  96,  97,  98, 100, 101, 
    102, 104, 105, 106, 108, 109, 110, 112, 
    113, 115, 116, 118, 119, 120, 122, 123, 
    125, 126, 128, 129, 131, 132, 134, 136, 
    137, 139, 140, 142, 143, 145, 147, 148, 
    150, 152, 153, 155, 157, 158, 160, 162, 
    164, 165, 167, 169, 171, 172, 174, 176, 
    178, 179, 181, 183, 185, 187, 189, 191, 
    192, 194, 196, 198, 200, 202, 204, 206, 
    208, 210, 212, 214, 216, 218, 220, 222, 
    224, 226, 228, 230, 232, 234, 237, 239, 
    241, 243, 245, 247, 249, 252, 254, 255
    };

    if(pin > 15)
        throw InvalidArgumentException("Pin number exceeds maximum pin id. Ignoring request.");

    this->pwm_v_values[pin] = GammaToLinear[lightvalue]; // cache the (gamma-corrected) provided value for returning and for updating pwm_value on change in pwm_ticks);
    this->pwm_values[pin] = GammaToLinear[lightvalue] / (256/this->pwm_ticks);
}


//! Get the PWM state (on/off) for a specific pin
bool Mcp23017::getPwmState(uint8_t pin)
{
    if(pin > 15)
        throw InvalidArgumentException("Pin number exceeds maximum pin id. Ignoring request.");

    return ( (this->pwm_mask & (1 << pin)) > 0);
}

//! Set the PWM state (on/off) for a specific pin
void Mcp23017::setPwmState(uint8_t pin, bool state)
{
    if(pin > 15)
        throw InvalidArgumentException("Pin number exceeds maximum pin id. Ignoring request.");

    if(state)
        this->pwm_mask |= (1 << pin);
    else
        this->pwm_mask &= ~(1 << pin);
}

//! Set the PWM Configuration
void Mcp23017::setPwmConfig(uint32_t tick_delay_us, uint8_t ticks)
{
    int i;

    this->pwm_tick_delay_us = tick_delay_us;
    this->pwm_ticks = ticks;
    
    // update the actual PWM values, according to the 
    for(i=0;i<16;i++)
    {
        if(this->pwm_mask & (1 << i)) // Check if pwm_mask is enabled for this pin
        {    
            // if so, update the actually used pwm value, to reflect the new setting of ticks
            this->pwm_values[i] = this->pwm_v_values[i] / (256/this->pwm_ticks);
        }
    }
}

//! Get PWM Configuration
PwmConfig Mcp23017::getPwmConfig()
{

    PwmConfig pcfg;
    pcfg.tick_delay_us = pwm_tick_delay_us;
    pcfg.ticks = pwm_ticks;
    return pcfg;
}

//! Driver function for PWM thread
void Mcp23017::ThreadFunc(void)
{
    uint8_t ctr = 0; 
    int i;
    uint16_t pwm_out = 0x00;
    // 

    MakeRealtime();

    while(ThreadRunning())
    {

        pwm_out = this->pwm_mask; // start with pin high for all pins that have pwm_enabled

        // Now check for each pin if it should be low in this step...
        for(i=0;i<16;i++)
        {
            if(this->pwm_mask & (1 << i)) // Check if pwm_mask is enabled for this pin
            {
                if(ctr >= this->pwm_values[i] ) // check if the counter is greater than the pwm value for this pin. If so, turn it off.
                {
                    pwm_out &= ~(1 << i); // mask this pin out to 0, for it should be stopped
                }
            }
        }
        
        if(pwm_out != this->pwm_prev_val)
        {
            // write out the result, masked with the pwm_mask
            muteI2CMaskedWrite16(OLAT, pwm_out, this->pwm_mask);
            this->pwm_prev_val = pwm_out;
        }

        ctr++;
        if(ctr >= (this->pwm_ticks - 1))
        {
            ctr = 0;
        }

        usleep(this->pwm_tick_delay_us);
    }

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
uint8_t Mcp23017::tryI2CRead8(uint8_t reg)
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
        throw OperationFailedException("Error writing register address, attempted to read 8bit value from register %s (0x%2x) ",Mcp23017Registers8[reg], reg);
    else if(ret == -2)
        throw OperationFailedException("Error reading value, attempted to read 8bit value from register %s (0x%2x)",Mcp23017Registers8[reg], reg);
    else if(ret < 0)
        throw OperationFailedException("Unknown error [%d], attempted to read 8bit value from register %s (0x%2x)",ret, Mcp23017Registers8[reg], reg);

    return (uint8_t)ret;
}

/*! Try to write an 8 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Mcp23017::tryI2CWrite8(uint8_t reg, uint8_t value)
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
       throw OperationFailedException("Error writing to register, attempted to write 8bit value 0x%2x to register %s (0x%2x)",value, Mcp23017Registers8[reg], reg);
    else if(ret < 0)
       throw OperationFailedException("Unknown error [%d], attempted to write 8bit value 0x%2x to register %s (0x%2x)",ret, value, Mcp23017Registers8[reg], reg);


}

/*! Try to write specific bits in an 8 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Mcp23017::tryI2CMaskedWrite8(uint8_t reg, uint8_t value, uint8_t mask)
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
            throw OperationFailedException("Error writing register address, attempted to read 8bit value from register %s (0x%2x) for masked write", Mcp23017Registers8[reg], reg);
        else if(ret == -2)
            throw OperationFailedException("Error reading value, attempted to read 8bit value from register %s (0x%2x) for masked write", Mcp23017Registers8[reg], reg);
        else if(ret < 0)
            throw OperationFailedException("Unknown error [%d], attempted to read 8bit value from register %s (0x%2x) for masked write",ret, Mcp23017Registers8[reg], reg);
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
            throw OperationFailedException("Error writing to register, attempted to write 8bit value 0x%2x to register %s (0x%2x) for masked write",value, Mcp23017Registers8[reg], reg);
        else if(ret < 0)
            throw OperationFailedException("Unknown error [%d], attempted to write 8bit value 0x%2x to register %s (0x%2x) for masked write",ret, value, Mcp23017Registers8[reg], reg);
    } 

    // unlock process
    MutexUnlock();
}

/*! Try to read a 16 bit value from a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
uint16_t Mcp23017::tryI2CRead16(uint8_t reg)
{
    uint8_t hwreg = reg;
    int ret;

    // lock process
    MutexLock();

    // Ensure that we're initially reading from the right register in relation to the AB swap settings
    // (With swap enabled, we should start reading from the odd register, with swap disabled, we should start reading from the even register)
    if(this->swapAB)
        hwreg |= 0x01;
    else
        hwreg &= 0xFE;

    // Now start reading
    ret = i2cReadReg16(this->fp,hwreg);
    
    // unlock process
    MutexUnlock();
    
    // And properly set any error messages
    if(this->swapAB)
    {
        if(ret == -1)
            throw OperationFailedException("Error writing register address, attempted to read 16bit value from register %s (0x%2x) [AB Swap active, actual register read is %s (0x%2x)]", Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
        else if(ret == -2)
            throw OperationFailedException("Error reading value, attempted to read 16bit value from register %s (0x%2x) [AB Swap active, actual register read is %s (0x%2x)]", Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
        else if(ret < 0)
            throw OperationFailedException("Unknown error [%d], attempted to read 16bit value from register %s (0x%2x) [AB Swap active, actual register read is %s (0x%2x)]",ret, Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
    }
    else
    {
        if(ret == -1)
            throw OperationFailedException("Error writing register address, attempted to read 16bit value from register %s (0x%2x)", Mcp23017Registers16[reg], reg);
        else if(ret == -2)
            throw OperationFailedException("Error reading value, attempted to read 16bit value from register %s (0x%2x)", Mcp23017Registers16[reg], reg);
        else if(ret < 0)
            throw OperationFailedException("Unknown error [%d], attempted to read 16bit value from register %s (0x%2x)",ret, Mcp23017Registers16[reg], reg);
    }

return (uint16_t)ret;
}

/*! Try to write a 16 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Mcp23017::tryI2CWrite16(uint8_t reg,uint16_t value)
{
    uint8_t hwreg = reg;
    int ret;

    // lock process
    MutexLock();

    // Ensure that we're initially reading from the right register in relation to the AB swap settings
    // (With swap enabled, we should start reading from the odd register, with swap disabled, we should start reading from the even register)
    if(this->swapAB)
        hwreg |= 0x01;
    else
        hwreg &= 0xFE;

    // Now start writing
    ret = i2cWriteReg16(this->fp,hwreg, value);

    // unlock process
    MutexUnlock();

    // And properly set any error messages
    if(this->swapAB)
    {
        if(ret == -1)
           throw OperationFailedException("Error writing to register, attempted to write 16bit value 0x%4x to register %s (0x%2x) [AB Swap active, actual register read is %s (0x%2x)]",value, Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
        else if(ret < 0)
           throw OperationFailedException("Unknown error [%d], attempted to write 16bit value 0x%4x to register %s (0x%2x) [AB Swap active, actual register read is %s (0x%2x)]",ret, value, Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
    }
    else
    {
        if(ret == -1)
            throw OperationFailedException("Error writing to register, attempted to write 16bit value 0x%4x to register %s (0x%2x)",value, Mcp23017Registers16[reg], reg);
        else if(ret < 0)
            throw OperationFailedException("Unknown error [%d], attempted to write 16bit value 0x%4x to register %s (0x%2x)",ret, value, Mcp23017Registers16[reg], reg);
    }
    

}

/*! Try to write specific bits in a 16 bit value to a register
    In case of an error, The IOKey error value will be, 
    and the function will return prematurely.
*/
void Mcp23017::tryI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask)
{
    int ret;
    uint16_t newval;
    uint8_t hwreg = reg;
    
    // lock process
    MutexLock();

    // Ensure that we're initially reading from the right register in relation to the AB swap settings
    // (With swap enabled, we should start reading from the odd register, with swap disabled, we should start reading from the even register)
    if(this->swapAB)
        hwreg |= 0x01;
    else
        hwreg &= 0xFE;

    // read current value;
    if( (ret = i2cReadReg16(this->fp,hwreg)) < 0)
    {
        // unlock process
        MutexUnlock();
        
        // And properly set any error messages
        if(this->swapAB)
        {
            if(ret == -1)
                throw OperationFailedException("Error writing register address, attempted to read 16bit value from register %s (0x%2x) for masked write [AB Swap active, actual register read is %s (0x%2x)]", Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
            else if(ret == -2)
                throw OperationFailedException("Error reading value, attempted to read 16bit value from register %s (0x%2x) for masked write [AB Swap active, actual register read is %s (0x%2x)]", Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
            else if(ret < 0)
                throw OperationFailedException("Unknown error [%d], attempted to read 16bit value from register %s (0x%2x) for masked write [AB Swap active, actual register read is %s (0x%2x)]",ret, Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
        }
        else
        {
            if(ret == -1)
                throw OperationFailedException("Error writing register address, attempted to read 16bit value from register %s (0x%2x) for masked write", Mcp23017Registers16[reg], reg);
            else if(ret == -2)
                throw OperationFailedException("Error reading value, attempted to read 16bit value from register %s (0x%2x) for masked write", Mcp23017Registers16[reg], reg);
            else if(ret < 0)
                throw OperationFailedException("Unknown error [%d], attempted to read 16bit value from register %s (0x%2x) for masked write",ret, Mcp23017Registers16[reg], reg);
        }
    } 
    
    // copy result to new variable
    newval = (uint16_t)(ret);
    // keep only the non-masked bits
    newval &= ~mask;
    // overwrite the masked bits with the new value
    newval |= (value & mask);
    
    if( (ret = i2cWriteReg16(this->fp,hwreg,newval)) < 0)
    {
        // unlock process
        MutexUnlock();

        // And properly set any error messages
        if(this->swapAB)
        {
            if(ret == -1)
               throw OperationFailedException("Error writing to register, attempted to write 16bit value 0x%4x to register %s (0x%2x) for masked write [AB Swap active, actual register read is %s (0x%2x)]",value, Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
            else if(ret < 0)
               throw OperationFailedException("Unknown error [%d], attempted to write 16bit value 0x%4x to register %s (0x%2x) for masked write [AB Swap active, actual register read is %s (0x%2x)]",ret, value, Mcp23017Registers16[reg], reg, Mcp23017Registers16[hwreg], hwreg);
        }
        else
        {
            if(ret == -1)
                throw OperationFailedException("Error writing to register, attempted to write 16bit value 0x%4x to register %s (0x%2x) for masked write",value, Mcp23017Registers16[reg], reg);
            else if(ret < 0)
                throw OperationFailedException("Unknown error [%d], attempted to write 16bit value 0x%4x to register %s (0x%2x) for masked write",ret, value, Mcp23017Registers16[reg], reg);
        }
    } 
    
    // unlock process
    MutexUnlock();
}

/*! Attempt to write specific bits in a 16 bit value to a register
    In case of an error, the function will return prematurely, 
    without setting an error.
*/
void Mcp23017::muteI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask)
{
    int result;
    uint16_t newval;
    uint8_t hwreg = reg;
   
    // lock process
    MutexLock();
    // Ensure that we're initially reading from the right register in relation to the AB swap settings
    // (With swap enabled, we should start reading from the odd register, with swap disabled, we should start reading from the even register)
    if(this->swapAB)
        hwreg |= 0x01;
    else
        hwreg &= 0xFE;

    // read current value;
    if( (result = i2cReadReg16(this->fp,hwreg)) < 0)
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
    
    if( (result = i2cWriteReg16(this->fp,hwreg,newval)) < 0)
    {
//        fprintf(stderr,"WARNING: Got error code [%d] attempting to write\r\n",result);
        // unlock process
        MutexUnlock();
        return;
    } 
    
    // unlock process
    MutexUnlock();

}
