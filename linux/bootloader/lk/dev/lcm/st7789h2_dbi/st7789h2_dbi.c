#include <stdio.h>
#include <string.h>
#include <platform/mt_gpio.h>

#include "cust_gpio_usage.h"
#include "lcm_drv.h"

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

/* #define ESD_SUPPORT */

#define FRAME_WIDTH  (240)
#define FRAME_HEIGHT (240)

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
static unsigned int gpio_mode_backup[9];
static int is_io_backup;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)           (lcm_util.udelay(n))
#define MDELAY(n)           (lcm_util.mdelay(n))

#define LCM_ID0             (0x85)
#define LCM_ID1             (0x85)
#define LCM_ID2             (0x52)

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

static inline void send_ctrl_cmd(unsigned int cmd)
{
	lcm_util.send_cmd(cmd);
}

static inline void send_data_cmd(unsigned int data)
{
	lcm_util.send_data(data & 0xFF);
}

static inline unsigned char read_data_cmd(void)
{
	return 0xFF & lcm_util.read_data();
}

static inline void set_lcm_register(unsigned int regIndex, unsigned int regData)
{
	send_ctrl_cmd(regIndex);
	send_data_cmd(regData);
}

static inline void set_lcm_gpio_output_low(unsigned int GPIO)
{
	lcm_util.set_gpio_mode(GPIO, GPIO_MODE_00);
	lcm_util.set_gpio_dir(GPIO, GPIO_DIR_OUT);
	lcm_util.set_gpio_out(GPIO, GPIO_OUT_ZERO);
}

static inline void set_lcm_gpio_mode(unsigned int GPIO, unsigned int mode)
{
	lcm_util.set_gpio_mode(GPIO, mode);
}

static inline int get_lcm_gpio_mode(unsigned int GPIO)
{
	return mt_get_gpio_mode(GPIO);
}

static void backup_io_mode(void)
{
	if (is_io_backup)
		return;

#ifdef GPIO_LCD_IO_0_PIN
	gpio_mode_backup[0] = get_lcm_gpio_mode(GPIO_LCD_IO_0_PIN);
#endif

#ifdef GPIO_LCD_IO_1_PIN
	gpio_mode_backup[1] = get_lcm_gpio_mode(GPIO_LCD_IO_1_PIN);
#endif

#ifdef GPIO_LCD_IO_2_PIN
	gpio_mode_backup[2] = get_lcm_gpio_mode(GPIO_LCD_IO_2_PIN);
#endif

#ifdef GPIO_LCD_IO_3_PIN
	gpio_mode_backup[3] = get_lcm_gpio_mode(GPIO_LCD_IO_3_PIN);
#endif

#ifdef GPIO_LCD_IO_4_PIN
	gpio_mode_backup[4] = get_lcm_gpio_mode(GPIO_LCD_IO_4_PIN);
#endif

#ifdef GPIO_LCD_IO_5_PIN
	gpio_mode_backup[5] = get_lcm_gpio_mode(GPIO_LCD_IO_5_PIN);
#endif

#ifdef GPIO_LCD_IO_6_PIN
	gpio_mode_backup[6] = get_lcm_gpio_mode(GPIO_LCD_IO_6_PIN);
#endif

#ifdef GPIO_LCD_IO_7_PIN
	gpio_mode_backup[7] = get_lcm_gpio_mode(GPIO_LCD_IO_7_PIN);
#endif

#ifdef GPIO_PWM_BL_PIN
	gpio_mode_backup[8] = get_lcm_gpio_mode(GPIO_PWM_BL_PIN);
#endif

	is_io_backup = 1;
}

static void set_io_gpio_mode(void)
{
#ifdef GPIO_LCD_IO_0_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_0_PIN);
#endif

#ifdef GPIO_LCD_IO_1_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_1_PIN);
#endif

#ifdef GPIO_LCD_IO_2_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_2_PIN);
#endif

#ifdef GPIO_LCD_IO_3_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_3_PIN);
#endif

#ifdef GPIO_LCD_IO_4_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_4_PIN);
#endif

#ifdef GPIO_LCD_IO_5_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_5_PIN);
#endif

#ifdef GPIO_LCD_IO_6_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_6_PIN);
#endif

#ifdef GPIO_LCD_IO_7_PIN
	set_lcm_gpio_output_low(GPIO_LCD_IO_7_PIN);
#endif

#ifdef GPIO_PWM_BL_PIN
	set_lcm_gpio_output_low(GPIO_PWM_BL_PIN);
#endif
}

