#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <unistd.h>

#include "gpio.hpp"
#include "c_gpio.h"
#include "../log/log.hpp"
#include <iostream>


#define GPIO_0_2_R1        0      /*!< \def Gpio pin  0/2 (rev1/rev2) with rev2 board code */
#define GPIO_1_3_R1        1      /*!< \def Gpio pin  1/3 (rev1/rev2) with rev2 board code */
#define GPIO_21_27_R1     21      /*!< \def Gpio pin 21/27 (rev1/rev2) with rev2 board code */

#define RDBUF_LEN	      10      // length of read buffer
#define POLL_TIMEOUT     100      // timeout for polling function in ms (also the maximum delay time before thread is stopped


#define FALSE           0
#define TRUE            1

using namespace std;

/****************************
*                           *
*     PRIVATE DEFINTIONS    *
*                           *
*****************************/

static unsigned int HardwareRevision(void);

/****************************
*                           *
*     IOPIN OBJECT FUNCS    *
*                           *
*****************************/

//! Create new IOPin object
GpioPin::GpioPin(int gpiopin, GpioDirection direction, GpioEdge edge)
{
    int result;
    int pin_id;
    char fTemp[GPIO_FN_MAXLEN];

    // Set tag to empty pointer
    Tag = NULL; 

    pin_id = VerifyPin(gpiopin);
    if(pin_id < 0)
        throw OperationFailedException("Gpio pin %d is not a valid Gpio for this Raspberry Pi board revision",gpiopin);
    
    // ensure that the Gpio pin is exported
    try
    {
        pinPreExported = !(exportPin(pin_id));
    }
    catch(OperationFailedException x)
    {
        throw x;
    }

    // Prepare the iopin object
    pin = pin_id;
    
    // Prepare the file names for the different files
    snprintf(fTemp, GPIO_FN_MAXLEN-1, "/sys/class/gpio/gpio%d/direction", pin_id);
    fnDirection = std::string(fTemp);
    snprintf(fTemp,      GPIO_FN_MAXLEN-1, "/sys/class/gpio/gpio%d/edge", pin_id);
    fnEdge = std::string(fTemp);
    snprintf(fTemp,     GPIO_FN_MAXLEN-1, "/sys/class/gpio/gpio%d/value", pin_id);
    fnValue = std::string(fTemp);

    // Initialize callbacks to NULL

    try
    {
        // set initial direction or die trying
        setDirection(direction);
        // set initial edge or die trying
        setEdge(edge);
    }
    catch(OperationFailedException x)
    {
        if(!pinPreExported)
            unexportPin(pin);
        throw x;
    }
}


//! Close the IOPin connection
GpioPin::~GpioPin()
{
    if(!pinPreExported)
        unexportPin(pin);
}

//! Get the actual used pin number of the IO Pin
int GpioPin::getPinNr()
{
    return pin;
}   

//! Get current direction of pin
GpioDirection GpioPin::getDirection()
{
    
    std::string s = readFile(fnDirection);
    // got enough info in the first byte
	if(s[0] == 'i') 
        return kDirectionIn;
    else
        return kDirectionOut;
}

//! Set new value of pin
void GpioPin::setDirection(GpioDirection direction)
{
         if (direction == kDirectionIn)     writeFile(fnDirection,"in\n");
    else if (direction == kDirectionOut)    writeFile(fnDirection,"out\n");
}

//! Get current edge detection type
GpioEdge GpioPin::getEdge()
{
    std::string s = readFile(fnEdge);
	switch(s[0]) // as the first letters of each result are all different
    {
        default :
        case 'n':
        case 'N':
            return kEdgeNone;
        case 'r':
        case 'R':
            return kEdgeRising;
        case 'f':
        case 'F':
            return kEdgeFalling;
        case 'b':
        case 'B':
            return kEdgeBoth;
    
    }
}

//! Set edge detection type
void GpioPin::setEdge(GpioEdge edge)
{
         if (edge == kEdgeNone)     writeFile(fnEdge,"none\n"); 
    else if (edge == kEdgeRising)   writeFile(fnEdge,"rising\n"); 
    else if (edge == kEdgeFalling)  writeFile(fnEdge,"falling\n"); 
    else if (edge == kEdgeBoth)     writeFile(fnEdge,"both\n"); 
}

//! Set current internal pullup status on inputs
void GpioPin::setPullUp(bool pu)
{
    if(pu)
    {
        gpio_set_pullupdn(this->pin,GPIO_PUD_UP);
    }
    else
    {
        gpio_set_pullupdn(this->pin,GPIO_PUD_OFF);
    }
}

//! Set current internal pullup status on inputs
void GpioPin::setPullDown(bool pd)
{
    if(pd)
    {
        gpio_set_pullupdn(this->pin,GPIO_PUD_DOWN);
    }
    else
    {
        gpio_set_pullupdn(this->pin,GPIO_PUD_OFF);
    }
}


