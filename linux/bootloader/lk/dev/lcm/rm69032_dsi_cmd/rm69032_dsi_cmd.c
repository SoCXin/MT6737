#include <stdio.h>
#include <string.h>

#include "lcm_drv.h"

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

/* #define ESD_SUPPORT */

#define FRAME_WIDTH  (320)
#define FRAME_HEIGHT (320)

#define PHYSICAL_WIDTH  (29)	/* mm */
#define PHYSICAL_HEIGHT (29)	/* mm */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define LCM_PRINT printf

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)           (lcm_util.udelay(n))
#define MDELAY(n)           (lcm_util.mdelay(n))

#define LCM_ID1             (0x00)
#define LCM_ID2             (0x80)
#define LCM_ID3             (0x00)

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->physical_width = PHYSICAL_WIDTH;
	params->physical_height = PHYSICAL_HEIGHT;

	params->dsi.mode = CMD_MODE;
	params->dsi.LANE_NUM = LCM_ONE_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.PLL_CLOCK = 221;
}

static void init_lcm_registers(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00063902;
	data_array[1] = 0x52AA55F0;
	data_array[2] = 0x00000008;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00063902;
	data_array[1] = 0x142003BD;
	data_array[2] = 0x0000004B;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00063902;
	data_array[1] = 0x142003BE;
	data_array[2] = 0x0000014B;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00063902;
	data_array[1] = 0x142003BF;
	data_array[2] = 0x0000004B;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x070707BB;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000040C7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00063902;
	data_array[1] = 0x52AA55F0;
	data_array[2] = 0x00000208;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x02EB1500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00033902;
	data_array[1] = 0x005008FE;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x0495F2C3;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x383600E9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x04CA1500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00063902;
	data_array[1] = 0x52AA55F0;
	data_array[2] = 0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x030303B0;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x050505B1;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x010101B2;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x070707B4;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x030303B5;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x555555B6;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x363636B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x232323B8;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x030303B9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x030303BA;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x703032BE;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00083902;
	/* data_array[1] = 0x95D4FFCF; */
	data_array[1] = 0x95D400CF;
	data_array[2] = 0x04004FE8;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x01351500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00361500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x20C01500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00073902;
	data_array[1] = 0x171717C2;
	data_array[2] = 0x000B1717;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00003200;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(300);

	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(5);

	init_lcm_registers();
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x014F1500;
	dsi_set_cmdq(data_array, 1, 1);
}

static void lcm_resume(void)
{
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1 + 160;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = (x0 >> 8) & 0xFF;
	unsigned char x0_LSB = x0 & 0xFF;
	unsigned char x1_MSB = (x1 >> 8) & 0xFF;
	unsigned char x1_LSB = x1 & 0xFF;
	unsigned char y0_MSB = (y0 >> 8) & 0xFF;
	unsigned char y0_LSB = y0 & 0xFF;
	unsigned char y1_MSB = (y1 >> 8) & 0xFF;
	unsigned char y1_LSB = y1 & 0xFF;

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2A;
	data_array[2] = x1_LSB;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2B;
	data_array[2] = y1_LSB;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002C3909;
	dsi_set_cmdq(data_array, 1, 0);
}

static void lcm_setbacklight(unsigned int level)
{
	unsigned int data_array[16];

	LCM_PRINT("lcm_setbacklight = %d\n", level);

	if (level > 255)
		level = 255;

#if 0
	if (level < 4)
		level = 4;
#endif

	data_array[0] = 0x00063902;
	data_array[1] = 0x52AA55F0;
	data_array[2] = 0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00083902;
	data_array[1] = 0x95D400CF | (level << 8);
	data_array[2] = 0x04004FE8;
	dsi_set_cmdq(data_array, 3, 1);
}

static unsigned int lcm_compare_id(void)
{
	unsigned char buffer[3] = { 0 };
	unsigned int data_array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(5);

	data_array[0] = 0x00033700;
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x04, buffer, 3);

	return ((LCM_ID1 == buffer[0]) && (LCM_ID2 == buffer[1]) && (LCM_ID3 == buffer[2])) ? 1 : 0;
}

#ifdef ESD_SUPPORT

static unsigned int lcm_esd_check(void)
{
	unsigned int result = TRUE;
	unsigned int data_array[16];
	unsigned char buffer[16] = { 0 };

	data_array[0] = 0x00013700;
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);
	if (buffer[0] == 0x9C)
		result = FALSE;

	return result;
}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();

	return TRUE;
}

#endif

static void lcm_read_fb(unsigned char *buffer)
{
	unsigned int array[2];

	array[0] = 0x000A3700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x2E, buffer, 10);
	read_reg_v2(0x3E, buffer + 10, 10);
	read_reg_v2(0x3E, buffer + 10 * 2, 10);
	read_reg_v2(0x3E, buffer + 10 * 3, 10);
	read_reg_v2(0x3E, buffer + 10 * 4, 10);
	read_reg_v2(0x3E, buffer + 10 * 5, 10);
}

static void lcm_enter_idle(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00390500;
	dsi_set_cmdq(data_array, 1, 1);
}

static void lcm_exit_idle(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00380500;
	dsi_set_cmdq(data_array, 1, 1);
}

/* --------------------------------------------------------------------------- */
/* Get LCM Driver Hooks */
/* --------------------------------------------------------------------------- */
LCM_DRIVER rm69032_dsi_cmd_lcm_drv = {
	.name = "rm69032_dsi_cmd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.set_backlight = lcm_setbacklight,
	.compare_id = lcm_compare_id,
	.update = lcm_update,
#ifdef ESD_SUPPORT
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
#endif
	.read_fb = lcm_read_fb,
	.enter_idle = lcm_enter_idle,
	.exit_idle = lcm_exit_idle,
};
