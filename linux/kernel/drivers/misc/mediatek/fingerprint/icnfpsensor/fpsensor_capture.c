/*++

 File Name:    fpsensor_capture.c
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/

#include <linux/input.h>
#include <linux/delay.h>
#include <linux/time.h>
#include "fpsensor_common.h"
#include "fpsensor_capture.h"
#include "fpsensor_platform.h"

/* -------------------------------------------------------------------- */
/* function prototypes                          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* function definitions                         */
/* -------------------------------------------------------------------- */
int fpsensor_init_capture(fpsensor_data_t *fpsensor)
{
    fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_IDLE;
    fpsensor->capture.current_mode = FPSENSOR_MODE_IDLE;
    fpsensor->capture.available_bytes = 0;
    fpsensor->capture.deferred_finger_up = false;

    init_waitqueue_head(&fpsensor->capture.wq_data_avail);

    return 0;
}


/* -------------------------------------------------------------------- */
int fpsensor_write_capture_setup(fpsensor_data_t *fpsensor)
{
    return fpsensor_write_sensor_setup(fpsensor);
}

int fpsensor_write_gesture_setup(fpsensor_data_t *fpsensor, u8 sample_ratio)
{
    int error = 0;
    u32 temp_u32;
    fpsensor_reg_access_t reg;

    switch (sample_ratio)
    {
        case 4:
            temp_u32 = 0x01aa00;
            break;
        case 16:
            temp_u32 = 0x028800;
            break;
        case 8:
            temp_u32 = 0x02aa00;
            break;
    }

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMG_SMPL_SETUP, &temp_u32);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;

}
/* -------------------------------------------------------------------- */
int fpsensor_write_test_setup(fpsensor_data_t *fpsensor, u16 pattern)
{
    int error = 0;
    u8 config = 0x04;
    fpsensor_reg_access_t reg;

    fpsensor_trace( "%s, pattern 0x%x\n", __func__, pattern);

    error = fpsensor_write_sensor_setup(fpsensor);
    if (error)
    {
        goto out;
    }

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_TST_COL_PATTERN_EN, &pattern);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &config);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    error = fpsensor_capture_settings(fpsensor, 0);
    if (error)
    {
        goto out;
    }

out:
    return error;
}


int fpsensor_write_cb_test_setup_160_3(fpsensor_data_t *fpsensor, bool invert)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;

    temp_u16 = (invert) ? 0x55aa : 0xaa55;
    fpsensor_trace( "%s, pattern 0x%x\n", __func__, temp_u16);

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_TST_COL_PATTERN_EN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = 0x04;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xf0e;//(invert) ? 0x0f1b : 0x0f0f;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0x0c00;//(invert) ? 0x0800 : 0x00;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

int fpsensor_write_cb_test_setup_160_2(fpsensor_data_t *fpsensor, bool invert)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;


    temp_u16 = (invert) ? 0x55aa : 0xaa55;
    fpsensor_trace( "%s, pattern 0x%x\n", __func__, temp_u16);

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_TST_COL_PATTERN_EN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = 0x04;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xf0e;//(invert) ? 0x0f1b : 0x0f0f;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0x0c00;//(invert) ? 0x0800 : 0x00;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

int fpsensor_write_cb_test_setup_56_3(fpsensor_data_t *fpsensor, bool invert)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;


    temp_u16 = (invert) ? 0x55aa : 0xaa55;
    fpsensor_trace( "%s, pattern 0x%x\n", __func__, temp_u16);

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_TST_COL_PATTERN_EN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = 0x04;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xf0e;//(invert) ? 0x0f1b : 0x0f0f;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0x0c00;//(invert) ? 0x0800 : 0x00;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_write_cb_test_setup_56_2(fpsensor_data_t *fpsensor, bool invert)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;

    temp_u16 = (invert) ? 0x55aa : 0xaa55;
    fpsensor_trace( "%s, pattern 0x%x\n", __func__, temp_u16);

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_TST_COL_PATTERN_EN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = 0x14;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xf0e;//(invert) ? 0x0f1a : 0x0f0e;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0x0c00;//(invert) ? 0x0800 : 0x0000;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}
/* -------------------------------------------------------------------- */
int fpsensor_write_cb_test_setup_88_3(fpsensor_data_t *fpsensor, bool invert)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;

    temp_u16 = (invert) ? 0x55aa : 0xaa55;
    fpsensor_trace( "%s, pattern 0x%x\n", __func__, temp_u16);

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_TST_COL_PATTERN_EN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = 0x14;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xf0e;//(invert) ? 0x0f1a : 0x0f0e;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0x0c00;//(invert) ? 0x0800 : 0x0000;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}
/* -------------------------------------------------------------------- */
int fpsensor_write_cb_test_setup_88_2(fpsensor_data_t *fpsensor, bool invert)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;

    temp_u16 = (invert) ? 0x55aa : 0xaa55;
    fpsensor_trace( "%s, pattern 0x%x\n", __func__, temp_u16);

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_TST_COL_PATTERN_EN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = 0x14;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xf0e;//(invert) ? 0x0f1a : 0x0f0e;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0x0c00;//(invert) ? 0x0800 : 0x0000;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

