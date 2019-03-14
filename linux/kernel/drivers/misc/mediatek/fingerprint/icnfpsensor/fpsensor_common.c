/*++


 File Name:    fpsensor_common.c
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include "fpsensor_common.h"
#include "fpsensor_regs.h"
#include "fpsensor_capture.h"
#include "fpsensor_platform.h"


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
const bool target_little_endian = true;
#else
#warning BE target not tested!
const bool target_little_endian = false;
#endif


/* -------------------------------------------------------------------- */
/* fpsensor data types                           */
/* -------------------------------------------------------------------- */
struct chip_struct
{
    fpsensor_chip_t type;
    u16 hwid;
    u8  revision;
    u8  pixel_rows;
    u8  pixel_columns;
    u8  adc_group_size;
    u16 spi_max_khz;
};


/* -------------------------------------------------------------------- */
/* fpsensor driver constants                     */
/* -------------------------------------------------------------------- */
#define FPSENSOR56_ROWS        192u
#define FPSENSOR56_COLUMNS     56u

#define FPSENSOR160_ROWS        160u
#define FPSENSOR160_COLUMNS     160u

#define FPSENSOR88_ROWS      112u
#define FPSENSOR88_COLUMNS   88u
#define FPSENSOR_ROWS        192u
#define FPSENSOR_COLUMNS     192u
#define FPSENSOR_ADC_GROUP_SIZE  8u

#define FPSENSOR_EXT_HWID_CHECK_ID_ROWS 5u
#define HWID_160_160_3    0x7183
#define HWID_160_160_2    0x8210
#define HWID_56_192_3     0x140b
#define HWID_56_192_2     0x9400
#define HWID_88_112_3     0x7153
#define HWID_88_112_2     0xF150

static const char *chip_text[] =
{
    "N/A",      /* FPSENSOR_CHIP_NONE */
    "fpsensor",     /* FPSENSOR_CHIP_TEST */
    "fpsensor160_3",     /* FPSENSOR_CHIP_160_160_3 */
    "fpsensor160_2",     /* FPSENSOR_CHIP_160_160_2 */
    "fpsensor56_3",     /* FPSENSOR_CHIP_56_192_3 */
    "fpsensor56_2",     /* FPSENSOR_CHIP_56_192_2 */
    "fpsensor88_3",     /* FPSENSOR_CHIP_88_112_3*/
    "fpsensor88_2",     /* FPSENSOR_CHIP_88_112_2*/
};

static const struct chip_struct chip_data[] =
{
    {FPSENSOR_CHIP_160_160_3, HWID_160_160_3, 1, FPSENSOR160_ROWS, FPSENSOR160_COLUMNS, FPSENSOR_ADC_GROUP_SIZE, 8000},
    {FPSENSOR_CHIP_160_160_2, HWID_160_160_2, 1, FPSENSOR160_ROWS, FPSENSOR160_COLUMNS, FPSENSOR_ADC_GROUP_SIZE, 8000},
    {FPSENSOR_CHIP_56_192_3,  HWID_56_192_3, 1, FPSENSOR56_ROWS, FPSENSOR56_COLUMNS, FPSENSOR_ADC_GROUP_SIZE, 8000},
    {FPSENSOR_CHIP_56_192_2,  HWID_56_192_2, 1, FPSENSOR56_ROWS, FPSENSOR56_COLUMNS, FPSENSOR_ADC_GROUP_SIZE, 8000},
    {FPSENSOR_CHIP_88_112_3,  HWID_88_112_3, 1, FPSENSOR88_ROWS, FPSENSOR88_COLUMNS, FPSENSOR_ADC_GROUP_SIZE, 8000},
    {FPSENSOR_CHIP_88_112_2,  HWID_88_112_2, 1, FPSENSOR88_ROWS, FPSENSOR88_COLUMNS, FPSENSOR_ADC_GROUP_SIZE, 8000},
    {FPSENSOR_CHIP_NONE,  0,      0, 0,            0,               0,                      0}
};


const fpsensor_setup_t fpsensor_setup_default_160_3 =
{
    .adc_gain               = {2, 2, 2, 6},
    .adc_shift              = {10, 10, 10, 20},
    .pxl_ctrl               = {0x1e, 0x0e, 0x0a, 0x10},
    .adc_et1                = {3, 3, 3, 3},
    .capture_settings_mux   = 0,
    .capture_count          = 1,
    .capture_mode           = FPSENSOR_MODE_WAIT_AND_CAPTURE,
    .capture_row_start      = 0,
    .capture_row_count      = FPSENSOR160_ROWS,
    .capture_col_start      = 0,
    .capture_col_groups     = FPSENSOR160_COLUMNS / FPSENSOR_ADC_GROUP_SIZE,
    .capture_finger_up_threshold    = 0,
    .capture_finger_down_threshold  = 6,
    .finger_detect_threshold        = 0x50,
    .wakeup_detect_rows             = {76, 76},
    .wakeup_detect_cols             = {56, 88},
    .tiny_capture_enable    = 0,
    .sleep_dect             = 1,
};

const fpsensor_setup_t fpsensor_setup_default_160_2 =
{
    .adc_gain               = {2, 2, 2, 6},
    .adc_shift              = {10, 10, 10, 20},
    .pxl_ctrl               = {0x1e, 0x0e, 0x0a, 0x10},
    .adc_et1                = {3, 3, 3, 3},
    .capture_settings_mux   = 0,
    .capture_count          = 1,
    .capture_mode           = FPSENSOR_MODE_WAIT_AND_CAPTURE,
    .capture_row_start      = 0,
    .capture_row_count      = FPSENSOR160_ROWS,
    .capture_col_start      = 0,
    .capture_col_groups     = FPSENSOR160_COLUMNS / FPSENSOR_ADC_GROUP_SIZE,
    .capture_finger_up_threshold    = 0,
    .capture_finger_down_threshold  = 6,
    .finger_detect_threshold        = 0x50,
    .wakeup_detect_rows             = {76, 76},
    .wakeup_detect_cols             = {56, 88},
    .tiny_capture_enable    = 0,
    .sleep_dect             = 1,
};

