#ifndef __IOGROUP_PCA9685_HPP
#define __IOGROUP_PCA9685_HPP

#include "iogroup-hwpwm.hpp"
#include "pca9685/pca9685.hpp"


class IoGroupPCA9685: public IoGroupHwPwm
{
public:
    IoGroupPCA9685(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry);
    ~IoGroupPCA9685();

protected:
    // Overridden to set the actual PWM value
    virtual void setPwmPin(PwmPin *pin);

    // Overridden to set the actual PWM value
    virtual void getPwmPin(PwmPin *pin);

    // Overridden to prepare pins during configuration
    virtual void preparePwmPin(PwmPin *pin);

    // Called at the start of the configuration round to allow for subclass
    // specific settings to be set in the config
    virtual void beginConfig(libconfig::Setting &setting);
    
    // called when registering pins.
	virtual uint16_t getPinId(const int32_t &i);

    // Called at the end of the configuration round to allow the subclass to 
    // finalize configuration
    virtual void endConfig(void);

private:
    Pca9685::Pca9685Config cfg;
    Pca9685 * pca;

	uint8_t	 hw_address;
};

#endif//__IOGROUP_MCP23017_HPP
