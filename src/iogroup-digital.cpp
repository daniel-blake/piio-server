#include "iogroup-digital.hpp"
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <vector>

using namespace std;
using namespace libconfig;
// public

IoGroupDigital::IoGroupDigital(DBus::Connection &connection, std::string &dbuspath, GpioRegistry &registry) 
    : IoGroupBase(connection, dbuspath, registry)//, DBus::ObjectAdaptor(connection, dbuspath)
{

}

void IoGroupDigital::Initialize(libconfig::Setting &setting)
{
    IoGroupBase::Initialize(setting);
    
	uint32_t time_shortPress = 25;
	uint32_t time_longPress = 6000;

    // Check if we have an IO, and if we have defined ios, otherwise just ignore everything here
    if(setting.exists("io"))
    {
        Setting& io_list = setting["io"];
		if(io_list.getLength() > 0)
		{

			// Read button timer settings from setting
			setting.lookupValue("button-shortpress-time",time_shortPress);
			setting.lookupValue("button-longpress-time",time_longPress);

			// Initialize button timer
			this->btnTimer = new ButtonTimer(time_shortPress,time_longPress); // Short press should take at leas 25 ms, and a Long press takes 6 seconds
			onShortPressConnection = this->btnTimer->onShortPress.connect(boost::bind(&IoGroupDigital::onShortPress, this, _1));
			onLongPressConnection = this->btnTimer->onLongPress.connect(boost::bind(&IoGroupDigital::onLongPress, this, _1));
			onValidatePressConnection = this->btnTimer->onValidatePress.connect(boost::bind(&IoGroupDigital::onValidatePress, this, _1));

			// Nofify subclass of start of configuration iteration
			this->beginConfig(setting);

			for(int i=0; i<io_list.getLength(); ++i)
			{
				Setting& io = io_list[i];
				string handle = io.getName();
				string iotype = "";

                io.lookupValue("type",iotype);
                clog << kLogDebug << this->Name() << " -  IO '" << handle << "' of type '" << iotype << "'" << endl;

				if(io.lookupValue("type",iotype))
				{
					if(boost::iequals(iotype,"button"))
                    {
                        clog << kLogDebug << this->Name() << "." << handle << ": registering as Button" << endl;
                        this->registerButton(handle,io);
                    }
                    else if(boost::iequals(iotype,"inputpin"))
                    {
                        clog << kLogDebug << this->Name() << "." << handle << ": registering as Input" << endl;
                        this->registerInput(handle,io);
                    }
                    else if(boost::iequals(iotype,"outputpin"))
                    {
                        clog << kLogDebug << this->Name() << "." << handle << ": registering as Output" << endl;
                        this->registerOutput(handle,io);
                    }
                    else if(boost::iequals(iotype,"pwmpin"))
                    {
                        clog << kLogDebug << this->Name() << "." << handle << ": registering as Pwm" << endl;
                        this->registerPwm(handle,io);
                    }
                    else if(boost::iequals(iotype,"multibitin"))
                    {
                        clog << kLogDebug << this->Name() << "." << handle << ": registering as Multibit Input" << endl;
                        this->registerMultiBitInput(handle,io);
                    }
                    else if(boost::iequals(iotype,"multibitout"))
                    {
                        clog << kLogDebug << this->Name() << "." << handle << ": registering as Multibit Output" << endl;
                        this->registerMultiBitOutput(handle,io);
                    }
                    else
                    {
                        clog << kLogError << this->Name() << ": Handle '" << handle << "' has specified type '" << iotype << "' which does not conform to a known type name." << endl;

                    }
                    
				}
			}
			// Nofify subclass of start of configuration iteration
			this->endConfig();
		}
    }

}

IoGroupDigital::~IoGroupDigital()
{
	if (btnTimer != NULL)
	{
		delete btnTimer; 
		btnTimer = NULL;
	}
}

std::vector<std::string> IoGroupDigital::Buttons()
{
    std::vector<std::string> output(this->buttonList.begin(), this->buttonList.end());
    return output;
}

