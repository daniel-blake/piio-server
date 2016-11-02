#include "iogroup-hwpwm.hpp"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;
using namespace libconfig;

IoGroupHwPwm::PwmPin::PwmPin(std::string handle, IoGroupHwPwm *iogroup, libconfig::Setting &setting)
{
	int32_t inttemp;
	double doubletemp;

	this->gamma = 2.8;
	this->maxValue = 1.0;
	this->minValue = 0.0;
	this->value = 0.0;
	this->offset = 0.0;
	this->servoMinTimeMs = 1.0;
	this->servoMaxTimeMs = 2.0;
	this->valueInverse = false;
	this->servoInverse = false;

	this->iogroup = iogroup;
	this->handle = handle;

	clog << "Initializing PWM pin '"<< handle << "'" << endl;

	// default values
	this->filter = PwmFilterNone;
	this->id = -1;

	// parse
	this->id = this->iogroup->getPinId(setting);

	string pwmfilter = "none";
	setting.lookupValue("filter",pwmfilter);
	setting.lookupValue("gamma",this->gamma);

	if(setting.lookupValue("max",doubletemp))
		this->maxValue = doubletemp;
	else if(setting.lookupValue("max",inttemp))
		this->maxValue = (double)inttemp;
	if(setting.lookupValue("min",doubletemp))
		this->minValue = doubletemp;
	else if(setting.lookupValue("min",inttemp))
		this->minValue = (double)inttemp;
	if(setting.lookupValue("default",doubletemp))
		this->value = doubletemp;
	else if(setting.lookupValue("default",inttemp))
		this->value = (double)inttemp;
	if(setting.lookupValue("servo-min-ms",doubletemp))
		this->servoMinTimeMs = doubletemp;
	else if(setting.lookupValue("servo-min-ms",inttemp))
		this->servoMinTimeMs = (double)inttemp;
	if(setting.lookupValue("servo-max-ms",doubletemp))
		this->servoMaxTimeMs = doubletemp;
	else if(setting.lookupValue("servo-max-ms",inttemp))
		this->servoMaxTimeMs = (double)inttemp;
	if(setting.lookupValue("offset",doubletemp))
		this->offset = doubletemp;
	else if(setting.lookupValue("offset",inttemp))
		this->offset = (double)inttemp;


	if(this->offset < 0 || this->offset >= 1)
	{
		this->offset = 0;
	}

	if(this->servoMinTimeMs > this->servoMaxTimeMs)
	{
		double d = this->servoMinTimeMs;
		this->servoMinTimeMs = this->servoMaxTimeMs;
		this->servoMaxTimeMs = d;
		this->servoInverse = true;
	}

	if(this->minValue > this->maxValue)
	{
		double d = this->minValue;
		this->minValue = this->maxValue;
		this->maxValue = d;
		this->valueInverse = true;
	}

	if(this->value < this->minValue)
	{
		this->value = this->minValue;
	}
	else if(this->value > this->maxValue)
	{
		this->value = this->maxValue;
	}


	if(boost::iequals(pwmfilter,"led"))
	{
		this->filter = PwmPin::PwmFilterLed;
	}
	else if(boost::iequals(pwmfilter,"servo"))
	{
		this->filter = PwmPin::PwmFilterServo;
		this->gamma = 1;
	}
	else
	{
		this->filter = PwmPin::PwmFilterNone;
	}

	clog << "PWM pin config: " << endl;
	clog << kLogInfo << "id                : " << (int32_t)this->id << endl;
	clog << kLogInfo << "handle            : " << this->handle << endl;
	clog << kLogInfo << "filter            : " ;
	if(this->filter == PwmPin::PwmFilterServo)
		clog << "SERVO" << endl;
	else if(this->filter == PwmPin::PwmFilterLed)
		clog << "LED" << endl;
	else
		clog << "NONE" << endl;

	clog << kLogInfo << "invert value      : " << ((this->valueInverse)?"true":"false") << endl;
	clog << kLogInfo << "min               : " << this->minValue << endl;
	clog << kLogInfo << "max               : " << this->maxValue << endl;
	clog << kLogInfo << "default           : " << this->value << endl;
	clog << kLogInfo << "offset            : " << this->offset << endl;
	clog << kLogInfo << "gamma             : " << this->gamma << endl;
	clog << kLogInfo << "servo min         : " << this->servoMinTimeMs << "ms" << endl;
	clog << kLogInfo << "servo max         : " << this->servoMaxTimeMs << "ms" << endl;
	clog << kLogInfo << "servo invert      : " << ((this->servoInverse)?"true":"false") << endl;



}


