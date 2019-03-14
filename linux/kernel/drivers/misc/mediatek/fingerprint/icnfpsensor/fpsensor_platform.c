/*++

 File Name:    fpsensor_platform.c
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/

//#include <linux/regulator/consumer.h>

#include <linux/of.h>
#include "fpsensor.h"
#include "fpsensor_common.h"
#include "fpsensor_platform.h"
#include <linux/of_gpio.h>
#include <linux/wakelock.h>

static fpsensor_data_t *this_fpsensor;
static struct wake_lock fpsensor_timeout_wakelock;

#define FPSENSOR_SPI_BLOCK_SIZE      1024
static struct spi_transfer fpsensor_xfer[100];
static u8 rx_buf0[FPSENSOR_REG_MAX_SIZE] = {0};
static u8 tx_buf0[FPSENSOR_REG_MAX_SIZE] = {0};
static u8 rx_buf1[FPSENSOR_REG_MAX_SIZE] = {0};
static u8 tx_buf1[FPSENSOR_REG_MAX_SIZE] = {0};


int plat_power(int power)
{
    int ret = 1;

    #if 0
    mt_set_gpio_mode(GPIO_FPS_POWER_3P3V_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_FPS_POWER_3P3V_PIN, GPIO_DIR_OUT);
    if (power)
    {
        mt_set_gpio_out(GPIO_FPS_POWER_3P3V_PIN, GPIO_OUT_ONE);
    }
    else
    {
        mt_set_gpio_out(GPIO_FPS_POWER_3P3V_PIN, GPIO_OUT_ZERO);
    }
    #endif


    return (ret == 1) ? 0 : -1;
}

//fpc1020
struct mt_chip_conf fpsensor_spi_conf_mt65xx =
{
    .setuptime = 20,
    .holdtime = 20,
    .high_time = 50,
    .low_time = 50,
    .cs_idletime = 5,
    .rx_mlsb = 1,
    .tx_mlsb = 1,
    .tx_endian = 0,
    .rx_endian = 0,
    .cpol = 0,
    .cpha = 0,
    .com_mod = FIFO_TRANSFER,
    .pause = 1,
    .finish_intr = 1,
    .deassert = 0,
    .tckdly = 0,
};

typedef enum
{
    SPEED_500KHZ = 500,
    SPEED_1MHZ = 1000,
    SPEED_2MHZ = 2000,
    SPEED_3MHZ = 3000,
    SPEED_4MHZ = 4000,
    SPEED_6MHZ = 6000,
    SPEED_8MHZ = 8000,
    SPEED_KEEP,
    SPEED_UNSUPPORTED
} SPI_SPEED;

int fpsensor_data_init(fpsensor_data_t *fpsensor)
{
    fpsensor_trace( "[fpsensor]----%s---\n", __func__);
    this_fpsensor = fpsensor;
    wake_lock_init(&fpsensor_timeout_wakelock, WAKE_LOCK_SUSPEND, "fpsensor timeout wakelock");
    return 0;
}

// for dts gpio control
static DEFINE_MUTEX(spidev_set_gpio_mutex);
static void spidev_gpio_as_int(fpsensor_data_t *fpsensor)
{
    fpsensor_trace( "[fpsensor]----%s---\n", __func__);
    mutex_lock(&spidev_set_gpio_mutex);
    printk("[fpsensor]spidev_gpio_as_int\n");
    pinctrl_select_state(fpsensor->pinctrl1, fpsensor->eint_as_int);
    mutex_unlock(&spidev_set_gpio_mutex);
}

void fpsensor_gpio_output_dts(int gpio, int level)
{
    mutex_lock(&spidev_set_gpio_mutex);
    printk("[fpsensor]fpsensor_gpio_output_dts: gpio= %d, level = %d\n", gpio, level);
    if (gpio == FPSENSOR_RST_PIN)
    {
        if (level)
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_rst_high);
        }
        else
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_rst_low);
        }
    }
    else if (gpio == FPSENSOR_SPI_CS_PIN)
    {
        if (level)
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_cs_high);
        }
        else
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_cs_low);
        }
    }
    else if (gpio == FPSENSOR_SPI_MO_PIN)
    {
        if (level)
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_mo_high);
        }
        else
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_mo_low);
        }
    }
    else if (gpio == FPSENSOR_SPI_CK_PIN)
    {
        if (level)
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_ck_high);
        }
        else
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_ck_low);
        }
    }
    else if (gpio == FPSENSOR_SPI_MI_PIN)
    {
        if (level)
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_mi_high);
        }
        else
        {
            pinctrl_select_state(this_fpsensor->pinctrl1, this_fpsensor->fp_mi_low);
        }
    }
    mutex_unlock(&spidev_set_gpio_mutex);
}

int fpsensor_spidev_dts_init(fpsensor_data_t *fpsensor)
{
    struct device_node *node;
    int ret = 0;
    fpsensor_printk( "%s\n", __func__);
    node = of_find_compatible_node(NULL, NULL, "mediatek,sunwave-finger");
    if (node)
    {
        fpsensor->fp_rst_low = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_rst_en0");
        if (IS_ERR(fpsensor->fp_rst_low))
        {
            ret = PTR_ERR(fpsensor->fp_rst_low);
            fpsensor_error("fpensor Cannot find fp pinctrl fp_rst_low!\n");
            return ret;
        }
        fpsensor->fp_rst_high = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_rst_en1");
        if (IS_ERR(fpsensor->fp_rst_high))
        {
            ret = PTR_ERR(fpsensor->fp_rst_high);
            fpsensor_error( "fpsensor Cannot find fp pinctrl fp_rst_high!\n");
            return ret;
        }

        fpsensor->eint_as_int = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_eint_pulldown"); //eint_in_low; eint
        if (IS_ERR(fpsensor->eint_as_int))
        {
            ret = PTR_ERR(fpsensor->eint_as_int);
            fpsensor_error( "fpsensor Cannot find fp pinctrl eint_as_int!\n");
            return ret;
        }

        fpsensor->fp_cs_low = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_cs");
        if (IS_ERR(fpsensor->fp_cs_low))
        {
            ret = PTR_ERR(fpsensor->fp_cs_low);
            fpsensor_error("fpensor Cannot find fp pinctrl fp_cs_low!\n");
            return ret;
        }
        fpsensor->fp_cs_high = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_cs");
        if (IS_ERR(fpsensor->fp_cs_high))
        {
            ret = PTR_ERR(fpsensor->fp_cs_high);
            fpsensor_error( "fpsensor Cannot find fp pinctrl fp_cs_high!\n");
            return ret;
        }

        fpsensor->fp_mo_high = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_mosi");
        if (IS_ERR(fpsensor->fp_mo_high))
        {
            ret = PTR_ERR(fpsensor->fp_mo_high);
            fpsensor_error( "fpsensor Cannot find fp pinctrl fp_mo_high!\n");
            return ret;
        }
        fpsensor->fp_mo_low = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_mosi");
        if (IS_ERR(fpsensor->fp_mo_low))
        {
            ret = PTR_ERR(fpsensor->fp_mo_low);
            fpsensor_error("fpensor Cannot find fp pinctrl fp_mo_low!\n");
            return ret;
        }

        fpsensor->fp_mi_high = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_miso");
        if (IS_ERR(fpsensor->fp_mi_high))
        {
            ret = PTR_ERR(fpsensor->fp_mi_high);
            fpsensor_error( "fpsensor Cannot find fp pinctrl fp_mi_high!\n");
            return ret;
        }
        fpsensor->fp_mi_low = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_miso");
        if (IS_ERR(fpsensor->fp_mi_low))
        {
            ret = PTR_ERR(fpsensor->fp_mi_low);
            fpsensor_error("fpensor Cannot find fp pinctrl fp_mi_low!\n");
            return ret;
        }

        fpsensor->fp_ck_high = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_sck");
        if (IS_ERR(fpsensor->fp_ck_high))
        {
            ret = PTR_ERR(fpsensor->fp_ck_high);
            fpsensor_error( "fpsensor Cannot find fp pinctrl fp_ck_high!\n");
            return ret;
        }
        fpsensor->fp_ck_low = pinctrl_lookup_state(fpsensor->pinctrl1, "finger_sck");
        if (IS_ERR(fpsensor->fp_ck_low))
        {
            ret = PTR_ERR(fpsensor->fp_ck_low);
            fpsensor_error("fpensor Cannot find fp pinctrl fp_ck_low!\n");
            return ret;
        }

        fpsensor_gpio_output_dts(FPSENSOR_SPI_MO_PIN, 0);
        fpsensor_gpio_output_dts(FPSENSOR_SPI_MI_PIN, 0);
        fpsensor_gpio_output_dts(FPSENSOR_SPI_CK_PIN, 0);
    }
    else
    {
        fpsensor_error("fpensor Cannot find node!\n");
    }
    return 0;
}
// end dts gpio control
/* -------------------------------------------------------------------- */
int fpsensor_get_of_pdata(struct device *dev, struct fpsensor_platform_data *pdata)
{
    pdata->reset_gpio = FPSENSOR_RST_PIN;//GPIOG(18);//GPIOM(0);//GPIOC(2);//-EINVAL;
    pdata->irq_gpio   = FPSENSOR_INT_PIN;//GPIOM(1);//-EINVAL;
    pdata->cs_gpio    = FPSENSOR_SPI_CS_PIN;//GPIOC(1);//-EINVAL;
    pdata->force_hwid = 0;

    pdata->external_supply_mv = 0;
    pdata->txout_boost = 0;

    pdata->use_regulator_for_bezel = 0;
    pdata->without_bezel = 0;

    return 0;
}


