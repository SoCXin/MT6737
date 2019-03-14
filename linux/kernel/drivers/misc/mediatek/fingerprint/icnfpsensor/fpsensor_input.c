/*++

 File Name:    fpsensor_input.c
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/

#include <linux/input.h>
#include <linux/delay.h>
#include "fpsensor_common.h"
#include "fpsensor_input.h"
#include "fpsensor_capture.h"
#include "fpsensor_platform.h"



/* -------------------------------------------------------------------- */
/* function prototypes                          */
/* -------------------------------------------------------------------- */
static int fpsensor_write_lpm_setup(fpsensor_data_t *fpsensor);
static int fpsensor_wait_finger_present_lpm(fpsensor_data_t *fpsensor);


/* -------------------------------------------------------------------- */
/* driver constants                         */
/* -------------------------------------------------------------------- */
#define FPSENSOR_KEY_FINGER_PRESENT  KEY_WAKEUP  /* 143*/

#define FPSENSOR_INPUT_POLL_TIME_MS  1000u

//#define FPSENSOR_KEY_GESTURE_UP                DPAD_UP
//#define FPSENSOR_KEY_GESTURE_DOWN              DPAD_DOWN
//#define FPSENSOR_KEY_GESTURE_LEFT              DPAD_LEFT
//#define FPSENSOR_KEY_GESTURE_RIGHT             DPAD_RIGHT
//#define FPSENSOR_KEY_GESTURE_CLICK             ENTER
#define FPSENSOR_KEY_GESTURE_F11               KEY_F11
#define FPSENSOR_KEY_GESTURE_F10               KEY_F10


/* -------------------------------------------------------------------- */
/* function definitions                         */
/* -------------------------------------------------------------------- */
int fpsensor_input_init(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace( "%s\n", __func__);

    fpsensor->input_dev = input_allocate_device();

    if (!fpsensor->input_dev)
    {
        fpsensor_error( "Input_allocate_device failed.\n");
        error  = -ENOMEM;
    }

    if (!error)
    {
        fpsensor->input_dev->name = FPSENSOR_DEV_NAME;

        set_bit(EV_KEY,     fpsensor->input_dev->evbit);

        set_bit(FPSENSOR_KEY_FINGER_PRESENT, fpsensor->input_dev->keybit);

//        set_bit(FPSENSOR_KEY_GESTURE_UP, fpsensor->input_dev->keybit);
//        set_bit(FPSENSOR_KEY_GESTURE_DOWN, fpsensor->input_dev->keybit);
//        set_bit(FPSENSOR_KEY_GESTURE_LEFT, fpsensor->input_dev->keybit);
//        set_bit(FPSENSOR_KEY_GESTURE_RIGHT, fpsensor->input_dev->keybit);
//        set_bit(FPSENSOR_KEY_GESTURE_CLICK, fpsensor->input_dev->keybit);
        set_bit(FPSENSOR_KEY_GESTURE_F11, fpsensor->input_dev->keybit);
        set_bit(FPSENSOR_KEY_GESTURE_F10, fpsensor->input_dev->keybit);

        error = input_register_device(fpsensor->input_dev);
    }

    if (error)
    {
        fpsensor_error( "Input_register_device failed.\n");
        input_free_device(fpsensor->input_dev);
        fpsensor->input_dev = NULL;
    }

    return error;
}


/* -------------------------------------------------------------------- */
void fpsensor_input_destroy(fpsensor_data_t *fpsensor)
{
    fpsensor_trace( "%s\n", __func__);

    if (fpsensor->input_dev != NULL)
    {
        input_free_device(fpsensor->input_dev);
    }
}


/* -------------------------------------------------------------------- */
int fpsensor_input_enable(fpsensor_data_t *fpsensor, bool enabled)
{
    fpsensor_trace( "%s\n", __func__);

    fpsensor->input.enabled = enabled;

    return 0;
}


/* -------------------------------------------------------------------- */
int fpsensor_input_task(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace( "%s\n", __func__);

    while (!fpsensor->worker.stop_request && !error)
    {

        error = fpsensor_wait_finger_present_lpm(fpsensor);
        fpsensor_trace( "%s, error: %d\n", __func__, error);
        if (error == 0)
        {
            fpsensor_trace( "report finger down!!!!!!!!!!!!!!!!!!!!\n");
            input_report_key(fpsensor->input_dev,
                             FPSENSOR_KEY_FINGER_PRESENT, 1);
            input_report_key(fpsensor->input_dev,
                             FPSENSOR_KEY_FINGER_PRESENT, 0);

            input_sync(fpsensor->input_dev);
        }
    }
    return error;
}


