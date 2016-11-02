/*
Copyright (c) 2012-2013 Ben Croston
Copyright (c) 2014 P.M. Kuipers (Mostly minor additions)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int rev;
    char hw[16]; // 8 would suffice, but in case of future stuff....
} HwInfo;

HwInfo HardwareInfo(void );
uint32_t gpio_get_base_address();
int gpio_init(void);
void gpio_setup(int gpio, int direction, int pud);
int gpio_function(int gpio);
void gpio_output(int gpio, int value);
int gpio_input(int gpio);
void gpio_set_pullupdn(int gpio, int pud);
void gpio_set_rising_event(int gpio, int enable);
void gpio_set_falling_event(int gpio, int enable);
void gpio_set_high_event(int gpio, int enable);
void gpio_set_low_event(int gpio, int enable);
int gpio_eventdetected(int gpio);
void gpio_cleanup(void);

#define GPIO_SETUP_OK          0
#define GPIO_SETUP_DEVMEM_FAIL 1
#define GPIO_SETUP_MALLOC_FAIL 2
#define GPIO_SETUP_MMAP_FAIL   3

#define GPIO_INPUT  1 // is really 0 for control register!
#define GPIO_OUTPUT 0 // is really 1 for control register!
#define GPIO_ALT0   4

#define GPIO_HIGH 1
#define GPIO_LOW  0

#define GPIO_PUD_OFF  0
#define GPIO_PUD_DOWN 1
#define GPIO_PUD_UP   2

#ifdef __cplusplus
}
#endif