/* -------------------------------------------------------------------- */
int fpsensor_param_init(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata)
{
    fpsensor->vddtx_mv    = pdata->external_supply_mv;
    fpsensor->txout_boost = pdata->txout_boost;

    if (fpsensor->vddtx_mv > 0)
    {
        fpsensor_trace("External TxOut supply (%d mV)\n",
                       fpsensor->vddtx_mv);
    }
    else
    {
        fpsensor_trace("Internal TxOut supply (boost %s)\n",
                       (fpsensor->txout_boost) ? "ON" : "OFF");
    }

    fpsensor->force_hwid = pdata->force_hwid;
    fpsensor->use_regulator_for_bezel = pdata->use_regulator_for_bezel;
    fpsensor->without_bezel = pdata->without_bezel;

    return 0;
}



/* -------------------------------------------------------------------- */
int fpsensor_supply_init(fpsensor_data_t *fpsensor)
{
    int error = 0;

    // Determine is we should use external regulator for
    // power sully to the bezel.
    if ( fpsensor->use_regulator_for_bezel )
    {
        error = fpsensor_regulator_configure(fpsensor);
        if (error)
        {
            fpsensor_error("fpsensor_probe - regulator configuration failed.\n");
            goto err;
        }

        error = fpsensor_regulator_set(fpsensor, true);
        if (error)
        {
            fpsensor_error("fpsensor_probe - regulator enable failed.\n");
            goto err;
        }
    }

err:
    return error;
}



