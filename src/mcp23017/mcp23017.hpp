#ifndef __MCP23017_H_
#define __MCP23017_H_

#include "../exception/baseexceptions.hpp"
#include "../thread/thread.hpp"
#include <stdint.h>

/*! \file MCP23017 interface functions. Header file.
*/

/****************************
*                           *
*     PUBLIC STRUCTS        *
*                           *
*****************************/

// Structure for IO Configuration
//! \typedef HWConfig Structure containing hardware configuration for the MCP23017
class HWConfig
{
    public:
        bool DISSLEW ; /*!< Disable slew rate control (useful in noisy circuits operating at 400kHz and below), Default: false*/
        bool INT_MIRROR; /*!< Mirror INTA and INTB pins: Both pins act as a single INT pin responding to both port A and port B, Default: true */
        bool INT_ODR; /*!< Set interrupt pins as open drain output, Default: false */
        bool INT_POL; /*!< Set interrupt polatity: 1 is active-high, 0 is active-low, Default: false */
       
        HWConfig(); 
        uint8_t parse(); /*!< parse into usable uint8_t */
};

// Structure for IO Configuration
//! \struct PWMConfig Structure containing current PWM configuration (only used as return value)
struct PwmConfig
{
    uint32_t tick_delay_us;     /*!< Delay between ticks in us */
    uint8_t ticks;            /*!< Number of ticks in a cycle */
};

/************************************
*                                   *
*       MAIN CLASS                  *
*                                   *
*************************************/

class Mcp23017 : public Thread
{
    private:
        uint8_t     adr;                // I2C Address of the IO expander chip
        int         fp;                 // File pointer for the I2C connection
        bool        swapAB;             // Option to swap ports A and B for 8bit and 16 bit operations

        HWConfig    hwConfig;           // Configuration of the port

        bool        pwm_enabled;        // Boolean used to stop the PWM thread safely
        uint16_t    pwm_mask;           // Masks the pins on which PWM operates
        uint8_t     pwm_values[16];     // PWM value for each possible pin
        uint8_t     pwm_v_values[16];   // Cache for the PWM values provided, before downconversion
        uint32_t    pwm_tick_delay_us;  // Interval between PWM steps in us
        uint8_t     pwm_ticks;          // Number of PWM steps before coming full circle
        uint16_t    pwm_prev_val;       // Keeps state of pwm output;

        uint8_t     tryI2CRead8 (uint8_t reg);
        void        tryI2CWrite8(uint8_t reg, uint8_t value);
        void        tryI2CMaskedWrite8(uint8_t reg, uint8_t value, uint8_t mask);
        
