#include "gpioregistry.hpp"
#include "gpio/gpio.hpp"

using namespace std;

GpioRegistry::GpioRegistry()
{

}

GpioRegistry::~GpioRegistry()
{

}

bool GpioRegistry::requestExclusiveLease(uint16_t pin, std::string &user, std::string &usage)
{
    if(!GpioPin::CheckPin(pin) || this->exclusiveLease.count(pin) > 0 || this->sharedLease.count(pin) > 0)
    {
        return false;
    }
    else
    {
        this->leasers[pin] = user;
        this->usages[pin] = usage;
        return true;
    }
}

bool GpioRegistry::requestSharedLease(uint16_t pin, std::string &user, std::string &usage)
{
    if(!GpioPin::CheckPin(pin) || this->exclusiveLease.count(pin) > 0 )
    {
        return false;
    }
    else
    {
        if (this->sharedLease.count(pin) > 0)
        {
            this->leasers[pin] = this->leasers[pin] + ", " + user;
            this->usages[pin]  = this->usages[pin] + ", " + usage;
        }
        else
        {
            this->leasers[pin] = user;
            this->usages[pin]  = usage;
        }
        return true;
    }
}

bool GpioRegistry::isExclusivelyLeased(uint16_t pin)
{
    if(this->exclusiveLease.count(pin) > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::string GpioRegistry::getCurrentLeaser(uint16_t pin)
{
    if(GpioPin::CheckPin(pin))
    {
        if(this->exclusiveLease.count(pin) > 0 || this->sharedLease.count(pin) > 0)
        {
            return this->leasers[pin];
        }
        else
        {
            return "no one";
        }
    }
    else
    {
        return "n/a";
    }
}

std::string GpioRegistry::getCurrentUsage(uint16_t pin)
{
    if(GpioPin::CheckPin(pin))
    {
        if(this->exclusiveLease.count(pin) > 0 || this->sharedLease.count(pin) > 0)
        {
            return this->usages[pin] ;
        }
        else
        {
            return "pin unclaimed";
        }
    }
    else
    {
        return "Invalid pin number";
    }
}