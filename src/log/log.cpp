#include "log.hpp"
#include <string.h>
#include <stdio.h>
#include <cstdarg>
#include <stdint.h>

std::string logPrintf(std::string &format,...)
{
    uint32_t BUFFER_LEN = 256;
    char buffer[BUFFER_LEN];

    va_list argptr;
    va_start(argptr, format);

    // force buffer to zeroes
	memset(buffer, 0x00, BUFFER_LEN);
    // use vsnprintf to parse
    vsnprintf(buffer, BUFFER_LEN-1, format.c_str(), argptr);
    va_end(argptr);

    return std::string(buffer);
}

Log::Log(std::string ident, int facility) {
    facility_ = facility;
    priority_ = LOG_DEBUG;
    strncpy(ident_, ident.c_str(), sizeof(ident_));
    ident_[sizeof(ident_)-1] = '\0';

    openlog(ident_, LOG_PID, facility_);
}

int Log::sync() {
    if (buffer_.length()) {
        syslog(priority_, "%s", buffer_.c_str());
        buffer_.erase();
        priority_ = LOG_DEBUG; // default to debug for each message
    }
    return 0;
}

int Log::overflow(int c) {
    if (c != EOF) {
        buffer_ += static_cast<char>(c);
    } else {
        sync();
    }
    return c;
}

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority) {
    static_cast<Log *>(os.rdbuf())->priority_ = (int)log_priority;
    return os;
}

// Static init function
void Log::Init(std::string ident, int facility)
{
    std::clog.rdbuf(new Log(ident, facility));

}

// Static init function with default facility
void Log::Init(std::string ident)
{
    Log::Init(ident,LOG_LOCAL0);
}
