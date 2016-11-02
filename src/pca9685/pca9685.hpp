#ifndef __PCA9685_H_
#define __PCA9685_H_

#include "../exception/baseexceptions.hpp"
#include "../thread/thread.hpp"
#include <stdint.h>

/*! \file MCP23017 interface functions. Header file.
*/

/************************************
*                                   *
*       MAIN CLASS                  *
*                                   *
*************************************/

class Pca9685 : Thread
{
public:
	// Structure for IO Configuration
	//! \typedef Pca9685Config Structure containing hardware configuration for the MCP23017
	class Pca9685Config
	{
		friend class Pca9685; // Allow
	public:
		enum OutNotEnabledMode { OutputLow = 0, OutputHigh = 1, OutputHighZ = 2 };
		enum OutputDriveType { OpenDrain = 0, TotemPole = 1};
	public:

		// Mode2
		bool Invert; /*!< Invert output logic state, Default: false*/
		bool OutputChangeOnAck; /*!< Change outputs on Ack instead of stop command, Default: false */
		OutputDriveType OutputDrive; /*!< Type of output driver, Default: TotemPole*/
		OutNotEnabledMode OutNotEnabled; /*!< Status of ouputs when /OE = 1, Default: OutputLow */

		// Mode1
		bool ExtClk;

		// Prescale
		uint32_t OscillatorClock;
		uint16_t Frequency;

		Pca9685Config();
		uint8_t getMode1(); /*!< parse into usable uint8_t */
		uint8_t getMode2(); /*!< parse into usable uint8_t */
		uint8_t getPrescaler(); /*!< parse into usable uint8_t prescaler value */
		uint16_t getActualFrequency(); /*!< return actual frequency used */

		void setMode1(uint8_t); /*!< parse from read uint8_t */
		void setMode2(uint8_t);/*!< parse from read uint8_t */
	private:
		// Allow this only to be set by friend class Pca9685
		// Mode1
		bool Restart;
		bool AutoIncrement;
		bool Sleep;

		// Mode 2
		bool Sub1;
		bool Sub2;
		bool Sub3;
		bool AllCall;

		double round(double number);
	};

private:
	uint8_t     adr;                // I2C Address of the IO expander chip
	int         fp;                 // File pointer for the I2C connection

	Pca9685Config  config;     	// Initial configuration of the chip

	uint8_t     tryI2CRead8 (uint8_t reg);
	void        tryI2CWrite8(uint8_t reg, uint8_t value);
	void        tryI2CMaskedWrite8(uint8_t reg, uint8_t value, uint8_t mask);

	uint16_t    tryI2CRead16(uint8_t reg);
	void        tryI2CWrite16(uint8_t reg, uint16_t value);
	void        tryI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask);
	void        muteI2CMaskedWrite16(uint8_t reg, uint16_t value, uint16_t mask);

public:
	//! Open a new connection to the PCA9685 device, and initialize it.
	/*!
		\param adr The I2C address of the IC to connect to
		\param config Configuration for the IC
	*/
	Pca9685(   	uint8_t     adr, Pca9685Config    config);
	~Pca9685();



	/************************************
	*                                   *
	*     REGULAR I/O FUNCTIONS         *
	*                                   *
	************************************/

	//! Set set new
	void setValue(uint8_t pin, uint16_t off, uint16_t on);

	//! Get current on time of the PWM
	uint16_t getOnValue(uint8_t pin);

	//! Get current off time of the PWM
	uint16_t getOffValue(uint8_t pin);

};


#endif