/* -------------------------------------------------------------------- */
int fpsensor_write_cb_test_setup(fpsensor_data_t *fpsensor, bool invert)
{

    fpsensor_trace( "%s\n", __func__);
    switch (fpsensor->chip.type)
    {
        case FPSENSOR_CHIP_160_160_3:
            return fpsensor_write_cb_test_setup_160_3(fpsensor, invert);

        case FPSENSOR_CHIP_160_160_2:
            return fpsensor_write_cb_test_setup_160_2(fpsensor, invert);

        case FPSENSOR_CHIP_56_192_3:
            return fpsensor_write_cb_test_setup_56_3(fpsensor, invert);

        case FPSENSOR_CHIP_56_192_2:
            return fpsensor_write_cb_test_setup_56_2(fpsensor, invert);

        case FPSENSOR_CHIP_88_112_3:
            return fpsensor_write_cb_test_setup_88_3(fpsensor, invert);
        case FPSENSOR_CHIP_88_112_2:
            return fpsensor_write_cb_test_setup_88_2(fpsensor, invert);
        default:
            break;
    }

    return -EINVAL;


}


/* -------------------------------------------------------------------- */
bool fpsensor_capture_check_ready(fpsensor_data_t *fpsensor)
{
    fpsensor_capture_state_t state = fpsensor->capture.state;
//    fpsensor_trace( "%s, state: %d\n", __func__, state);

    return (state == FPSENSOR_CAPTURE_STATE_IDLE) ||
           (state == FPSENSOR_CAPTURE_STATE_COMPLETED) ||
           (state == FPSENSOR_CAPTURE_STATE_FAILED);
}