//! Get current value of pin
bool GpioPin::getValue()
{
    std::string s = readFile(fnValue);
    // got enough info in the first byte
	//if(gpio_input(this->pin) 
    //    return true;
    //else
    //    return false;
}

//! Set new value of pin
void GpioPin::setValue(bool value)
{
    if(value)   gpio_output(this->pin, 1);
    else        gpio_output(this->pin, 0);
    //if (value)  writeFile(fnValue,"1\n");
    //else        writeFile(fnValue,"0\n");
}



/****************************
*                           *
*     INTERRUPT FUNCS       *
*                           *
*****************************/

void GpioPin::InterruptStart()
{
    if(!ThreadRunning())
    {
        this->ThreadStart();
    }
}

void GpioPin::InterruptStop()
{
    if(ThreadRunning())
    {
        this->ThreadStop();
    }
}


void GpioPin::ThreadFunc()
{
	int fd,ret;
	struct pollfd pfd;
	char rdbuf[RDBUF_LEN];

	memset(rdbuf, 0x00, RDBUF_LEN);

	fd=open(fnValue.c_str(), O_RDONLY);
	if(fd<0)
        throw OperationFailedException("Could not open file %s for reading: [%d] %s",fnValue.c_str(), errno, strerror(errno));

	pfd.fd=fd;
	pfd.events=POLLPRI;
	
	ret=read(fd, rdbuf, RDBUF_LEN-1);
	if(ret<0)
    {
        close(fd);
        throw OperationFailedException("Could not read from  %s: [%d] %s",fnValue.c_str(), errno, strerror(errno));
	}
    
	while(ThreadRunning())
    {
		memset(rdbuf, 0x00, RDBUF_LEN);
		lseek(fd, 0, SEEK_SET);
		ret=poll(&pfd, 1, POLL_TIMEOUT);
		if(ret<0)   // negative result is error
        {
            close(fd);
            throw OperationFailedException("Could not poll %s: [%d] %s",fnValue.c_str(), errno, strerror(errno));
        }
        
		if(ret==0) 
			continue; // 0 bytes read is timeout, we should retry read
        // ok, poll succeesed, now we read the value
		ret=read(fd, rdbuf, RDBUF_LEN-1);
		if(ret<0)
        {
            close(fd);
            throw OperationFailedException("Could not read from %s: [%d] %s",fnValue.c_str(), errno, strerror(errno));
        }
        
        // Kill the loop now if the thread stopped during our poll
        if(!ThreadRunning())
            break;
        // Continue with doing the callback, if we're still enabled.
        // Now, rdbuf[0] contains 0 or 1 depending on the trigger
        onInterrupt(this, kEdgeFalling, !(rdbuf[0] == '0'));
	}
	close(fd);
    
}




/****************************
*                           *
*     SUPPORT FUNCTIONS     *
*                           *
*****************************/

//! Check if a certain gpio pin number is valid for the raspberry pi
bool GpioPin::CheckPin(int gpiopin)
{
    return (VerifyPin(gpiopin) >= 0);
}

//! open a file for writing and write text to it
void GpioPin::writeFile(std::string &fname, std::string &value)
{
    writeFile(fname,value.c_str());
}

void GpioPin::writeFile(std::string &fname, const char *value)
{
    FILE *fd;
    if ((fd = fopen (fname.c_str(), "w")) == NULL)
        throw OperationFailedException("Could not open %s for writing",fname.c_str());

    fputs (value,fd);
    fclose(fd);
}



//! open a file for reading and read some text from it
/*!
    function will throw an exception on empty string, since the files we use it on
    will always return a value. If they don't we have serious problems
*/
std::string GpioPin::readFile(std::string &fname)
{
    FILE *fd;
    char rdbuf[RDBUF_LEN];
    int ret;
    
    if ((fd = fopen (fname.c_str(), "r")) == NULL)
        throw OperationFailedException("Could not open %s for reading",fname.c_str());

    ret = fread(rdbuf,1,RDBUF_LEN -1, fd);
    fclose(fd);

	if(ret<0)
    {
        throw OperationFailedException("Error reading from %s: [%d] %s",fname.c_str(), errno, strerror(errno));
    }
    else if(ret == 0)
    {   
        throw OperationFailedException("Got empty string reading from %s: [%d] %s",fname.c_str(), errno, strerror(errno));
    }

    rdbuf[ret] = '\0'; // Ensure null termination
    
    return std::string(rdbuf);
}


