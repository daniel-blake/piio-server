
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>

#include <libconfig.h++>
#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>

// For getting current time
#include <time.h>
#ifndef CLOCK_MONOTIC
#   define CLOCK_MONOTIC CLOCK_REALTIME
#endif

using namespace libconfig;
using namespace std;
namespace po = boost::program_options;

void parseConfigGPIO(Setting & setting);
void parseConfigMCP23017(Setting & setting);
static const char *DEFAULT_CFGFILE_PATH = "/etc/pi-io.conf";

int main(int argc, char ** argv)
{
    Config config;
    
    // first parse options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Show this help message")
        ("config,c", po::value< vector<string> >(), "specify configuration file")
    ;
     
    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv,desc),vm);
    po::notify(vm);
    
    
    if (vm.count("help"))
    {
        cout << desc << endl;
        return 1;
    }
    
    if (vm.count("config"))
    {
        string cfgfile = vm["config"].as< vector<string> >()[0];
        try
        {
            config.readFile(cfgfile.c_str());
        }
        catch(const FileIOException& x)
        {
            cout << "Cannot read file '" << cfgfile <<"'." << endl;
            return 1;
        }
    }
    else
    {
        try
        {
            config.readFile(DEFAULT_CFGFILE_PATH);
        }
        catch(const FileIOException& x)
        {
            cout << "Cannot read file '" << DEFAULT_CFGFILE_PATH <<"'." << endl;
            return 1;
        }
    }
    
    Setting& root = config.getRoot();

    for(int i=0; i < root.getLength(); ++i)
    {
        Setting& setting = root[i];

        string type;
        cout << "" << setting.getName() << ":" << endl;
        if(setting.lookupValue("type",type))
        {
            cout << "\tType is " << type << endl;
            if(type == "GPIO")
                parseConfigGPIO(setting);
            else if(type == "MCP23017")
                parseConfigMCP23017(setting);
        }
        else
        {
            cout << "\tType not set or not a string" << endl;
        }
    
    }
}

void parseConfigGPIO(Setting & setting)
{
    // Got a gpio setting to parse
    if(setting.exists("io"))
    {
        cout << "\tDefined: IO interfaces" << endl;
        Setting& io = setting["io"];
        for(int i=0; i<io.getLength(); ++i)
        {
            Setting& io_unit = io[i];
            cout << "\t\t" << io_unit.getName() << ":" << endl;
            
        }
    }

}

void parseConfigMCP23017(Setting & setting)
{
    // Got an mcp23017 setting
    int address;
    if(setting.lookupValue("address",address))
    {
        cout << "\tI2C Address: 0x"<< hex << address << endl;
        
        if(setting.exists("io"))
        {
            cout << "\tDefined: IO interfaces" << endl;
            Setting& io = setting["io"];
            for(int i=0; i<io.getLength(); ++i)
            {
                Setting& io_unit = io[i];
                cout << "\t\t"<<io_unit.getName() << ":" << endl;
                
            }
        }    
    }
    else
    {
        cout << "\tNo I2C address provided. Ignoring section" << endl;
    }

}
