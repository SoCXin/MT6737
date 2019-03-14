/*++


 File Name:    fpsensor_input.h
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/


#ifndef LINUX_SPI_FPSENSOR_INPUT_H
#define LINUX_SPI_FPSENSOR_INPUT_H

typedef enum
{
    FPSENSOR_WAKEUP_LEVEL_NORMAL       = 1 << 0,
    FPSENSOR_WAKEUP_LEVEL_ALL          = 1 << 1,

} fpsensor_wakeup_level_t;


extern int fpsensor_input_init(fpsensor_data_t *fpsensor);

extern void fpsensor_input_destroy(fpsensor_data_t *fpsensor);

extern int fpsensor_input_enable(fpsensor_data_t *fpsensor, bool enabled);

extern int fpsensor_input_task(fpsensor_data_t *fpsensor);

extern int fpsensor_input_wakeup(fpsensor_data_t *fpsensor, fpsensor_wakeup_level_t wakeup);

extern int fpsensor_input_key(fpsensor_data_t *fpsensor, int key, int downUp, bool sync);


#endif /* LINUX_SPI_FPSENSOR_NAV_H */

