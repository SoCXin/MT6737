/*
 * (C) Copyright 2008
 * MediaTek <www.mediatek.com>
 * Infinity Chen <infinity.chen@mediatek.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

/*=======================================================================*/
/* HEADER FILES                                                          */
/*=======================================================================*/

#include <video_fb.h>
#include <platform/disp_drv_platform.h>
#include <target/board.h>
#include <platform/env.h>

#include "lcm_drv.h"
#include <string.h>
#include <stdlib.h>
#include <platform/mt_gpt.h>
#include <platform/primary_display.h>
#include <arch/arm/mmu.h>
// ---------------------------------------------------------------------------
//  Export Functions - Display
// ---------------------------------------------------------------------------
unsigned long long  fb_addr_pa_k    = 0;
static void  *fb_addr_pa      = NULL;
static void  *fb_addr      = NULL;
static void  *logo_db_addr = NULL;
static void  *logo_db_addr_pa = NULL;
static UINT32 fb_size      = 0;
static UINT32 fb_offset_logo = 0; // counter of fb_size
static UINT32 fb_isdirty   = 0;
static UINT32 redoffset_32bit = 1; // ABGR

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
extern LCM_PARAMS *lcm_params;


UINT32 mt_disp_get_vram_size(void)
{
	return DISP_GetVRamSize();
}

extern void disp_log_enable(int enable);
extern void dbi_log_enable(int enable);
extern void * memset(void *,int,unsigned int);
#if 0
static disp_dfo_item_t disp_dfo_setting[] = {
	{"LCM_FAKE_WIDTH",  0},
	{"LCM_FAKE_HEIGHT", 0},
	{"DISP_DEBUG_SWITCH",   0}
};

#define MT_DISP_DFO_DEBUG
#ifdef MT_DISP_DFO_DEBUG
#define disp_dfo_printf(string, args...) dprintf(INFO,"[DISP_DFO]"string, ##args)
#else
#define disp_dfo_printf(string, args...) ()
#endif

unsigned int mt_disp_parse_dfo_setting(void)
{
	unsigned int i, j=0 ;
	char tmp[11];
	char *buffer = NULL;
	char *ptr = NULL;

	buffer = (char *)get_env("DFO");
	disp_dfo_printf("env buffer = %s\n", buffer);

	if (buffer != NULL) {
		for (i = 0; i< (sizeof(disp_dfo_setting)/sizeof(disp_dfo_item_t)); i++) {
			j = 0;

			memset((void*)tmp, 0, sizeof(tmp)/sizeof(tmp[0]));

			ptr = strstr(buffer, disp_dfo_setting[i].name);

			if (ptr == NULL) continue;

			disp_dfo_printf("disp_dfo_setting[%d].name = [%s]\n", i, ptr);

			do {} while ((*ptr++) != ',');

			do {tmp[j++] = *ptr++;}
			while (*ptr != ',' && j < sizeof(tmp)/sizeof(tmp[0]));

			disp_dfo_setting[i].value = atoi((const char*)tmp);

			disp_dfo_printf("disp_dfo_setting[%d].name = [%s|%d]\n", i, tmp, disp_dfo_setting[i].value);
		}
	} else {
		disp_dfo_printf("env buffer = NULL\n");
	}

	return 0;
}


int mt_disp_get_dfo_setting(const char *string, unsigned int *value)
{
	char *disp_name;
	int  disp_value;
	unsigned int i = 0;

	if (string == NULL)
		return -1;

	for (i=0; i<(sizeof(disp_dfo_setting)/sizeof(disp_dfo_item_t)); i++) {
		disp_name = disp_dfo_setting[i].name;
		disp_value = disp_dfo_setting[i].value;
		if (!strcmp(disp_name, string)) {
			*value = disp_value;
			disp_dfo_printf("%s = [DEC]%d [HEX]0x%08x\n", disp_name, disp_value, disp_value);
			return 0;
		}
	}

	return 0;
}
#endif
#define FB_LAYER            0
#define BOOT_MENU_LAYER     3

