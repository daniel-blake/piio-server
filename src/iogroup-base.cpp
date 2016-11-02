#include "iogroup-base.hpp"


IoGroupBase::IoGroupBase(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry)
 : DBus::ObjectAdaptor(connection, dbuspath),
   gpioRegistry(registry)
{
    this->path = DBus::Path(dbuspath);
}

void IoGroupBase::Initialize(libconfig::Setting &setting)
{
    this->name = setting.getName();
}

IoGroupBase::~IoGroupBase()
{

}

void IoGroupBase::Name(std::string s)
{
    this->name = s;
}


std::string IoGroupBase::Name()
{
    return this->name;
}

DBus::Path IoGroupBase::Path()
{
    return this->path;
}   