/* -------------------------------------------------------------------- */
int fpsensor_regulator_configure(fpsensor_data_t *fpsensor)
{
    fpsensor_trace( "%s\n", __func__);
    return 0;
}
/* -------------------------------------------------------------------- */
int fpsensor_regulator_release(fpsensor_data_t *fpsensor)
{
    return 0;
}
/* -------------------------------------------------------------------- */
int fpsensor_regulator_set(fpsensor_data_t *fpsensor, bool enable)
{
    fpsensor->power_enabled = enable;
    return 0;
}

/* -------------------------------------------------------------------- */
int fpsensor_reset_init(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata)
{
    int error = 0;
    fpsensor_trace( "[fpsensor]----%s---\n", __func__);
    //for debug
    #if FPSENSOR_SOFT_RST_ENABLE
    fpsensor_trace( "Using soft reset\n");
    fpsensor_gpio_wirte(fpsensor->reset_gpio, 1);
    fpsensor->soft_reset_enabled = true;
    #else    //
    fpsensor->soft_reset_enabled = false;
    fpsensor->reset_gpio = pdata->reset_gpio;
    fpsensor_trace("Assign HW reset -> GPIO%d\n", pdata->reset_gpio);
    #endif
    return error;
}


int fpsensor_gpio_wirte(int gpio, int value)
{
    //    gpio_set_value(gpio, value);
    //gpio_direction_output(gpio, value);

    fpsensor_gpio_output_dts(gpio, value);

    return 0;
}
int fpsensor_gpio_read(int gpio)
{
    return gpio_get_value(gpio);
}
int fpsensor_gpio_valid(int gpio)
{
    //return 1;
    return gpio_is_valid(gpio);
}
int fpsensor_gpio_free(int gpio)
{
    gpio_free(gpio);
    return 0;
}