static void _mtkfb_draw_block(unsigned int addr, unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int color)
{
	int i = 0;
	int j = 0;
	unsigned int start_addr = addr+ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*4*y+x*4;
	for (j=0; j<h; j++) {
		for (i = 0; i<w; i++) {
			*(unsigned int*)(start_addr + i*4 + j*ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*4) = color;
		}
	}
}


static int _mtkfb_internal_test(unsigned int va, unsigned int w, unsigned int h)
{
	// this is for debug, used in bring up day
	unsigned int i = 0;
	unsigned int color = 0;
	int _internal_test_block_size =240;
	for (i=0; i<w*h/_internal_test_block_size/_internal_test_block_size; i++) {
		color = (i&0x1)*0xff;
		//color += ((i&0x2)>>1)*0xff00;
		//color += ((i&0x4)>>2)*0xff0000;
		color += 0xff000000U;
		_mtkfb_draw_block(va,
		                  i%(w/_internal_test_block_size)*_internal_test_block_size,
		                  i/(w/_internal_test_block_size)*_internal_test_block_size,
		                  _internal_test_block_size,
		                  _internal_test_block_size,
		                  color);
	}
	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	mdelay(100);
	primary_display_diagnose();
	return 0;
/*
	_internal_test_block_size = 20;
	for (i=0; i<w*h/_internal_test_block_size/_internal_test_block_size; i++) {
		color = (i&0x1)*0xff;
		color += ((i&0x2)>>1)*0xff00;
		color += ((i&0x4)>>2)*0xff0000;
		color += 0xff000000U;
		_mtkfb_draw_block(va,
		                  i%(w/_internal_test_block_size)*_internal_test_block_size,
		                  i/(w/_internal_test_block_size)*_internal_test_block_size,
		                  _internal_test_block_size,
		                  _internal_test_block_size,
		                  color);
	}

	mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	mdelay(100);
	primary_display_diagnose();
	return 0;*/
}


void mt_disp_init(void *lcdbase)
{
	unsigned int lcm_fake_width = 0;
	unsigned int lcm_fake_height = 0;
	UINT32 boot_mode_addr = 0;
	/// fb base pa and va
	fb_addr_pa_k = arm_mmu_va2pa(lcdbase);

	fb_addr_pa   = fb_addr_pa_k & 0xffffffffull;
	fb_addr      = lcdbase;

	dprintf(0,"fb_va: 0x%08x, fb_pa: 0x%08x, fb_pa_k: 0x%llx\n", fb_addr, fb_addr_pa, fb_addr_pa_k);

	fb_size = ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT) * CFG_DISPLAY_HEIGHT * CFG_DISPLAY_BPP / 8;
	// pa;
	boot_mode_addr = ((UINT32)fb_addr_pa + fb_size);
	logo_db_addr_pa = (void *)((UINT32)fb_addr_pa - 4 * 1024 * 1024);

	// va;
	logo_db_addr = (void *)((UINT32)fb_addr - 4 * 1024 * 1024);

	fb_offset_logo = 0;

	primary_display_init(NULL);
	memset((void*)lcdbase, 0x0, DISP_GetVRamSize());

	disp_input_config input;
	memset(&input, 0, sizeof(disp_input_config));
	input.layer     = BOOT_MENU_LAYER;
	input.layer_en  = 1;
	input.fmt       = redoffset_32bit ? eBGRA8888 : eRGBA8888;
	input.addr      = boot_mode_addr;
	input.src_x     = 0;
	input.src_y     = 0;
	input.src_w     = CFG_DISPLAY_WIDTH;
	input.src_h     = CFG_DISPLAY_HEIGHT;
	input.src_pitch = CFG_DISPLAY_WIDTH*4;
	input.dst_x     = 0;
	input.dst_y     = 0;
	input.dst_w     = CFG_DISPLAY_WIDTH;
	input.dst_h     = CFG_DISPLAY_HEIGHT;
	input.aen       = 1;
	input.alpha     = 0xff;

	primary_display_config_input(&input);

	memset(&input, 0, sizeof(disp_input_config));
	input.layer     = FB_LAYER;
	input.layer_en  = 1;
	input.fmt       = redoffset_32bit ? eBGRA8888 : eRGBA8888;
	input.addr      = fb_addr_pa;
	input.src_x     = 0;
	input.src_y     = 0;
	input.src_w     = CFG_DISPLAY_WIDTH;
	input.src_h     = CFG_DISPLAY_HEIGHT;
	input.src_pitch = ALIGN_TO(CFG_DISPLAY_WIDTH, MTK_FB_ALIGNMENT)*4;
	input.dst_x     = 0;
	input.dst_y     = 0;
	input.dst_w     = CFG_DISPLAY_WIDTH;
	input.dst_h     = CFG_DISPLAY_HEIGHT;

	input.aen       = 1;
	input.alpha     = 0xff;
	primary_display_config_input(&input);


	//_mtkfb_internal_test(fb_addr, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);

