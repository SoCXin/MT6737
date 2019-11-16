#ifndef __DDP_DPI_H__
#define __DDP_DPI_H__

/*#include <mach/mt_typedefs.h>*/
#include <linux/types.h>

#include "lcm_drv.h"
#include "ddp_info.h"
#include "cmdq_record.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE  (1)
#endif

#ifndef NULL
#define NULL  (0)
#endif

#define DPI_PHY_ADDR 0x14012000

#define DPI_CHECK_RET(expr)  \
	do { \
		enum DPI_STATUS ret = (expr); \
		ASSERT(DPI_STATUS_OK == ret);  \
	} while (0) \

/*for legacy DPI Driver*/
enum LCD_IF_ID {
	LCD_IF_PARALLEL_0 = 0,
	LCD_IF_PARALLEL_1 = 1,
	LCD_IF_PARALLEL_2 = 2,
	LCD_IF_SERIAL_0 = 3,
	LCD_IF_SERIAL_1 = 4,

	LCD_IF_ALL = 0xFF,
};

struct LCD_REG_CMD_ADDR {
	unsigned rsv_0:4;
	unsigned addr:4;
	unsigned rsv_8:24;
};

struct LCD_REG_DAT_ADDR {
	unsigned rsv_0:4;
	unsigned addr:4;
	unsigned rsv_8:24;
};

enum LCD_IF_FMT_COLOR_ORDER {
	LCD_IF_FMT_COLOR_ORDER_RGB = 0,
	LCD_IF_FMT_COLOR_ORDER_BGR = 1,
};


enum LCD_IF_FMT_TRANS_SEQ {
	LCD_IF_FMT_TRANS_SEQ_MSB_FIRST = 0,
	LCD_IF_FMT_TRANS_SEQ_LSB_FIRST = 1,
};


enum LCD_IF_FMT_PADDING {
	LCD_IF_FMT_PADDING_ON_LSB = 0,
	LCD_IF_FMT_PADDING_ON_MSB = 1,
};


enum LCD_IF_FORMAT {
	LCD_IF_FORMAT_RGB332 = 0,
	LCD_IF_FORMAT_RGB444 = 1,
	LCD_IF_FORMAT_RGB565 = 2,
	LCD_IF_FORMAT_RGB666 = 3,
	LCD_IF_FORMAT_RGB888 = 4,
};

enum LCD_IF_WIDTH {
	LCD_IF_WIDTH_8_BITS = 0,
	LCD_IF_WIDTH_9_BITS = 2,
	LCD_IF_WIDTH_16_BITS = 1,
	LCD_IF_WIDTH_18_BITS = 3,
	LCD_IF_WIDTH_24_BITS = 4,
	LCD_IF_WIDTH_32_BITS = 5,
};

enum DPI_STATUS {
	DPI_STATUS_OK = 0,

	DPI_STATUS_ERROR,
};

enum DPI_POLARITY {
	DPI_POLARITY_RISING = 0,
	DPI_POLARITY_FALLING = 1
};

enum DPI_RGB_ORDER {
	DPI_RGB_ORDER_RGB = 0,
	DPI_RGB_ORDER_BGR = 1,
};

enum DPI_CLK_FREQ {
	DPI_CLK_480p = 27027,
	DPI_CLK_480p_3D = 27027 * 2,
	DPI_CLK_720p = 74250,
	DPI_CLK_1080p = 148500
};

struct LCD_REG_WROI_CON {
	unsigned RGB_ORDER:1;
	unsigned BYTE_ORDER:1;
	unsigned PADDING:1;
	unsigned DATA_FMT:3;
	unsigned IF_FMT:2;
	unsigned COMMAND:5;
	unsigned rsv_13:2;
	unsigned ENC:1;
	unsigned rsv_16:8;
	unsigned SEND_RES_MODE:1;
	unsigned IF_24:1;
	unsigned rsv_6:6;
};

int ddp_dpi_stop(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_power_on(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_power_off(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_dump(DISP_MODULE_ENUM module, int level);
int ddp_dpi_start(DISP_MODULE_ENUM module, void *cmdq);
int ddp_dpi_init(DISP_MODULE_ENUM module, void *cmdq);
int ddp_dpi_deinit(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_config(DISP_MODULE_ENUM module, disp_ddp_path_config *config,
		   void *cmdq_handle);
int ddp_dpi_trigger(DISP_MODULE_ENUM module, void *cmdq);
int ddp_dpi_reset(DISP_MODULE_ENUM module, void *cmdq_handle);
int ddp_dpi_ioctl(DISP_MODULE_ENUM module, void *cmdq_handle, unsigned int ioctl_cmd,
		  unsigned long *params);

int _Enable_Interrupt(void);

enum AviColorSpace_e {
	acsRGB = 0, acsYCbCr422 = 1, acsYCbCr444 = 2, acsFuture = 3
};
extern struct DPI_REGS *DPI_REG;
#ifdef __cplusplus
}
#endif
#endif				/*__DPI_DRV_H__*/