static void set_io_lcm_mode(void)
{
#ifdef GPIO_LCD_IO_0_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_0_PIN, gpio_mode_backup[0]);
#endif

#ifdef GPIO_LCD_IO_1_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_1_PIN, gpio_mode_backup[1]);
#endif

#ifdef GPIO_LCD_IO_2_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_2_PIN, gpio_mode_backup[2]);
#endif

#ifdef GPIO_LCD_IO_3_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_3_PIN, gpio_mode_backup[3]);
#endif

#ifdef GPIO_LCD_IO_4_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_4_PIN, gpio_mode_backup[4]);
#endif

#ifdef GPIO_LCD_IO_5_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_5_PIN, gpio_mode_backup[5]);
#endif

#ifdef GPIO_LCD_IO_6_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_6_PIN, gpio_mode_backup[6]);
#endif

#ifdef GPIO_LCD_IO_7_PIN
	set_lcm_gpio_mode(GPIO_LCD_IO_7_PIN, gpio_mode_backup[7]);
#endif

#ifdef GPIO_PWM_BL_PIN
	set_lcm_gpio_mode(GPIO_PWM_BL_PIN, gpio_mode_backup[8]);
#endif
}

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

	params->type = LCM_TYPE_DBI;
	params->ctrl = LCM_CTRL_PARALLEL_DBI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	params->physical_width = PHYSICAL_WIDTH;
	params->physical_height = PHYSICAL_HEIGHT;

	params->dbi.port = 0;
	params->dbi.data_width = LCM_DBI_DATA_WIDTH_8BITS;
	params->dbi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dbi.data_format.trans_seq = LCM_DBI_TRANS_SEQ_MSB_FIRST;
	params->dbi.data_format.padding = LCM_DBI_PADDING_ON_LSB;
	params->dbi.data_format.format = LCM_DBI_FORMAT_RGB666;
	params->dbi.data_format.width = LCM_DBI_DATA_WIDTH_8BITS;
	params->dbi.cpu_write_bits = LCM_DBI_CPU_WRITE_8_BITS;
	params->dbi.io_driving_current = LCM_DRIVING_CURRENT_8MA;

	params->dbi.parallel.write_setup = 1;
	params->dbi.parallel.write_hold = 2;
	params->dbi.parallel.write_wait = 5;
	params->dbi.parallel.read_setup = 2;
	params->dbi.parallel.read_hold = 8;
	params->dbi.parallel.read_latency = 16;
	params->dbi.parallel.wait_period = 0;
	params->dbi.parallel.cs_high_width = 0;

	params->dbi.te_mode = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;
}

