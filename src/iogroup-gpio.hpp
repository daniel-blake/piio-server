#ifndef __IOGROUP_GPIO_HPP
#define __IOGROUP_GPIO_HPP

#include "gpio/gpio.hpp"
#include "iogroup-softpwm.hpp"


class IoGroupGpio: public IoGroupSoftPWM
{
public:
    IoGroupGpio(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry);
    ~IoGroupGpio();

protected:
    // Override in child to get input value by id
    virtual bool getInputPin(uint16_t id);
    // Override in child to actually set the output by id
    virtual bool setOutputPin(uint16_t id, bool value);
    // Override in child to actually set the PWM value
    
    // Called at the start of the configuration round to allow for subclass
    // specific settings to be set in the config
    virtual void beginConfig(libconfig::Setting &setting);
    
    // called when registering pins.
	virtual uint16_t getPinId(int32_t i);

    virtual void prepareInputPin(uint16_t pinid, bool invert, bool pullup, bool pulldown, bool inten);
    virtual void prepareOutputPin(uint16_t pinid);

    // Called at the end of the configuration round to allow the subclass to 
    // finalize configuration
    virtual void endConfig(void);

private:
    std::map<uint16_t, GpioPin*> gpioPins;
    std::map<uint16_t, bool> gpioInvert;
    std::set<uint16_t> gpioInputPins;
    std::set<uint16_t> gpioIntPins;
    std::set<uint16_t> gpioOutputPins;
    
    std::map<uint16_t, boost::signals2::connection> gpioIntConnection;
    std::map<uint16_t, boost::signals2::connection> gpioIntErrorConnection;

    void onInterrupt(GpioPin * sender, GpioEdge edge, bool pinval);
    void onInterruptError(Thread* sender, ThreadException x);

};

#endif//__IOGROUP_MCP23017_HPP