#if 0
	mt_disp_parse_dfo_setting();

	if ((0 == mt_disp_get_dfo_setting("LCM_FAKE_WIDTH", &lcm_fake_width)) && (0 == mt_disp_get_dfo_setting("LCM_FAKE_HEIGHT", &lcm_fake_height))) {
		if (DISP_STATUS_OK != DISP_Change_LCM_Resolution(lcm_fake_width, lcm_fake_height)) {
			dprintf(INFO,"[DISP_DFO]WARNING!!! Change LCM Resolution FAILED!!!\n");
		}
	}
#endif

}


void mt_disp_power(BOOL on)
{
	return;
/*#ifndef CFG_MT6577_FPGA
	if (on) {
//		disp_path_ddp_clock_on();
		DISP_PowerEnable(TRUE);
		DISP_PanelEnable(TRUE);
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	} else {
		DISP_PanelEnable(FALSE);
		DISP_PowerEnable(FALSE);
//		disp_path_ddp_clock_off();
	}
#endif*/
}


void* mt_get_logo_db_addr(void)
{
	dprintf(0,"mt_get_logo_db_addr: 0x%08x\n",logo_db_addr);
	return logo_db_addr;
}

void* mt_get_logo_db_addr_pa(void)
{
	dprintf(0,"mt_get_logo_db_addr_pa: 0x%08x\n",logo_db_addr_pa);
	return logo_db_addr_pa;
}

void* mt_get_fb_addr(void)
{
	fb_isdirty = 1;
	return (void*)((UINT32)fb_addr + fb_offset_logo * fb_size);
}

void* mt_get_tempfb_addr(void)
{
	//use offset = 2 as tempfb for decompress logo
	dprintf(0,"mt_get_tempfb_addr: 0x%08x\n",(void*)((UINT32)fb_addr + 2*fb_size));
	return (void*)((UINT32)fb_addr + 2*fb_size);
}

UINT32 mt_get_fb_size(void)
{
	return fb_size;
}

extern void arch_clean_cache_range(addr_t start, size_t len);
void mt_disp_update(UINT32 x, UINT32 y, UINT32 width, UINT32 height)
{
	{

		unsigned int va = fb_addr;
		dprintf(0,"fb dump: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", *(unsigned int*)va, *(unsigned int*)(va+4), *(unsigned int*)(va+8), *(unsigned int*)(va+0xC));
	}
	arch_clean_cache_range((unsigned int)fb_addr, DISP_GetFBRamSize());
	primary_display_trigger(TRUE);
	if (!primary_display_is_video_mode()) {
		/*video mode no need to wait*/
		mdelay(30);
	}

	/*
	    // TODO: Fixit!!!!!
	    if(fb_isdirty)
	    {
	        fb_isdirty = 0;
	        MASKREG32(0x1400E000, 0x1, 0x1); //Enable DISP MUTEX0
	        MASKREG32(0x1400E004, 0x1, 0x0);
	        LCD_CHECK_RET(LCD_LayerSetAddress(FB_LAYER - 1, (UINT32)fb_addr + fb_offset_logo * fb_size));
	        printk("[wwy] hardware address = %x, fb_offset_logo = %d\n",(UINT32)fb_addr + fb_offset_logo * fb_size,fb_offset_logo);
	        arch_clean_cache_range((unsigned int)fb_addr, DISP_GetFBRamSize());
	        DISP_CHECK_RET(DISP_UpdateScreen(x, y, width, height));
	        //wait reg update to set fb_offset_logo
	        DISP_WaitRegUpdate();
	        fb_offset_logo = fb_offset_logo ? 0 : 3;

	    }
	    else
	    {
	    arch_clean_cache_range((unsigned int)fb_addr, DISP_GetFBRamSize());
	    DISP_CHECK_RET(DISP_UpdateScreen(x, y, width, height));
	    }
	    */
}


