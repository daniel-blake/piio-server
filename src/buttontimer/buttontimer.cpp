#include "buttontimer.hpp"

#include <time.h>
#ifndef CLOCK_MONOTIC
#   define CLOCK_MONOTIC CLOCK_REALTIME
#endif

ButtonTimer::ButtonTimer(uint32_t shortpress_min_ms, uint32_t longpress_ms)
{
    shortpressMinTime = shortpress_min_ms;
    longpressTime = longpress_ms;
    eventLock = false;
    
    ThreadStart();
}

ButtonTimer::~ButtonTimer()
{
    ThreadStop();

}

void ButtonTimer::RegisterPress(uint16_t keycode)
{
    MutexLock();
    // Prevent trouble when calling this from within one of our event listeners
    if(eventLock) { MutexUnlock(); return; }

    pressRegistry[keycode] = now_ms();
    MutexUnlock();
}


void ButtonTimer::RegisterRelease(uint16_t keycode)
{
    uint64_t now = now_ms();
    uint64_t then;
    MutexLock();
    // Prevent trouble when calling this from within one of our event listeners
    if(eventLock) { MutexUnlock(); return; }

    
    
    if(pressRegistry.count(keycode))
    {
        // if it was a long press, the key code would already have been erased, so we
        // can safely fire the onShortPress event
        then = pressRegistry[keycode];
        // remove from registry after release, if it was a long press, the event should have already been fired
        pressRegistry.erase(keycode); 
        
        if(now - then > shortpressMinTime)
        {
            eventLock = true; // lock out trouble
            onShortPress(keycode);
            eventLock = false; // risk of trouble gone
        }
    }

    MutexUnlock();
}

void ButtonTimer::CancelPress(uint16_t keycode)
{
    // remove button id from map (but only if it is in the map already)
    MutexLock();
    // Prevent trouble when calling this from within one of our event listeners
    if(eventLock) { MutexUnlock(); return; }

    if(pressRegistry.count(keycode))
    {
        pressRegistry.erase(keycode);
    }
    MutexUnlock();
}

void ButtonTimer::ThreadLoop()
{
    uint64_t now = now_ms();
    std::list<uint16_t> btnList;
    boost::optional<bool> valid;
    MutexLock();
    // list through all the items 

    for( std::map<uint16_t,uint64_t>::iterator ii=pressRegistry.begin(); ii!=pressRegistry.end(); ++ii)
    {
        if(now - (ii->second) >= longpressTime) // if it is in overtime
        {
            // Schedule to process the thing
            btnList.push_back(ii->first);
        }
    }    

    // Process listed items
    for (std::list<uint16_t>::iterator it=btnList.begin(); it != btnList.end(); ++it)
    {
        pressRegistry.erase(*it);
        // If any validators are connected, they can retun false to indicate that this connection is not 
        // allowed
        eventLock = true; // lock out trouble
        
        valid = onValidatePress(*it);
        if(valid.get_value_or(true))
        {
            onLongPress(*it);
        }
        eventLock = false; // risk of trouble gone
    }
    
    MutexUnlock();
    usleep(50000);
}

int64_t ButtonTimer::now_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTIC, &now);

    return ((int64_t)now.tv_sec)*1000LL + (now.tv_nsec/1000000);
}
