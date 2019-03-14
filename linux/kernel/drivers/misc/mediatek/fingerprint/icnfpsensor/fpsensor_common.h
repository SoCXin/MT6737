/*++

 File Name:    fpsensor_common.h
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/


#ifndef LINUX_SPI_FPSENSOR_COMMON_H
#define LINUX_SPI_FPSENSOR_COMMON_H


#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/spi/spi.h>
#include <linux/wait.h>

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/device.h>


#include "fpsensor.h"
#include "fpsensor_regs.h"


/* -------------------------------------------------------------------- */
/* fpsensor driver constants                     */
/* -------------------------------------------------------------------- */
extern const bool target_little_endian;

#define FPSENSOR_DEV_NAME                        "fpsensor"

/* set '0' for dynamic assignment, or '> 0' for static assignment */
#define FPSENSOR_MAJOR                           0

#define FPSENSOR_BUFFER_MAX_IMAGES               3
#define FPSENSOR_MAX_ADC_SETTINGS                (FPSENSOR_BUFFER_MAX_IMAGES + 1)

#define FPSENSOR_DEFAULT_IRQ_TIMEOUT_MS          (100 * HZ / 1000)

#define FPSENSOR_STATUS_REG_RESET_VALUE          0x1e

#define FPSENSOR_STATUS_REG_MODE_MASK ( \
                                        FPSENSOR_STATUS_REG_BIT_MAIN_IDLE_CMD | \
                                        FPSENSOR_STATUS_REG_BIT_SYNC_PWR_IDLE | \
                                        FPSENSOR_STATUS_REG_BIT_PWR_DWN_OSC_HIN)

#define FPSENSOR_STATUS_REG_IN_DEEP_SLEEP_MODE   0

#define FPSENSOR_STATUS_REG_IN_SLEEP_MODE        0

#define FPSENSOR_STATUS_REG_IN_IDLE_MODE ( \
                                           FPSENSOR_STATUS_REG_BIT_MAIN_IDLE_CMD | \
                                           FPSENSOR_STATUS_REG_BIT_SYNC_PWR_IDLE | \
                                           FPSENSOR_STATUS_REG_BIT_PWR_DWN_OSC_HIN)

#define FPSENSOR_SLEEP_RETRIES                   5
#define FPSENSOR_SLEEP_RETRY_TIME_US             1000

#define FPSENSOR_RESET_RETRIES                   2
#define FPSENSOR_RESET_LOW_US                    1000
#define FPSENSOR_RESET_HIGH1_US                  100
#define FPSENSOR_RESET_HIGH2_US                  1250

#define FPSENSOR_CAPTURE_WAIT_FINGER_DELAY_MS    20

#define FPSENSOR_WAKEUP_DETECT_ZONE_COUNT        2
#define FPSENSOR_WAKEUP_DETECT_ROWS              8
#define FPSENSOR_WAKEUP_DETECT_COLS              8

#define FPSENSOR_PXL_BIAS_CTRL                   0x0F00


/* -------------------------------------------------------------------- */
/* fpsensor data types                           */
/* -------------------------------------------------------------------- */
typedef enum
{
    FPSENSOR_STATUS_REG_BIT_IRQ                 = 1 << 0,
    FPSENSOR_STATUS_REG_BIT_MAIN_IDLE_CMD       = 1 << 1,
    FPSENSOR_STATUS_REG_BIT_SYNC_PWR_IDLE       = 1 << 2,
    FPSENSOR_STATUS_REG_BIT_PWR_DWN_OSC_HIN     = 1 << 3,
    FPSENSOR_STATUS_REG_BIT_FIFO_EMPTY          = 1 << 4,
    FPSENSOR_STATUS_REG_BIT_FIFO_FULL           = 1 << 5,
    FPSENSOR_STATUS_REG_BIT_MISO_EDGRE_RISE_EN  = 1 << 6
} fpsensor_status_reg_t;

typedef enum
{
    FPSENSOR_CMD_FINGER_PRESENT_QUERY           = 0x24,
    FPSENSOR_CMD_WAIT_FOR_FINGER_PRESENT        = 0x28,
    FPSENSOR_CMD_ACTIVATE_SLEEP_MODE            = 0x34,
    FPSENSOR_CMD_ACTIVATE_DEEP_SLEEP_MODE       = 0x38,
    FPSENSOR_CMD_ACTIVATE_IDLE_MODE             = 0x20,
    FPSENSOR_CMD_CAPTURE_IMAGE                  = 0x30,
    FPSENSOR_CMD_READ_IMAGE                     = 0x2C,
    FPSENSOR_CMD_SOFT_RESET                     = 0xF8
} fpsensor_cmd_t;

