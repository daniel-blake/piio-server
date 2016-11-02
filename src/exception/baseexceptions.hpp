#ifndef __BASEEXCEPTIONS_HPP_
#define __BASEEXCEPTIONS_HPP_

#include <cstdarg>
#include <string>

class MsgException : public std::exception
{
    public:
        MsgException();
        MsgException(const std::string &message);
        MsgException(const std::string &format, ...);
        MsgException(const char * format,...);
        ~MsgException() throw();
        virtual const char* what();
        std::string Message();

    protected:
        virtual std::string type() { return "MsgException"; };
        void init(const char * format,va_list mArgs);  
        void init(const char * s);

        std::string myMsg;
        std::string formattedMsg;
};

#define QUOTE(name) #name
#define DefineNewMsgException(cName) \
class cName : public MsgException   \
{   \
    public: \
        cName(const std::string &format,...) { va_list argptr; va_start(argptr, format); init(format.c_str(),argptr); va_end(argptr); }  \
        cName(const char * format,...) { va_list argptr; va_start(argptr, format); init(format,argptr); va_end(argptr); }    \
    protected:  \
        virtual std::string type() { return QUOTE(cName); };    \
};


DefineNewMsgException(WarningException);
DefineNewMsgException(InvalidArgumentException);
DefineNewMsgException(OperationFailedException);


/*
class WarningException : public MsgException
{
    protected:
        virtual std::string type() { return "Warning"; };
};


class InvalidArgumentException : public MsgException
{
    protected:
        virtual std::string type() { return "InvalidArgumentException"; };
};

class OperationFailedException : public MsgException
{
    MsgException(const std::string& message);
    MsgException(const std::string &format,...);
    MsgException(const char * format,...) 
    protected:
        virtual std::string type() { return "OperationFailedException"; };
};

*/

#endif