#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>

#include "../gpio/c_gpio.h"
#include <iostream>

using namespace std;

int main(int argc, char ** argv)
{
    int in23 = 0;
    int in24 = 0;
    
    int result = gpio_init();

    HwInfo hardware = HardwareInfo(); 
    uint32_t gpio_base = gpio_get_base_address();

    printf("Hardware: '%s'\nRevision: '%d'\nGPIO Base: 0x%X\n",hardware.hw,hardware.rev,gpio_base);
    
    if( access( "/proc/device-tree/soc/ranges", F_OK ) != -1 ) 
    {
        clog << "Devicetree available" << endl;
    }
    else
    {
        clog << "Devicetree NOT available" << endl;
    }
    
    if(result != GPIO_SETUP_OK)
    {
        clog << "Could not open gpio memory map. This program must be run as root" << endl;
        return 1;
    }
    
    gpio_setup(23, GPIO_INPUT, GPIO_PUD_DOWN);
    gpio_setup(24, GPIO_INPUT, GPIO_PUD_UP);
    
    // allow some time to pass by
    int i = 0;
    for(i = 0; i < 30; ++i)
    {
        sleep(1);
        
        in23 = gpio_input(23);
        in24 = gpio_input(24);
        
        cout << "---------------" << endl;
        cout << "Input 23 is " << in23 << endl;
        cout << "Input 24 is " << in24 << endl;
        
    }
    cout << "---------------" << endl;
    cout << "DONE (" << i << "seconds)" << endl;
    
    return 0;
}