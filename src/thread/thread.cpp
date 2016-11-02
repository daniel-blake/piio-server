#include "thread.hpp"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

#include "../log/log.hpp"

using namespace std;

void * thread_threadStarter(void *);

Thread::Thread()
{
    running = false;

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &mutexAttr);    
    
}

Thread::~Thread()
{
    // clean up the mutex
    pthread_mutex_destroy(&mutex);

    // Stop the thread if needed
    if(running)
        ThreadStop(); 
}

bool Thread::ThreadRunning()
{
    return running;
}


void Thread::ThreadStart()
{
    int32_t result;
    if(!running)
    {
        running = true;

        result = pthread_create(&wthread, NULL, thread_threadStarter, this);
        if(result != 0) // success
        {
            running = false;
            if(result == EAGAIN)
                throw ThreadException("Cannot create thread: Error EAGAIN - The system lacked the neccesary resources to create another thread, or PTHREAD_THREADS_MAX is exceeded");
            else if(result == EINVAL)
                throw ThreadException("Cannot create thread: Error EINVAL - The value specified by attr (in pthread_create) is invalid");
            else if(result == EPERM)
                throw ThreadException("Cannot create thread: Error EPERM - The caller does not have approproate permission to set the required scheduling permissions or scheduling policy");
        }
    }
}

void Thread::ThreadStop()
{
    int32_t result;
    if(running)
    {
        running = false;    // stops the thread loop
        result = pthread_join(wthread, NULL); // wait for completion
        if(result != 0)
        {
            if(result == EINVAL)
                throw ThreadException("Cannot end thread: Error EINVAL - The implementation has detected that the value specified by thread does not refer to a joinable thread.");
            else if(result == ESRCH)
                throw ThreadException("Cannot create thread: Error ESRCH - No thread could be found corresponding to that specified by the given thread ID");
            else if(result == EDEADLK)
                throw ThreadException("Cannot create thread: Error EINVAL - A deadlock was detected or the value of thread specifies the calling thread.");
        }
    }
}

void Thread::ThreadStarter()
{
   try
   {
        ThreadFunc();
        running = false;
   }
   catch(std::exception x)
   {
        running = false;

        // (debug) Log the exception, before calling the callback
        // clog << kLogErr << x.what() << endl;
        onThreadError(this, ThreadException(x.what()));

   }
}

bool Thread::MakeRealtime()
{
    struct sched_param params;
    int32_t ret;
    
    params.sched_priority = sched_get_priority_max(SCHED_FIFO);
    ret = pthread_setschedparam(wthread, SCHED_FIFO, &params); // This thread is always key->pwm_thread, and if not, then it should be  

    if (ret != 0) {
        // Print the error
        clog << kLogWarning << "Unsuccessful in setting thread realtime priority" << endl; 
        return false;
    }
    else
        return true;
}

void Thread::ThreadLoop(void)
{
    usleep(25000);
}

void Thread::ThreadFunc(void)
{
    while(running)
    {
        ThreadLoop();
    }
}

void Thread::MutexLock()
{
    pthread_mutex_lock(&mutex);
}
void Thread::MutexUnlock()
{
    pthread_mutex_unlock(&mutex); 
}



// static function that calls the real function
void * thread_threadStarter(void * obj)
{
    Thread * wt = (Thread*)obj;
    wt->ThreadStarter();
}
