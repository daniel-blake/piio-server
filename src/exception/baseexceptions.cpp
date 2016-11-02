#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "baseexceptions.hpp"



/************************************
*                                   *
*       MsgException                *
*                                   *
*************************************/
MsgException::MsgException()
{
}

MsgException::MsgException(const std::string& message)
{
    myMsg = message;
}

MsgException::MsgException(const std::string &format,...)
{
    va_list argptr;
    va_start(argptr, format);
    init(format.c_str(),argptr);
    va_end(argptr);
}

MsgException::MsgException(const char * format,...)
{
    va_list argptr;
    va_start(argptr, format);
    init(format,argptr);
    va_end(argptr);
}

void MsgException::init(const char * format,va_list mArgs)
{
    uint32_t BUFFER_LEN = 256;
    char buffer[BUFFER_LEN];
    // force buffer to zeroes
	memset(buffer, 0x00, BUFFER_LEN);
    // use vsnprintf to parse
    vsnprintf(buffer, BUFFER_LEN-1, format, mArgs);
    myMsg = std::string(buffer);
}

void MsgException::init(const char * s)
{
    myMsg = std::string(s);
}


std::string MsgException::Message()
{
    return myMsg;
}

const char* MsgException::what()
{
    formattedMsg = type() + " - " + myMsg;
    return formattedMsg.c_str();
}

MsgException::~MsgException() throw ()
{

}




