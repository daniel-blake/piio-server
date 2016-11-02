#ifndef __USER_I2C_H__
#define __USER_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

int i2cInit(unsigned char address);
void i2cClose(int fd);
int i2cReadReg8(int fd, unsigned char reg);
int i2cWriteReg8(int fd, unsigned char reg, unsigned char value);
int i2cReadReg16(int fd, unsigned char reg);
int i2cWriteReg16(int fd, unsigned char reg,unsigned short value);

#ifdef __cplusplus
}
#endif

#endif