typedef enum
{
    FPSENSOR_IRQ_REG_BIT_FINGER_DOWN   = 1 << 0,
    FPSENSOR_IRQ_REG_BIT_ERROR         = 1 << 2,
    FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA = 1 << 5,
    FPSENSOR_IRQ_REG_BIT_COMMAND_DONE  = 1 << 7,
    FPSENSOR_IRQ_REG_BITS_REBOOT       = 0xff
} fpsensor_irq_reg_t;

typedef enum
{
    FPSENSOR_CAPTURE_STATE_IDLE = 0,
    FPSENSOR_CAPTURE_STATE_STARTED,
    FPSENSOR_CAPTURE_STATE_PENDING,
    FPSENSOR_CAPTURE_STATE_WRITE_SETTINGS,
    FPSENSOR_CAPTURE_STATE_WAIT_FOR_FINGER_DOWN,
    FPSENSOR_CAPTURE_STATE_ACQUIRE,
    FPSENSOR_CAPTURE_STATE_FETCH,
    FPSENSOR_CAPTURE_STATE_WAIT_FOR_FINGER_UP,
    FPSENSOR_CAPTURE_STATE_COMPLETED,
    FPSENSOR_CAPTURE_STATE_FAILED,
} fpsensor_capture_state_t;

typedef struct fpsensor_worker_struct
{
    struct task_struct *thread;
    struct semaphore sem_idle;
    wait_queue_head_t wq_wait_job;
    int req_mode;
    bool stop_request;
} fpsensor_worker_t;

typedef struct fpsensor_capture_struct
{
    fpsensor_capture_mode_t  current_mode;
    fpsensor_capture_state_t state;
    u32         read_offset;
    u32         available_bytes;
    wait_queue_head_t   wq_data_avail;
    int         last_error;
    bool            read_pending_eof;
    bool            deferred_finger_up;
} fpsensor_capture_task_t;

typedef struct fpsensor_input_struct
{
    bool enabled;
    bool gesture_key;
} fpsensor_input_task_t;


typedef struct fpsensor_setup
{
    u8 adc_gain[FPSENSOR_MAX_ADC_SETTINGS];
    u8 adc_shift[FPSENSOR_MAX_ADC_SETTINGS];
    u16 pxl_ctrl[FPSENSOR_MAX_ADC_SETTINGS];
    u8 adc_et1[FPSENSOR_MAX_ADC_SETTINGS];
    u8 capture_settings_mux;
    u8 capture_count;
    fpsensor_capture_mode_t capture_mode;
    u8 capture_row_start;   /* Row 0-191        */
    u8 capture_row_count;   /* Rows <= 192      */
    u8 capture_col_start;   /* ADC group 0-23   */
    u8 capture_col_groups;  /* ADC groups, 1-24 */
    u8 capture_finger_up_threshold;
    u8 capture_finger_down_threshold;
    u8 finger_detect_threshold;
    u8 wakeup_detect_rows[FPSENSOR_WAKEUP_DETECT_ZONE_COUNT];
    u8 wakeup_detect_cols[FPSENSOR_WAKEUP_DETECT_ZONE_COUNT];
    bool tiny_capture_enable;
    bool sleep_dect;
} fpsensor_setup_t;

typedef struct fpsensor_diag
{
    const char *chip_id;    /* RO */
    u8  selftest;       /* RO */
    u16 spi_register;   /* RW */
    u8  spi_regsize;    /* RO */
    u8  spi_data;       /* RW */
    u16 last_capture_time;  /* RO*/
    u16 finger_present_status;  /* RO*/
    u8  wakeup_system;
    u8  gesture_key;
} fpsensor_diag_t;

typedef struct fpsensor_chip_info
{
    fpsensor_chip_t         type;
    u16                    hwid;
    u8                     sensorid;
    u8                     revision;
    u8                     pixel_rows;
    u8                     pixel_columns;
    u8                     adc_group_size;
    u16                    spi_max_khz;
} fpsensor_chip_info_t;

typedef struct
{
    struct spi_device      *spi;
    struct class           *class;
    struct device          *device;
    struct cdev            cdev;
    dev_t                  devno;

    fpsensor_chip_info_t    chip;
    u32                    cs_gpio;
    u32                    reset_gpio;
    u32                    irq_gpio;
    int                    irq;
    struct pinctrl *pinctrl1;
    struct pinctrl_state *pins_default;
    struct pinctrl_state *eint_as_int, *fp_rst_low, *fp_rst_high, *fp_cs_high, *fp_cs_low, *fp_mo_low, 
    *fp_mo_high, *fp_mi_low, *fp_mi_high, *fp_ck_low, *fp_ck_high;
  
 
    wait_queue_head_t      wq_irq_return;
    bool                   interrupt_done;
    struct semaphore       mutex;
    u8                     *huge_buffer;
    size_t                 huge_buffer_size;
    fpsensor_worker_t       worker;
    fpsensor_capture_task_t capture;
    fpsensor_setup_t        setup;
    fpsensor_diag_t         diag;
    bool                   soft_reset_enabled;
    struct regulator       *vcc_spi;
    struct regulator       *vdd_ana;
    struct regulator       *vdd_io;
    struct regulator       *vdd_tx;
    bool                   power_enabled;
    int                    vddtx_mv;
    bool                   txout_boost;
    u16                    force_hwid;
    bool                   use_regulator_for_bezel;
    u16                    spi_freq_khz;
    struct input_dev       *input_dev;
    fpsensor_input_task_t   input;
    bool                   without_bezel;
    bool                   fpsensor_init_done;

} fpsensor_data_t;

