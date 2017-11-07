#include "ads1115.h"
#include <sys/ioctl.h>

ADS1115::ADS1115(QString sBus, int iAddress)
    : fd(0)
    , i2cBus(sBus)
    , ads1115_address(iAddress)
{
}


ADS1115::~ADS1115(){
    close(fd);
}


int
ADS1115::Initialize() {
    // open device on /dev/i2c-1
    if((fd = open(i2cBus.toLatin1(), O_RDWR)) < 0) {
      printf("ADS1115 Error: Couldn't open device! %d\n", fd);
      return -1;
    }
    // connect to ADS1115 as i2c slave
    if(ioctl(fd, I2C_SLAVE, ads1115_address) < 0) {
      printf("Error: Couldn't find ADS1115 on address!\n");
      return -1;
    }
    quint16 myConfig = Ch1Differential | FS2048 | ContinuousConversion;
    char config[3] = {0};
    // Select configuration register(0x01)
    config[0] = 0x01;
    config[1] = myConfig >> 8;
    config[2] = myConfig & 0xff;
    if(write(fd, config, 3) != 3) {
        printf("Error: Couldn't configure ADS1115\n");
        return -1;
    }
    conversionFactor = 2.048 / 32768.0;
    return 0;
}


double
ADS1115::ReadData() {
    qint16 raw_adc;
    // Read 2 bytes of data from register(0x00)
    // raw_adc msb, raw_adc lsb
    char reg[1] = {0x00};
    write(fd, reg, 1);
    quint8 data[2]={0};
    if(read(fd, data, 2) != 2) {
      printf("Error : Input/Output Error \n");
      raw_adc = 0;
    }
    else {
      // Convert the data
      raw_adc = ((data[0] << 8) + data[1]);
    }
    return double(raw_adc * conversionFactor);
}