void fpsensor_spi_set_mode(struct spi_device *spi, SPI_SPEED speed, int flag)
{
    struct mt_chip_conf *mcc = &fpsensor_spi_conf_mt65xx;
    if (flag == 0)
    {
        mcc->com_mod = FIFO_TRANSFER;
    }
    else
    {
        mcc->com_mod = DMA_TRANSFER;
    }

    switch (speed)
    {
        case SPEED_500KHZ:
            mcc->high_time = 120;
            mcc->low_time = 120;
            break;
        case SPEED_1MHZ:
            mcc->high_time = 60;
            mcc->low_time = 60;
            break;
        case SPEED_2MHZ:
            mcc->high_time = 30;
            mcc->low_time = 30;
            break;
        case SPEED_3MHZ:
            mcc->high_time = 20;
            mcc->low_time = 20;
            break;
        case SPEED_4MHZ:
            mcc->high_time = 15;
            mcc->low_time = 15;
            break;
        case SPEED_6MHZ:
            mcc->high_time = 10;
            mcc->low_time = 10;
            break;
        case SPEED_8MHZ:
            mcc->high_time = 8;
            mcc->low_time = 8;
            break;
        case SPEED_KEEP:
        case SPEED_UNSUPPORTED:
            break;
    }
    if (spi_setup(spi) < 0)
    {
        fpsensor_error("fpsensor:Failed to set spi.\n");
    }
}


void fpsensor_spi_pins_config(void)
{
    /*cs*/
    #if  FPSENSOR_MANUAL_CS
    //mt_set_gpio_mode(FPSENSOR_SPI_CS_PIN, GPIO_MODE_00);
    //mt_set_gpio_pull_enable(FPSENSOR_SPI_CS_PIN, GPIO_PULL_ENABLE);
    //mt_set_gpio_pull_select(FPSENSOR_SPI_CS_PIN, GPIO_PULL_UP);
    //mt_set_gpio_dir(FPSENSOR_SPI_CS_PIN, GPIO_DIR_OUT);
    //mt_set_gpio_out(FPSENSOR_SPI_CS_PIN, GPIO_OUT_ONE);
    #else

    #endif
    fpsensor_gpio_output_dts(FPSENSOR_SPI_MO_PIN, 0);
    fpsensor_gpio_output_dts(FPSENSOR_SPI_MI_PIN, 0);
    fpsensor_gpio_output_dts(FPSENSOR_SPI_CK_PIN, 0);

    msleep(1);
}


irqreturn_t fpsensor_interrupt(int irq, void *_fpsensor)
{
    fpsensor_data_t *fpsensor = _fpsensor;
    //fpsensor_trace("*************%s, 0x%x, %d\n", __func__, (int)fpsensor, fpsensor->fpsensor_init_done);
    //wake up cpu for syber
    //wake_lock_timeout(&fpsensor_timeout_wakelock, msecs_to_jiffies(200));

    if (fpsensor->fpsensor_init_done == 0)
    {
        return 0;
    }

    if (fpsensor_gpio_read(fpsensor->irq_gpio))
    {
        fpsensor->interrupt_done = true;
        wake_up_interruptible(&fpsensor->wq_irq_return);
        return 0;
    }
    return 0;
}

/* -------------------------------------------------------------------- */
int fpsensor_irq_init(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata)
{
    int error = 0;
    //     int ret = 0;
    struct device_node *node;
    u32 ints[2] = {0, 0};
    fpsensor_printk("%s\n", __func__);

    spidev_gpio_as_int(fpsensor);

    node = of_find_compatible_node(NULL, NULL, "mediatek,finger_irq");
    if ( node)
    {
        of_property_read_u32_array( node, "debounce", ints, ARRAY_SIZE(ints));
        gpio_request(ints[0], "fpsensor-irq");
        gpio_set_debounce(ints[0], ints[1]);
        fpsensor_printk("[fpsensor]ints[0] = %d,is irq_gpio , ints[1] = %d!!\n", ints[0], ints[1]);
        fpsensor->irq_gpio = ints[0];
        fpsensor->irq = irq_of_parse_and_map(node, 0);  // get irq number
        if (!fpsensor->irq)
        {
            printk("fpsensor irq_of_parse_and_map fail!!\n");
            return -EINVAL;
        }
        fpsensor_printk(" [fpsensor]fpsensor->irq= %d,fpsensor>irq_gpio = %d\n", fpsensor->irq,
                        fpsensor->irq_gpio);
    }
    else
    {
        printk("fpsensor null irq node!!\n");
        return -EINVAL;
    }
    error = request_irq(fpsensor->irq, fpsensor_interrupt,
                        IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND, FPSENSOR_DEV_NAME, fpsensor);

    if (error)
    {
        dev_err(&fpsensor->spi->dev,
                "request_irq %i failed.\n",
                fpsensor->irq);
        fpsensor->irq = -EINVAL;
    }

    //fpsensor_irq_disable(fpsensor);
    return error;
}