//! Verifies gpio pin number, and translates pin numbers (REV2) to the proper REV1 or REV2 board gpio pin numbers
/*!
    \param gpiopin The pin number to verify
    \return verified and translated gpio pin, or -1 if invalid
*/
int GpioPin::VerifyPin(int gpiopin)
{
    // List of valid Gpio pins 
    //                        0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32
    int validpinsRev1[17] = { 0, 1, 4, 7, 8, 9,10,11,14,15,17,18,21,22,23,24,25};
    int validpinsRev2[21] = { 2, 3, 4, 7, 8, 9,10,11,14,15,17,18,22,23,24,25,27,28,29,30,31};
    int validpinsRPi2[32] = { 0, 1 ,2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
    unsigned int i;
    
    // get hardware revision
    HwInfo hwinfo = HardwareInfo();
    
    if (strcmp(hwinfo.hw, "BCM2709") == 0)
    {
        // probably raspberry pi 2
        // Verify that the pin number is valid, otherwise return -1 
        for(i=0; i<sizeof(validpinsRPi2);i++)
        {
            if(gpiopin == validpinsRPi2[i])
                return gpiopin;
        }
        return -1;        
    }
    else
    {
        if (hwinfo.rev < 4)
        {	// REV 1 BOARD
        
            // Translate pins to rev 1 equivalent
            if(gpiopin == GPIO_0_2)
                gpiopin = GPIO_0_2_R1;
            else if(gpiopin ==  GPIO_1_3)
                gpiopin =  GPIO_1_3_R1;
            else if(gpiopin ==  GPIO_21_27)
                gpiopin =  GPIO_21_27_R1;
                
            // Verify that the pin number is valid, otherwise return -1 
            for(i=0; i<sizeof(validpinsRev1);i++)
            {
                if(gpiopin == validpinsRev1[i])
                    return gpiopin;
            }
            return -1;
        }
        else
        {   // REV 2 BOARD

            // Verify that the pin number is valid, otherwise return -1 
            for(i=0; i<sizeof(validpinsRev2);i++)
            {
                if(gpiopin == validpinsRev2[i])
                    return gpiopin;
            }
            return -1;
        }
    }
}

//! Export a certain Gpio pin
/*!
    \param gpiopin The gpio pin to export
    \return true if pin was exported by us, false if previously exported
*/
bool GpioPin::exportPin(int gpiopin)
{
    FILE *fd ;
    int pin_id;

    pin_id = VerifyPin(gpiopin);    // verify that the pin is correct
    if(pin_id < 0)
        throw InvalidArgumentException("Pin %d is not a usable Raspberry Pi GPIO pin",pin_id);

    if ((fd = fopen ("/sys/class/gpio/export", "w")) == NULL)
    {
        throw OperationFailedException("Pin %d cannot be exported (cannot write to /sys/class/gpio/export)",pin_id);
    }
      
    fprintf (fd, "%d\n", pin_id) ;
      
    if(fclose (fd) != 0)
    {
//        fprintf(stderr, "Got error code %d - %s\n", errno,strerror(errno));

        if(errno == EBUSY) // indicates the pin is currently already exported      
            return false;
        else
            throw OperationFailedException("Pin %d cannot be exported",pin_id);
    }
    else
        return true;
}

//! Unexport a certain Gpio pin
/*!
    \param gpiopin The gpio pin to unexport
    \return true if pin was unexported by us, false if previously unexported
*/
bool GpioPin::unexportPin(int gpiopin)
{
    FILE *fd ;
    int pin_id;

    pin_id = VerifyPin(gpiopin);    // verify that the pin is correct
    if(pin_id < 0)
        throw InvalidArgumentException("Pin %d is not a usable Raspberry Pi GPIO pin",pin_id);

    if ((fd = fopen ("/sys/class/gpio/unexport", "w")) == NULL)
    {
        throw OperationFailedException("Pin %d cannot be unexported (cannot write to /sys/class/gpio/unexport)",pin_id);
    }
      
    fprintf (fd, "%d\n", pin_id) ;
      
    if(fclose (fd) != 0)
    {
//        fprintf(stderr, "Got error code %d - %s\n", errno,strerror(errno));

        if(errno == EINVAL) // indicates the pin is not currently exported      
            return false;
        else
            throw OperationFailedException("Pin %d cannot be unexported",pin_id);
    }
    else
        return true;
}

static unsigned int HardwareRevision(void)
{
   FILE * filp;
   unsigned rev;
   char buf[512];
   char term;

   rev = 0;

   filp = fopen ("/proc/cpuinfo", "r");

   if (filp != NULL)
   {
      while (fgets(buf, sizeof(buf), filp) != NULL)
      {
         if (!strncasecmp("revision\t", buf, 9))
         {
            if (sscanf(buf+strlen(buf)-5, "%x%c", &rev, &term) == 2)
            {
               if (term == '\n') break;
               rev = 0;
            }
         }
      }
      fclose(filp);
   }
   return rev;
}
