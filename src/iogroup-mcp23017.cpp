#include "iogroup-mcp23017.hpp"
#include <sstream>

// For getting current time
#include <time.h>
#ifndef CLOCK_MONOTIC
#   define CLOCK_MONOTIC CLOCK_REALTIME
#endif


using namespace std;

// function to get current time in ms
int64_t now_ms(void);

// get current time in milliseconds
int64_t now_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTIC, &now);

    return ((int64_t)now.tv_sec)*1000LL + (now.tv_nsec/1000000);
}


IoGroupMCP23017::IoGroupMCP23017(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry)
 : IoGroupDigital(connection,dbuspath, registry)
{
	this->hw_intpin = 0xFF;
	this->hw_use_gpioint = false;
	
	this->hw_address = 0x20;
	this->hw_swapab = false;
	this->hw_intpol = false;
	this->hw_intodr = false;
    this->hw_iodir = 0xffff;
    this->hw_ipol = 0x0000;
	this->hw_pullup = 0x0000;
	this->hw_inten = 0x0000;

    this->hw_tick_delay_us = 800; //us
    this->hw_ticks = 16; // ticks
    
    this->hw_noisetimeout_ms = 400;
    this->hw_noisemargin = 4;

}

// Called at the start of the configuration round to allow for subclass
// specific settings to be set in the config
void IoGroupMCP23017::beginConfig(libconfig::Setting &setting)
{
    
    uint32_t cfg_intpin_id;
    uint32_t cfg_i2c_address;
    uint32_t u_cfg_tmp;

    // Read PWM settings if provided
    if(setting.lookupValue("pwm-tickdelay-us",u_cfg_tmp))
        this->hw_tick_delay_us = (uint16_t)u_cfg_tmp;
        
    if(setting.lookupValue("pwm-ticks",u_cfg_tmp))
    {
        if(u_cfg_tmp > 255)
            u_cfg_tmp = 255;
            
        this->hw_ticks = (uint16_t)u_cfg_tmp;
    }
    // Read Noise margin settings if provided
    if(setting.lookupValue("noisemargin",u_cfg_tmp))
        this->hw_noisemargin = (uint16_t)u_cfg_tmp;

    setting.lookupValue("noisetimeout",this->hw_noisetimeout_ms);

    clog << kLogInfo << this->Name() << ": Initializing with: " << endl;
    clog << kLogInfo << "pwm-tickdelay-us: " << (int32_t)this->hw_tick_delay_us << endl;
    clog << kLogInfo << "pwm-ticks: " << (int32_t)this->hw_ticks << endl;
    clog << kLogInfo << "noisemargin: " << (int32_t)this->hw_noisemargin << endl;
    clog << kLogInfo << "noisetimeout: " << (int32_t)this->hw_noisetimeout_ms << endl;

    // Check if an interrupt pin was provided
    if(setting.lookupValue("intpin", cfg_intpin_id))
    {
        if(GpioPin::CheckPin(cfg_intpin_id))
        {

			// store int pin etc
			this->hw_use_gpioint = true;
			this->hw_intpin = (uint16_t)cfg_intpin_id;

			// Read interrupt config settings if set
			setting.lookupValue("int-opendrain",this->hw_intodr);
			setting.lookupValue("int-activehigh",this->hw_intpol);
		
            clog << kLogInfo << "intpin: " << (int32_t)this->hw_intpin << endl;
            clog << kLogInfo << "int-opendrain: " << this->hw_intodr << endl;
            clog << kLogInfo << "int-activehigh: " << this->hw_intpol << endl;
        }
    }
    
    // Figure out MCP configuration
    if(setting.lookupValue("address",cfg_i2c_address))
    {
        if(cfg_i2c_address <= 0x7F)
        {
            
            if(cfg_i2c_address >= 0x20 && cfg_i2c_address  <= 0x27)
            {
            
				this->hw_address = (uint8_t)cfg_i2c_address;
				setting.lookupValue("swapab",this->hw_swapab);

                clog << showbase << internal << setfill('0');
                clog << kLogInfo << "addres: 0x" << hex << cfg_i2c_address << dec << endl;
                clog << kLogInfo << "swapab: " << this->hw_swapab << endl;            
            }
            else
            {
                clog << showbase << internal << setfill('0');
                clog << kLogError << this->Name() << ": Invalid I2C address for an MCP23017 chip (must be 0x20...0x27): " << hex  << cfg_i2c_address << dec  << endl;  
                throw ConfigInvalidException("Incompatible I2C Address provided");
            }
        }
        else
        {
            clog << showbase << internal << setfill('0');
            clog << kLogError << this->Name() << ": Invalid I2C address provided: " << hex << cfg_i2c_address<< dec  << endl;  
            throw ConfigInvalidException("Invalid I2C Address provided");
        }
    }
    else
    {
        clog << kLogError << this->Name() << ": No I2C address provided" << endl;  
        throw ConfigInvalidException("MCP23017 io type given, but no I2C Address provided");
    }

}

