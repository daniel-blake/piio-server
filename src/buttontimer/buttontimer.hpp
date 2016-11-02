#ifndef __BUTTONTIMER_HPP
#define __BUTTONTIMER_HPP

#include "../thread/thread.hpp"
#include <stdint.h>
#include <boost/signals2.hpp>

#include <map>

// combiner which perfoms a kind of and function on all signal returns, returns true if no signals connected
class ButtonTimer : protected Thread
{
    public:
        ButtonTimer(uint32_t shortpress_min_ms, uint32_t longpress_ms);
        ~ButtonTimer();
        
        void RegisterPress(uint16_t keycode);
        void RegisterRelease(uint16_t keycode);
        void CancelPress(uint16_t id);
        
        boost::signals2::signal<void (uint16_t keycode)> onShortPress;
        boost::signals2::signal<void (uint16_t keycode)> onLongPress;
        boost::signals2::signal<bool (uint16_t keycode)> onValidatePress;
    
    protected:
        virtual void ThreadLoop(void);
    
    private:
        boost::signals2::connection onThreadErrorConnection;
        uint32_t longpressTime;
        uint32_t shortpressMinTime;
        std::map<uint16_t, uint64_t> pressRegistry;

        bool eventLock;
        static int64_t now_ms(void);

};

#endif