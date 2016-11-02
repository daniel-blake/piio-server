#ifndef __LOG_HPP__
#define __LOG_HPP__


#include <syslog.h>
#include <iostream>

enum LogPriority {
    kLogEmerg    = LOG_EMERG,   // system is unusable
    kLogAlert    = LOG_ALERT,   // action must be taken immediately
    kLogCrit     = LOG_CRIT,    // critical conditions
    kLogCritical = LOG_CRIT,    // critical conditions
    kLogErr      = LOG_ERR,     // error conditions
    kLogError    = LOG_ERR,     // error conditions
    kLogWarning  = LOG_WARNING, // warning conditions
    kLogNotice   = LOG_NOTICE,  // normal, but significant, condition
    kLogInfo     = LOG_INFO,    // informational message
    kLogDebug    = LOG_DEBUG    // debug-level message
};

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);

class Log : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    explicit Log(std::string ident, int facility);
    static void Init(std::string ident, int facility);
    static void Init(std::string ident);
protected:
    int sync();
    int overflow(int c);

private:
    friend std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority);
    std::string buffer_;
    int facility_;
    int priority_;
    char ident_[50];
};

std::string logPrintf(std::string &format,...);

#endif