#include "mcp4725.h"
#include <sys/ioctl.h>

MCP4725::MCP4725(QString sBus, int iAddress)
    : fd(0)
    , i2cBus(sBus)
    , mcp4725_address(iAddress)
{
}


MCP4725::~MCP4725(){
    close(fd);
}


int
MCP4725::Initialize() {
    // open device on /dev/i2c-1
    if((fd = open(i2cBus.toLatin1(), O_RDWR)) < 0) {
      printf("Error: Couldn't open device! %d\n", fd);
      return -1;
    }
    // connect to ads1115 as i2c slave
    if(ioctl(fd, I2C_SLAVE, mcp4725_address) < 0) {
      printf("Error: Couldn't find device on address!\n");
      return -1;
    }
    return 0;
}


int
MCP4725::WriteData(int16_t iData) {
    // 12-bit device values from 0-4095

    // page 18-19 spec sheet
    writeBuf[0] = 0b01000000; // control byte
    // bits 7-5; 010 write DAC; 011 write DAC and EEPROM
    // bits 4-3 unused
    // bits 2-1 PD1, PD0 PWR down P19 00 normal.
    // bit 0 unused

    // write number to MCP4725 DAC
    writeBuf[1] = iData >> 4; // MSB 11-4 shift right 4 places
    writeBuf[2] = iData << 4; // LSB 3-0 shift left 4 places

    if(write(fd, writeBuf, 3) != 3) {
      perror("Write to register 1");
      return -1;
    }
    return 0;
}