#define SAMPLE_RATIO   4//16  //4
/* -------------------------------------------------------------------- */
int fpsensor_capture_task(fpsensor_data_t *fpsensor)
{
    struct timespec ts_t1, ts_t2, ts_t3, ts_delta;
    int time_settings_us[FPSENSOR_BUFFER_MAX_IMAGES];
    int time_capture_us[FPSENSOR_BUFFER_MAX_IMAGES];
    int time_capture_sum_us;
    int error = 0;
    bool wait_finger_down = false;
    bool wait_finger_up = false;
    bool adjust_settings;
    bool gesture = false;
    fpsensor_capture_mode_t mode = fpsensor->capture.current_mode;
    int current_capture, capture_count;
    int image_offset;
    size_t image_byte_size;
    fpsensor_trace( "%s\n", __func__);

    fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_WRITE_SETTINGS;
    fpsensor_trace( "tiny capture enable:%d\n", fpsensor->setup.tiny_capture_enable);
    if (0 == fpsensor->setup.tiny_capture_enable)
    {
        error = fpsensor_wake_up(fpsensor);
    }
    if (error < 0)
    {
        goto out_error;
    }

    switch (mode)
    {
        case FPSENSOR_MODE_CAPTURE_AND_WAIT_FINGER_UP:
            wait_finger_up   = true;
            capture_count = fpsensor->setup.capture_count;
            adjust_settings = true;
            if (0 == fpsensor->setup.tiny_capture_enable)
            {
                error = fpsensor_write_capture_setup(fpsensor);
            }
            break;

        case FPSENSOR_MODE_WAIT_AND_CAPTURE:
            wait_finger_down =
                wait_finger_up   = true;

        case FPSENSOR_MODE_SINGLE_CAPTURE:
        case FPSENSOR_MODE_SINGLE_CAPTURE_CAL:
            capture_count = fpsensor->setup.capture_count;
            adjust_settings = true;
            if (0 == fpsensor->setup.tiny_capture_enable)
            {
                error = fpsensor_write_capture_setup(fpsensor);
            }
            break;

        case FPSENSOR_MODE_CHECKERBOARD_TEST_NORM:
            capture_count = 1;
            adjust_settings = true;
            error = fpsensor_write_cb_test_setup(fpsensor, false);
            break;

        case FPSENSOR_MODE_CHECKERBOARD_TEST_INV:
            capture_count = 1;
            adjust_settings = true;
            error = fpsensor_write_cb_test_setup(fpsensor, true);
            break;

        case FPSENSOR_MODE_BOARD_TEST_ONE:
            capture_count = 1;
            adjust_settings = true;
            error = fpsensor_write_test_setup(fpsensor, 0xffff);
            break;

        case FPSENSOR_MODE_BOARD_TEST_ZERO:
            capture_count = 1;
            adjust_settings = true;
            error = fpsensor_write_test_setup(fpsensor, 0x0000);
            break;

        case FPSENSOR_MODE_WAIT_FINGER_DOWN:
            wait_finger_down = true;
            capture_count = 0;
            adjust_settings = false;
            if (0 == fpsensor->setup.tiny_capture_enable)
            {
                error = fpsensor_write_capture_setup(fpsensor);
            }
            break;

        case FPSENSOR_MODE_WAIT_FINGER_UP:
            wait_finger_up = true;
            capture_count = 0;
            adjust_settings = false;
            if (0 == fpsensor->setup.tiny_capture_enable)
            {
                error = fpsensor_write_capture_setup(fpsensor);
            }
            break;
        case FPSENSOR_MODE_GESTURE:
            gesture = true ;
            wait_finger_up = 0 ;
            wait_finger_down = 0;
            capture_count = 1;
            adjust_settings = false;
            break;
        case FPSENSOR_MODE_IDLE:
        default:
            capture_count = 0;
            adjust_settings = false;
            error = -EINVAL;
            break;
    }


    if (error < 0)
    {
        goto out_error;
    }

    error = fpsensor_capture_set_crop(fpsensor,
                                      fpsensor->setup.capture_col_start,
                                      fpsensor->setup.capture_col_groups,
                                      fpsensor->setup.capture_row_start,
                                      fpsensor->setup.capture_row_count);
    if (error < 0)
    {
        goto out_error;
    }

    image_byte_size = fpsensor_calc_image_size(fpsensor);

    fpsensor_trace("Start capture, mode %d, (%d frames)\n",
                   mode,
                   capture_count);

    if (!wait_finger_down)
    {
        fpsensor->capture.deferred_finger_up = false;
    }

    if (wait_finger_down)
    {
        error = fpsensor_capture_finger_detect_settings(fpsensor);
        if (error < 0)
        {
            goto out_error;
        }
    }

    if (wait_finger_down && fpsensor->capture.deferred_finger_up)
    {
        fpsensor->capture.state =
            FPSENSOR_CAPTURE_STATE_WAIT_FOR_FINGER_UP;

        fpsensor_trace( "Waiting for (deferred) finger up\n");

        error = fpsensor_capture_wait_finger_up(fpsensor);

        if (error < 0)
        {
            goto out_error;
        }

        fpsensor_trace( "Finger up\n");

        fpsensor->capture.deferred_finger_up = false;
    }

    if (wait_finger_down)
    {
        fpsensor->capture.state =
            FPSENSOR_CAPTURE_STATE_WAIT_FOR_FINGER_DOWN;

        error = fpsensor_capture_wait_finger_down(fpsensor);

        if (error < 0)
        {
            goto out_error;
        }

        fpsensor_trace( "Finger down !!!!!!!\n");

        if (mode == FPSENSOR_MODE_WAIT_FINGER_DOWN)
        {

            fpsensor->capture.available_bytes = 4;
            fpsensor->huge_buffer[0] = 'F';
            fpsensor->huge_buffer[1] = ':';
            fpsensor->huge_buffer[2] = 'D';
            fpsensor->huge_buffer[3] = 'N';
        }
    }

    current_capture = 0;
    image_offset = 0;

    fpsensor->diag.last_capture_time = 0;

    if (mode == FPSENSOR_MODE_SINGLE_CAPTURE_CAL)
    {
        error = fpsensor_capture_set_sample_mode(fpsensor, true);
        if (error)
        {
            goto out_error;
        }
    }

    while (capture_count && (error >= 0))
    {
        getnstimeofday(&ts_t1);

        fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_ACQUIRE;

        fpsensor_trace("Capture, frame #%d \n", current_capture + 1);

        error = (!adjust_settings) ? 0 :
                fpsensor_capture_settings(fpsensor, current_capture);

        if (error < 0)
        {
            goto out_error;
        }
        if (true == gesture)
        {
            error = fpsensor_write_gesture_setup(fpsensor, SAMPLE_RATIO);
            if (error < 0)
            {
                goto out_error;
            }
            image_byte_size = image_byte_size / SAMPLE_RATIO;
        }

        getnstimeofday(&ts_t2);

        error = fpsensor_cmd(fpsensor,
                             FPSENSOR_CMD_CAPTURE_IMAGE,
                             FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA);

        if (error < 0)
        {
            goto out_error;
        }

        fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_FETCH;

        error = fpsensor_fetch_image(fpsensor,
                                     fpsensor->huge_buffer,
                                     image_offset,
                                     image_byte_size,
                                     (size_t)fpsensor->huge_buffer_size);
        if (error < 0)
        {
            goto out_error;
        }

        fpsensor->capture.available_bytes += (error >= 0) ?
                                             (int)image_byte_size : 0;
        fpsensor->capture.last_error = error;

        getnstimeofday(&ts_t3);

        ts_delta = timespec_sub(ts_t2, ts_t1);
        time_settings_us[current_capture] =
            ts_delta.tv_sec * USEC_PER_SEC +
            (ts_delta.tv_nsec / NSEC_PER_USEC);

        ts_delta = timespec_sub(ts_t3, ts_t2);
        time_capture_us[current_capture] =
            ts_delta.tv_sec * USEC_PER_SEC +
            (ts_delta.tv_nsec / NSEC_PER_USEC);

        capture_count--;
        current_capture++;
        image_offset += (int)image_byte_size;
    }

    error = fpsensor_capture_set_sample_mode(fpsensor, false);
    if (error)
    {
        goto out_error;
    }

    /* Update finger_present_status after image capture */
    fpsensor_check_finger_present_raw(fpsensor);

    if (mode != FPSENSOR_MODE_WAIT_FINGER_UP)
    {
        wake_up_interruptible(&fpsensor->capture.wq_data_avail);
    }

    if (wait_finger_up)
    {
        fpsensor->capture.state =
            FPSENSOR_CAPTURE_STATE_WAIT_FOR_FINGER_UP;

        error = fpsensor_capture_finger_detect_settings(fpsensor);
        if (error < 0)
        {
            goto out_error;
        }

        error = fpsensor_capture_wait_finger_up(fpsensor);

        if (error == -EINTR)
        {
            fpsensor_trace( "Finger up check interrupted\n");
            fpsensor->capture.deferred_finger_up = true;
            goto out_interrupted;

        }
        else if (error < 0)
        {
            goto out_error;
        }

        if (mode == FPSENSOR_MODE_WAIT_FINGER_UP)
        {
            fpsensor->capture.available_bytes = 4;
            fpsensor->huge_buffer[0] = 'F';
            fpsensor->huge_buffer[1] = ':';
            fpsensor->huge_buffer[2] = 'U';
            fpsensor->huge_buffer[3] = 'P';

            wake_up_interruptible(&fpsensor->capture.wq_data_avail);
        }

        fpsensor_trace( "Finger up\n");
    }

out_interrupted:

    capture_count = 0;
    time_capture_sum_us = 0;

    while (current_capture)
    {

        current_capture--;

        fpsensor_printk("Frame #%d acq. time %d+%d=%d (us)\n",
                        capture_count + 1,
                        time_settings_us[capture_count],
                        time_capture_us[capture_count],
                        time_settings_us[capture_count] +
                        time_capture_us[capture_count]);

        time_capture_sum_us += time_settings_us[capture_count];
        time_capture_sum_us += time_capture_us[capture_count];

        capture_count++;
    }
    fpsensor->diag.last_capture_time = time_capture_sum_us / 1000;

    fpsensor_trace("Total acq. time %d (us)\n", time_capture_sum_us);

out_error:
    fpsensor->capture.last_error = error;

    if (error)
    {
        fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_FAILED;
        if (error < 0)
        {
            fpsensor->capture.available_bytes = 4;
            fpsensor->huge_buffer[0] = 'E';
            fpsensor->huge_buffer[1] = 'R';
            fpsensor->huge_buffer[2] = 'R';
            fpsensor->huge_buffer[3] = 'O';
            wake_up_interruptible(&fpsensor->capture.wq_data_avail);
        }
        fpsensor_error( "%s %s %d\n", __func__,
                        (error == -EINTR) ? "TERMINATED" : "FAILED", error);
    }
    else
    {
        fpsensor->capture.state = FPSENSOR_CAPTURE_STATE_COMPLETED;
        fpsensor_trace( "%s OK\n", __func__);
    }
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_capture_wait_finger_down(fpsensor_data_t *fpsensor)
{
    int error;
    bool finger_down = false;
    fpsensor_trace( "%s\n", __func__);

    error = fpsensor_wait_finger_present(fpsensor);

    while (!finger_down && (error >= 0))
    {

        if (fpsensor->worker.stop_request)
        {
            error = -EINTR;
        }
        else
        {
            error = fpsensor_check_finger_present_sum(fpsensor);
        }

        if (error > fpsensor->setup.capture_finger_down_threshold)
        {
            finger_down = true;
        }
        else
        {
            error = fpsensor_wait_finger_present(fpsensor);
            // msleep(FPSENSOR_CAPTURE_WAIT_FINGER_DELAY_MS);
        }
    }

    fpsensor_read_irq(fpsensor, true);
    fpsensor_trace( "%s, end\n", __func__);
    fpsensor_get_finger_status_value(fpsensor, NULL);
    return (finger_down) ? 0 : error;
}


/* -------------------------------------------------------------------- */
int fpsensor_capture_wait_finger_up(fpsensor_data_t *fpsensor)
{
    int error = 0;
    bool finger_up = false;
    fpsensor_trace( "%s\n", __func__);

    while (!finger_up && (error >= 0))
    {

        if (fpsensor->worker.stop_request)
        {
            error = -EINTR;
        }
        else
        {
            error = fpsensor_check_finger_present_sum(fpsensor);
        }

        if ((error >= 0) && (error < fpsensor->setup.capture_finger_up_threshold + 1))
        {
            finger_up = true;
        }
        else
        {
            msleep(FPSENSOR_CAPTURE_WAIT_FINGER_DELAY_MS);
        }
    }

    fpsensor_read_irq(fpsensor, true);
    fpsensor_trace( "%s end\n", __func__);

    return (finger_up) ? 0 : error;
}


/* -------------------------------------------------------------------- */
int fpsensor_capture_settings(fpsensor_data_t *fpsensor, int select)
{
    int error = 0;
    fpsensor_reg_access_t reg;

    u16 pxlCtrl;
    u16 adc_shift_gain;

    //fpsensor_trace( "%s #%d\n", __func__, select);
    fpsensor_trace("select: %d, fpsensor->setup.adc_gain[select]: 0x%x\n", select,
                   fpsensor->setup.adc_gain[select]);
    fpsensor_trace("select: %d, fpsensor->setup.adc_shift[select]: 0x%x\n", select,
                   fpsensor->setup.adc_shift[select]);
    fpsensor_trace("select: %d, fpsensor->setup.pxl_ctrl[select]: 0x%x\n", select,
                   fpsensor->setup.pxl_ctrl[select]);
    fpsensor_trace("select: %d, fpsensor->setup.adc_et1[select]: 0x%x\n", select,
                   fpsensor->setup.adc_et1[select]);

    if (select >= FPSENSOR_MAX_ADC_SETTINGS)
    {
        error = -EINVAL;
        goto out_err;
    }

    pxlCtrl = fpsensor->setup.pxl_ctrl[select];
    pxlCtrl |= FPSENSOR_PXL_BIAS_CTRL;

    adc_shift_gain = fpsensor->setup.adc_shift[select];
    adc_shift_gain <<= 8;
    adc_shift_gain |= fpsensor->setup.adc_gain[select];

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_PXL_CTRL, &pxlCtrl);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out_err;
    }

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_ADC_SHIFT_GAIN, &adc_shift_gain);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out_err;
    }


