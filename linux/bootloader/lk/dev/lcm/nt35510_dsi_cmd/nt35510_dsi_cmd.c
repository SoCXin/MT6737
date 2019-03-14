#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lcm_drv.h"

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

/* #define ESD_SUPPORT */

#define LCM_FRAME_WIDTH  (480)
#define LCM_FRAME_HEIGHT (800)

#define PHYSICAL_WIDTH  (56)	/* mm */
#define PHYSICAL_HEIGHT (93)	/* mm */

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

static unsigned int frame_width = 480;
static unsigned int frame_height = 800;
static unsigned int x_offset;
static unsigned int y_offset;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)           (lcm_util.udelay(n))
#define MDELAY(n)           (lcm_util.mdelay(n))

#define LCM_ID1             (0x55)
#define LCM_ID2             (0xC1)
#define LCM_ID3             (0x80)

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
	frame_width = atoi(LCM_WIDTH);
	frame_height = atoi(LCM_HEIGHT);

	if (strncmp(MTK_LCM_PHYSICAL_ROTATION, "180", 3) == 0) {
		x_offset = LCM_FRAME_WIDTH - frame_width;
		y_offset = LCM_FRAME_HEIGHT - frame_height;
	}

	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = frame_width;
	params->height = frame_height;

	params->physical_width = PHYSICAL_WIDTH * frame_width / LCM_FRAME_WIDTH;
	params->physical_height = PHYSICAL_HEIGHT * frame_height / LCM_FRAME_HEIGHT;

	params->dsi.mode = CMD_MODE;
	params->dsi.LANE_NUM = LCM_TWO_LANE;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.PLL_CLOCK = 221;
}

