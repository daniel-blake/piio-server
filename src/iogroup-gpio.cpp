#include "iogroup-gpio.hpp"
#include <sstream>

using namespace std;


IoGroupGpio::IoGroupGpio(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry)
 : IoGroupSoftPWM(connection,dbuspath, registry)
 { 
 }

// Called at the start of the configuration round to allow for subclass
// specific settings to be set in the config
void IoGroupGpio::beginConfig(libconfig::Setting &setting)
{
    clog << kLogInfo << this->Name() << ": Begin config" << endl;
}

// called when registering pins.
uint16_t IoGroupGpio::getPinId(int32_t pinid)
{
    int realpin = GpioPin::VerifyPin(pinid);
    
    clog << kLogInfo << "Retrieving pin id for pin " << pinid << ". Real id is " << realpin << "." << endl;

	if(realpin >= 0)
	{
        string usage = "IO group pin";
        string name = this->Name();
        if(this->gpioRegistry.requestExclusiveLease((uint16_t)realpin,name, usage))
        {
            return (uint16_t)realpin;
        }
        else
        {
            std::ostringstream oss;
            oss << "Gpio pin " << pinid << " is already registered by io group " << this->gpioRegistry.getCurrentLeaser(realpin) << " for use as " << this->gpioRegistry.getCurrentUsage(realpin);
            throw IoPinInvalidException(oss.str());
        }
	}
	else
	{
		std::ostringstream oss;
		oss << "Pin id " << pinid << " is not valid for this IO Group type (must be < 16)";
		throw IoPinInvalidException(oss.str());
	}
}

void IoGroupGpio::prepareInputPin(uint16_t pinid, bool invert, bool pullup, bool pulldown, bool inten)
{
    clog << kLogDebug << "Opening input pin " << pinid << endl; 
    GpioPin * pin = new GpioPin(    pinid,   		            // Pin number
                                    kDirectionIn,               // Data direction
                                    (inten)?kEdgeBoth:kEdgeNone // Interrupt edge - using both edges on interrupt, or none on no interrupt
                                );  
    clog << kLogDebug << "Appending pin data to pin maps for pin" << pinid << endl; 
    this->gpioPins[pinid] = pin;
    this->gpioInvert[pinid] = invert;
    this->gpioInputPins.insert(pinid);
    
    // Set the internal pullup on or off
    //clog << kLogDebug << "Setting pullup to " << pullup << " for pin " << pinid << endl; 
    
    if(pullup)
    {        
        pin->setPullUp(true);
    }
    else if(pulldown)
    {
        pin->setPullDown(true);
    }
    else
    {
        pin->setPullUp(false);
    }
    

    if(inten)
    {
        clog << kLogDebug << "Registering interrupts for pin " << pinid << endl; 

        this->gpioIntPins.insert(pinid);
        this->gpioIntConnection[pinid] = pin->onInterrupt.connect(boost::bind(&IoGroupGpio::onInterrupt, this, _1, _2, _3));
        this->gpioIntErrorConnection[pinid] = pin->onThreadError.connect(boost::bind(&IoGroupGpio::onInterruptError, this, _1, _2));

        clog << kLogDebug << "Starting interrup listener for pin " << pinid << endl; 
        pin->InterruptStart();
    }

    clog << kLogDebug << "Preparation done for pin " << pinid << endl; 

}

void IoGroupGpio::prepareOutputPin(uint16_t pinid)
{
    GpioPin * pin = new GpioPin(    pinid,   		    // Pin number
                                    kDirectionOut,      // Data direction
                                    kEdgeNone           // Interrupt edge - using both edges on interrupt, or none on no interrupt
                                );  
    this->gpioPins[pinid] = pin;
    this->gpioOutputPins.insert(pinid);
}


// Called at the end of the configuration round to allow the subclass to 
// finalize configuration
void IoGroupGpio::endConfig(void)
{
    clog << kLogInfo << this->Name() << ": End config" << endl;

}

// destructor
IoGroupGpio::~IoGroupGpio()
{
    // delete all pins
    typedef std::map<uint16_t, GpioPin*>::iterator it_type;
    for(it_type iterator = this->gpioPins.begin(); iterator != this->gpioPins.end(); iterator++) 
    {
        uint16_t pinid = iterator->first;
        GpioPin *pin = iterator->second;
        
        if( pin != NULL)
        {
            delete pin;
        }
    }
}


// Override in child to get input value by id
bool IoGroupGpio::getInputPin(uint16_t id)
{
    if(this->gpioInputPins.count(id) > 0)
    {
        bool value = this->gpioPins[id]->getValue();
        if(this->gpioInvert[id])
            value = !value;
        return value;
    }
    else
    {
        clog << kLogDebug << this->Name() << ".iogroup-gpio:  Input pin '" << id << "' not recognized as an output pin" << endl;
        return false;
    }
}

// Override in child to actually set the output by id
bool IoGroupGpio::setOutputPin(uint16_t id, bool value)
{
    if(this->gpioOutputPins.count(id) > 0)
    {
        this->gpioPins[id]->setValue(value);
        return true;
    }
    else
    {
        clog << kLogDebug << this->Name() << ".iogroup-gpio:  Output pin '" << id << "' not recognized as an output pin" << endl;
        return false;
    }
}


// Generic interrupt handler for all pins
void IoGroupGpio::onInterrupt(GpioPin * sender, GpioEdge edge, bool pinval)
{
    uint16_t pinid = (uint16_t)sender->getPinNr();
    bool value = pinval;
    if(this->gpioInvert[pinid])
        value = !pinval;
        
    this->inputChanged(pinid,value);
}

void IoGroupGpio::onInterruptError(Thread* sender, ThreadException x)
{
    // When this function is called, the interrupt thread will have stopped
    clog << kLogErr << "Error in interrupt listener: " << x.what() << endl;
    
}