out_err:
    if (error)
    {
        fpsensor_error( "%s Error %d\n", __func__, error);
    }

    return error;
}

int fpsensor_capture_set_sample_mode(fpsensor_data_t *fpsensor, bool single)
{
    int error = 0;
    fpsensor_reg_access_t reg;

    u8 image_setup;
    u8 image_rd;

    fpsensor_trace( "%s single %s\n", __func__, single ?
                    "true" : "false");

    if (single)
    {
        image_setup = 0x0A;
        image_rd = 0x0C;
    }
    else
    {
        image_setup = 0x03 | 0x08;
        image_rd = 0x0E;
    }

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMAGE_SETUP, &image_setup);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out_err;
    }

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMG_RD, &image_rd);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out_err;
    }

out_err:
    if (error)
    {
        fpsensor_error( "%s Error %d\n", __func__, error);
    }

    return error;
}

/* -------------------------------------------------------------------- */
int fpsensor_capture_finger_detect_settings(fpsensor_data_t *fpsensor)
{
    fpsensor_trace( "%s\n", __func__);

    return fpsensor_capture_settings(fpsensor, FPSENSOR_MAX_ADC_SETTINGS - 1);
}


/* -------------------------------------------------------------------- */
size_t fpsensor_calc_image_size(fpsensor_data_t *fpsensor)
{
    int image_byte_size = fpsensor->setup.capture_row_count *
                          fpsensor->setup.capture_col_groups *
                          fpsensor->chip.adc_group_size;

    fpsensor_trace( "%s Rows %d->%d,Cols %d->%d (%d bytes)\n",
                    __func__,
                    fpsensor->setup.capture_row_start,
                    fpsensor->setup.capture_row_start
                    + fpsensor->setup.capture_row_count - 1,
                    fpsensor->setup.capture_col_start
                    * fpsensor->chip.adc_group_size,
                    (fpsensor->setup.capture_col_start
                     * fpsensor->chip.adc_group_size)
                    + (fpsensor->setup.capture_col_groups *
                       fpsensor->chip.adc_group_size) - 1,
                    image_byte_size
                  );

    return image_byte_size;
}


