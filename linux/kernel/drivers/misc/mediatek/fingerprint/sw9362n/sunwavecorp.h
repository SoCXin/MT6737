#ifndef __SUNWAVECORP_H__
#define __SUNWAVECORP_H__

#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "finger.h"
#include "config.h"

#define SUNWAVE_IOC_MAGIC           'k'
#define SUNWAVE_SYSFS_DBG
#define SUNWAVE_SENSOR_MAJOR        150
#define N_SPI_MINORS                32

#define SPI_MODE_MASK       (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
                            | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
                            | SPI_NO_CS | SPI_READY)

#define SUNWAVE_WRITE   0
#define SUNWAVE_READ        1

struct sunwave_rw_operate {
    unsigned char flag;     //0-write 1-read
    unsigned short len;
    unsigned char*  buf;
};

/* Read / Write of SPI mode (SPI_MODE_0..SPI_MODE_3) */
//#define SPI_IOC_RD_MODE                           _IOR(SUNWAVE_IOC_MAGIC, 1, __u8)
//#define SPI_IOC_WR_MODE                           _IOW(SUNWAVE_IOC_MAGIC, 1, __u8)

/* Read / Write SPI bit justification */
//#define SPI_IOC_RD_LSB_FIRST                          _IOR(SUNWAVE_IOC_MAGIC, 2, __u8)
//#define SPI_IOC_WR_LSB_FIRST                      _IOW(SUNWAVE_IOC_MAGIC, 2, __u8)

/* Read / Write SPI device word length (1..N) */
//#define SPI_IOC_RD_BITS_PER_WORD                  _IOR(SUNWAVE_IOC_MAGIC, 3, __u8)
//#define SPI_IOC_WR_BITS_PER_WORD                  _IOW(SUNWAVE_IOC_MAGIC, 3, __u8)

/* Read / Write SPI device default max speed hz */
//#define SPI_IOC_RD_MAX_SPEED_HZ                   _IOR(SUNWAVE_IOC_MAGIC, 4, __u32)
//#define SPI_IOC_WR_MAX_SPEED_HZ                   _IOW(SUNWAVE_IOC_MAGIC, 4, __u32)

/* Write the value to be used by the GPIO */
#define SUNWAVE_IOC_RST_SENSOR                  _IO(SUNWAVE_IOC_MAGIC, 5)
#define SUNWAVE_IOC_SET_SENSOR_STATUS           _IOW(SUNWAVE_IOC_MAGIC, 7, __u8)
#define SUNWAVE_IOC_GET_SENSOR_STATUS           _IOR(SUNWAVE_IOC_MAGIC, 7, __u8)
#define SUNWAVE_IOC_IS_FINGER_ON                _IOR(SUNWAVE_IOC_MAGIC, 10, __u8)
#define SUNWAVE_KEY_REPORT                     _IOW(SUNWAVE_IOC_MAGIC, 12, __u8)
#define SUNWAVE_WAKEUP_SYSTEM                   _IOW(SUNWAVE_IOC_MAGIC, 13, __u8)
#define SUNWAVE_SET_INTERRUPT_WAKE_STATUS       _IOW(SUNWAVE_IOC_MAGIC, 14, __u8)
#define SUNWAVE_WRITE_READ_DATA                 _IOWR(SUNWAVE_IOC_MAGIC, 16, int)
#define SUNWAVE_IOC_ATTRIBUTE                   _IOW(SUNWAVE_IOC_MAGIC, 17, __u32)
#define SUNWAVE_SET_VERSION_INFO                _IOW(SUNWAVE_IOC_MAGIC, 18, __u8)

#endif
