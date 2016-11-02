#ifndef __IOGROUP_PWMDRIVER_HPP
#define __IOGROUP_PWMDRIVER_HPP

#include "iogroup-digital.hpp"
#include "thread/thread.hpp"


class IoGroupSoftPWM: public IoGroupDigital, protected Thread
{
public:

    IoGroupSoftPWM(DBus::Connection &connection,std::string &dbuspath, GpioRegistry &registry);
    ~IoGroupSoftPWM();
    
    virtual void Initialize(libconfig::Setting &setting);

protected:

    //! Start the PWM routine for this I/O expander
    void PwmStart();

    //! Stop the PWM routine for this I/O expander
    void PwmStop();

    void setPwmConfig(uint32_t tick_delay_us, uint8_t ticks);

    virtual bool setPwm(uint16_t id, uint8_t value);
    
    virtual void preparePwmPin(uint16_t pinid);

    virtual void ThreadFunc(void);  // Override this if you want the entire function customized

private:

	std::set<uint16_t> pwm_pins;
	std::set<uint16_t> active_pwms;
    std::map<uint16_t,uint8_t> pwm_values;
    std::map<uint16_t,uint8_t> pwm_v_values;

    bool        pwm_enabled;        // Boolean used to stop the PWM thread safely
    uint32_t    pwm_tick_delay_us;  // Interval between PWM steps in us
    uint8_t     pwm_ticks;          // Number of PWM steps before coming full circle
    uint16_t    pwm_prev_val;       // Keeps state of pwm output;

};

#endif//__IOGROUP_PWMDRIVER_HPP