#ifndef ADS1115_H
#define ADS1115_H


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>        // read/write usleep
#include <stdlib.h>        // exit function
#include <inttypes.h>      // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions

#include <QString>

class ADS1115
{
public:
    ADS1115(QString sBus, int iAddress);
    ~ADS1115();
    int Initialize();
    double ReadData();

private:
    int fd;
    QString i2cBus;
    int ads1115_address;
    uint8_t readBuf[3];
    double conversionFactor;

public:
    const quint16 Ch1SingleEnded  = 0x4000;// AIN0(+)-AIN1(-)
    const quint16 Ch1Differential = 0x0000;// AIN0(0)-GND
    const quint16 Ch2SingleEnded  = 0x3000;// AIN2(+)-AIN3(-)
    const quint16 Ch2Differential = 0x6000;// AIN2(0)-GND

    const quint16 FS6144 = 0x0000;// +/-6.144V
    const quint16 FS4096 = 0x0200;// +/-4.096V
    const quint16 FS2048 = 0x0400;// +/-2.048V
    const quint16 FS1024 = 0x0600;// +/-1.024V
    const quint16 FS512  = 0x0800;// +/-0.512V
    const quint16 FS256  = 0x0A00;// +/-0.256V

    const quint16 ContinuousConversion = 0x0000;
    const quint16 SingleShotConversion = 0x0100;

    const quint16 SPS8   = 0x0000; //   8 Samples per Second
    const quint16 SPS16  = 0x0020; //  16 Samples per Second
    const quint16 SPS32  = 0x0040; //  32 Samples per Second
    const quint16 SPS64  = 0x0060; //  64 Samples per Second
    const quint16 SPS128 = 0x0080; // 128 Samples per Second
    const quint16 SPS250 = 0x00A0; // 250 Samples per Second
    const quint16 SPS475 = 0x00C0; // 475 Samples per Second
    const quint16 SPS860 = 0x00E0; // 860 Samples per Second

    const quint8 DataReady = 0x80;
};

#endif // ADS1115_H