static void init_lcm_registers(void)
{
	int i;

	send_ctrl_cmd(0x30);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0xEF);

	send_ctrl_cmd(0x12);

	send_ctrl_cmd(0x35);
	send_data_cmd(0x00);

	send_ctrl_cmd(0x36);
	send_data_cmd(0x00);

	send_ctrl_cmd(0x3A);
	send_data_cmd(0x66);

	send_ctrl_cmd(0x2C);
	for (i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
		send_data_cmd(0xFF);
		send_data_cmd(0xFF);
		send_data_cmd(0xFF);
	}

	send_ctrl_cmd(0xB2);
	send_data_cmd(0x1C);
	send_data_cmd(0x1C);
	send_data_cmd(0x01);
	send_data_cmd(0xFF);
	send_data_cmd(0x33);

	send_ctrl_cmd(0xB3);
	send_data_cmd(0x10);
	send_data_cmd(0xFF);
	send_data_cmd(0x0F);

	send_ctrl_cmd(0xB4);
	send_data_cmd(0x0B);

	send_ctrl_cmd(0xB5);
	send_data_cmd(0x9F);

	send_ctrl_cmd(0xB7);
	send_data_cmd(0x35);

	send_ctrl_cmd(0xBB);
	send_data_cmd(0x28);

	send_ctrl_cmd(0xBC);
	send_data_cmd(0xEC);

	send_ctrl_cmd(0xBD);
	send_data_cmd(0xFE);

	send_ctrl_cmd(0xC0);
	send_data_cmd(0x2C);

	send_ctrl_cmd(0xC2);
	send_data_cmd(0x01);

	send_ctrl_cmd(0xC3);
	send_data_cmd(0x1E);

	send_ctrl_cmd(0xC4);
	send_data_cmd(0x20);

	send_ctrl_cmd(0xC6);
	send_data_cmd(0x0F);

	send_ctrl_cmd(0xD0);
	send_data_cmd(0xA4);
	send_data_cmd(0xA1);

	send_ctrl_cmd(0xE0);
	send_data_cmd(0xD0);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x08);
	send_data_cmd(0x07);
	send_data_cmd(0x05);
	send_data_cmd(0x29);
	send_data_cmd(0x54);
	send_data_cmd(0x41);
	send_data_cmd(0x3C);
	send_data_cmd(0x17);
	send_data_cmd(0x15);
	send_data_cmd(0x1A);
	send_data_cmd(0x20);

	send_ctrl_cmd(0xE1);
	send_data_cmd(0xD0);
	send_data_cmd(0x00);
	send_data_cmd(0x00);
	send_data_cmd(0x08);
	send_data_cmd(0x07);
	send_data_cmd(0x04);
	send_data_cmd(0x29);
	send_data_cmd(0x44);
	send_data_cmd(0x42);
	send_data_cmd(0x3B);
	send_data_cmd(0x16);
	send_data_cmd(0x15);
	send_data_cmd(0x1B);
	send_data_cmd(0x1F);

	send_ctrl_cmd(0x11);
	MDELAY(120);

	send_ctrl_cmd(0x29);
}

static void lcm_init(void)
{
	backup_io_mode();

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(10);

	init_lcm_registers();
}

static void lcm_suspend(void)
{
	send_ctrl_cmd(0x28);
	send_ctrl_cmd(0x10);
	MDELAY(5);

	backup_io_mode();
	set_io_gpio_mode();
}

static void lcm_resume(void)
{
	set_io_lcm_mode();

	send_ctrl_cmd(0x11);
	MDELAY(120);
	send_ctrl_cmd(0x29);
}

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned short x0, y0, x1, y1;
	unsigned short h_X_start, l_X_start, h_X_end, l_X_end, h_Y_start, l_Y_start, h_Y_end, l_Y_end;

	x0 = (unsigned short)x;
	y0 = (unsigned short)y;
	x1 = (unsigned short)x + width - 1;
	y1 = (unsigned short)y + height - 1;

	h_X_start = (x0 & 0xFF00) >> 8;
	l_X_start = x0 & 0x00FF;
	h_X_end = (x1 & 0xFF00) >> 8;
	l_X_end = x1 & 0x00FF;

	h_Y_start = (y0 & 0xFF00) >> 8;
	l_Y_start = y0 & 0x00FF;
	h_Y_end = (y1 & 0xFF00) >> 8;
	l_Y_end = y1 & 0x00FF;

	send_ctrl_cmd(0x2A);
	send_data_cmd(h_X_start);
	send_data_cmd(l_X_start);
	send_data_cmd(h_X_end);
	send_data_cmd(l_X_end);

	send_ctrl_cmd(0x2B);
	send_data_cmd(h_Y_start);
	send_data_cmd(l_Y_start);
	send_data_cmd(h_Y_end);
	send_data_cmd(l_Y_end);

	send_ctrl_cmd(0x2C);
}

static void lcm_setbacklight(unsigned int level)
{
	LCM_PRINT("lcm_setbacklight = %d\n", level);

	if (level > 255)
		level = 255;

	send_ctrl_cmd(0x51);
	send_data_cmd(level);
}