// called when registering pins.
uint16_t IoGroupMCP23017::getPinId(int32_t i)
{
	if(i < 16 && i >= 0)
	{
		return (uint16_t)i;
	}
	else
	{
		std::ostringstream oss;
		oss << "Pin id " << i << " is not valid for this IO Group type (must be < 16 and >= 0)";
		throw IoPinInvalidException(oss.str());
	}
}

void IoGroupMCP23017::prepareInputPin(uint16_t pinid, bool invert, bool pullup, bool pulldown, bool inten)
{

	if (invert)
	{
		this->hw_ipol |= (1 << pinid);
	}
	if (pullup) // MCP23017 does not support pulldown
	{
		this->hw_pullup |= (1 << pinid);
	}
	if (inten)
	{
		this->hw_inten |= (1 << pinid);
	}

}

void IoGroupMCP23017::prepareOutputPin(uint16_t pinid)
{

	this->hw_iodir &= ~(1 << pinid);
}

void IoGroupMCP23017::preparePwmPin(uint16_t pinid)
{

	this->hw_iodir &= ~(1 << pinid);
	this->pwm_pins.insert(pinid);
}

// Called at the end of the configuration round to allow the subclass to 
// finalize configuration
void IoGroupMCP23017::endConfig(void)
{
    string name = this->Name();
    string usage_sda = "I2C SDA";
    string usage_scl = "I2C SCL";

    if(
        this->gpioRegistry.requestSharedLease(GpioPin::VerifyPin(2),name,usage_sda) &&
        this->gpioRegistry.requestSharedLease(GpioPin::VerifyPin(3),name,usage_scl)
       )
    {
        clog << kLogInfo << "Opening MCP23017 IO expander on I2C address " << this->hw_address << endl;
        this->noiseTimeout = 0; // Value of 0 means: no noise timeout
        
        try
        {
            
            HWConfig hwConfig;
            unsigned short value;
            hwConfig.DISSLEW = false;   // Leave slew rate control enabled
            hwConfig.INT_MIRROR = true; // Interconnect I/O pins
            hwConfig.INT_ODR = this->hw_intodr;   // Interrupt is not an open drain
            hwConfig.INT_POL = this->hw_intpol;    // Interrupt is Active-Low 
            
            // Initialize chip system
            this->mcp = new Mcp23017( this->hw_address,   // adr
                                     this->hw_iodir,     // iodir
                                     this->hw_ipol,      // ipol
                                     this->hw_pullup,    // pullup
                                     hwConfig,       	// Hardware Config
                                     this->hw_swapab);  	// Swap A/B

            // apply pwm config settings
            this->mcp->setPwmConfig(hw_tick_delay_us, hw_ticks);  

            try
            {

                if (this->hw_use_gpioint)
                {
                    string usage_int = "MCP23017 Interrupt";
                    if(this->gpioRegistry.requestSharedLease(this->hw_intpin,name,usage_int))
                    {
                        clog << kLogInfo << "Opening GPIO pin " << this->hw_intpin << " for interrupt listening" << endl;
                        // open pin
                        // Hardware config
                        intpin = new GpioPin(   this->hw_intpin,   							// Pin number
                                                kDirectionIn,   							// Data direction
                                                (this->hw_intpol)?kEdgeRising:kEdgeFalling	// Interrupt edge
                                            );  

                        // Start interrupt event on interrupt pin
                        intpin->InterruptStart();
                        // Initialize interrupts
                       
                        onInterruptErrorConnection.disconnect();
                        onInterruptConnection.disconnect();
                        onInterruptErrorConnection = intpin->onThreadError.connect(boost::bind(&IoGroupMCP23017::onInterruptError, this, _1, _2));
                        onInterruptConnection = intpin->onInterrupt.connect(boost::bind(&IoGroupMCP23017::onInterrupt, this, _1, _2, _3));

                        clog << kLogInfo << "Enabling interrupts on IO Expander" << endl;

                        // Configure interrupt
                        mcp->IntConfig( 0x0000,  		// defval
                                        0x0000,  		// intcon
                                        this->hw_inten); // int enable


                        clog << kLogInfo << "Starting interrupt listener on GPIO pin " <<  this->hw_intpin << endl;

                    }
                    else
                    {
                        clog << kLogError << "Could not use pin " << this->hw_intpin << " for interrupt listening, because it is already registered by io group " << this->gpioRegistry.getCurrentLeaser(this->hw_intpin) << " for use as " << this->gpioRegistry.getCurrentUsage(this->hw_intpin) << endl;
                    }
                }


                clog << kLogInfo << "Performing initial read of IO Expander values to clear any pending interrupts and initialize Jack state" << endl;
                // Read initial value to clear any current interrupts
                value = mcp->getValue();

                // Now setup the PWM pins
                std::set<uint16_t>::iterator p;
                for(p = this->pwm_pins.begin(); p != this->pwm_pins.end(); ++p) 
                {
                    this->mcp->setPwmState(*p,true);
                }
                
                // And set up initialization values for output pins
                std::map<uint16_t, bool>::iterator it;
                for(it = initial_outputvalue.begin(); it != initial_outputvalue.end(); it++) {
                    // it->first = key
                    // it->second = value
                    this->mcp->setPin(it->first, it->second);
                }
                
                // clear list
                initial_outputvalue.clear();

            }
            catch(OperationFailedException x)
            {
                clog << kLogError << this->Name() << ": Error setting up MCP interface: " << x.Message() << endl;
                delete mcp; mcp = NULL;
                throw x;
            }
                
        }
        catch(OperationFailedException x)
        {
            clog << kLogError << this->Name() << ": Error setting up MCP interface: " << x.Message() << endl;
            delete intpin; intpin = NULL;
            throw x;
        }
    }
    else
    {   
        clog << kLogError << "I2C Pins are already registered for exclusive use" << endl;
        throw OperationFailedException("I2C Pins are already registered for exclusive use");
    }
}

