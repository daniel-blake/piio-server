#include "iogroup-pca9685.hpp"
#include "gpio/gpio.hpp"
#include <boost/algorithm/string.hpp>
#include <sstream>


using namespace std;



IoGroupPCA9685::IoGroupPCA9685(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry)
 : IoGroupHwPwm(connection,dbuspath, registry)
{


}

// Called at the start of the configuration round to allow for subclass
// specific settings to be set in the config
void IoGroupPCA9685::beginConfig(libconfig::Setting &setting)
{
	uint32_t cfg_i2c_address;
	uint32_t tmp_uint32;
    bool tmp_bool;
    string tmp_string;

    // read the oscillator frequency
	uint32_t f_osc = 25000000;
    setting.lookupValue("osc-frequency",f_osc);
    
	// freqency was already loaded by parent class
	this->cfg.Frequency = this->GetPwmFrequency();
	this->cfg.OscillatorClock = f_osc;
	this->SetPwmFrequency(cfg.getActualFrequency()); // Set this objects' frequency to the actual frequency used so calculations are accurate

	// read external clock value
	setting.lookupValue("external-clock",this->cfg.ExtClk);

	// read output-invert
	setting.lookupValue("ouput-invert",this->cfg.Invert);

	// determine output-type
	if(setting.lookupValue("output-type",tmp_string))
	{
		if(boost::iequals("opendrain",tmp_string))
		{
			this->cfg.OutputDrive = Pca9685::Pca9685Config::OpenDrain;
		}
		else //if(boost::iequals("totempole",tmp_string))
		{
			this->cfg.OutputDrive = Pca9685::Pca9685Config::TotemPole;
		}
	}

	// determine output mode on output-disabled
	if(setting.lookupValue("ouput-on-disabled",tmp_string))
	{
		if(boost::iequals("high",tmp_string))
		{
			this->cfg.OutNotEnabled = Pca9685::Pca9685Config::OutputHigh;
		}
		else if(boost::iequals("float",tmp_string))
		{
			this->cfg.OutNotEnabled= Pca9685::Pca9685Config::OutputHighZ;
		}
		else //if(boost::iequals("low",tmp_string))
		{
			this->cfg.OutNotEnabled = Pca9685::Pca9685Config::OutputLow;
		}
	}

    clog << kLogInfo << this->Name() << ": Initializing with: " << endl;

    clog << kLogInfo << "external-clock    : " << ((this->cfg.ExtClk)?"true":"false") << endl;
    clog << kLogInfo << "osc-frequency     : " << (int32_t)f_osc << endl;
    clog << kLogInfo << "desired frequency : " << (int32_t)this->cfg.Frequency << endl;
    clog << kLogInfo << "actual frequency  : " << (int32_t)this->GetPwmFrequency() << endl;
    clog << kLogInfo << "ouput-invert      : " << ((this->cfg.Invert)?"true":"false") << endl;
    clog << kLogInfo << "output-type       : ";
    if(this->cfg.OutputDrive == Pca9685::Pca9685Config::OpenDrain)
    	clog << kLogInfo << "open drain" << endl;
    else
    	clog << kLogInfo << "totempole" << endl;
    
    clog << kLogInfo << "output-on-disable : ";
    if(this->cfg.OutNotEnabled == Pca9685::Pca9685Config::OutputHigh)
    	clog << kLogInfo << "high" << endl;
    else if(this->cfg.OutNotEnabled == Pca9685::Pca9685Config::OutputHighZ)
    	clog << kLogInfo << "float" << endl;
	else
		clog << kLogInfo << "low" << endl;

    // Figure out MCP configuration
    if(setting.lookupValue("address",cfg_i2c_address))
    {
        if(cfg_i2c_address <= 0x7F)
        {
            
            if(cfg_i2c_address >= 0x40 && cfg_i2c_address  <= 0x7F)
            {
            
				this->hw_address = (uint8_t)cfg_i2c_address; // cannot be done directly as setting.lookupvalue(..) cannot directly parse to uint8_t)
                clog << kLogInfo << "addres: " << hex << cfg_i2c_address << dec << endl;
            }
            else
            {
                clog << kLogError << this->Name() << ": Invalid I2C address for a PCA9685 chip (must be 0x40...0x7F): " << hex  << cfg_i2c_address << dec  << endl;
                throw ConfigInvalidException("Incompatible I2C Address provided");
            }
        }
        else
        {
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
uint16_t IoGroupPCA9685::getPinId(const int32_t &i)
{
	if(i < 16 && i >= 0)
	{
		clog << "valid" << endl;
		return (uint16_t)i;
	}
	else
	{
		clog << "invalid - throwing exception" << endl;

		std::ostringstream oss;
		oss << "Pin id " << i << " is not valid for this IO Group type (must be < 16 and >= 0)";
		throw IoPinInvalidException(oss.str());
	}
}

void IoGroupPCA9685::preparePwmPin(PwmPin * pin)
{
	// not needed in this implementation
}

// Called at the end of the configuration round to allow the subclass to 
// finalize configuration
void IoGroupPCA9685::endConfig(void)
{
    string name = this->Name();
    string usage_sda = "I2C SDA";
    string usage_scl = "I2C SCL";

    clog << "Finalizing PCA9685 config" << endl;
    if(
        this->gpioRegistry.requestSharedLease(GpioPin::VerifyPin(2),name,usage_sda) &&
        this->gpioRegistry.requestSharedLease(GpioPin::VerifyPin(3),name,usage_scl)
       )
    {
        clog << kLogInfo << "Opening PCA9685 PWM driver on I2C address " << hex << (uint32_t)this->hw_address << dec <<endl;
        
        try
        {

        	this->pca = new Pca9685(this->hw_address, this->cfg);

            try
            {

                clog << kLogInfo << "Initializing all pwm pins to proper value" << endl;
                // Read initial value to clear any current interrupts

                // Now setup the PWM pins
                std::set<PwmPin*>::iterator p;
                std::set<PwmPin*> pins = this->GetPwmPins();

                for(p = pins.begin(); p != pins.end(); ++p)
                {
                    this->setPwmPin(*p);
                }

            }
            catch(OperationFailedException x)
            {
                clog << kLogError << this->Name() << ": Error setting up MCP interface: " << x.Message() << endl;
                delete pca; pca = NULL;
                throw x;
            }
                
        }
        catch(OperationFailedException x)
        {
            clog << kLogError << this->Name() << ": Error setting up MCP interface: " << x.Message() << endl;
            throw x;
        }
    }
    else
    {   
        clog << kLogError << "I2C Pins are already registered for exclusive use" << endl;
        throw OperationFailedException("I2C Pins are already registered for exclusive use");
    }
}

IoGroupPCA9685::~IoGroupPCA9685()
{
	if(this->pca != NULL)
	{
		delete this->pca;
		this->pca = NULL;
	}
}

// Override in child to get or set the actual PWM value
void IoGroupPCA9685::setPwmPin(PwmPin *pin)
{
	clog << kLogInfo << "*Setting pin value for pin " << pin->GetHandle() << " (pin " << pin->GetId() << ")" << endl;

	double value = pin->GetFilteredValue();
	double offset = pin->GetOffset();


	// determine on and off value
	uint16_t ontick = 0;
	uint16_t offtick = 0;

	if(value == 0)
	{
		// pca9685 has specific values for fully off
		offtick = 4096;
		ontick = 0;
	}
	else if(value == 1)
	{
		// pca9685 has specific values for fully on
		offtick = 0;
		ontick = 4096;
	}
	else
	{
		// calculate values
		uint16_t tickvalue = (uint16_t)(4096 * value);
		uint16_t tickoffset = (uint16_t)(4096 * offset);

		// shift on and off values according to offset
		offtick = (tickvalue + tickoffset) % 4096; // modulo 4096 should make sure the value is properly wrapped around
		ontick = tickoffset;
	}

	clog << "    Offset fraction     : " << offset << endl;
	clog << "    On at tick          : " << ontick << endl;
	clog << "    Off at tick         : " << offtick << endl;

	pca->setValue(pin->GetId(),offtick,ontick);

}

// Throws FeatureNotImplementedException unless overridden in subclass
void IoGroupPCA9685::getPwmPin(PwmPin *pin)
{
	uint16_t ontick = pca->getOnValue(pin->GetId());
	uint16_t offtick = pca->getOffValue(pin->GetId());
	double offset = ((double)(ontick & 0x0FFF)) / 4096.0 ;

	if(offtick & 4096 > 0)
	{
		// always off

		// fractional value is 0, set offset
		pin->SetFromFilteredValue(0,offset);
	}
	else if(ontick & 4096 > 0)
	{
		// always on

		// fractional value is 1, set offset
		pin->SetFromFilteredValue(1,offset);
	}
	else
	{
		// filter usable parts of the on- and offtick
		offtick &= 0x0FFF;
		ontick &= 0x0FFF;

		// add 4096 if the offtick is wrapped around
		if(ontick > offtick)
		{
			offtick += 4096;
		}

		// and calculate the fractional value
		double value = ((double)((offtick)- (ontick))) / 4096.0 ;

		// set it;
		pin->SetFromFilteredValue(value,offset);

	}


}