static unsigned int lcm_compare_id(void)
{
	unsigned char read_buf[4];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(10);

	send_ctrl_cmd(0x04);
	read_buf[0] = read_data_cmd();	/* dummy read */
	read_buf[1] = read_data_cmd();
	read_buf[2] = read_data_cmd();
	read_buf[3] = read_data_cmd();

	LCM_PRINT("[ST] ID : 0x%X, 0x%X, 0x%X\n", read_buf[1], read_buf[2], read_buf[3]);

	return ((LCM_ID0 == read_buf[1]) && (LCM_ID1 == read_buf[2]) && (LCM_ID2 == read_buf[3])) ? 1 : 0;
}

#ifdef ESD_SUPPORT

static unsigned int lcm_esd_check(void)
{
	unsigned int readData;
	unsigned int result = TRUE;

	send_ctrl_cmd(0x0A);
	readData = read_data_cmd();	/* dummy read */
	readData = read_data_cmd();

	if ((readData & 0x97) == 0x94)
		result = FALSE;

	return result;
}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();

	return TRUE;
}

#endif

static unsigned int lcm_ata_check(unsigned char *buffer)
{
	unsigned int ret = 0;
	unsigned char read_buf[4];

	send_ctrl_cmd(0x04);
	read_buf[0] = read_data_cmd();	/* dummy read */
	read_buf[1] = read_data_cmd();
	read_buf[2] = read_data_cmd();
	read_buf[3] = read_data_cmd();

	LCM_PRINT("ID = 0x%x, 0x%x, 0x%x\n", read_buf[1], read_buf[2], read_buf[3]);

	if ((read_buf[1] == LCM_ID0) && (read_buf[2] == LCM_ID1) && (read_buf[3] == LCM_ID2))
		ret = 1;
	else
		ret = 0;

	return ret;
}

static void lcm_enter_idle(void)
{
	send_ctrl_cmd(0x39);

#ifdef GPIO_PWM_BL_PIN
	if (is_io_backup == 0)
		gpio_mode_backup[8] = get_lcm_gpio_mode(GPIO_PWM_BL_PIN);
	set_lcm_gpio_output_low(GPIO_PWM_BL_PIN);
#endif
}

static void lcm_exit_idle(void)
{
#ifdef GPIO_PWM_BL_PIN
	set_lcm_gpio_mode(GPIO_PWM_BL_PIN, gpio_mode_backup[8]);
#endif

	send_ctrl_cmd(0x38);
}

static void lcm_change_fps(unsigned int mode)
{
	if (mode == 0) {	/* slowdown frame rate in idle mode */
		send_ctrl_cmd(0xB2);
		send_data_cmd(0x1C);
		send_data_cmd(0x1C);
		send_data_cmd(0x01);
		send_data_cmd(0xFF);
		send_data_cmd(0x33);

		send_ctrl_cmd(0xB3);
		send_data_cmd(0x10);
		send_data_cmd(0xFF);
		send_data_cmd(0x0F);

		send_ctrl_cmd(0xBC);
		send_data_cmd(0xEC);
	} else {		/* normal frame rate in idle mode */

		send_ctrl_cmd(0xB2);
		send_data_cmd(0x1C);
		send_data_cmd(0x1C);
		send_data_cmd(0x01);
		send_data_cmd(0x33);
		send_data_cmd(0x33);

		send_ctrl_cmd(0xB3);
		send_data_cmd(0x10);
		send_data_cmd(0x0F);
		send_data_cmd(0x0F);

		send_ctrl_cmd(0xBC);
		send_data_cmd(0xED);
	}
}

/* --------------------------------------------------------------------------- */
/* Get LCM Driver Hooks */
/* --------------------------------------------------------------------------- */
LCM_DRIVER st7789h2_dbi_lcm_drv = {
	.name = "st7789h2_dbi",
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
	.ata_check = lcm_ata_check,
	.enter_idle = lcm_enter_idle,
	.exit_idle = lcm_exit_idle,
	.change_fps = lcm_change_fps,
};