bool IoGroupDigital::GetButton(const std::string &handle)
{
    if(this->buttonList.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Getting button state on handle '" << handle << "'" << endl;
        uint16_t id = this->idMap[handle];
        return this->getInputPin(id);
    }
    else
    {
        clog << kLogWarning << this->Name() << ": Attempt to call GetButton for nonexisting handle '" << handle << "'" << endl;
        return false;
    }
}

std::vector<std::string> IoGroupDigital::Inputs()
{
    std::vector<std::string> output(this->inputList.begin(), this->inputList.end());
    return output;
}

bool IoGroupDigital::GetInput(const std::string &handle)
{
    if(this->inputList.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Getting input on handle '" << handle << "'" << endl;
        uint16_t id = this->idMap[handle];
        return this->getInputPin(id);
    }
    else
    {
        clog << kLogWarning << this->Name() << ": Attempt to call GetInput for nonexisting handle '" << handle << "'" << endl;
        return false;
    }
}

std::vector<std::string> IoGroupDigital::Outputs()
{
    std::vector<std::string> output(this->outputList.begin(), this->outputList.end());
    return output;
}

void IoGroupDigital::SetOutput(const std::string &handle, const bool &value)
{
    if(this->outputList.count(handle) > 0)
    {
        if(value != this->outputValueMap[handle])
        {
        
            clog << kLogDebug << this->Name() << ": Setting output on handle '" << handle << "' to '" << value << "'" << endl;
            uint16_t id = this->idMap[handle];
            this->setOutputPin(id,value);
            this->outputValueMap[handle] = value;
            this->onOutputChanged(this,handle,value);
            this->OutputChanged(handle,value);
        }
        else
        {
            clog << kLogDebug << this->Name() << ": No change for output on handle '" << handle << "' to '" << value << "'" << endl;
        }
    }
    else
    {
        clog << kLogWarning << this->Name() << ": Attempt to call SetOutput for nonexisting handle '" << handle << "'" << endl;
    }
}

bool IoGroupDigital::GetOutput(const std::string &handle)
{
    if(this->outputList.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Getting output on handle '" << handle << "'" << endl;
        if(this->outputValueMap.count(handle) > 0)
        {
            return this->outputValueMap[handle];
        }
        else
        {
            clog << kLogDebug << this->Name() << ": Debug - value not previously set for output handle '" << handle << "', returning false" << endl;
            return false;
        }
    }
    else
    {
        clog << kLogWarning << this->Name() << ": Attempt to call GetOuput for nonexisting handle '" << handle << "'" << endl;    
        return false;
    }
}

std::vector<std::string> IoGroupDigital::MbInputs()
{
    std::vector<std::string> output(this->mbInputList.begin(), this->mbInputList.end());
    return output;
}

uint32_t IoGroupDigital::GetMbInput(const std::string &handle)
{
    uint32_t value = 0;
    
    if(this->mbInputList.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Getting multibit input on handle '" << handle << "'" << endl;
        for(std::vector<uint16_t>::size_type i = 0; i != this->mbIdMap[handle].size(); i++)
        {
            uint16_t pinid = this->mbIdMap[handle][i];
            if(this->getInputPin(pinid))
            {
                value |= 1 << i;
            }
        }
        
        return value;
    }
    else
    {
        clog << kLogWarning << this->Name() << ":Attempt to call GetMbInput for nonexisting handle '" << handle << "'" << endl;    
        return 0; // or 0xFFFFFFFF; still have to decide that
    }
}

std::vector<std::string> IoGroupDigital::MbOutputs()
{
    std::vector<std::string> output(this->mbOutputList.begin(), this->mbOutputList.end());
    return output;
}