double IoGroupHwPwm::PwmPin::GetFilteredValue()
{
	double fractionValue = (this->value - this->minValue) / (this->maxValue - this->minValue);
	double result = 0;

	double t_ms;
	double valServoMin;
	double valServoMax;

	// if the max and min were reversed in the configuration,
	clog << kLogDebug;
	clog << "    Set value           : " << this->value << endl;
	clog << "    Min value           : " << this->minValue << endl;
	clog << "    Max value           : " << this->maxValue << endl;

	clog << "    Input value swap    : ";
	if(this->valueInverse)
	{
		fractionValue = 1 - fractionValue;
		clog << "true" << endl;
	}
	else
	{
		clog << "false" << endl;
	}

	clog << "    Unfiltered fraction : " << fractionValue << endl;

	clog << "    Using filter        : " ;
	switch(this->filter)
	{
	case PwmPin::PwmFilterLed:
		clog << "LED" << endl;
		result = this->ForwardGamma(fractionValue);
		break;
	case PwmPin::PwmFilterServo:
		clog << "SERVO" << endl;
		// Determine min and max pulse fractions for the servo control
		t_ms = this->iogroup->GetPwmPeriodMs();
		valServoMin = this->servoMinTimeMs / t_ms;
		valServoMax = this->servoMaxTimeMs / t_ms;

		clog << "    PWM cycle duration  : " << t_ms << "ms" << endl;
		clog << "    Min pulse fraction  : " << valServoMin << "(" << this->servoMinTimeMs << "ms)" << endl;
		clog << "    Max pulse fraction  : " << valServoMax << "(" << this->servoMaxTimeMs << "ms)" << endl;

		if(valServoMax >= 1)
		{
			// if we cannot provide the full range, don't do anything at all.
			clog << kLogWarning << this->iogroup->Name() << "|" << this->handle << " : Cannot set servo values.";
			clog << "Maximum servo pulse of " << this->servoMaxTimeMs << "ms exceeds pwm period time of " << t_ms << "ms." << endl;
			clog << kLogDebug;
			result = 0;
		}
		else
		{
			// if the servo max and min were reversed in the configuration, inverse value again
			clog << "    Servo directon swap : ";
			if(this->servoInverse)
			{
				fractionValue = 1 - fractionValue;
				clog << "true" << endl;
			}
			else
			{
				clog << "false" << endl;
			}
			result = valServoMin + fractionValue * (valServoMax - valServoMin);
		}
		break;
	default:
		clog << "NONE" << endl;
		result = fractionValue;
	}

	clog << "    Filtered fraction   : " << result << endl;

	return result;
}

void IoGroupHwPwm::PwmPin::SetFromFilteredValue(const double &filteredValue,const double &offset)
{
	double t_ms;
	double valServoMin;
	double valServoMax;

	double fractionValue = 0;
	switch(this->filter)
	{
	case PwmPin::PwmFilterLed:
		fractionValue = this->ReverseGamma(filteredValue);
		break;
	case PwmPin::PwmFilterServo:
		// Determine min and max pulse fractions for the servo control
		t_ms = this->iogroup->GetPwmPeriodMs();
		valServoMin = this->servoMinTimeMs / t_ms;
		valServoMax = this->servoMaxTimeMs / t_ms;

		if(valServoMax >= 1)
		{
			// if we cannot provide the full range, don't do anything at all.
			clog << kLogWarning << this->iogroup->Name() << "|" << this->handle << " : Cannot set servo values.";
			clog << "Maximum servo pulse of " << this->servoMaxTimeMs << "ms exceeds pwm period time of " << t_ms << "ms." << endl;
			fractionValue = 0;
		}
		else
		{
			fractionValue = (filteredValue - valServoMin) / (valServoMax - valServoMin);
			// if the max and min were reversed in the configuration,
			if(this->servoInverse)
			{
				fractionValue = 1 - fractionValue;
			}
		}
		break;
	default:
		fractionValue = filteredValue;
	}

	// if the max and min were reversed in the configuration,
	if(this->valueInverse)
	{
		fractionValue = 1 - fractionValue;
	}

	this->value = this->minValue + fractionValue * (this->maxValue - this->minValue);

	if(offset >= 0 && offset < 1)
	{
		this->offset = offset;
	}
}

double IoGroupHwPwm::PwmPin::ForwardGamma(const double &value)
{
	return pow(value,this->gamma);
}

double IoGroupHwPwm::PwmPin::ReverseGamma(const double &value)
{
	double invgamma = 0;
	if(this->gamma != 0)
	{
		invgamma = 1/this->gamma;
	}
	return pow(value/1,invgamma);
}

uint16_t IoGroupHwPwm::PwmPin::GetId()
{
	return this->id;
}

const std::string IoGroupHwPwm::PwmPin::GetHandle()
{
	return this->handle;
}

IoGroupHwPwm::PwmPin::PwmFilter IoGroupHwPwm::PwmPin::GetFilter()
{
	return this->filter;
}