static void init_lcm_registers(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = 0x2555AAFF;
	data_array[2] = 0x00000001;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00093902;
	data_array[1] = 0x000201F8;
	data_array[2] = 0x00133320;
	data_array[3] = 0x00000048;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x00063902;
	data_array[1] = 0x52AA55F0;
	data_array[2] = 0x00000108;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x0D0D0DB0;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x343434B6;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x0D0D0DB1;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x343434B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x000000B2;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x242424B8;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000001BF;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x0F0F0FB3;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x343434B9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x080808B5;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000003C2;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x242424BA;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x007800BC;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x007800BD;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00033902;
	data_array[1] = 0x006400BE;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00353902;
	data_array[1] = 0x003300D1;
	data_array[2] = 0x003A0034;
	data_array[3] = 0x005C004A;
	data_array[4] = 0x00A60081;
	data_array[5] = 0x011301E5;
	data_array[6] = 0x01820154;
	data_array[7] = 0x020002CA;
	data_array[8] = 0x02340201;
	data_array[9] = 0x02840267;
	data_array[10] = 0x02B702A4;
	data_array[11] = 0x02DE02CF;
	data_array[12] = 0x03FE02F2;
	data_array[13] = 0x03330310;
	data_array[14] = 0x0000006D;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0] = 0x00353902;
	data_array[1] = 0x003300D2;
	data_array[2] = 0x003A0034;
	data_array[3] = 0x005C004A;
	data_array[4] = 0x00A60081;
	data_array[5] = 0x011301E5;
	data_array[6] = 0x01820154;
	data_array[7] = 0x020002CA;
	data_array[8] = 0x02340201;
	data_array[9] = 0x02840267;
	data_array[10] = 0x02B702A4;
	data_array[11] = 0x02DE02CF;
	data_array[12] = 0x03FE02F2;
	data_array[13] = 0x03330310;
	data_array[14] = 0x0000006D;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0] = 0x00353902;
	data_array[1] = 0x003300D3;
	data_array[2] = 0x003A0034;
	data_array[3] = 0x005C004A;
	data_array[4] = 0x00A60081;
	data_array[5] = 0x011301E5;
	data_array[6] = 0x01820154;
	data_array[7] = 0x020002CA;
	data_array[8] = 0x02340201;
	data_array[9] = 0x02840267;
	data_array[10] = 0x02B702A4;
	data_array[11] = 0x02DE02CF;
	data_array[12] = 0x03FE02F2;
	data_array[13] = 0x03330310;
	data_array[14] = 0x0000006D;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0] = 0x00353902;
	data_array[1] = 0x003300D4;
	data_array[2] = 0x003A0034;
	data_array[3] = 0x005C004A;
	data_array[4] = 0x00A60081;
	data_array[5] = 0x011301E5;
	data_array[6] = 0x01820154;
	data_array[7] = 0x020002CA;
	data_array[8] = 0x02340201;
	data_array[9] = 0x02840267;
	data_array[10] = 0x02B702A4;
	data_array[11] = 0x02DE02CF;
	data_array[12] = 0x03FE02F2;
	data_array[13] = 0x03330310;
	data_array[14] = 0x0000006D;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0] = 0x00353902;
	data_array[1] = 0x003300D5;
	data_array[2] = 0x003A0034;
	data_array[3] = 0x005C004A;
	data_array[4] = 0x00A60081;
	data_array[5] = 0x011301E5;
	data_array[6] = 0x01820154;
	data_array[7] = 0x020002CA;
	data_array[8] = 0x02340201;
	data_array[9] = 0x02840267;
	data_array[10] = 0x02B702A4;
	data_array[11] = 0x02DE02CF;
	data_array[12] = 0x03FE02F2;
	data_array[13] = 0x03330310;
	data_array[14] = 0x0000006D;
	dsi_set_cmdq(data_array, 15, 1);

	data_array[0] = 0x00353902;
	data_array[1] = 0x003300D6;
	data_array[2] = 0x003A0034;
	data_array[3] = 0x005C004A;
	data_array[4] = 0x00A60081;
	data_array[5] = 0x011301E5;
	data_array[6] = 0x01820154;
	data_array[7] = 0x020002CA;
	data_array[8] = 0x02340201;
	data_array[9] = 0x02840267;
	data_array[10] = 0x02B702A4;
	data_array[11] = 0x02DE02CF;
	data_array[12] = 0x03FE02F2;
	data_array[13] = 0x03330310;
	data_array[14] = 0x0000006D;
	dsi_set_cmdq(data_array, 15, 1);
	MDELAY(10);

	data_array[0] = 0x00063902;
	data_array[1] = 0x52AA55F0;
	data_array[2] = 0x00000008;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00033902;
	data_array[1] = 0x00007CB1;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902;
	data_array[1] = 0x000005B6;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00033902;
	data_array[1] = 0x007070B7;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00053902;
	data_array[1] = 0x030301B8;
	data_array[2] = 0x00000003;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043902;
	data_array[1] = 0x000002BC;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00063902;
	data_array[1] = 0x5002D0C9;
	data_array[2] = 0x00005050;
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00351500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x773A1500;
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(20);
}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(20);

	init_lcm_registers();
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00100500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00280500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);

	data_array[0] = 0x014F1500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(40);
}

static void lcm_resume(void)
{
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x + x_offset;
	unsigned int y0 = y + y_offset;
	unsigned int x1 = x0 + width - 1;
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

	data_array[0] = 0x00023902;
	data_array[1] = 0x51 | (level << 8);
	dsi_set_cmdq(data_array, 2, 1);
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[2] = { 0 };
	unsigned int data_array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(20);

	data_array[0] = 0x00063902;
	data_array[1] = 0x52AA55F0;
	data_array[2] = 0x00000108;
	dsi_set_cmdq(data_array, 3, 1);
	MDELAY(10);

	data_array[0] = 0x00023700;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xC5, buffer, 2);
	id = buffer[0];

	return (LCM_ID1 == id) ? 1 : 0;
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
LCM_DRIVER nt35510_dsi_cmd_lcm_drv = {
	.name = "nt35510_dsi_cmd",
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