typedef struct
{
    fpsensor_reg_t reg;
    bool          write;
    u16           reg_size;
    u8            *dataptr;
} fpsensor_reg_access_t;


/* -------------------------------------------------------------------- */
/* function prototypes                          */
/* -------------------------------------------------------------------- */
extern size_t fpsensor_calc_huge_buffer_minsize(fpsensor_data_t *fpsensor);

extern int fpsensor_manage_huge_buffer(fpsensor_data_t *fpsensor,
                                       size_t new_size);

extern int fpsensor_setup_defaults(fpsensor_data_t *fpsensor);

extern int fpsensor_gpio_reset(fpsensor_data_t *fpsensor);

extern int fpsensor_spi_reset(fpsensor_data_t *fpsensor);

extern int fpsensor_reset(fpsensor_data_t *fpsensor);

extern int fpsensor_check_hw_id(fpsensor_data_t *fpsensor);

extern const char *fpsensor_hw_id_text(fpsensor_data_t *fpsensor);

extern int fpsensor_write_sensor_setup(fpsensor_data_t *fpsensor);

extern int fpsensor_wait_for_irq(fpsensor_data_t *fpsensor, int timeout);

extern int fpsensor_read_irq(fpsensor_data_t *fpsensor, bool clear_irq);

extern int fpsensor_read_status_reg(fpsensor_data_t *fpsensor);

extern int fpsensor_wait_finger_present(fpsensor_data_t *fpsensor);
extern int fpsensor_wait_finger_present_new(fpsensor_data_t *fpsensor);

extern int fpsensor_get_finger_present_status(fpsensor_data_t *fpsensor);

extern int fpsensor_check_finger_present_raw(fpsensor_data_t *fpsensor);

extern int fpsensor_check_finger_present_sum(fpsensor_data_t *fpsensor);

extern int fpsensor_wake_up(fpsensor_data_t *fpsensor);

extern int fpsensor_sleep(fpsensor_data_t *fpsensor, bool deep_sleep);

extern bool fpsensor_check_in_range_u64(u64 val, u64 min, u64 max);

extern int fpsensor_calc_finger_detect_threshold_min(fpsensor_data_t *fpsensor);

extern int fpsensor_set_finger_detect_threshold(fpsensor_data_t *fpsensor,
                                                int measured_val);
extern int fpsensor_get_finger_status_value(fpsensor_data_t *fpsensor , u8 *buffer_u8);

#define FPSENSOR_MK_REG_READ_BYTES(__dst, __reg, __count, __ptr) {   \
        (__dst).reg      = FPSENSOR_REG_TO_ACTUAL((__reg));      \
        (__dst).reg_size = (__count);                   \
        (__dst).write    = false;                   \
        (__dst).dataptr  = (__ptr); }

#define FPSENSOR_MK_REG_READ(__dst, __reg, __ptr) {          \
        (__dst).reg      = FPSENSOR_REG_TO_ACTUAL((__reg));      \
        (__dst).reg_size = FPSENSOR_REG_SIZE((__reg));           \
        (__dst).write    = false;                   \
        (__dst).dataptr  = (u8 *)(__ptr); }

#define FPSENSOR_MK_REG_WRITE_BYTES(__dst, __reg, __count, __ptr) {  \
        (__dst).reg      = FPSENSOR_REG_TO_ACTUAL((__reg));      \
        (__dst).reg_size = (__count);                   \
        (__dst).write    = true;                    \
        (__dst).dataptr  = (__ptr); }

#define FPSENSOR_MK_REG_WRITE(__dst, __reg, __ptr) {         \
        (__dst).reg      = FPSENSOR_REG_TO_ACTUAL((__reg));      \
        (__dst).reg_size = FPSENSOR_REG_SIZE((__reg));           \
        (__dst).write    = true;                    \
        (__dst).dataptr  = (u8 *)(__ptr); }

#define FPSENSOR_FINGER_DETECT_ZONE_MASK     0x0FFFU

#endif /* LINUX_SPI_FPSENSOR_COMMON_H */