double IoGroupHwPwm::PwmPin::GetGamma()
{
	return this->gamma;
}

double IoGroupHwPwm::PwmPin::GetValue()
{
	return this->value;
}

double IoGroupHwPwm::PwmPin::GetOffset()
{
	return this->offset;
}

void IoGroupHwPwm::PwmPin::SetValue(const double &value)
{
	if(value < this->minValue)
	{
		this->value = this->minValue;
	}
	else if (value > this->maxValue)
	{
		this->value = this->maxValue;
	}
	else
	{
		this->value = value;
	}
}

double IoGroupHwPwm::PwmPin::GetMin(){
	return this->minValue;
}

double IoGroupHwPwm::PwmPin::GetMax(){
	return this->maxValue;
}


// public

IoGroupHwPwm::IoGroupHwPwm(DBus::Connection &connection, std::string &dbuspath, GpioRegistry &registry) 
    : IoGroupBase(connection, dbuspath, registry)//, DBus::ObjectAdaptor(connection, dbuspath)
{

}

IoGroupHwPwm::~IoGroupHwPwm()
{
	// delete all PwmPin objects
	std::set<PwmPin*>::iterator it;
	for( it = this->pwmPins.begin(); it != this->pwmPins.end(); ++ it)
	{
		delete *it;
	}
}

void IoGroupHwPwm::Initialize(libconfig::Setting &setting)
{
	clog << kLogDebug << "IoGroupHwPwm Initializing " << endl;


    IoGroupBase::Initialize(setting);
	this->pwmFrequency = 100;

    // Check if we have an IO, and if we have defined ios, otherwise just ignore everything here
    if(setting.exists("io"))
    {
        Setting& io_list = setting["io"];
		if(io_list.getLength() > 0)
		{

			// Read button timer settings from setting
			setting.lookupValue("pwm-frequency",this->pwmFrequency);

			clog << kLogDebug << "PWM frequency: " << this->pwmFrequency << endl;

			// Initialize button timer

			// Nofify subclass of start of configuration iteration
			this->beginConfig(setting);

			for(int i=0; i<io_list.getLength(); ++i)
			{
				Setting& io = io_list[i];
				string handle = io.getName();
				try
				{
					PwmPin * pin = new PwmPin(handle,this,io);

					if(this->registerPwmPin(pin)) //verify if handle can be registered (no double registrations for handle or id
					{
						this->preparePwmPin(pin);
						this->pwmPins.insert(pin);	// cached list of pins used for proper cleanup in destructor
						this->pwmList.insert(pin->GetHandle()); // cached list of handles
					}
			        else
			        {
			        	clog << kLogWarning << this->Name() << ": Could not register handle '" << handle << "' with id '" << pin->GetId() << "' as pwm output" << endl;
			        	delete pin;
			        }
				}
				catch(IoPinInvalidException &x)
				{
			        clog << kLogError << this->Name() << ": Invalid IO pin specified for handle '" << handle << "' :" << x.what() << endl;
				}
			    catch(FeatureNotImplementedException &x)
			    {
			        clog << kLogError << this->Name() << ": " << x.what() << endl;
			    }

			}
			// Nofify subclass of start of configuration iteration
			this->endConfig();
		}
    }

}


std::vector<std::string> IoGroupHwPwm::Pwms()
{
    std::vector<std::string> output(this->pwmList.begin(), this->pwmList.end());
    return output;
}


void IoGroupHwPwm::SetValue(const std::string &handle, const double &value)
{
    try
    {
        if(this->handleMap.count(handle) > 0)
        {
            clog << kLogDebug << this->Name() << ": Setting pwm value on handle '" << handle << "' to '" << value << "'" << endl;
            PwmPin * pin = this->handleMap[handle];
            pin->SetValue(value);
            this->setPwmPin(pin);

            this->PwmValueChanged(handle,value);
        }
        else
        {
            clog << kLogWarning << this->Name() << ": Attempt to call SetValue for nonexisting handle '" << handle << "'" << endl;
        }
    }
    catch(FeatureNotImplementedException &x)
    {
        clog << kLogError << this->Name() << ": PWM Not implemented, but SetValue called nonetheless'" << handle << "'" << endl;
    }
}

double IoGroupHwPwm::GetValue(const std::string &handle)
{
    try
    {
		if(this->handleMap.count(handle) > 0)
		{
			clog << kLogDebug << this->Name() << ": Getting pwm value on handle '" << handle << "'" << endl;
			PwmPin * pin = this->handleMap[handle];
			this->getPwmPin(pin);
			return pin->GetValue();
		}
		else
		{
			clog << kLogWarning << this->Name() << ": Attempt to call GetValue for non-existing handle '" << handle << "'" << endl;
			return 0;
		}
    }
    catch(FeatureNotImplementedException &x)
    {
        clog << kLogError << this->Name() << ": PWM Not implemented, but GetValue called nonetheless'" << handle << "'" << endl;
        return 0;
    }
}