void mt_disp_wait_idle(void)
{
	if (lcm_params->type==LCM_TYPE_DPI) {
		DPI_WaitVsync();
	} else if (lcm_params->type==LCM_TYPE_DBI) {
		LCD_WaitForNotBusy();
	} else if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode!=CMD_MODE) {
		DSI_WaitVsync();
		DSI_WaitVsync();
	} else if (lcm_params->type==LCM_TYPE_DSI && lcm_params->dsi.mode ==CMD_MODE) {
		DSI_WaitForNotBusy();
	}
}
static void mt_disp_adjusting_hardware_addr(void)
{
	dprintf(0,"[wwy] mt_disp_adjusting_hardware_addr fb_offset_logo = %d\n",fb_offset_logo);
	if (fb_offset_logo == 0) {
		mt_get_fb_addr();
		memcpy(fb_addr,(void *)((UINT32)fb_addr + 3 * fb_size),fb_size);
		mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
	}
}
UINT32 mt_disp_get_lcd_time(void)
{
	unsigned int fps = 0;
	// TODO: will enable after te enabled
	fps = primary_display_get_vsync_interval();
	dprintf(CRITICAL, "%s, fps=%d\n", __func__, fps);
	if (fps)
		return fps;
	else
		return 6000;
}

// Attention: this api indicates whether the lcm is connected
int DISP_IsLcmFound(void)
{
	return primary_display_is_lcm_connected();
}

const char* mt_disp_get_lcm_id(void)
{
	return primary_display_get_lcm_name();
}

void disp_get_fb_address(UINT32 *fbVirAddr, UINT32 *fbPhysAddr)
{
	*fbVirAddr = (UINT32)fb_addr;
	*fbPhysAddr = (UINT32)fb_addr;
}

UINT32 mt_disp_get_redoffset_32bit(void)
{
	return redoffset_32bit;
}


// ---------------------------------------------------------------------------
//  Export Functions - Console
// ---------------------------------------------------------------------------

#ifdef CONFIG_CFB_CONSOLE
//  video_hw_init -- called by drv_video_init() for framebuffer console

extern UINT32 memory_size(void);

void *video_hw_init (void)
{
	static GraphicDevice s_mt65xx_gd;

	memset(&s_mt65xx_gd, 0, sizeof(GraphicDevice));
	//xuecheng, use new calculate formula;
	s_mt65xx_gd.frameAdrs  = (UINT32)fb_addr+fb_size;//CFG_DISPLAY_WIDTH*2*(CFG_DISPLAY_HEIGHT-80);//fb_size;//memory_size() - mt_disp_get_vram_size() + fb_size;
	s_mt65xx_gd.winSizeX   = CFG_DISPLAY_WIDTH;
	s_mt65xx_gd.winSizeY   = CFG_DISPLAY_HEIGHT;
	s_mt65xx_gd.gdfIndex   = CFB_X888RGB_32BIT;
	dprintf(0, "s_mt65xx_gd.gdfIndex=%d", s_mt65xx_gd.gdfIndex);
	s_mt65xx_gd.gdfBytesPP = CFG_DISPLAY_BPP / 8;
	s_mt65xx_gd.memSize    = s_mt65xx_gd.winSizeX * s_mt65xx_gd.winSizeY * s_mt65xx_gd.gdfBytesPP;

	return &s_mt65xx_gd;
}


void video_set_lut(unsigned int index,  /* color number */
                   unsigned char r,     /* red */
                   unsigned char g,     /* green */
                   unsigned char b)     /* blue */
{
}
#endif  // CONFIG_CFB_CONSOLE
