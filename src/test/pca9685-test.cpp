#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../pca9685/pca9685.hpp"

int main(int argc, char ** argv)
{
	Pca9685::Pca9685Config cfg;
	cfg.Frequency = 50;
	
	Pca9685 pca(0x40,cfg);
	
	pca.setValue(4,205,0);
	usleep(1000000);
	pca.setValue(4,320,0);
	usleep(1000000);
	pca.setValue(4,410,0);
	usleep(1000000);
	pca.setValue(4,4096,0);
	
	
	return 0;
}