        uint16_t    tryI2CRead16(uint8_t reg);
        void        tryI2CWrite16(uint8_t reg, uint16_t value);
        void        tryI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask);
        void        muteI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask);

    protected:
        virtual void ThreadFunc(void);  // Override this if you want the entire function customized
        
    public:
        //! Open a new connection to the MCP23017 device, and initialize it.
        /*!
            \param adr The I2C address of the IC to connect to
            \param iodir Initial I/O direction mask (HIGH is input, LOW is output)
            \param ipol Initial input polarity mask (HIGH inverts polarity, LOW keeps it the same)
            \param pullup Initial pullup mask (HIGH enables pullup, LOW disables)
            \param hwcfg Hardware configuration for the IC
            \param swapAB Swap A and B registers in the 16 bit operations
            \sa MCP23017Close
        */
        Mcp23017(   uint8_t     adr, 
                    uint16_t    iodir, 
                    uint16_t    ipol,
                    uint16_t    pullup, 
                    HWConfig    hwcfg, 
                    bool        swapAB
                );
        
        ~Mcp23017();

        /************************************
        *                                   *
        *     INTERRUPT FUNCTIONS           *
        *                                   *
        ************************************/

        //! Set interrupt config for the MCP23017
        /*! 
            \param intcon The interrupt control mask (HIGH bit compares to default value, LOW bit to previous value)
            \param defval Initial default value for the pins
            \param int_enable The interrupt enable mask (HIGH bit enables, LOW bit disables)
        */
        void IntConfig(uint16_t defval, uint16_t intcon, uint16_t int_enable);

        //! Get the interrupt condition of the interrupt-enabled pins
        /*! 
            \return Current interrupt trigger state for pins
        */
        uint16_t getIntF();

        //! //! Get state of input pins on latest interrupt 
        /*! 
            \return The state of the interrupt pins on the last interrupt
        */
        uint16_t getIntCap();

        //! Set Default value for pins
        /*! 
            \param value The new default value for the pins
        */
        void setDefault(uint16_t value);

        //! Get default value for input pins
        /*! 
            \return Current default value for the input pins
        */
        uint16_t getDefault();

        //! Set Interrupt enable mask
        /*! 
            \param value The new interrupt enable mask (HIGH bit enables, LOW bit disables)
        */
        void setIntEnable(uint16_t value);

        //! Get Interrupt enable mask
        /*! 
            \return Current interrupt enable mask
        */
        uint16_t getIntEnable();

        //! Set Interrupt control value
        /*! 
            \param value The new interrupt control mask (HIGH bit compares to default value, LOW bit to previous value)
        */
        void setIntControl(uint16_t value);

        //! Get Interrupt control value
        /*! 
            \return Current interrupt control state
        */
        uint16_t getIntControl();

        /************************************
        *                                   *
        *     REGULAR I/O FUNCTIONS         *
        *                                   *
        ************************************/

        //! Set input polarity
        /*! 
            \param value The input polarity - HIGH inverts input polarity, LOW does not 
        */
        void setIPol(uint16_t value);

        //! Get input polarity
        /*! 
            \return Current value of the input polarity
        */
        uint16_t getIPol();

        //! Get output latch value
        /*! 
            \return Current value of the output latches
        */
        uint16_t getOLat();

        //! Set IO Direction
        /*! 
            \param key The new I/O direction (HIGH bit is input, LOW bit is output)
        */
        void setDirection(uint16_t value);

        //! Get IO Direction
        /*! 
            \return Current value of the IO direction
        */
        uint16_t getDirection();

        //! Set new output value of the I/O pins
        /*! 
            \param value The new output value of the pins
        */
        void setValue(uint16_t value);

        //! Get current input value of the I/O pins
        /*! 
            \return Current value of the port
        */
        uint16_t getValue();

        //! Set new output value for a subset of the I/O pins (masked by 'mask', where high bits indicat bits to set in the output)
        /*! 
            \param value The new output value of the pins
            \param mask The mask indicating which bits to apply (HIGH bit indicated bit will be applied)
        */
        void setMaskedValue(uint16_t value, uint16_t mask);
        //! Get the values of a specific pin;
        /*! 
            \return The current state (1 or 0) of the pin
        */
        bool getPin(uint8_t pin);

        //! Set the value of a specific pin
        /*! 
            \param value Boolean indicating the new value of the pin
        */
        void setPin(uint8_t pin, bool value);

        /************************************
        *                                   *
        *     PWM FUNCTIONS                 *
        *                                   *
        ************************************/

        //! Start the PWM routine for this I/O expander
        void PwmStart();

        //! Stop the PWM routine for this I/O expander
        void PwmStop();

        //! Get the PWM value for a specific pin (can be different from previously set value, due to rounding errors)
        /*! 
            \param pin The pin number of which to retrieve the value
            \return The current PWM value for the pin
        */
        uint8_t getPwmValue(uint8_t pin);

        //! Set the PWM value for a specific pin as 0-255 value
        /*! 
            \param pin The pin number of which to retrieve the value
            \param value The new PWM value
        */
        void setPwmValue(uint8_t pin, uint8_t value);

        //! Set the PWM value for a specific pin as 0-255 value and apply gamma correction for LED light levels
        /*! 
            \param pin The pin number of which to retrieve the value
            \param value The new PWM value
        */
        void setPwmLedValue(uint8_t, uint8_t lightvalue);

        //! Get the PWM state (on/off) for a specific pin
        /*! 
            \param pin The pin number of which to retrieve the state
            \return Boolean indicating the current state of the pin
        */
        bool getPwmState(uint8_t pin);

        //! Set the PWM state (on/off) for a specific pin
        /*! 
            \param pin The pin number of which to set the state
            \param state Boolean indicating the new state of pwm on this pin (HIGH is enabled, LOW is disabled) 
        */
        void setPwmState(uint8_t pin, bool state);

        //! Set the PWM Configuration
        /*! Default value results in a 4 bit PWM of around 80Hz
            Note: PWM has a fairly heavy CPU load. With default settings (4 bit pwm, 800us/step) this is around 4.5% on a raspberry pi.
            Doubling the resolution to 5 bit, 400us/step takes around 7% cpu load. Use at your own discretion.

            \param tick_delay_us The number of microseconds between PWM ticks (default: 313 us)
            \param ticks The number of ticks in one PWM cycle (default: 32 ticks)
        */
        void setPwmConfig(uint32_t tick_delay_us, uint8_t ticks);

        //! Get PWM Configuration
        /*! 
            \return PWMConfig object containing the current settings
        */
        PwmConfig getPwmConfig();

};


#endif