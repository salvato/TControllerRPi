#ifndef MCP4725_H
#define MCP4725_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>        // read/write usleep
#include <stdlib.h>        // exit function
#include <inttypes.h>      // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions

#include <QString>

class MCP4725
{
public:
    MCP4725(QString sBus, int iAddress);
    ~MCP4725();
    int Initialize();
    int WriteData(int16_t iData);

private:
    int fd;
    QString i2cBus;
    int mcp4725_address;
    uint8_t writeBuf[3];
};

#endif // MCP4725_H