double IoGroupHwPwm::GetMin(const std::string &handle)
{
    try
    {
		if(this->handleMap.count(handle) > 0)
		{
			clog << kLogDebug << this->Name() << ": Getting minimum value on handle '" << handle << "'" << endl;
			PwmPin * pin = this->handleMap[handle];
			this->getPwmPin(pin);
			return pin->GetMin();
		}
		else
		{
			clog << kLogWarning << this->Name() << ": Attempt to call GetMin for non-existing handle '" << handle << "'" << endl;
			return 0;
		}
    }
    catch(FeatureNotImplementedException &x)
    {
        clog << kLogError << this->Name() << ": PWM Not implemented, but GetMin called nonetheless'" << handle << "'" << endl;
        return 0;
    }
}

double IoGroupHwPwm::GetMax(const std::string &handle)
{
    try
    {
		if(this->handleMap.count(handle) > 0)
		{
			clog << kLogDebug << this->Name() << ": Getting maximum value on handle '" << handle << "'" << endl;
			PwmPin * pin = this->handleMap[handle];
			this->getPwmPin(pin);
			return pin->GetMax();
		}
		else
		{
			clog << kLogWarning << this->Name() << ": Attempt to call GetMax for non-existing handle '" << handle << "'" << endl;
			return 0;
		}
    }
    catch(FeatureNotImplementedException &x)
    {
        clog << kLogError << this->Name() << ": PWM Not implemented, but GetMax called nonetheless'" << handle << "'" << endl;
        return 0;
    }
}

// Frequency and timing functions
double IoGroupHwPwm::GetPwmPeriodMs()
{
	return this->pwmPeriodMs;
}

uint32_t IoGroupHwPwm::GetPwmFrequency()
{
	return this->pwmFrequency;
}

// SetPwmFrequency is protected, so subclasses can correct the pwm frequency to the proper value if needed
uint32_t IoGroupHwPwm::SetPwmFrequency(uint32_t frequency)
{
	if(frequency > 0)
	{
		this->pwmFrequency = frequency;
		this->pwmPeriodMs = (1.0/((double)frequency)) * 1000.0;
	}
}

std::set<IoGroupHwPwm::PwmPin*> IoGroupHwPwm::GetPwmPins()
{
	return this->pwmPins;
}


// private

bool IoGroupHwPwm::registerPwmPin(PwmPin * pin)
{
    if(this->handleMap.count(pin->GetHandle()) > 0)
    {
    	clog << "Registration error: handle alredy in use: " << pin->GetHandle() << endl;
        // handle already in use
        return false;
    }
    else if(this->idMap.count(pin->GetId()) > 0)
    {
    	clog << "Registration error: id alredy in use: " << pin->GetId() << endl;
        // id already in use
        return false;
    }
    else
    {
        this->handleMap[pin->GetHandle()] = pin;
        this->idMap[pin->GetId()] = pin;
        return true;
    }
}

uint16_t IoGroupHwPwm::getPinId(libconfig::Setting &io)
{
    string s;
    int32_t i;
    clog << "Determining pin id ... ";
    if(io.lookupValue("pin",i))
    {
    	clog << "found INT id '" << i << "' validating ..." << endl;

    	uint16_t id = this->getPinId(i);
    	return id;
    }
    else if(io.lookupValue("pin",s))
    {
    	clog << "found STRING id ... validating ..." << endl;
        return this->getPinId(s);
    }
    else
    {
    	throw IoPinInvalidException("No valid pin definition found in setting");
    }
}

// Functions that must be overridden in subclass

uint16_t IoGroupHwPwm::getPinId(const std::string &s)
{
    throw IoPinInvalidException("This subclass does not support string pin id's");
}
uint16_t IoGroupHwPwm::getPinId(const int32_t &i)
{
    throw IoPinInvalidException("This subclass does not support integer pin id's");
}

// Throws FeatureNotImplementedException unless overridden
void IoGroupHwPwm::setPwmPin(PwmPin * pin)
{
    throw FeatureNotImplementedException("PWM is not supported in this subclass");
}

// Throws FeatureNotImplementedException unless overridden
void IoGroupHwPwm::getPwmPin(PwmPin * pin)
{
    throw FeatureNotImplementedException("PWM is not supported in this subclass");
}

// Throws FeatureNotImplementedException unless overridden
void IoGroupHwPwm::preparePwmPin(PwmPin * pin)
{
    throw FeatureNotImplementedException("PWM is not supported in this subclass");
}

// Make sure these functions exist but do nothing in case a subclass doesn't need them.
void IoGroupHwPwm::beginConfig(libconfig::Setting &setting)
{
}
	
void IoGroupHwPwm::endConfig(void)
{
}

