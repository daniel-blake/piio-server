#ifndef __GPIOREGISTRY_HPP
#define __GPIOREGISTRY_HPP

#include <stdint.h>
#include <string>
#include <map>
#include <set>

class GpioRegistry 
{
public:

    GpioRegistry();
    ~GpioRegistry();

    bool requestExclusiveLease(uint16_t pin, std::string &user, std::string &usage);
    bool requestSharedLease(uint16_t pin, std::string &user, std::string &usage);
    std::string getCurrentLeaser(uint16_t pin);
    std::string getCurrentUsage(uint16_t pin);
    bool isExclusivelyLeased(uint16_t pin);
    

private:
    std::map<uint16_t, std::string> leasers;
    std::map<uint16_t, std::string> usages;
    std::set<uint16_t> exclusiveLease;
    std::set<uint16_t> sharedLease;
};

#endif//__IOGROUP_BASE_HPP