void IoGroupDigital::SetMbOutput(const std::string &handle, const uint32_t &value)
{
    if(this->mbInputList.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Setting multibit output on handle '" << handle << "' to '" << value << "'" << endl;
        if(value != this->mbOutputValueMap[handle])
        {
            this->mbOutputValueMap[handle] = value;
            for(std::vector<uint16_t>::size_type i = 0; i != this->mbIdMap[handle].size(); i++)
            {
                uint16_t pinid = this->mbIdMap[handle][i];
                
                // Loop through the pins and set the value
                if(value & (1 << i))
                {
                    this->setOutputPin(pinid,true);
                }
                else
                {
                    this->setOutputPin(pinid,false);
                }
            }
            
            this->onMbOutputChanged(this,handle,value);
            this->MbOutputChanged(handle,value);
        }
        else
        {
            clog << kLogDebug << this->Name() << ": No change for multibit output on handle '" << handle << "' to '" << value << "'" << endl;
        }
    }
    else
    {
        clog << kLogWarning << this->Name() << ":Attempt to call SetMbOutput for nonexisting handle '" << handle << "'" << endl;    
    }
}

uint32_t IoGroupDigital::GetMbOutput(const std::string &handle)
{
    if(this->outputValueMap.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Getting multibit input on handle '" << handle << "'" << endl;
        return this->mbOutputValueMap[handle];
    }
    else
    {
        clog << kLogWarning << this->Name() << ":Attempt to call GetMbOutput for nonexisting handle '" << handle << "'" << endl;    
        return 0; // or 0xFFFFFFFF; still have to decide tha;
    }
}

std::vector<std::string> IoGroupDigital::Pwms()
{
    std::vector<std::string> output(this->pwmList.begin(), this->pwmList.end());
    return output;
}

void IoGroupDigital::SetLedPwm(const std::string &handle, const uint8_t &value)
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

    if(this->pwmList.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Setting LED pwm on handle '" << handle << "' to '" << (int32_t)value << "'" << endl;
        uint16_t id = this->idMap[handle];
        this->setPwm(id,GammaToLinear[value]);
    }
    else
    {
        clog << kLogWarning << this->Name() << ":Attempt to call SetLedPwm for nonexisting handle '" << handle << "'" << endl;    
    }    
}



void IoGroupDigital::SetPwm(const std::string &handle, const uint8_t &value)
{
    try
    {
        if(this->pwmList.count(handle) > 0)
        {
            clog << kLogDebug << this->Name() << ": Setting pwm on handle '" << handle << "' to '" << (int32_t)value << "'" << endl;
            uint16_t id = this->idMap[handle];
            this->setPwm(id,value);
     
            this->onPwmValueChanged(this,handle,value);
            this->PwmValueChanged(handle,value);
        }
        else
        {
            clog << kLogWarning << this->Name() << ": Attempt to call SetPwm for nonexisting handle '" << handle << "'" << endl;    
        }
    }
    catch(FeatureNotImplementedException x)
    {
        clog << kLogError << this->Name() << ": PWM Not supported, but SetPwm called nonetheless'" << handle << "'" << endl;    
    }
}

uint8_t IoGroupDigital::GetPwm(const std::string &handle)
{
    if(this->pwmValueMap.count(handle) > 0)
    {
        clog << kLogDebug << this->Name() << ": Getting LED pwm on handle '" << handle << "'" << endl;
        return this->pwmValueMap[handle];
    }
    else
    {
        if(this->pwmList.count(handle) > 0)
        {
            clog << kLogInfo << this->Name() << ": Attempt to call GetPwm before value was set for handle '" << handle << "'" << endl;    
        }
        else
        {
            clog << kLogWarning << this->Name() << ": Attempt to call GetPwm for nonexisting handle '" << handle << "'" << endl;    
        }
        return 0;
    }
}

// Button timer callback functions
bool IoGroupDigital::onValidatePress(uint16_t id)
{
    // Check if the key is still pressed before sending out a long press
    clog << kLogDebug << this->Name() << ": Event - Verifying pin '"<<id<<"' for long press. Pin state is '" << this->getInputPin(id) << "'" << endl;
    
    if(this->getInputPin(id))
        return true;
    else
        return false;
}

