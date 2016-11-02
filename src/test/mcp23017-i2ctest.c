#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../i2c/i2c.h"

int main(int argc, char ** argv)
{
	int fd,i;
	int result;
	
	char * Registers8[22] = 
	{
		"IODIRA  ",  // 00
		"IODIRB  ",  // 01
		"IPOLA   ",  // 02
		"IPOLB   ",  // 03
		"GPINTENA",  // 04
		"GPINTENB",  // 05
		"DEFVALA ",  // 06
		"DEFVALB ",  // 07
		"INTCONA ",  // 08
		"INTCONB ",  // 09
		"IOCON   ",  // 0A
		"IOCON   ",  // 0B
		"GPPUA   ",  // 0C
		"GPPUB   ",  // 0D
		"INTFA   ",  // 0E
		"INTFB   ",  // 0F
		"INTCAPA ",  // 10
		"INTCAPB ",  // 11
		"GPIOA   ",  // 12
		"GPIOB   ",  // 13
		"OLATA   ",  // 14
		"OLATB   "   // 15
	};
	
	char * Registers16[22] = 
	{
		"IODIR  ",  // 00
		"IODIR  ",  // 01
		"IPOL   ",  // 02
		"IPOL   ",  // 03
		"GPINTEN",  // 04
		"GPINTEN",  // 05
		"DEFVAL ",  // 06
		"DEFVAL ",  // 07
		"INTCON ",  // 08
		"INTCON ",  // 09
		"IOCON  ",  // 0A
		"IOCON  ",  // 0B
		"GPPU   ",  // 0C
		"GPPU   ",  // 0D
		"INTF   ",  // 0E
		"INTF   ",  // 0F
		"INTCAP ",  // 10
		"INTCAP ",  // 11
		"GPIO   ",  // 12
		"GPIO   ",  // 13
		"OLAT   ",  // 14
		"OLAT   "  	// 15
	};

	fd = i2cInit(0x20);

	if(argc > 2)
	{
		int reg = -1;
		int val = -1;
		
		if(strlen(argv[1]) == 4 && strncmp(argv[1],"0x",2) == 0)
			reg = strtol(argv[1],NULL,16);
		
		if( (strlen(argv[2]) == 4 || strlen(argv[2]) == 6) && strncmp(argv[2],"0x",2) == 0)
			val = strtol(argv[2],NULL,16);

		if(reg < 0)
			printf ("Please provide an 8 bit hexadecimal value for the register in the form of 0xHH\n");
		if(val < 0)
			printf ("Please provide an 8 or 16 bit hexadecimal value for the value in the form of 0xHH\n");
	
		if(reg >= 0 && val >= 0)
		{
			if(strlen(argv[2]) == 4)
			{
				printf("Setting register %s (0x%02x) to 8 bit value 0x%02x\n\n", Registers8[reg], reg, val);
				i2cWriteReg8(fd,(unsigned char)reg, (unsigned char) val);
			}
			else if (strlen(argv[2]) == 6)
			{
				printf("Setting register %s (0x%02x) to 16 bit value 0x%04x\n\n", Registers16[reg], reg, val);
				i2cWriteReg16(fd,(unsigned char)reg, (unsigned short) val);
			}
		}
	
	}


	printf("Reading registers as 8 bit values\n");
	for(i=0x00; i < 0x16; i++)
	{
		result = i2cReadReg8(fd,i);
		printf("    %s (0x%02x) : 0x%02x\n",Registers8[i], i,result);
	}
	
	printf("\n");
	printf("Reading registers as 16 bit values\n");
	
	for(i=0x00; i < 0x16; i+=2)
	{
		result = i2cReadReg16(fd,i);
		printf("    %s (0x%02x) : 0x%04x\n",Registers16[i], i,result);
	}

	printf("\n");
	printf("Reading registers as 16 bit values with A/B swapped\n");
	
	for(i=0x01; i < 0x16; i+=2)
	{
		result = i2cReadReg16(fd,i);
		printf("    %s (0x%02x) : 0x%04x\n",Registers16[i], i,result);
	}	

	return 0;
}
