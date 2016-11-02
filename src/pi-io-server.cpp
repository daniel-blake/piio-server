#ifdef HAVE_CONFIG_H
#include <config.hpp>
#endif

#include "pi-io-server.hpp"
#include "log/log.hpp"
#include "gpioregistry.hpp"
#include "iogroup-gpio.hpp"
#include "iogroup-mcp23017.hpp"
#include "iogroup-pca9685.hpp"

#include "gpio/c_gpio.h"


#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <pthread.h>

#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <vector>


using namespace std;
using namespace libconfig;
namespace po = boost::program_options;

static const std::string SERVER_DBUS_INTF = "nl.miqra.PiIo";
static const std::string SERVER_DBUS_PATH = "/nl/miqra/PiIo";
static const std::string IOGROUP_DBUS_PATH = "/nl/miqra/PiIo/IoGroups";
static const std::string DEFAULT_CFGFILE_PATH = "/etc/piio.conf";

// signal handler
void niam(int sig);

PiIoServer::PiIoServer(DBus::Connection &connection, Config &config)
  : DBus::ObjectAdaptor(connection, SERVER_DBUS_PATH)
{
    this->gpioRegistry = new GpioRegistry();

    // Innitialize hardware
    try
    {
        initHardware(config);
    }
    catch(std::exception x)
    {
        clog << kLogCrit << "Fatal error during hardware initialization: " << x.what() << endl << "Quitting now... " << endl;
        niam(0);
    }

    clog << kLogInfo << "Initialization complete, listening to HID requests." << endl; 
}

PiIoServer::~PiIoServer()
{
    std::set<IoGroupBase*>::iterator it;
    for (it = this->iogroups.begin(); it != this->iogroups.end(); ++it)
    {
        delete *it;
    }

    if(this->gpioRegistry != NULL)
    {
        delete this->gpioRegistry;
        this->gpioRegistry = NULL;
    
    }

    clog << kLogInfo << "Stopping normally" << endl;
}

void PiIoServer::initHardware(Config &config)
{
    // Loop thhrough the config file looking for settings
    Setting& root = config.getRoot();
    
    clog << "Parsing config..." << endl;

    for(int i=0; i < root.getLength(); ++i)
    {
        Setting& setting = root[i];
        
        clog << "Creating IO Group" << endl;
        try
        {
            
            IoGroupBase* g = this->createIoGroup(setting);
            g->onCriticalError.connect(boost::bind(&PiIoServer::criticalError, this, _1, _2));
            
            if(g->Interface() == "nl.miqra.PiIo.IoGroup.Digital")
            {
                IoGroupDigital* d = (IoGroupDigital*)g;
                d->onButtonPress.connect(boost::bind(&PiIoServer::buttonPress, this, _1,_2));
                d->onButtonHold.connect(boost::bind(&PiIoServer::buttonHold, this, _1,_2));
                d->onInputChanged.connect(boost::bind(&PiIoServer::inputChanged, this, _1,_2,_3));
                d->onMbInputChanged.connect(boost::bind(&PiIoServer::mbInputChanged, this, _1,_2,_3));
            }
            
            this->iogroups.insert(g);
        }
        catch(InvalidArgumentException x)
        {
            // Ignore things, log has been made
            clog << kLogWarning << "Error setting up iogroup: " << x.what() << endl;
        }
        catch(std::exception x)
        {
            clog << kLogError << "Error setting up iogroup: " << x.what() << endl;
        }
    }
}