void IoGroupDigital::onShortPress(uint16_t id)
{
    // Send the button press signal
    string handle = this->handleMap[id];

    clog << kLogDebug << this->Name() << ": Event - Short press on pin id '" <<  id << "' - handle '" << handle << "'" << endl;

    this->onButtonPress(this,handle);
    this->ButtonPress(handle);
}

void IoGroupDigital::onLongPress(uint16_t id)
{
    // Send the button press signal
    string handle = this->handleMap[id];

    clog << kLogDebug << this->Name() << ": Event - Long press on pin id '" <<  id << "' - handle '" << handle << "'" << endl;

    this->onButtonHold(this,handle);
    this->ButtonHold(handle);
}

//protected
void IoGroupDigital::inputChanged(uint16_t id, bool value)
{
    if(this->buttonIdList.find(id) != this->buttonIdList.end())
    {
        // It's  button - that is handled by the button timer
        if(value)
        {
            btnTimer->RegisterPress(id);
        }
        else
        {
            btnTimer->RegisterRelease(id);
        }
    }
    else if(this->mbInputIdList.find(id) != this->mbInputIdList.end())
    {
        // It's part of a multibit input
        string handle = this->handleMap[id];
        // read the value of the mb input
        uint32_t mb_value = this->GetMbInput(handle);
        // And send the signal
        this->onMbInputChanged(this,handle,mb_value);
        this->MbInputChanged(handle,mb_value);
    }
    else
    {
        // It's an input. Send the signal and the new value
        string handle = this->handleMap[id];
        this->onInputChanged(this,handle,value);
        this->InputChanged(handle,value);
    }
}

void IoGroupDigital::registerButton(std::string handle, libconfig::Setting &io)
{
	try
	{
		uint16_t id = this->getPinId(io);
        bool invert = true;
        bool pullup = true;
        bool pulldown = false;
        bool inten = true;

        io.lookupValue("invert", invert);
        io.lookupValue("pullup", pullup);
        io.lookupValue("pulldown", pulldown);
        io.lookupValue("int-enabled", inten);
                
		if(this->registerHandle(handle, id))
		{
			this->prepareInputPin(id,invert,pullup,pulldown,inten);
			this->buttonList.insert(handle);
			this->buttonIdList.insert(id);
		}
        else
        {
           clog << kLogWarning << this->Name() << ": Could not register handle '" << handle << "' with id '" << id <<"' as button" << endl;    
        }
	}
	catch(IoPinInvalidException x)
	{
        clog << kLogError << this->Name() << ": Invalid IO pin specified for handle '" << handle << "' :" << x.what() << endl;    
	}
    catch(FeatureNotImplementedException x)
    {
        clog << kLogError << this->Name() << ": " << x.what() << endl;    
    }
}
void IoGroupDigital::registerInput(std::string handle, libconfig::Setting &io)
{
	try
	{
		uint16_t id = this->getPinId(io);
        bool invert = false;
        bool pullup = false;
        bool pulldown = false;
        bool inten = true;

        io.lookupValue("invert", invert);
        io.lookupValue("pullup", pullup);
        io.lookupValue("pulldown", pulldown);
        io.lookupValue("int-enabled", inten);

		if(this->registerHandle(handle, id))
		{
            this->prepareInputPin(id,invert,pullup,pulldown,inten);
			this->inputList.insert(handle);
		}
        else
        {
           clog << kLogWarning << this->Name() << ": Could not register handle '" << handle << "' with id '" << id <<"' as input" << endl;    
        }
	}
	catch(IoPinInvalidException x)
	{
        clog << kLogError << this->Name() << ": Invalid IO pin specified for handle '" << handle << "' :" << x.what() << endl;    
	}
    catch(FeatureNotImplementedException x)
    {
        clog << kLogError << this->Name() << ": " << x.what() << endl;    
    }

}
void IoGroupDigital::registerOutput(std::string handle, libconfig::Setting &io)
{
	try
	{
		uint16_t id = this->getPinId(io);
		if(this->registerHandle(handle, id))
		{
			this->prepareOutputPin(id);
			this->outputList.insert(handle);
            
            // Initialize output to false;
            this->setOutputPin(id,false);
            this->outputValueMap[handle] = false;
		}
        else
        {
           clog << kLogWarning << this->Name() << ": Could not register handle '" << handle << "' with id '" << id <<"' as output" << endl;    
        }
	}
	catch(IoPinInvalidException x)
	{
        clog << kLogError << this->Name() << ": Invalid IO pin specified for handle '" << handle << "' :" << x.what() << endl;    
	}
    catch(FeatureNotImplementedException x)
    {
        clog << kLogError << this->Name() << ": " << x.what() << endl;    
    }
}

