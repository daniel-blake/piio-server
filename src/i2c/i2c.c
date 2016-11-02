
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "i2c.h"

static unsigned int HardwareRevision(void);

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

int i2cInit(unsigned char address)
{
	int fd;														// File descrition
	char *fileName = "/dev/i2c-1";								// Name of the port we will be using (using revision 2 board's /dev/i2c-1 by default)

	unsigned int rev = HardwareRevision();
	if (rev < 4){												// Switch to revision 1 board's /dev/i2c-0 if revision number is below 4;
		strcpy(fileName,"/dev/i2c-0");
	}

	if ((fd = open(fileName, O_RDWR)) < 0) {					// Open port for reading and writing
		return -1;
	}
	
	if (ioctl(fd, I2C_SLAVE, address) < 0) {					// Set the port options and set the address of the device we wish to speak to
		return -2;
	}

	return fd;
}

void i2cClose(int fd)
{
    close(fd);
}

int i2cReadReg8(int fd, unsigned char reg)
{
	unsigned char buf[1];										// Buffer for data being read/ written on the i2c bus
	buf[0] = reg;													// This is the register we wish to read from
	
	if ((write(fd, buf, 1)) != 1) {								// Send register to read from
		return -1;
	}
	
	if (read(fd, buf, 1) != 1) {								// Read back data into buf[]
		return -2;
	}
	
	return buf[0];
}

int i2cWriteReg8(int fd, unsigned char reg, unsigned char value)
{
	unsigned char buf[2];
	buf[0] = reg;													// Commands for performing a ranging on the SRF08
	buf[1] = value;
	
	if ((write(fd, buf, 2)) != 2) {								// Write commands to the i2c port
		return -1;
	}
	return 0;
}

int i2cReadReg16(int fd, unsigned char reg)
{
	unsigned char buf[2];										// Buffer for data being read/ written on the i2c bus
	buf[0] = reg;													// This is the register we wish to read from
	
	if ((write(fd, buf, 1)) != 1) {								// Send register to read from
		return -1;
	}
	
	if (read(fd, buf, 2) != 2) {								// Read back data into buf[]
		return -2;
	}
	return (int)(buf[1] << 8) | (int)buf[0];
	

}

int i2cWriteReg16(int fd, unsigned char reg,unsigned short value)
{
	unsigned char buf[3];
	buf[0] = reg;													// Commands for performing a ranging on the SRF08
	buf[1] = (unsigned char)( ( value >> 0 ) & 0xFF );
	buf[2] = (unsigned char)( ( value >> 8 ) & 0xFF ); 
	
	if ((write(fd, buf, 3)) != 3) {								// Write commands to the i2c port
		return -1;
	}
	return 0;
}