IoGroupBase* PiIoServer::createIoGroup(Setting &setting)
{
    string type = "";
    string name = setting.getName();
    
    clog << "Creating IO Group '" << name << "'" << endl;

    if(setting.lookupValue("type",type))
    {
        string buspath = IOGROUP_DBUS_PATH + "/" + name;
    
        if(boost::iequals(type,"GPIO"))
        {
        	clog << "Initializing GPIO IO Group" << endl;
            IoGroupGpio* g = new IoGroupGpio(this->conn(), buspath, *(this->gpioRegistry));
            g->Initialize(setting);
            return g;
        }
        else if(boost::iequals(type,"MCP23017"))
        {
        	clog << "Initializing MCP23017 IO Group" << endl;
            IoGroupMCP23017* g =  new IoGroupMCP23017(this->conn(), buspath, *(this->gpioRegistry));
            g->Initialize(setting);
            return g;
        }
        else if(boost::iequals(type,"PCA9685"))
        {
        	clog << "Initializing PCA9685 IO Group" << endl;
        	IoGroupPCA9685 * g =  new IoGroupPCA9685(this->conn(), buspath, *(this->gpioRegistry));
            g->Initialize(setting);
            return g;
        }
        else
        {
            clog << kLogError << "Type '" << type << "' is not recognized in iogroup '" << name << "'" << endl;
            throw InvalidArgumentException("Type is not recognized");
        }
    }
    else
    {
        clog << kLogError << "Type for iogroup '" << name << "' is not set or not a string" << endl;
        throw InvalidArgumentException("Type not set or not a string");
    }
}

std::vector< ::DBus::Path > PiIoServer::IoGroups()
{
    std::vector< ::DBus::Path > groups;
    
    std::set<IoGroupBase*>::iterator it;
    for (it = this->iogroups.begin(); it != this->iogroups.end(); ++it)
    {
        groups.push_back((*it)->Path());
    }

    return groups;
}

void PiIoServer::criticalError(IoGroupBase * sender, std::string message)
{
    clog << kLogCritical << sender->Name() << ": Fatal error - " << message << endl;
    clog << kLogCritical << "Exiting..." << endl;
    niam(1);
}

void PiIoServer::buttonPress(IoGroupDigital* sender, std::string handle)
{
    string longname = sender->Name() + "." + handle;
    this->OnButtonPress(longname);
}

void PiIoServer::buttonHold(IoGroupDigital* sender, std::string handle)
{
    string longname = sender->Name() + "." + handle;
    this->OnButtonHold(longname);
}
    
void PiIoServer::inputChanged(IoGroupDigital* sender, std::string handle, bool value)
{
    string longname = sender->Name() + "." + handle;
    this->OnInputChanged(longname, value);
}

void PiIoServer::mbInputChanged(IoGroupDigital* sender, std::string handle, uint32_t value)
{
    string longname = sender->Name() + "." + handle;
    this->OnMbInputChanged(longname, value);
}


DBus::BusDispatcher dispatcher;

void niam(int sig)
{
    dispatcher.leave();
}

int main(int argc, char ** argv)
{
    signal(SIGTERM, niam);
    signal(SIGINT, niam);

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
        catch(const libconfig::ParseException& x)
        {
            cout << "Error parsing config." << endl;
            cout << x.getError() << endl << " on line " << x.getLine() << " of '" << x.getFile() << "'" << endl;
            return 1;
        }
    }
    else
    {
        try
        {
            config.readFile(DEFAULT_CFGFILE_PATH.c_str());
        }
        catch(const FileIOException& x)
        {
            cout << "Cannot read file '" << DEFAULT_CFGFILE_PATH <<"'." << endl;
            return 1;
        }
        catch(const libconfig::ParseException& x)
        {
            cout << "Error parsing config." << endl;
            cout << x.getError() << endl << " on line " << x.getLine() << " of '" << x.getFile() << "'" << endl;
            return 1;
        }
    }

    DBus::default_dispatcher = &dispatcher;

    // Initialize clog to be redirected to syslog key "mediacore.hid.server"
    Log::Init("piio");

    DBus::Connection systemBus = DBus::Connection::SystemBus();
    systemBus.request_name(SERVER_DBUS_INTF.c_str());

    int result = gpio_init(); // initialize the c_gpio subsystem;
    
    if(result == GPIO_SETUP_OK)
    {
        PiIoServer server(systemBus, config);
        dispatcher.enter();
        gpio_cleanup();
        return 0;
    }
    else
    {
        clog << "Could not open gpio memory map. This program must be run as root" << endl;
        return 1;
    }
}