const fpsensor_setup_t fpsensor_setup_default_56_3 =
{
    .adc_gain               = {2, 2, 2, 2},
    .adc_shift              = {10, 10, 10, 10},
    .pxl_ctrl               = {0x1e, 0x0e, 0x0a, 0x0a},
    .adc_et1                = {3, 3, 3, 3},
    .capture_settings_mux   = 0,
    .capture_count          = 1,
    .capture_mode           = FPSENSOR_MODE_WAIT_AND_CAPTURE,
    .capture_row_start      = 0,
    .capture_row_count      = FPSENSOR56_ROWS,
    .capture_col_start      = 0,
    .capture_col_groups     = FPSENSOR56_COLUMNS / FPSENSOR_ADC_GROUP_SIZE,
    .capture_finger_up_threshold    = 0,
    .capture_finger_down_threshold  = 7,
    .finger_detect_threshold        = 0x50,
    .wakeup_detect_rows             = {66, 118},
    .wakeup_detect_cols             = {24, 24},
    .tiny_capture_enable    = 0,
    .sleep_dect             = 1,
};

const fpsensor_setup_t fpsensor_setup_default_56_2 =
{
    .adc_gain               = {2, 2, 2, 2},
    .adc_shift              = {10, 10, 10, 10},
    .pxl_ctrl               = {0x1e, 0x0e, 0x0a, 0x0a},
    .adc_et1                = {3, 3, 3, 3},
    .capture_settings_mux   = 0,
    .capture_count          = 1,
    .capture_mode           = FPSENSOR_MODE_WAIT_AND_CAPTURE,
    .capture_row_start      = 0,
    .capture_row_count      = FPSENSOR56_ROWS,
    .capture_col_start      = 0,
    .capture_col_groups     = FPSENSOR56_COLUMNS / FPSENSOR_ADC_GROUP_SIZE,
    .capture_finger_up_threshold    = 0,
    .capture_finger_down_threshold  = 7,
    .finger_detect_threshold        = 0x50,
    .wakeup_detect_rows             = {66, 118},
    .wakeup_detect_cols             = {24, 24},
    .tiny_capture_enable    = 0,
    .sleep_dect             = 1,
};

const fpsensor_setup_t fpsensor_setup_default_88_3 =
{
    .adc_gain               = {2, 2, 2, 6},
    .adc_shift              = {10, 10, 10, 20},
    .pxl_ctrl               = {0x1e, 0x0e, 0x0a, 0x10},
    .adc_et1                = {3, 3, 3, 3},
    .capture_settings_mux   = 0,
    .capture_count          = 1,
    .capture_mode           = FPSENSOR_MODE_WAIT_AND_CAPTURE,
    .capture_row_start      = 0,
    .capture_row_count      = FPSENSOR88_ROWS,
    .capture_col_start      = 0,
    .capture_col_groups     = FPSENSOR88_COLUMNS / FPSENSOR_ADC_GROUP_SIZE,
    .capture_finger_up_threshold    = 0,
    .capture_finger_down_threshold  = 7,
    .finger_detect_threshold        = 0x50,
    .wakeup_detect_rows             = {40, 56},
    .wakeup_detect_cols             = {32, 48},
    .tiny_capture_enable    = 0,
    .sleep_dect             = 1,
};

const fpsensor_setup_t fpsensor_setup_default_88_2 =
{
    .adc_gain               = {2, 2, 2, 6},
    .adc_shift              = {10, 10, 10, 20},
    .pxl_ctrl               = {0x1e, 0x0e, 0x0a, 0x10},
    .adc_et1                = {3, 3, 3, 3},
    .capture_settings_mux   = 0,
    .capture_count          = 1,
    .capture_mode           = FPSENSOR_MODE_WAIT_AND_CAPTURE,
    .capture_row_start      = 0,
    .capture_row_count      = FPSENSOR88_ROWS,
    .capture_col_start      = 0,
    .capture_col_groups     = FPSENSOR88_COLUMNS / FPSENSOR_ADC_GROUP_SIZE,
    .capture_finger_up_threshold    = 0,
    .capture_finger_down_threshold  = 7,
    .finger_detect_threshold        = 0x50,
    .wakeup_detect_rows             = {40, 56},
    .wakeup_detect_cols             = {32, 48},
    .tiny_capture_enable    = 0,
    .sleep_dect             = 1,
};


const fpsensor_diag_t fpsensor_diag_default =
{
    .selftest     = 0,
    .spi_register = 0,
    .spi_regsize  = 0,
    .spi_data     = 0,
};


/* -------------------------------------------------------------------- */
/* function prototypes                          */
/* -------------------------------------------------------------------- */

static int fpsensor_write_sensor_160_3_setup(fpsensor_data_t *fpsensor);
static int fpsensor_write_sensor_160_2_setup(fpsensor_data_t *fpsensor);
static int fpsensor_write_sensor_56_3_setup(fpsensor_data_t *fpsensor);
static int fpsensor_write_sensor_56_2_setup(fpsensor_data_t *fpsensor);
static int fpsensor_write_sensor_88_3_setup(fpsensor_data_t *fpsensor);
static int fpsensor_write_sensor_88_2_setup(fpsensor_data_t *fpsensor);
static int fpsensor_check_irq_after_reset(fpsensor_data_t *fpsensor);

/* -------------------------------------------------------------------- */
/* function definitions                         */
/* -------------------------------------------------------------------- */
size_t fpsensor_calc_huge_buffer_minsize(fpsensor_data_t *fpsensor)
{
    const size_t buff_min = FPSENSOR_EXT_HWID_CHECK_ID_ROWS *
                            FPSENSOR_COLUMNS;
    size_t buff_req;

    buff_req = (fpsensor->chip.type == FPSENSOR_CHIP_NONE) ? buff_min :
               (fpsensor->chip.pixel_columns *
                fpsensor->chip.pixel_rows *
                FPSENSOR_BUFFER_MAX_IMAGES);

    return (buff_req > buff_min) ? buff_req : buff_min;
}