void IoGroupDigital::registerPwm(std::string handle, libconfig::Setting &io)
{
	try
	{
		uint16_t id = this->getPinId(io);

		if(this->registerHandle(handle, id))
		{
			this->preparePwmPin(id);
			this->pwmList.insert(handle);
			this->pwmIdList.insert(id);
		}
        else
        {
           clog << kLogWarning << this->Name() << ": Could not register handle '" << handle << "' with id '" << id <<"' as pwm output" << endl;    
        }
	}
	catch(IoPinInvalidException x)
	{
        clog << kLogError << this->Name() << ": Invalid IO pin specified for handle '" << handle << "' :" << x.what() << endl;    
	}
    catch(FeatureNotImplementedException x)
    {
        clog << kLogError << this->Name() << ": " << x.what() << endl;    
    }
}

void IoGroupDigital::registerMultiBitInput(std::string handle, libconfig::Setting &io)
{
	try
	{
        if(io.exists("pins") && io["pins"].isArray())
        {
            bool invert = false;
            bool pullup = false;
            bool pulldown = false;
            bool inten = true;

            io.lookupValue("invert", invert);
            io.lookupValue("pullup", pullup);
            io.lookupValue("pulldown", pulldown);
            io.lookupValue("int-enabled", inten);
            
            std::vector<uint16_t> v_pins = this->getMbPinIds(io["pins"]);
            
            if(this->registerMbHandle(handle,v_pins))
            {
                for(std::vector<uint16_t>::size_type i = 0; i != v_pins.size(); i++)
                {
                    this->prepareInputPin(v_pins[i],invert,pullup,pulldown,inten);
                    this->mbInputIdList.insert(v_pins[i]);
                }
                this->mbInputList.insert(handle);
            }
            else
            {
               clog << kLogWarning << this->Name() << ": Could not register handle '" << handle << "' as multibit input" << endl;    
            }
        }
        else
        {
           clog << kLogWarning << this->Name() << ": Multibit input '" << handle << "' has no proper pin list" << endl;    
        }
	}
	catch(IoPinInvalidException x)
	{
        clog << kLogError << this->Name() << ": Invalid IO pin specified for handle '" << handle << "' :" << x.what() << endl;    
	}
    catch(FeatureNotImplementedException x)
    {
        clog << kLogError << this->Name() << ": " << x.what() << endl;    
    }
}

void IoGroupDigital::registerMultiBitOutput(std::string handle, libconfig::Setting &io)
{
	try
	{
        if(io.exists("pins") && io["pins"].isArray())
        {
            std::vector<uint16_t> v_pins = this->getMbPinIds(io["pins"]);
            
            if(this->registerMbHandle(handle,v_pins))
            {
                for(std::vector<uint16_t>::size_type i = 0; i != v_pins.size(); i++)
                {
                    this->prepareOutputPin(v_pins[i]);
                    // Initialize output to false;
                    this->setOutputPin(v_pins[i],false);
                }
                this->mbInputList.insert(handle);
                this->mbOutputValueMap[handle] = 0;
            }
            else
            {
               clog << kLogWarning << this->Name() << ": Could not register handle '" << handle << "' as mutibit output" << endl;    
            }
        }
        else
        {
           clog << kLogWarning << this->Name() << ": Multibit input '" << handle << "' has no proper pin list" << endl;    
        }
	}
	catch(IoPinInvalidException x)
	{
        clog << kLogError << this->Name() << ": Invalid IO pin specified for handle '" << handle << "' :" << x.what() << endl;    
	}
    catch(FeatureNotImplementedException x)
    {
        clog << kLogError << this->Name() << ": " << x.what() << endl;    
    }
}