int fpsensor_irq_disable(fpsensor_data_t *fpsensor)
{
    disable_irq(fpsensor->irq);
    //sw_gpio_eint_set_enable(fpsensor->irq_gpio,0);
    return 0;
}
int fpsensor_irq_enable(fpsensor_data_t *fpsensor)
{
    enable_irq(fpsensor->irq);
    //sw_gpio_eint_set_enable(fpsensor->irq_gpio,1);
    return 0;
}
int fpsensor_irq_free(fpsensor_data_t *fpsensor)
{
    if (fpsensor->irq > 0)
        //free_irq(fpsensor->irq, fpsensor);
    {
        fpsensor_gpio_free(fpsensor->irq);
    }
    return 0;
}

/* -------------------------------------------------------------------- */
int fpsensor_spi_setup(fpsensor_data_t *fpsensor, struct fpsensor_platform_data *pdata)
{
    int error = 0;

    fpsensor_printk("%s\n", __func__);

    #if FPSENSOR_MANUAL_CS
    fpsensor->cs_gpio = pdata->cs_gpio;
    #endif
    fpsensor_spi_pins_config();

    fpsensor->spi->mode = SPI_MODE_0;
    fpsensor->spi->bits_per_word = 8;
    //    fpsensor->spi->chip_select = 0;
    fpsensor->spi->controller_data = (void *)&fpsensor_spi_conf_mt65xx;
    error = spi_setup(fpsensor->spi);

    if (error)
    {
        fpsensor_error( "spi_setup failed\n");
        goto out_err;
    }
    fpsensor_spi_set_mode(fpsensor->spi, fpsensor->spi_freq_khz, 0);

out_err:
    return error;
}

