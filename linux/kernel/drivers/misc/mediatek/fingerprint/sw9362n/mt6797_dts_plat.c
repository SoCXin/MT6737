#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>

#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include "finger.h"
#include "config.h"
#include <mt_spi.h>


#define SUNWAVE_NAME "sunwave_fp"
//#define SUNWAVE_NAME "spidev"

static struct mt_chip_conf sunwave_chip_config = {
    /*
            .setuptime = 20,
            .holdtime = 20,
            .high_time = 13,
            .low_time = 12,
            .cs_idletime = 10,
            .ulthgh_thrsh = 0,
            .cpol = 0,
            .cpha = 0,
            .rx_mlsb = 1,
            .tx_mlsb = 1,
            .tx_endian = 0,
            .rx_endian = 0,
        #ifdef MTK_SPI_DMA_MODE
            .com_mod = DMA_TRANSFER,
        #else
            .com_mod = FIFO_TRANSFER,
        #endif
            .pause = PAUSE_MODE_ENABLE,
            .finish_intr = 1,
            .deassert = 0,
            .ulthigh = 0,
            .tckdly = 0,
    */
    .setuptime = 15,
    .holdtime = 15,
    .high_time = 50,
    .low_time = 50,
    .cs_idletime = 20,
    .ulthgh_thrsh = 0,
    .cpol = 0,
    .cpha = 0,
    .rx_mlsb = 1,
    .tx_mlsb = 1,
    .tx_endian = 0,
    .rx_endian = 0,
#if __SUNWAVE_SPI_DMA_MODE_EN
    .com_mod = DMA_TRANSFER,
#else // SPI FIFO MODE
    .com_mod = FIFO_TRANSFER,
#endif

    .pause = 0,
    .finish_intr = 1,
    .deassert = 0,
    .ulthigh = 0,
    .tckdly = 0,
};

static struct spi_board_info spi_board_devs[] __initdata = {
    [0] = {
        .modalias = SUNWAVE_NAME,
        .bus_num = 0,
        .chip_select = 0,
        .mode = SPI_MODE_0,
        .controller_data = &sunwave_chip_config,
    },
};
static int __init mt6797_dts_register_platform(void)
{
    /*NOTE: spi_register_board_info  don't use modules (*.ko)
    * WARNING: spi_register_board_info" [sunwave.ko] undefined!
    */
    spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
    return 0;
}

module_init(mt6797_dts_register_platform);

static void __exit mt6797_dts__dev_exit(void)
{
}
module_exit(mt6797_dts__dev_exit);

MODULE_AUTHOR("Jone.Chen, <yuhua8688@tom.com>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:sunwave_fp");