// private

bool IoGroupDigital::registerHandle(std::string handle, uint16_t id)
{
    if(this->idMap.count(handle) > 0 || this->mbIdMap.count(handle) > 0)
    {
        // handle already in use
        return false;
    }
    else if(this->handleMap.count(id) > 0)
    {
        // id already in use
        return false;
    }
    else
    {
        this->idMap[handle] = id;
        this->handleMap[id] = handle;
        return true;
    }
}

bool IoGroupDigital::registerMbHandle(std::string handle, std::vector<uint16_t> ids)
{
    if(this->idMap.count(handle) > 0 || this->mbIdMap.count(handle) > 0)
    {
        // handle already in use
        return false;
    }
    else 
    {
        bool ids_inuse = false;
        
        for(std::vector<uint16_t>::size_type i = 0; i != ids.size(); i++)
        {
            if(this->handleMap.count(ids[i]) > 0)
            {
                ids_inuse = true;
            }
        }
        
        if(ids_inuse)
        {
            // id already in use
            return false;
        }
        else
        {
            this->mbIdMap[handle] = ids;

            for(std::vector<uint16_t>::size_type i = 0; i != ids.size(); i++)
            {
                this->handleMap[ ids[i] ] = handle;
            }            
            
            return true;
        }
    }
}

uint16_t IoGroupDigital::getPinId(libconfig::Setting &io)
{
    string s;
    int32_t i;
    
    if(io.lookupValue("pin",i))
    {
        return this->getPinId(i);
    }
    else if(io.lookupValue("pin",s))
    {
        return this->getPinId(s);
    }
    else
    {
        throw FeatureNotImplementedException("This subclass does not support pin ids");
    }
}

std::vector<uint16_t> IoGroupDigital::getMbPinIds(libconfig::Setting &pins)
{
    std::vector<uint16_t> v_pins = std::vector<uint16_t>();
                
    for(int i=0; i<pins.getLength(); ++i)
    {
        uint16_t id;
        
        Setting &pin = pins[i];
    
        if(pin.getType() == libconfig::Setting::TypeString)
        {
            string s = pin;
            id = this->getPinId(s);
        }
        else if(pin.getType() == libconfig::Setting::TypeInt)
        {
            int32_t i = pin;
            id = this->getPinId(i);
        }
        else
        {
            throw FeatureNotImplementedException("This subclass does not support pin ids");
        }    
    
        // reverse the pin order so we have proper ordering (msb first in config file, but lsb at index 0 in array) 
        v_pins.insert(v_pins.begin(),id);
    }
            
    return v_pins;
}


uint16_t IoGroupDigital::getPinId(std::string s)
{
    throw FeatureNotImplementedException("This subclass does not support string pin ids");
}
uint16_t IoGroupDigital::getPinId(int32_t i)
{
    throw FeatureNotImplementedException("This subclass does not support integer pin ids");
}

// Throws FeatureNotImplementedException unless overridden
bool IoGroupDigital::setPwm(uint16_t id, uint8_t value)
{
    throw FeatureNotImplementedException("PWM is not supported in this subclass");
}

// Throws FeatureNotImplementedException unless overridden
void IoGroupDigital::preparePwmPin(uint16_t pinid)
{
    throw FeatureNotImplementedException("PWM is not supported in this subclass");
}

// Make sure these functions exist but do nothing in case a subclass doesn't need them.
void IoGroupDigital::beginConfig(libconfig::Setting &setting)
{
}
	
void IoGroupDigital::endConfig(void)
{
}

