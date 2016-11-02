#ifndef __GPIO_HPP_
#define __GPIO_HPP_

#include "../exception/baseexceptions.hpp"
#include "../thread/thread.hpp"

#include <boost/signals2.hpp>

/*! \file Gpio interrupt capture functions. Header file.
*/

// P1 Header pins (both board revisions)
#define GPIO_0_2         2    /*!< \def Gpio pin  0/2 (rev1/rev2) */
#define GPIO_1_3         3    /*!< \def Gpio pin  1/3 (rev1/rev2) */
#define GPIO_4           4    /*!< \def Gpio pin  4 */
#define GPIO_7           7    /*!< \def Gpio pin  7 */
#define GPIO_8           8    /*!< \def Gpio pin  8 */
#define GPIO_9           9    /*!< \def Gpio pin  9 */
#define GPIO_10         10    /*!< \def Gpio pin 10 */
#define GPIO_11         11    /*!< \def Gpio pin 11 */
#define GPIO_14         14    /*!< \def Gpio pin 14 */
#define GPIO_15         15    /*!< \def Gpio pin 15 */
#define GPIO_17         17    /*!< \def Gpio pin 17 */
#define GPIO_18         18    /*!< \def Gpio pin 18 */
#define GPIO_21_27      27    /*!< \def Gpio pin 21/27 (rev1/rev2) */
#define GPIO_22         22    /*!< \def Gpio pin 22 */
#define GPIO_23         23    /*!< \def Gpio pin 23 */
#define GPIO_24         24    /*!< \def Gpio pin 24 */
#define GPIO_25         25    /*!< \def Gpio pin 25 */

// P5 Header pins (only rev 2)
#define GPIO_28         28    /*!< \def Gpio pin 28 */
#define GPIO_29         29    /*!< \def Gpio pin 29 */
#define GPIO_30         30    /*!< \def Gpio pin 30 */
#define GPIO_31         31    /*!< \def Gpio pin 31 */

#define GPIO_FN_MAXLEN   128      // length of file name field

class GpioException : MsgException
{
    protected:
        virtual std::string type() { return "GpioException"; }
};

//! Enum for specifying input/output direction
enum GpioDirection
{
    kDirectionOut = 0,
    kDirectionIn = 1
};

//! Enum for specifying edge detection type
enum GpioEdge
{
    kEdgeNone       = 0,
    kEdgeRising     = 1,
    kEdgeFalling    = 2,
    kEdgeBoth       = 3
};

class GpioPin : public Thread
{
    public:
        GpioPin(int pinnr, GpioDirection direction, GpioEdge edge);
        ~GpioPin();
 
        //! Get the actual used pin number of the IO Pin
        int getPinNr();

        //! Get current direction of pin
        GpioDirection getDirection();
        //! Set new direction of pin
        void setDirection(GpioDirection direction);

        //! Get current edge detection type
        GpioEdge getEdge();
        //! Set edge detection type
        void setEdge(GpioEdge edge);

        //! Set current internal pullup status
        void setPullUp(bool pu);

        //! Set current internal pulldown status
        void setPullDown(bool pd);

        //! Get current value of pin
        bool getValue();
        //! Set new value of pin
        void setValue(bool value);
        
        

        //! Start interrupt listener
        void InterruptStart();
        //! Stop interrupt listener
        void InterruptStop();
       
        //! Signal on interrupt
        boost::signals2::signal<void (GpioPin *, GpioEdge, bool)> onInterrupt;
       
        // Tag to store application-dependant data
        void * Tag;
       
        virtual void ThreadFunc();
        
        //! Check if a certain gpio pin number is valid for the raspberry pi
        static bool CheckPin(int gpiopin);

        //! Verifies gpio pin number, and translates pin numbers (REV2) to the proper REV1 or REV2 board gpio pin numbers
        static int VerifyPin(int gpiopin);       
    private:
        int             pin;                            // Gpio pin number
        std::string     fnDirection;    // File name for Direction file
        std::string     fnEdge;         // File name for Edge file
        std::string     fnValue;        // File name for Value file
        bool            pinPreExported;                 // Bool indicates if the pin was already exported

        


        //! Export a certain Gpio pin
        static bool exportPin(int gpiopin);

        //! Unexport a certain Gpio pin
        static bool unexportPin(int gpiopin);

        //! open a file for writing and write text to it.
        static void writeFile(std::string &fname, const char *value);
        static void writeFile(std::string &fname, std::string &value);

        //! open a file for reading and read some text from it
        static std::string readFile(std::string &fname);
      
};


#endif