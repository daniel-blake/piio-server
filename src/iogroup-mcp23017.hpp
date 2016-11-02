#ifndef __IOGROUP_MCP23017_HPP
#define __IOGROUP_MCP23017_HPP

#include "iogroup-digital.hpp"
#include "mcp23017/mcp23017.hpp"
#include "gpio/gpio.hpp"
#include "thread/thread.hpp"


class IoGroupMCP23017: public IoGroupDigital
{
public:
    IoGroupMCP23017(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry);
    ~IoGroupMCP23017();

    void onInterrupt(GpioPin * sender, GpioEdge edge, bool pinval);
    void onInterruptError(Thread * sender, ThreadException x);

protected:
    // Override in child to get input value by id
    virtual bool getInputPin(uint16_t id);
    // Override in child to actually set the output by id
    virtual bool setOutputPin(uint16_t id, bool value);
    // Override in child to actually set the PWM value
    // Throws FeatureNotImplementedException unless overridden in subclass
    virtual bool setPwm(uint16_t id, uint8_t value);
    
    // Called at the start of the configuration round to allow for subclass
    // specific settings to be set in the config
    virtual void beginConfig(libconfig::Setting &setting);
    
    // called when registering pins.
	virtual uint16_t getPinId(int32_t i);
    virtual void prepareInputPin(uint16_t pinid, bool invert, bool pullup, bool pulldown, bool inten);
    virtual void prepareOutputPin(uint16_t pinid);
    virtual void preparePwmPin(uint16_t pinid);

    // Called at the end of the configuration round to allow the subclass to 
    // finalize configuration
    virtual void endConfig(void);

private:
    Mcp23017 * mcp;
    GpioPin * intpin;
    
	uint8_t	 hw_intpin;
	bool	 hw_use_gpioint;
	
	uint8_t	 hw_address;
	bool 	 hw_swapab;
	bool	 hw_intpol;
	bool 	 hw_intodr;
    uint16_t hw_iodir;
    uint16_t hw_ipol;
	uint16_t hw_pullup;
	uint16_t hw_inten;

    uint32_t hw_tick_delay_us; 
    uint8_t  hw_ticks; 
    
	std::set<uint16_t> pwm_pins;
	std::set<uint16_t> active_pwms;
    std::map<uint16_t, bool> initial_outputvalue;

    uint32_t hw_noisetimeout_ms; 
    uint16_t hw_noisemargin;
    int64_t noiseTimeout;
    
    boost::signals2::connection onInterruptConnection;
	boost::signals2::connection onInterruptErrorConnection;    
};

#endif//__IOGROUP_MCP23017_HPP