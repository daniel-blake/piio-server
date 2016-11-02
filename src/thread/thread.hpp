#ifndef __THREAD_HPP
#define __THREAD_HPP

#include <pthread.h>
#include <string>
#include "../exception/baseexceptions.hpp"
#include <boost/signals2.hpp>

DefineNewMsgException(ThreadException);

/*
class ThreadException : public MsgException
{
    protected:
        virtual std::string type() { return "ThreadException"; }
};
*/

class Thread
{
    friend void * thread_threadStarter(void * obj);
    public:
        ~Thread();
        boost::signals2::signal<void (Thread *, ThreadException)> onThreadError;

    protected:
        Thread();

        void ThreadStart();
        void ThreadStop();
        bool ThreadRunning();
        void MutexLock();
        void MutexUnlock();
        
        bool MakeRealtime();
        virtual void ThreadFunc(void);  // Override this if you want the entire function custom
        virtual void ThreadLoop(); //
        
    private:
        pthread_t wthread;
        pthread_mutex_t mutex;
        pthread_mutexattr_t mutexAttr;
        bool running;
        void ThreadStarter();
};


#endif//__MC_HID_SERVER_HPP