/* -------------------------------------------------------------------- */
int fpsensor_capture_set_crop(fpsensor_data_t *fpsensor,
                              int first_column,
                              int num_columns,
                              int first_row,
                              int num_rows)
{
    fpsensor_reg_access_t reg;
    u32 temp_u32;

    temp_u32 = first_row;
    temp_u32 <<= 8;
    temp_u32 |= num_rows;
    temp_u32 <<= 8;
    temp_u32 |= (first_column * fpsensor->chip.adc_group_size);
    temp_u32 <<= 8;
    temp_u32 |= (num_columns * fpsensor->chip.adc_group_size);

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMG_CAPT_SIZE, &temp_u32);
    return fpsensor_reg_access(fpsensor, &reg);
}


/* -------------------------------------------------------------------- */
int fpsensor_capture_buffer(fpsensor_data_t *fpsensor,
                            u8 *data,
                            size_t offset,
                            size_t image_size_bytes)
{
    int error = 0;

    fpsensor_trace("%s, image_size_bytes: %d\n", __func__, (int)image_size_bytes);

    error = fpsensor_cmd(fpsensor,
                         FPSENSOR_CMD_CAPTURE_IMAGE,
                         FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA);

    if (error < 0)
    {
        fpsensor_error( "%s, error\n", __func__);
        goto out_error;
    }

    error = fpsensor_fetch_image(fpsensor,
                                 data,
                                 offset,
                                 image_size_bytes,
                                 (size_t)fpsensor->huge_buffer_size);
    if (error < 0)
    {
        goto out_error;
    }

    return 0;

out_error:
    fpsensor_trace( "%s FAILED %d\n", __func__, error);

    return error;
}


/* -------------------------------------------------------------------- */
extern int fpsensor_capture_deferred_task(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace( "%s\n", __func__);

    error = (fpsensor->capture.deferred_finger_up) ?
            fpsensor_capture_wait_finger_up(fpsensor) : 0;

    if (error >= 0)
    {
        fpsensor->capture.deferred_finger_up = false;
    }

    return error;
}


/* -------------------------------------------------------------------- */