/* -------------------------------------------------------------------- */
int fpsensor_reg_access(fpsensor_data_t *fpsensor,
                        fpsensor_reg_access_t *reg_data)
{
    int error = 0;

    u8 tx_buffer[FPSENSOR_REG_MAX_SIZE] = {0};
    u8 rx_buffer[FPSENSOR_REG_MAX_SIZE] = {0};
    u8 cmd_len = 1 + FPSENSOR_REG_ACCESS_DUMMY_BYTES(reg_data->reg);
    struct spi_message msg;

    struct spi_transfer cmd =
    {
        .cs_change = 0,
        .delay_usecs = 0,
        .speed_hz = (u32)fpsensor->spi_freq_khz * 1000u,
        .tx_buf = tx_buffer,
        .rx_buf = rx_buffer,
        .len    = cmd_len + reg_data->reg_size,
        .tx_dma = 0,
        .rx_dma = 0,
        .bits_per_word = 0,
    };

    if (reg_data->reg_size > sizeof(tx_buffer))
    {
        fpsensor_error(
            "%s : illegal register size\n",
            __func__);

        error = -ENOMEM;
        goto out;
    }

    #if FPSENSOR_MANUAL_CS
    //if (fpsensor_gpio_valid(fpsensor->cs_gpio))
    fpsensor_gpio_wirte(fpsensor->cs_gpio, 0);
    #endif

    tx_buffer[0] = reg_data->reg;

    if (reg_data->write)
    {
        if (target_little_endian)
        {
            int src = 0;
            int dst = reg_data->reg_size - 1;
            while (src < reg_data->reg_size)
            {
                tx_buffer[cmd_len + dst] = reg_data->dataptr[src];
                src++;
                dst--;
            }
        }
        else
        {
            memcpy(tx_buffer + cmd_len,
                   reg_data->dataptr,
                   reg_data->reg_size);
        }
    }
    fpsensor_spi_set_mode(fpsensor->spi, (u32)fpsensor->spi_freq_khz, 0); //0: FIFO 1:DMA
    spi_message_init(&msg);
    spi_message_add_tail(&cmd,  &msg);
    error = spi_sync(fpsensor->spi, &msg);

    if (error)
    {
        fpsensor_error( "spi_sync failed.\n");
    }
    #if FPSENSOR_MANUAL_CS
    // if (fpsensor_gpio_valid(fpsensor->cs_gpio))
    fpsensor_gpio_wirte(fpsensor->cs_gpio, 1);
    #endif

    if (!reg_data->write)
    {
        if (target_little_endian)
        {
            int src = reg_data->reg_size - 1;
            int dst = 0;

            while (dst < reg_data->reg_size)
            {
                reg_data->dataptr[dst] = rx_buffer[src + cmd_len];
                src--;
                dst++;
            }
        }
        else
        {
            memcpy(reg_data->dataptr,
                   rx_buffer + cmd_len,
                   reg_data->reg_size);
        }
    }

    fpsensor_trace(
        "%s %s 0x%x/%d (%d bytes) %x %x %x %x : %x %x %x %x\n",
        __func__,
        (reg_data->write) ? "WRITE" : "READ",
        reg_data->reg,
        reg_data->reg,
        reg_data->reg_size,
        (reg_data->reg_size > 0) ? ((reg_data->write) ? tx_buffer[cmd_len] : rx_buffer[cmd_len]) : 0,
        (reg_data->reg_size > 1) ? ((reg_data->write) ? tx_buffer[cmd_len + 1] : rx_buffer[cmd_len + 1]) :
        0,
        (reg_data->reg_size > 2) ? ((reg_data->write) ? tx_buffer[cmd_len + 2] : rx_buffer[cmd_len + 2]) :
        0,
        (reg_data->reg_size > 3) ? ((reg_data->write) ? tx_buffer[cmd_len + 3] : rx_buffer[cmd_len + 3]) :
        0,
        (reg_data->reg_size > 4) ? ((reg_data->write) ? tx_buffer[cmd_len + 4] : rx_buffer[cmd_len + 4]) :
        0,
        (reg_data->reg_size > 5) ? ((reg_data->write) ? tx_buffer[cmd_len + 5] : rx_buffer[cmd_len + 5]) :
        0,
        (reg_data->reg_size > 6) ? ((reg_data->write) ? tx_buffer[cmd_len + 6] : rx_buffer[cmd_len + 6]) :
        0,
        (reg_data->reg_size > 7) ? ((reg_data->write) ? tx_buffer[cmd_len + 7] : rx_buffer[cmd_len + 7]) :
        0);

out:
    return error;
}


/* -------------------------------------------------------------------- */
int fpsensor_cmd(fpsensor_data_t *fpsensor,
                 fpsensor_cmd_t cmd,
                 u8 wait_irq_mask)
{
    int error = 0;
    struct spi_message msg;
    int i = 3;

    struct spi_transfer t =
    {
        .cs_change = 0,
        .delay_usecs = 0,
        .speed_hz = (u32)fpsensor->spi_freq_khz * 1000u,
        .tx_buf = &cmd,
        .rx_buf = NULL,
        .len    = 1,
        .tx_dma = 0,
        .rx_dma = 0,
        .bits_per_word = 0,
    };

    #if FPSENSOR_MANUAL_CS
    // if (fpsensor_gpio_valid(fpsensor->cs_gpio))
    fpsensor_gpio_wirte(fpsensor->cs_gpio, 0);
    #endif
    fpsensor_spi_set_mode(fpsensor->spi, (u32)fpsensor->spi_freq_khz, 0); //0: FIFO 1:DMA
    spi_message_init(&msg);
    spi_message_add_tail(&t,  &msg);
    error = spi_sync(fpsensor->spi, &msg);


    if (error)
    {
        fpsensor_error( "spi_sync failed.\n");
    }
    #if FPSENSOR_MANUAL_CS
    //if (fpsensor_gpio_valid(fpsensor->cs_gpio))
    fpsensor_gpio_wirte(fpsensor->cs_gpio, 1);
    #endif

    if ((error >= 0) && wait_irq_mask)
    {
        while (i--)
        {
            error = fpsensor_wait_for_irq(fpsensor, FPSENSOR_DEFAULT_IRQ_TIMEOUT_MS);
            if (error >= 0)
            {
                break;
            }
            fpsensor_printk("fpsensor cmd wait_irq_mask............\n");
        }

        if (error >= 0)
        {
            error = fpsensor_read_irq(fpsensor, true);
        }
        else
        {
            fpsensor_printk("fpsensor cmd wait_irq_mask error\n");
        }
    }


    fpsensor_trace( "%s 0x%x/%d, 0x%x\n", __func__, cmd, cmd, error);

    return error;
}

