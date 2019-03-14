/*++

 File Name:    fpsensor.h
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/


#ifndef LINUX_SPI_FPSENSOR_H
#define LINUX_SPI_FPSENSOR_H


struct fpsensor_platform_data
{
    int irq_gpio;
    int reset_gpio;
    int cs_gpio;
    int external_supply_mv;
    int txout_boost;
    int force_hwid;
    int use_regulator_for_bezel;
    int without_bezel;
};

typedef enum
{
    FPSENSOR_MODE_IDLE           = 0,
    FPSENSOR_MODE_WAIT_AND_CAPTURE       = 1,
    FPSENSOR_MODE_SINGLE_CAPTURE     = 2,
    FPSENSOR_MODE_CHECKERBOARD_TEST_NORM = 3,
    FPSENSOR_MODE_CHECKERBOARD_TEST_INV  = 4,
    FPSENSOR_MODE_BOARD_TEST_ONE     = 5,
    FPSENSOR_MODE_BOARD_TEST_ZERO        = 6,
    FPSENSOR_MODE_WAIT_FINGER_DOWN       = 7,
    FPSENSOR_MODE_WAIT_FINGER_UP     = 8,
    FPSENSOR_MODE_SINGLE_CAPTURE_CAL     = 9,
    FPSENSOR_MODE_CAPTURE_AND_WAIT_FINGER_UP = 10,
    FPSENSOR_MODE_GESTURE = 11 ,
} fpsensor_capture_mode_t;

typedef enum
{
    FPSENSOR_CHIP_NONE  = 0,
    FPSENSOR_CHIP_TEST  = 1,
    FPSENSOR_CHIP_160_160_3 = 2,
    FPSENSOR_CHIP_160_160_2 = 3,
    FPSENSOR_CHIP_56_192_3 = 4,
    FPSENSOR_CHIP_56_192_2 = 5,
    FPSENSOR_CHIP_88_112_3 = 6,
    FPSENSOR_CHIP_88_112_2 = 7,

} fpsensor_chip_t;


#define DBG_FPSENSOR_TRACE
#define DBG_FPSENSOR_ERROR
#define DBG_FPSENSOR_PRINTK

#ifdef DBG_FPSENSOR_TRACE
#define fpsensor_trace(fmt, args...)     \
    do{                                 \
        printk("fpDebug "fmt, ##args);            \
    }while(0)
#else
#define fpsensor_trace(fmt, args...)
#endif

#ifdef DBG_FPSENSOR_ERROR
#define fpsensor_error(fmt, args...)     \
    do{                                 \
        printk("fpDebug "fmt, ##args);            \
    }while(0)
#else
#define fpsensor_error(fmt, args...)
#endif


#ifdef DBG_FPSENSOR_PRINTK
#define fpsensor_printk(fmt, args...)     \
    do{                                 \
        printk("fpDebug "fmt, ##args);            \
    }while(0)
#else
#define fpsensor_printk(fmt, args...)
#endif

#endif