/* -------------------------------------------------------------------- */
int fpsensor_manage_huge_buffer(fpsensor_data_t *fpsensor, size_t new_size)
{
    int error = 0;
    int buffer_order_new, buffer_order_curr;

    buffer_order_curr = get_order(fpsensor->huge_buffer_size);
    buffer_order_new  = get_order(new_size);

    if (new_size == 0)
    {
        if (fpsensor->huge_buffer)
        {
            free_pages((unsigned long)fpsensor->huge_buffer,
                       buffer_order_curr);

            fpsensor->huge_buffer = NULL;
        }
        fpsensor->huge_buffer_size = 0;
        error = 0;

    }
    else
    {
        if (fpsensor->huge_buffer &&
            (buffer_order_curr != buffer_order_new))
        {

            free_pages((unsigned long)fpsensor->huge_buffer,
                       buffer_order_curr);

            fpsensor->huge_buffer = NULL;
        }

        if (fpsensor->huge_buffer == NULL)
        {
            fpsensor->huge_buffer =
                (u8 *)__get_free_pages(GFP_KERNEL,
                                       buffer_order_new);

            fpsensor->huge_buffer_size = (fpsensor->huge_buffer) ?
                                         (size_t)PAGE_SIZE << buffer_order_new : 0;

            error = (fpsensor->huge_buffer_size == 0) ? -ENOMEM : 0;
        }
    }


    if (error)
    {
        fpsensor_error( "%s, failed %d\n",
                        __func__, error);
    }
    else
    {
        fpsensor_trace( "%s, size=%d bytes\n",
                        __func__, (int)fpsensor->huge_buffer_size);
    }

    return error;
}
/*--------------------------------------------------------------------- */
static int fpsensor_remap(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 temp_u8 ;
    fpsensor_reg_access_t reg;
    temp_u8 = 0x55;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_ADDRESS_REMAP, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_setup_defaults(fpsensor_data_t *fpsensor)
{
    int error = 0;
    const fpsensor_setup_t *ptr;

    memcpy((void *)&fpsensor->diag,
           (void *)&fpsensor_diag_default,
           sizeof(fpsensor_diag_t));

    switch (fpsensor->chip.type)
    {

        case FPSENSOR_CHIP_160_160_3:
            ptr = &fpsensor_setup_default_160_3;
            break;

        case FPSENSOR_CHIP_160_160_2:
            ptr = &fpsensor_setup_default_160_2;
            break;
        case FPSENSOR_CHIP_56_192_3:
            ptr = &fpsensor_setup_default_56_3;
            break;

        case FPSENSOR_CHIP_56_192_2:
            ptr = &fpsensor_setup_default_56_2;
            break;
        case FPSENSOR_CHIP_88_112_3:
            ptr = &fpsensor_setup_default_88_3;
            break;
        case FPSENSOR_CHIP_88_112_2:
            ptr = &fpsensor_setup_default_88_2;
            break;
        default:
            ptr = NULL;
            break;
    }

    error = (ptr == NULL) ? -EINVAL : 0;
    if (error)
    {
        goto out_err;
    }

    memcpy((void *)&fpsensor->setup, ptr, sizeof(fpsensor_setup_t));

    fpsensor_trace( "%s OK\n", __func__);

    return 0;

out_err:
    memset((void *)&fpsensor->setup, 0, sizeof(fpsensor_setup_t));
    fpsensor_error( "%s FAILED %d\n", __func__, error);

    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_gpio_reset(fpsensor_data_t *fpsensor)
{
    int error = 0;
    int counter = FPSENSOR_RESET_RETRIES;

    #if FPSENSOR_MANUAL_CS
    fpsensor_gpio_wirte(fpsensor->cs_gpio, 1);
    #endif
    while (counter)
    {
        counter--;

        fpsensor_gpio_wirte(fpsensor->reset_gpio, 1);
        udelay(FPSENSOR_RESET_HIGH1_US);

        fpsensor_gpio_wirte(fpsensor->reset_gpio, 0);
        udelay(FPSENSOR_RESET_LOW_US);

        fpsensor_gpio_wirte(fpsensor->reset_gpio, 1);
        udelay(FPSENSOR_RESET_HIGH2_US);

        error = fpsensor_gpio_read(fpsensor->irq_gpio) ? 0 : -EIO;

        if (!error)
        {
            fpsensor_trace(  "%s OK !\n", __func__);
            counter = 0;
        }
        else
        {

            fpsensor_trace("%s timed out,retrying ...\n", __func__);
            udelay(1250);
        }
    }
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_spi_reset(fpsensor_data_t *fpsensor)
{
    int error = 0;
    int counter = FPSENSOR_RESET_RETRIES;

    fpsensor_trace( "%s\n", __func__);
    #if FPSENSOR_MANUAL_CS
    fpsensor_gpio_wirte(fpsensor->cs_gpio, 1);
    #endif

    while (counter)
    {
        counter--;

        error = fpsensor_cmd(fpsensor,
                             FPSENSOR_CMD_SOFT_RESET,
                             false);
        /*
                if (error >= 0) {
                    error = fpsensor_wait_for_irq(fpsensor,
                            FPSENSOR_DEFAULT_IRQ_TIMEOUT_MS);
                }
        */
        udelay(FPSENSOR_RESET_HIGH2_US);
        if (error >= 0)
        {
            error = fpsensor_gpio_read(fpsensor->irq_gpio) ? 0 : -EIO;

            if (!error)
            {
                fpsensor_trace("%s OK !\n", __func__);

                counter = 0;

            }
            else
            {
                fpsensor_trace("%s timed out,retrying ...\n",  __func__);
            }
        }
    }
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_reset(fpsensor_data_t *fpsensor)
{
    int error = 0;

    fpsensor_trace( "%s\n", __func__);

    fpsensor_irq_disable(fpsensor);

    plat_power(0);
    msleep(10);
    plat_power(1);
    msleep(10);

    error = (fpsensor->soft_reset_enabled) ?
            fpsensor_spi_reset(fpsensor) :
            fpsensor_gpio_reset(fpsensor);

    fpsensor->interrupt_done = false;

    fpsensor_irq_enable(fpsensor);

    error = fpsensor_check_irq_after_reset(fpsensor);

    if (error < 0)
    {
        goto out;
    }

    error = (fpsensor_gpio_read(fpsensor->irq_gpio) != 0) ? -EIO : 0;

    if (error)
    {
        fpsensor_error( "IRQ pin, not low after clear.\n");
    }

    error = fpsensor_read_irq(fpsensor, true);

    if (error != 0)
    {
        fpsensor_error(
            "IRQ register, expected 0x%x, got 0x%x.\n",
            0,
            (u8)error);

        error = -EIO;
    }
    error = fpsensor_remap(fpsensor);
    if (error)
    {
        goto out;
    }
    fpsensor->capture.available_bytes = 0;
    fpsensor->capture.read_offset = 0;
    fpsensor->capture.read_pending_eof = false;
out:
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_check_hw_id(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u16 hardware_id;
    u16 version_id;
    fpsensor_reg_access_t reg;
    int counter = 0;
    bool match = false;

    FPSENSOR_MK_REG_READ(reg, FPSENSOR_REG_HWID, &hardware_id);
    error = fpsensor_reg_access(fpsensor, &reg);

    if (error)
    {
        return error;
    }

    if (fpsensor->force_hwid > 0)
    {
        fpsensor_printk("fpsensor Hardware id, detected 0x%x - forced setting 0x%x\n", hardware_id,
                        fpsensor->force_hwid);
        hardware_id = fpsensor->force_hwid;
    }
    else if (fpsensor->without_bezel)
    {
        fpsensor_printk( "Modify hwid from 0x%x\n", hardware_id);
        hardware_id |= 0x8000;  // Mark it as none bezel
        hardware_id &= 0xFFF0;  // Ignore manufacture
    }

    while (!match && chip_data[counter].type != FPSENSOR_CHIP_NONE)
    {
        if (chip_data[counter].hwid == hardware_id)
        {
            match = true;
        }
        else
        {
            counter++;
        }
    }

    if (match)
    {
        fpsensor->chip.type     = chip_data[counter].type;
        //fpsensor->chip.revision = chip_data[counter].revision;
        fpsensor->chip.hwid     = chip_data[counter].hwid;
        fpsensor->chip.pixel_rows     = chip_data[counter].pixel_rows;
        fpsensor->chip.pixel_columns  = chip_data[counter].pixel_columns;
        fpsensor->chip.adc_group_size = chip_data[counter].adc_group_size;
        fpsensor->chip.spi_max_khz    = chip_data[counter].spi_max_khz;


        FPSENSOR_MK_REG_READ(reg, FPSENSOR_REG_VID, &version_id);
        error = fpsensor_reg_access(fpsensor, &reg);
        if (error)
        {
            return error;
        }
        fpsensor->chip.sensorid = (u8)(version_id >> 8) & 0xff;
        fpsensor->chip.revision = (u8)(version_id & 0xff);

        fpsensor_printk( "version_id 0x%x\n", version_id);

        fpsensor_printk( "Hardware id: 0x%x (%s, rev.%d) \n",
                         hardware_id,
                         chip_text[fpsensor->chip.type],
                         fpsensor->chip.revision);
    }
    else
    {
        fpsensor_error("Hardware id mismatch: got 0x%x\n", hardware_id);

        fpsensor->chip.type = FPSENSOR_CHIP_NONE;
        fpsensor->chip.revision = 0;

        return -EIO;
    }

    return error;
}


/* -------------------------------------------------------------------- */
const char *fpsensor_hw_id_text(fpsensor_data_t *fpsensor)
{
    return chip_text[fpsensor->chip.type];
}


/* -------------------------------------------------------------------- */
int fpsensor_write_sensor_setup(fpsensor_data_t *fpsensor)
{
    fpsensor_trace( "%s\n", __func__);
    switch (fpsensor->chip.type)
    {
        case FPSENSOR_CHIP_160_160_3:
            return fpsensor_write_sensor_160_3_setup(fpsensor);

        case FPSENSOR_CHIP_160_160_2:
            return fpsensor_write_sensor_160_2_setup(fpsensor);

        case FPSENSOR_CHIP_56_192_3:
            return fpsensor_write_sensor_56_3_setup(fpsensor);

        case FPSENSOR_CHIP_56_192_2:
            return fpsensor_write_sensor_56_2_setup(fpsensor);
        case FPSENSOR_CHIP_88_112_3:
            return fpsensor_write_sensor_88_3_setup(fpsensor);
        case FPSENSOR_CHIP_88_112_2:
            return fpsensor_write_sensor_88_2_setup(fpsensor);

        default:
            break;
    }

    return -EINVAL;
}


/* -------------------------------------------------------------------- */
static int fpsensor_write_sensor_160_3_setup(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;
    const int mux = FPSENSOR_MAX_ADC_SETTINGS - 1;

    fpsensor_trace( "%s %d\n", __func__, mux);


    temp_u8 = (fpsensor->vddtx_mv > 0) ? 0x02 :  /* external supply */
              (fpsensor->txout_boost) ? 0x22 : 0x12;   /* internal supply */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
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

    temp_u8 = 0x03 | 0x08;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMAGE_SETUP, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = fpsensor->setup.finger_detect_threshold;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_THRES, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xFF06;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_CNTR, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

/* -------------------------------------------------------------------- */
static int fpsensor_write_sensor_160_2_setup(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;
    const int mux = FPSENSOR_MAX_ADC_SETTINGS - 1;

    fpsensor_trace( "%s %d, fpsensor->setup.finger_detect_threshold: %d\n", __func__, mux,
                    fpsensor->setup.finger_detect_threshold);

    if (fpsensor->vddtx_mv > 0)
    {
        fpsensor_error( "%s Ignoring external TxOut setting\n", __func__);
    }

    if (fpsensor->txout_boost)
    {
        fpsensor_error( "%s Ignoring TxOut boost setting\n", __func__);
    }

    temp_u8 = 0x12; /* internal supply, no boost */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
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

    temp_u8 = 0x03 | 0x08;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMAGE_SETUP, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = fpsensor->setup.finger_detect_threshold;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_THRES, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xFF06;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_CNTR, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_write_sensor_56_3_setup(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    u32 temp_u32;
    fpsensor_reg_access_t reg;
    const int mux = FPSENSOR_MAX_ADC_SETTINGS - 1;

    fpsensor_trace( "%s %d\n", __func__, mux);


    temp_u8 = (fpsensor->vddtx_mv > 0) ? 0x02 :  /* external supply */
              (fpsensor->txout_boost) ? 0x22 : 0x12;   /* internal supply */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
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

    temp_u8 = 0x03 | 0x08;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMAGE_SETUP, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u32 = 0x0001; /* fngrUpSteps */
    temp_u32 <<= 8;
    temp_u32 |= fpsensor->setup.finger_detect_threshold; /* fngrLstThr */
    temp_u32 <<= 8;
    temp_u32 |= fpsensor->setup.finger_detect_threshold; /* fngrDetThr */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR1_REG_FNGR_DET_THRES, &temp_u32);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u32 = 0x19010006;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR1_REG_FNGR_DET_CNTR, &temp_u32);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}


/* -------------------------------------------------------------------- */
static int fpsensor_write_sensor_56_2_setup(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    u32 temp_u32;
    fpsensor_reg_access_t reg;
    const int mux = FPSENSOR_MAX_ADC_SETTINGS - 1;

    fpsensor_trace( "%s %d\n", __func__, mux);

    if (fpsensor->vddtx_mv > 0)
    {
        fpsensor_error( "%s Ignoring external TxOut setting\n", __func__);
    }

    if (fpsensor->txout_boost)
    {
        fpsensor_error( "%s Ignoring TxOut boost setting\n", __func__);
    }

    temp_u8 = 0x12; /* internal supply, no boost */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
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

    temp_u8 = 0x03 | 0x08;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMAGE_SETUP, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u32 = 0x0001; /* fngrUpSteps */
    temp_u32 <<= 8;
    temp_u32 |= fpsensor->setup.finger_detect_threshold; /* fngrLstThr */
    temp_u32 <<= 8;
    temp_u32 |= fpsensor->setup.finger_detect_threshold; /* fngrDetThr */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR1_REG_FNGR_DET_THRES, &temp_u32);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u32 = 0x1901ffff;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR1_REG_FNGR_DET_CNTR, &temp_u32);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

/* -------------------------------------------------------------------- */
static int fpsensor_write_sensor_88_3_setup(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;

    fpsensor_reg_access_t reg;
    const int mux = FPSENSOR_MAX_ADC_SETTINGS - 1;

    fpsensor_trace( "%s %d\n", __func__, mux);


    temp_u8 = (fpsensor->vddtx_mv > 0) ? 0x02 :  /* external supply */
              (fpsensor->txout_boost) ? 0x22 : 0x12;   /* internal supply */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
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

    temp_u8 = 0x03 | 0x08;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMAGE_SETUP, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = fpsensor->setup.finger_detect_threshold;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_THRES, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xFF06;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_CNTR, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

/* -------------------------------------------------------------------- */
static int fpsensor_write_sensor_88_2_setup(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 temp_u8;
    u16 temp_u16;
    fpsensor_reg_access_t reg;
    const int mux = FPSENSOR_MAX_ADC_SETTINGS - 1;

    fpsensor_trace( "%s %d\n", __func__, mux);


    if (fpsensor->vddtx_mv > 0)
    {
        fpsensor_error( "%s Ignoring external TxOut setting\n", __func__);
    }

    if (fpsensor->txout_boost)
    {
        fpsensor_error( "%s Ignoring TxOut boost setting\n", __func__);
    }

    temp_u8 = 0x12; /* internal supply, no boost */
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
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

    temp_u8 = 0x03 | 0x08;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_IMAGE_SETUP, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u8 = fpsensor->setup.finger_detect_threshold;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_THRES, &temp_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    temp_u16 = 0xFF06;
    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FNGR_DET_CNTR, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

out:
    return error;
}

/* -------------------------------------------------------------------- */
static int fpsensor_check_irq_after_reset(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 irq_status;
    u8 i;

    fpsensor_reg_access_t reg_clear =
    {
        .reg = FPSENSOR_REG_READ_INTERRUPT_WITH_CLEAR,
        .write = false,
        .reg_size = FPSENSOR_REG_SIZE(
            FPSENSOR_REG_READ_INTERRUPT_WITH_CLEAR),
        .dataptr = &irq_status
    };

    for (i = 0; i < 2; i++)
    {
        error = fpsensor_reg_access(fpsensor, &reg_clear);
        if (irq_status == FPSENSOR_IRQ_REG_BITS_REBOOT)
        {
            break;
        }
    }
    if (i == 1)
    {
        fpsensor_error("Warring!!!, Maybe the CS pin is pull down when Chip reset!!!");
    }
    if (error < 0)
    {
        return error;
    }

    if (irq_status != FPSENSOR_IRQ_REG_BITS_REBOOT)
    {
        fpsensor_error("fpsensor IRQ register, expected 0x%x, got 0x%x.\n",
                       FPSENSOR_IRQ_REG_BITS_REBOOT,
                       irq_status);

        error = -EIO;
    }

    return (error < 0) ? error : irq_status;
}


/* -------------------------------------------------------------------- */
int fpsensor_wait_for_irq(fpsensor_data_t *fpsensor, int timeout)
{
    int result = 0;
//    fpsensor_trace( "%s %d\n", __func__, timeout);

    if (!timeout)
    {
        result = wait_event_interruptible(
                     fpsensor->wq_irq_return,
                     fpsensor->interrupt_done);
    }
    else
    {
        result = wait_event_interruptible_timeout(
                     fpsensor->wq_irq_return,
                     fpsensor->interrupt_done, timeout);
    }

    if (result < 0)
    {
        fpsensor_error("wait_event_interruptible interrupted by signal.\n");

        return result;
    }

    if (result || !timeout)
    {

        if ((fpsensor->interrupt_done == true) && (fpsensor_read_irq(fpsensor, false) > 0))
        {
            fpsensor->interrupt_done = false;
            return 0;
        }
        else
        {
            fpsensor->interrupt_done = false;
            return -ETIMEDOUT;
        }
    }

    return -ETIMEDOUT;
}


/* -------------------------------------------------------------------- */
int fpsensor_read_irq(fpsensor_data_t *fpsensor, bool clear_irq)
{
    int error = 0;
    u8 irq_status;
    fpsensor_reg_access_t reg_read =
    {
        .reg = FPSENSOR_REG_READ_INTERRUPT,
        .write = false,
        .reg_size = FPSENSOR_REG_SIZE(FPSENSOR_REG_READ_INTERRUPT),
        .dataptr = &irq_status
    };

    fpsensor_reg_access_t reg_clear =
    {
        .reg = FPSENSOR_REG_READ_INTERRUPT_WITH_CLEAR,
        .write = false,
        .reg_size = FPSENSOR_REG_SIZE(
            FPSENSOR_REG_READ_INTERRUPT_WITH_CLEAR),
        .dataptr = &irq_status
    };

    error = fpsensor_reg_access(fpsensor,
                                (clear_irq) ? &reg_clear : &reg_read);

    if (error < 0)
    {
        return error;
    }

    if (irq_status == FPSENSOR_IRQ_REG_BITS_REBOOT)
    {

        fpsensor_error("%s: unexpected irq_status = 0x%x\n"
                       , __func__, irq_status);

        error = -EIO;
    }

    return (error < 0) ? error : irq_status;
}


/* -------------------------------------------------------------------- */
int fpsensor_read_status_reg(fpsensor_data_t *fpsensor)
{
    int error = 0;
    u8 status;
    /* const */ fpsensor_reg_access_t reg_read =
    {
        .reg = FPSENSOR_REG_STATUS,
        .write = false,
        .reg_size = FPSENSOR_REG_SIZE(FPSENSOR_REG_STATUS),
        .dataptr = &status
    };

    error = fpsensor_reg_access(fpsensor, &reg_read);

    return (error < 0) ? error : status;
}

int fpsensor_get_finger_status_value(fpsensor_data_t *fpsensor , u8 *buffer_u8)
{
    int error = 0 ;
    int i = 0 ;
    u8 temp_buffer_u8[12];
    fpsensor_reg_access_t reg;
    FPSENSOR_MK_REG_READ(reg, FPSENSOR_REG_FNGR_DET_VAL, temp_buffer_u8);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (buffer_u8 != NULL)
    {
        memcpy(buffer_u8, temp_buffer_u8, 12);
        for ( i = 0; i < 12; ++i)
        {
            printk("  %d  ", buffer_u8[i]);
        }
        printk("\n");
    }
    return error;
}

/* -------------------------------------------------------------------- */
int fpsensor_wait_finger_present(fpsensor_data_t *fpsensor)
{
    int error = 0;
    const fpsensor_status_reg_t status_mask = FPSENSOR_STATUS_REG_MODE_MASK;
    bool sleep_ok;
    int retries = FPSENSOR_SLEEP_RETRIES;

    fpsensor_trace( "%s\n", __func__);
    fpsensor_get_finger_status_value(fpsensor, NULL);
    error = fpsensor_read_irq(fpsensor, true);
    if (error < 0)
    {
        return error;
    }
    if (fpsensor->setup.sleep_dect == 0)
    {
        error = fpsensor_cmd(fpsensor,
                             FPSENSOR_CMD_WAIT_FOR_FINGER_PRESENT, 0);
        if (error < 0)
        {
            return error;
        }
    }
    else
    {
        error = fpsensor_cmd(fpsensor, FPSENSOR_CMD_ACTIVATE_SLEEP_MODE, 0);
        if (error)
        {
            fpsensor_trace(
                "%s command failed %d\n", __func__, error);

            return error;
        }

        error = 0;
        sleep_ok = false;
        while (!sleep_ok && retries && (error >= 0))
        {

            error = fpsensor_read_status_reg(fpsensor);

            if (error < 0)
            {
                fpsensor_trace(
                    "%s read status failed %d\n", __func__, error);
            }
            else
            {
                error &= status_mask;
                sleep_ok = (error == FPSENSOR_STATUS_REG_IN_SLEEP_MODE);
            }
            if (!sleep_ok)
            {
                udelay(FPSENSOR_SLEEP_RETRY_TIME_US);
                retries--;
            }
        }
    }
//    sleep_ok = true;

    while (1)
    {
        error = fpsensor_wait_for_irq(fpsensor,
                                      FPSENSOR_DEFAULT_IRQ_TIMEOUT_MS);
        //fpsensor_trace("%s fpsensor_wait_for_irq %d\n", __func__, error);

        if (error >= 0)
        {
            error = fpsensor_read_irq(fpsensor, true);
            if (error < 0)
            {
                return error;
            }

            if (error & FPSENSOR_IRQ_REG_BIT_FINGER_DOWN)
            {

                fpsensor_trace( "Finger down\n");

                error = 0;
            }
            else
            {
                fpsensor_error("%s Unexpected IRQ = %d\n", __func__,
                               error);
                #if FPSENSOR_STILL_WAIT_INVALID_IRQ
                if (fpsensor->worker.stop_request)
                {
                    return -EINTR;
                }

                continue;
                #else
                error = -EIO;
                #endif
            }
            return error;
        }

        if (error < 0)
        {
            if (fpsensor->worker.stop_request)
            {
                return -EINTR;
            }
            if (error != -ETIMEDOUT)
            {
                return error;
            }
        }
    }

}

/* -------------------------------------------------------------------- */
int fpsensor_get_finger_present_status(fpsensor_data_t *fpsensor)
{
    int status;

    if (!down_trylock(&fpsensor->mutex))
    {
        if (!down_trylock(&fpsensor->worker.sem_idle))
        {
            status = fpsensor_capture_finger_detect_settings(fpsensor);
            status = fpsensor_check_finger_present_raw(fpsensor);

            up(&fpsensor->worker.sem_idle);
        }
        else
        {
            /* Return last recorded status */
            status = (int)fpsensor->diag.finger_present_status;
            fpsensor_trace( "%s, return last status 1: 0x%x\n", __func__, status);
        }
        up(&fpsensor->mutex);
    }
    else
    {
        /* Return last recorded status */
        status = (int)fpsensor->diag.finger_present_status;
        fpsensor_trace( "%s, return last status 2: 0x%x\n", __func__, status);
    }
    return status;
}

/* -------------------------------------------------------------------- */
int fpsensor_check_finger_present_raw(fpsensor_data_t *fpsensor)
{
    fpsensor_reg_access_t reg;
    u16 temp_u16;
    int error = 0;
    fpsensor_trace( "%s\n", __func__);

    error = fpsensor_read_irq(fpsensor, true);
    if (error < 0)
    {
        return error;
    }

    error = fpsensor_cmd(fpsensor,
                         FPSENSOR_CMD_FINGER_PRESENT_QUERY,
                         FPSENSOR_IRQ_REG_BIT_COMMAND_DONE);

    if (error < 0)
    {
        return error;
    }

    FPSENSOR_MK_REG_READ(reg, FPSENSOR_REG_FINGER_PRESENT_STATUS, &temp_u16);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        return error;
    }

    fpsensor->diag.finger_present_status = temp_u16;

    fpsensor_trace( "%s zonedata = 0x%x\n", __func__, temp_u16);
    fpsensor_get_finger_status_value(fpsensor, NULL);
    return temp_u16;
}


/* -------------------------------------------------------------------- */
int fpsensor_check_finger_present_sum(fpsensor_data_t *fpsensor)
{
    int zones = 0;
    u16 mask = FPSENSOR_FINGER_DETECT_ZONE_MASK;
    u8 count = 0;
    fpsensor_trace( "%s\n", __func__);

    zones = fpsensor_check_finger_present_raw(fpsensor);

    if (zones < 0)
    {
        return zones;
    }
    else
    {
        zones &= mask;
        while (zones && mask)
        {
            count += (zones & 1) ? 1 : 0;
            zones >>= 1;
            mask >>= 1;
        }
        // fpsensor_trace( "%s %d zones\n", __func__, count);
        return (int)count;
    }
}


/* -------------------------------------------------------------------- */
int fpsensor_wake_up(fpsensor_data_t *fpsensor)
{
    const fpsensor_status_reg_t status_mask = FPSENSOR_STATUS_REG_MODE_MASK;
    int reset  = fpsensor_reset(fpsensor);
    int status = fpsensor_read_status_reg(fpsensor);

    if (reset == 0 && status >= 0 &&
        (fpsensor_status_reg_t)(status & status_mask) ==
        FPSENSOR_STATUS_REG_IN_IDLE_MODE)
    {

        fpsensor_trace( "%s OK\n", __func__);

        return 0;
    }
    else
    {

        fpsensor_error( "%s FAILED\n", __func__);

        return -EIO;
    }
}


/* -------------------------------------------------------------------- */
int fpsensor_sleep(fpsensor_data_t *fpsensor, bool deep_sleep)
{
    const char *str_deep = "deep";
    const char *str_regular = "regular";
    int error;
    int retries = FPSENSOR_SLEEP_RETRIES;
    bool sleep_ok;

    const fpsensor_status_reg_t status_mask = FPSENSOR_STATUS_REG_MODE_MASK;

    error = fpsensor_wake_up(fpsensor);
    if (error < 0)
    {
        return -EAGAIN;
    }

    error = fpsensor_cmd(fpsensor,
                         (deep_sleep) ? FPSENSOR_CMD_ACTIVATE_DEEP_SLEEP_MODE :
                         FPSENSOR_CMD_ACTIVATE_SLEEP_MODE,
                         0);

    if (error)
    {
        fpsensor_trace("%s %s command failed %d\n", __func__,
                       (deep_sleep) ? str_deep : str_regular,
                       error);

        return error;
    }

    error = 0;

    sleep_ok = false;
    while (!sleep_ok && retries && (error >= 0))
    {

        error = fpsensor_read_status_reg(fpsensor);

        if (error < 0)
        {
            fpsensor_trace("%s %s read status failed %d\n", __func__,
                           (deep_sleep) ? str_deep : str_regular,
                           error);
        }
        else
        {
            error &= status_mask;
            sleep_ok = (deep_sleep) ?
                       error == FPSENSOR_STATUS_REG_IN_DEEP_SLEEP_MODE :
                       error == FPSENSOR_STATUS_REG_IN_SLEEP_MODE;
        }
        if (!sleep_ok)
        {
            udelay(FPSENSOR_SLEEP_RETRY_TIME_US);
            retries--;
        }
    }

//    sleep_ok = true;
    //if (!fpsensor->soft_reset_enabled && deep_sleep && sleep_ok && fpsensor_gpio_valid(fpsensor->reset_gpio))
    //    fpsensor_gpio_wirte(fpsensor->reset_gpio, 0);

    #if FPSENSOR_MANUAL_CS
    //if (deep_sleep && sleep_ok && fpsensor_gpio_valid(fpsensor->cs_gpio))
    //    fpsensor_gpio_wirte(fpsensor->cs_gpio, 0);
    #endif

    /* Optional: Also disable power supplies in sleep */
    /*
        if (deep_sleep && sleep_ok)
            error = fpsensor_regulator_set(fpsensor, false);
    */

    if (sleep_ok)
    {
        fpsensor_trace(
            "%s %s OK\n", __func__,
            (deep_sleep) ? str_deep : str_regular);
        return 0;
    }
    else
    {
        fpsensor_error(
            "%s %s FAILED\n", __func__,
            (deep_sleep) ? str_deep : str_regular);

        return (deep_sleep) ? -EIO : -EAGAIN;
    }
}


/* -------------------------------------------------------------------- */
bool fpsensor_check_in_range_u64(u64 val, u64 min, u64 max)
{
    return (val >= min) && (val <= max);
}


/* -------------------------------------------------------------------- */
u32 fpsensor_calc_pixel_sum(u8 *buffer, size_t count)
{
    size_t index = count;
    u32 sum = 0;
    /*
        u32 i;
        for(i=0; i<count; i++)
        {
            if(((i%16) == 0) && (i != 0) )
                fpsensor_printk("\n");
            fpsensor_printk(" %02x", buffer[i]);
        }
        fpsensor_printk("\n");
    */
    while (index)
    {
        index--;
        sum += ((0xff - buffer[index]) / 8);
    }
    return sum;
}

/* -------------------------------------------------------------------- */
static int fpsensor_set_finger_drive(fpsensor_data_t *fpsensor, bool enable)
{

    int error = 0;
    u8 config;
    fpsensor_reg_access_t reg;

    fpsensor_trace( "%s %s\n", __func__, (enable) ? "ON" : "OFF");

    FPSENSOR_MK_REG_READ(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &config);
    error = fpsensor_reg_access(fpsensor, &reg);
    if (error)
    {
        goto out;
    }

    if (enable)
    {
        config |= 0x02;
    }
    else
    {
        config &= ~0x02;
    }

    FPSENSOR_MK_REG_WRITE(reg, FPSENSOR_REG_FINGER_DRIVE_CONF, &config);
    error = fpsensor_reg_access(fpsensor, &reg);
out:
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_calc_finger_detect_threshold_min(fpsensor_data_t *fpsensor)
{
    int error = 0;
    int index;
    int first_col, first_row, adc_groups, row_count;
    u32 pixelsum[FPSENSOR_WAKEUP_DETECT_ZONE_COUNT] = {0, 0};
    u32 temp_u32;

    size_t image_size;

    fpsensor_trace( "%s\n", __func__);

    error = fpsensor_write_sensor_setup(fpsensor);

    if (!error)
    {
        error = fpsensor_capture_finger_detect_settings(fpsensor);
    }

    if (!error)
    {
        error = fpsensor_set_finger_drive(fpsensor, false);
    }

    adc_groups = FPSENSOR_WAKEUP_DETECT_COLS / fpsensor->chip.adc_group_size;
    image_size = (FPSENSOR_WAKEUP_DETECT_ROWS + 1) * adc_groups * fpsensor->chip.adc_group_size;
    row_count = FPSENSOR_WAKEUP_DETECT_ROWS + 1;

    index = FPSENSOR_WAKEUP_DETECT_ZONE_COUNT;
    while (index && !error)
    {

        index--;

        first_col = fpsensor->setup.wakeup_detect_cols[index] / fpsensor->chip.adc_group_size;
        first_row = fpsensor->setup.wakeup_detect_rows[index] - 1;

        error = fpsensor_capture_set_crop(fpsensor,
                                          first_col,
                                          adc_groups,
                                          first_row,
                                          row_count);

        if (!error)
        {
            error = fpsensor_capture_buffer(fpsensor,
                                            fpsensor->huge_buffer,
                                            0,
                                            image_size);
        }

        if (!error)
        {
            pixelsum[index] = fpsensor_calc_pixel_sum(
                                  fpsensor->huge_buffer + fpsensor->chip.adc_group_size,
                                  image_size - fpsensor->chip.adc_group_size);
        }
    }

    if (!error)
    {
        temp_u32 = 0;

        index = FPSENSOR_WAKEUP_DETECT_ZONE_COUNT;
        while (index)
        {
            index--;
            if (pixelsum[index] > temp_u32)
            {
                temp_u32 = pixelsum[index];
            }
        }
        error = (int)(temp_u32 / 2);

        if (error >= 0xff)
        {
            error = -EINVAL;
        }
    }

    fpsensor_trace( "%s : %s %d\n",
                    __func__,
                    (error < 0) ? "Error" : "measured min =",
                    error);

    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_set_finger_detect_threshold(fpsensor_data_t *fpsensor,
                                         int measured_val)
{
    int error = 0;
    int new_val;
    u8 old_val = fpsensor->setup.finger_detect_threshold;

    new_val = measured_val + 0x70; // Todo: awaiting calculated values

    if ((measured_val < 0) || (new_val >= 0xff))
    {
        error = -EINVAL;
    }

    if (!error)
    {
        fpsensor->setup.finger_detect_threshold = (u8)new_val;

        fpsensor_trace( "%s %d -> %d\n",
                        __func__,
                        old_val,
                        fpsensor->setup.finger_detect_threshold);
    }
    else
    {
        fpsensor_error("%s unable to set finger detect threshold %d\n",
                       __func__,
                       error);
    }

    return error;
}



/* -------------------------------------------------------------------- */