/* -------------------------------------------------------------------- */
int fpsensor_fetch_image(fpsensor_data_t *fpsensor,
                         u8 *buffer,
                         int offset,
                         size_t image_size_bytes,
                         size_t buff_size)
{
    int error = 0;
    struct spi_message msg;
    int spiBlockCount = image_size_bytes / FPSENSOR_SPI_BLOCK_SIZE;
    int spiBlocktLastNum = image_size_bytes % FPSENSOR_SPI_BLOCK_SIZE;
    int i = 0;

    memset(rx_buf0, 0x00, FPSENSOR_REG_MAX_SIZE);
    memset(tx_buf0, 0x00, FPSENSOR_REG_MAX_SIZE);
    memset(rx_buf1, 0x00, FPSENSOR_REG_MAX_SIZE);
    memset(tx_buf1, 0x00, FPSENSOR_REG_MAX_SIZE);

    fpsensor_trace("%s (+%d)\n", __func__, offset);
    fpsensor_trace("spiBlockCount: %d, spiBlocktLastNum: %d\n", spiBlockCount, spiBlocktLastNum);

    if ((offset + (int)image_size_bytes) > buff_size)
    {
        fpsensor_error("Image buffer too small for offset +%d (max %d bytes)",
                       offset,
                       (int)buff_size);

        error = -ENOBUFS;
    }

    if (!error)
    {
        #if FPSENSOR_MANUAL_CS
        fpsensor_gpio_wirte(fpsensor->cs_gpio, 0);
        #endif

        fpsensor_spi_set_mode(fpsensor->spi, fpsensor->spi_freq_khz, 1);  //DMA
        spi_message_init(&msg);

        tx_buf0[0] = FPSENSOR_CMD_READ_IMAGE;
        tx_buf0[1] = 0;
        fpsensor_xfer[0].tx_buf = tx_buf0;
        fpsensor_xfer[0].rx_buf = rx_buf0;
        fpsensor_xfer[0].len = 2;
        fpsensor_xfer[0].cs_change = 0;

        spi_message_add_tail(&fpsensor_xfer[0], &msg);

        for (i = 0; i < spiBlockCount; i++)
        {
            fpsensor_xfer[i + 1].rx_buf = &buffer[i * FPSENSOR_SPI_BLOCK_SIZE + offset];
            fpsensor_xfer[i + 1].tx_buf = &buffer[(i + spiBlockCount + 2) * FPSENSOR_SPI_BLOCK_SIZE + offset];
            fpsensor_xfer[i + 1].len = FPSENSOR_SPI_BLOCK_SIZE;
            fpsensor_xfer[i + 1].cs_change = 0;
            spi_message_add_tail(&fpsensor_xfer[i + 1], &msg);
        }
        if (spiBlocktLastNum > 0)
        {
            fpsensor_xfer[i + 1].rx_buf = &buffer[i * FPSENSOR_SPI_BLOCK_SIZE + offset];
            fpsensor_xfer[i + 1].tx_buf = &buffer[(i + spiBlockCount + 2) * FPSENSOR_SPI_BLOCK_SIZE + offset];
            fpsensor_xfer[i + 1].len = spiBlocktLastNum;
            fpsensor_xfer[i + 1].cs_change = 0;
            spi_message_add_tail(&fpsensor_xfer[i + 1], &msg);
        }

        error = spi_sync(fpsensor->spi, &msg);

        #if FPSENSOR_MANUAL_CS
        fpsensor_gpio_wirte(fpsensor->cs_gpio, 1);
        #endif
        if (error)
        {
            fpsensor_error( "spi_sync failed.\n");
        }

    }

    error = fpsensor_read_irq(fpsensor, true);

    if (error > 0)
    {
        error = (error & FPSENSOR_IRQ_REG_BIT_ERROR) ? -EIO : 0;
    }

    return error;
}