int fpsensor_input_wakeup(fpsensor_data_t *fpsensor, fpsensor_wakeup_level_t wakeup)
{
    if (wakeup == FPSENSOR_WAKEUP_LEVEL_NORMAL)
    {
        fpsensor_trace( "wakeup system request!\n");
        input_report_key(fpsensor->input_dev,
                         FPSENSOR_KEY_FINGER_PRESENT, 1);
        input_report_key(fpsensor->input_dev,
                         FPSENSOR_KEY_FINGER_PRESENT, 0);

        input_sync(fpsensor->input_dev);

        return 0;
    }
    else
    {
        return -EAGAIN;
    }
}
int fpsensor_input_key(fpsensor_data_t *fpsensor, int key, int downUp, bool sync)
{
    fpsensor_trace( "report key: %d, %d, %d\n", key, downUp, sync);
    input_report_key(fpsensor->input_dev,  key, downUp);
    if (sync)
    {
        input_sync(fpsensor->input_dev);
    }
    return 0;
}

/* -------------------------------------------------------------------- */
static int fpsensor_write_lpm_setup(fpsensor_data_t *fpsensor)
{
    const int mux = FPSENSOR_MAX_ADC_SETTINGS - 1;
    int error = 0;
    u16 temp_u16;
    fpsensor_reg_access_t reg;

    fpsensor_trace( "%s %d\n", __func__, mux);

    error = fpsensor_write_sensor_setup(fpsensor);
    if (error)
    {
        goto out;
    }

    temp_u16 = fpsensor->setup.adc_shift[mux];
    temp_u16 <<= 8;
    temp_u16 |= fpsensor->setup.adc_gain[mux];

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = fpsensor->setup.pxl_ctrl[mux];
    temp_u16 |= FPSENSOR_PXL_BIAS_CTRL;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

/* -------------------------------------------------------------------- */
static int fpsensor_wait_finger_present_lpm(fpsensor_data_t *fpsensor)
{
    const int lpm_poll_delay_ms = FPSENSOR_INPUT_POLL_TIME_MS;
    const int zmask_5 = 1 << 5;
    const int zmask_6 = 1 << 6;
    const int zmask_ext = FPSENSOR_FINGER_DETECT_ZONE_MASK;

    int error = 0;
    int zone_raw = 0;

    bool wakeup_center = false;
    bool wakeup_ext    = false;
    bool wakeup        = false;
    fpsensor_trace( "%s\n", __func__);

    error = fpsensor_wake_up(fpsensor);

    if (!error)
    {
        error = fpsensor_calc_finger_detect_threshold_min(fpsensor);
    }

    if (error >= 0)
    {
        error = fpsensor_set_finger_detect_threshold(fpsensor, error);
    }

    if (error >= 0)
    {
        error = fpsensor_write_lpm_setup(fpsensor);
    }

//TEST_SLEEP
    if (!error)
    {
        error = fpsensor_sleep(fpsensor, false);

        if (error == -EAGAIN)
        {
            error = fpsensor_sleep(fpsensor, false);

            if (error == -EAGAIN)
            {
                error = 0;
            }
        }
    }
//
    while (!fpsensor->worker.stop_request && !error && !wakeup)
    {
        if (!error)
        {
            error = fpsensor_wait_finger_present(fpsensor);
        }

        if (!error)
        {
            error = fpsensor_check_finger_present_raw(fpsensor);
        }

        zone_raw = (error >= 0) ? error : 0;

        if (error >= 0)
        {
            error = 0;

            wakeup_center = (zone_raw & zmask_5) ||
                            (zone_raw & zmask_6);

            /* Todo: refined extended processing ? */
            wakeup_ext = ((zone_raw & zmask_ext) == zmask_ext);

        }
        else
        {
            wakeup_center =
                wakeup_ext    = false;
        }

        if (wakeup_center && wakeup_ext)
        {
            fpsensor_trace("%s Wake up !\n", __func__);
            wakeup = true;
        }
//TEST_SLEEP
        if (!wakeup && !error)
        {
            error = fpsensor_sleep(fpsensor, false);

            if (error == -EAGAIN)
            {
                error = 0;
            }

            if (!error)
            {
                msleep(lpm_poll_delay_ms);
            }
        }
    }

    if (error < 0)
        fpsensor_trace("%s %s %d!\n", __func__,
                       (error == -EINTR) ? "TERMINATED" : "FAILED", error);

    return error;
}



