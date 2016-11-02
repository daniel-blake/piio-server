#include "iogroup-softpwm.hpp"

#include <sstream>

using namespace std;



IoGroupSoftPWM::IoGroupSoftPWM(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry)
 : IoGroupDigital(connection,dbuspath, registry)
{


}

void IoGroupSoftPWM::Initialize(libconfig::Setting &setting)
{
    IoGroupDigital::Initialize(setting);

    uint32_t tick_delay_us = 800; //us
    uint32_t ticks = 16; // tucks
    
    // Read button timer settings from setting
    setting.lookupValue("pwm-tickdelay-us",tick_delay_us);
    setting.lookupValue("pwm-ticks",ticks);

    if(ticks > 255)
        ticks = 255;

    this->setPwmConfig(tick_delay_us, (uint8_t)ticks);
}

IoGroupSoftPWM::~IoGroupSoftPWM()
{
    PwmStop();   // try to stop the PWM driver;
}

//! Start the PWM routine for this I/O expander
void IoGroupSoftPWM::PwmStart()
{

    if(!ThreadRunning())
    {
        ThreadStart();
    }
}

//! Stop the PWM routine for this I/O expander
void IoGroupSoftPWM::PwmStop()
{
    if(ThreadRunning())
    {
        ThreadStop();

        // set active PWMs to 0 on stop (to prevent unpredictable results)
        std::set<uint16_t>::iterator p;
        for(p = this->active_pwms.begin(); p != this->active_pwms.end(); ++p) 
        {
            this->setOutputPin(*p,false);   
        }
    }
}

void IoGroupSoftPWM::preparePwmPin(uint16_t pinid)
{
    cout << kLogDebug << this->Name() <<".PWM: Preparing pin '" << pinid << "' for PWM " << endl;
	this->prepareOutputPin(pinid);
	this->pwm_pins.insert(pinid);
}

//! Set the PWM value for a specific pin as 0-255 value 
bool IoGroupSoftPWM::setPwm(uint16_t id, uint8_t value)
{
    // Double-check if the specified id is a registered pwm pin
    if(this->pwm_pins.count(id) > 0)
    {
        clog << kLogDebug << this->Name() <<".PWM: Setting value for pin '" << id << "' to  '" << (int32_t)value << "'" << endl;
        // see if pwm should be used for this pin or not
        if(value == 0 || value == 255)
        {
            clog << kLogDebug << this->Name() <<".PWM: Using on/off for solid value '" << (int32_t)value << "' for " << endl;
            // on min/max value, don't use PWM for this pin
            this->active_pwms.erase(id);
            this->setOutputPin(id,(bool)value);
        }
        else
        {
            // in between, so use pwm
            this->active_pwms.insert(id);
            this->pwm_v_values[id] = value; // cache the provided value for returning and for updating pwm_value on change in pwm_ticks);
            this->pwm_values[id] = value / (256/this->pwm_ticks);
            clog << kLogDebug << this->Name() <<".PWM: Converted '" << (int32_t)value << "' to '" << (int32_t)(this->pwm_values[id]) << "'" << endl;  

        }

        // See if PWM for the mcp chip should be enabled or not
        if(this->active_pwms.empty())
        {
            clog << kLogDebug << this->Name() <<".PWM: No pwm driver needed - stopping pwm thread." << endl;  
            this->PwmStop();
        }
        else
        {
            clog << kLogDebug << this->Name() <<".PWM: Pwm driver needed - starting pwm thread." << endl;  
            this->PwmStart();
        }
        return true;
    }
    else
    {
        clog << kLogDebug << this->Name() <<".PWM: Unrecognized pin '" << id << "' in setPwm Request " << endl;
    }
    return false;
}

//! Set the PWM Configuration
void IoGroupSoftPWM::setPwmConfig(uint32_t tick_delay_us, uint8_t ticks)
{
    clog << kLogInfo << this->Name() << ".PWM: Setting Pwm to " << (int32_t)ticks << " ticks with " << tick_delay_us << " us delay between them" << endl; 
    this->pwm_tick_delay_us = tick_delay_us;
    this->pwm_ticks = ticks;
    
    // update the actual PWM values, according to the 
    std::set<uint16_t>::iterator p;
    for(p = this->active_pwms.begin(); p != this->active_pwms.end(); ++p) 
    { 
        // if so, update the actually used pwm value, to reflect the new setting of ticks
        this->pwm_values[*p] = this->pwm_v_values[*p] / (256/this->pwm_ticks);
    }
}

//! Driver function for PWM thread
void IoGroupSoftPWM::ThreadFunc(void)
{
    uint8_t ctr = 0; 
    int i;
    uint16_t pwm_out = 0x00;
    // 
    clog << kLogDebug << this->Name() <<".PWM.Threadfunc: Starting" << endl;  

    MakeRealtime();

    while(ThreadRunning())
    {

        std::set<uint16_t>::iterator p;
        for(p = this->active_pwms.begin(); p != this->active_pwms.end(); ++p) 
        {
            //clog << kLogDebug << this->Name() <<".PWM.Threadfunc: Counter at '" << (int32_t)ctr << "', pin '" << *p << "' value at '" << (int32_t)(this->pwm_values[*p]) << "'" << endl;  
            if(ctr >= this->pwm_values[*p] ) // check if the counter is greater than the pwm value for this pin. If so, turn it off.
            {
            //    clog << kLogDebug << this->Name() <<".PWM.Threadfunc: Trigger Pin '" << *p << "' Off" << endl;  
                this->setOutputPin(*p,false);
            }
            else
            {
                // otherwise turn it on
            //    clog << kLogDebug << this->Name() <<".PWM.Threadfunc: Trigger Pin '" << *p << "' On" << endl;  
                this->setOutputPin(*p,true);
            }
        }

        ctr++;
        if(ctr >= (this->pwm_ticks))
        {
            ctr = 0;
        }

        usleep(this->pwm_tick_delay_us);
    }
    clog << kLogDebug << this->Name() <<".PWM.ThreadFunc: Stopping" << endl;  

}