IoGroupMCP23017::~IoGroupMCP23017()
{
    // Make sure the gpiopin and the mcp object are removed
	if(intpin != NULL)
		delete intpin; intpin = NULL;
	
	if(mcp != NULL)
		delete mcp; mcp = NULL;
}

// Override in child to get input value by id
bool IoGroupMCP23017::getInputPin(uint16_t id)
{
	return this->mcp->getPin(id);
}
// Override in child to actually set the output by id
bool IoGroupMCP23017::setOutputPin(uint16_t id, bool value)
{
    if(this->mcp != NULL)
    {
        this->mcp->setPin(id,value);
        return true;
    }
    else
    {
        this->initial_outputvalue[id] = value;
        return true;
    }
}

// Override in child to actually set the PWM value
// Throws FeatureNotImplementedException unless overridden in subclass
bool IoGroupMCP23017::setPwm(uint16_t id, uint8_t value)
{
    // Double-check if the specified id is a registered pwm pin
    if(this->pwm_pins.count(id) > 0)
    {
        // see if pwm should be used for this pin or not
        if(value == 0 || value == 255)
        {
            // on min/max value, don't use PWM for this pin
            this->mcp->setPwmState(id,false);
            this->active_pwms.erase(id);
            this->mcp->setPin(id,(bool)value);
        }
        else
        {
            // in between, so use pwm
            this->mcp->setPwmState(id,true);
            this->active_pwms.insert(id);
            this->mcp->setPwmValue(id, value);
        }
        
        // See if PWM for the mcp chip should be enabled or not
        if(this->active_pwms.empty())
        {
            this->mcp->PwmStop();
        }
        else
        {
            this->mcp->PwmStart();
        }
        return true;
    }
    return false;
}


void IoGroupMCP23017::onInterrupt(GpioPin * sender, GpioEdge edge, bool pinval)
{
    uint16_t intf,intcap, keycode;
    uint8_t i, bitcount;
    
    if(edge != kEdgeFalling)   // Only continue on falling edge
        return;


    intf = this->mcp->getIntF();
    intcap = this->mcp->getIntCap();

    // Check if we are in a noise timeout, and cancel if so.
    if(this->noiseTimeout !=0 && this->noiseTimeout > now_ms())
        return;
    else
        this->noiseTimeout = 0;


    // Count the number of interrupt pins that changed
    // If more than a certain number if bits is set, we should assume noise
    bitcount = 0;
    for(i=0;i<16;i++)
    {
        if(intf & (1 << i)) // Check if this key is set
            bitcount++;
    }

    clog << showbase << internal << setfill('0');
    clog << kLogDebug << this->Name() << ": Interrupt!" << endl;
    clog << kLogDebug << "  INTF   : " << setw(4) << hex << intf << dec << endl;
    clog << kLogDebug << "  INTCAP : " << setw(4) << hex << intcap << dec << endl;
    
    if( bitcount > 0 )
    {
        if (this->hw_noisemargin == 0 || bitcount <= this->hw_noisemargin  ) // anything above (default: 4) interrupts at the same time is considered noise
        {
            
            for(i=0; i < 16; i++)
            {
                if(intf & (1 << i)) // check if this keycode is set
                {
                    bool value = (bool)( intcap & (1 << i) );
                    this->inputChanged(i,value);
                }
            }
        }
        else
        {
            clog << kLogDebug << "!!! Input Noise !!! (Timeout: " << this->hw_noisetimeout_ms << "ms)" << endl;
            this->noiseTimeout = now_ms() + this->hw_noisetimeout_ms;
        }
    }
}

void IoGroupMCP23017::onInterruptError(Thread* sender, ThreadException x)
{
    // When this function is called, the interrupt thread will have stopped
    clog << kLogErr << "Error in interrupt listener: " << x.what() << endl;
    clog << kLogErr << "Attempting to restart interrupt listener... " << endl;
    try
    {
        delete mcp; mcp = NULL;
        delete intpin; intpin = NULL;
        endConfig(); // Attempt to re-init the chip system. Quit on failure
        clog << kLogInfo << "Succesfully restarted interrupt listener" << endl;
    }
    catch(std::exception x2)
    {
        clog << kLogCrit << "Fatal error while attempting to restart interrupt listener : " << x2.what() << endl << "Quitting now... " << endl;
        this->onCriticalError(this, x2.what());
    }
}
