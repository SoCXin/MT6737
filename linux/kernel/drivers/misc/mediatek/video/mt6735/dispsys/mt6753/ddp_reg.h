#ifndef _DDP_REG_H_
#define _DDP_REG_H_
#include <mt-plat/sync_write.h>
/* #include <mach/mt_reg_base.h> */
#include <linux/types.h>
#include "display_recorder.h"
#include "cmdq_record.h"
#include "cmdq_core.h"
#include "ddp_hal.h"
#include "ddp_log.h"
#include "ddp_path.h"

/* MIPITX and DSI */
#define ENABLE_CLK_MGR


#define DDP_ENING_NUM    (13)

#define DDP_MOUT_NUM     4
#define DDP_SEL_OUT_NUM  2
#define DDP_SEL_IN_NUM   5
#define DDP_MUTEX_MAX    5

#ifdef DISP_LIST_SCENARIO
static unsigned int module_list_scenario[DDP_SCENARIO_MAX][DDP_ENING_NUM] = {
/*PRIMARY_DISP*/
	{DISP_MODULE_OVL0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,
	 DISP_MODULE_GAMMA, DISP_MODULE_OD,
	 DISP_MODULE_DITHER, DISP_MODULE_RDMA0, DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1, -1, -1},
/*PRIMARY_RDMA0_COLOR0_DISP*/
	{DISP_MODULE_RDMA0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,
	 DISP_MODULE_GAMMA, DISP_MODULE_OD,
	 DISP_MODULE_DITHER, DISP_MODULE_UFOE, DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1, -1, -1},
/*PRIMARY_RDMA0_DISP*/
	{DISP_MODULE_RDMA0, DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1},
/*PRIMARY_BYPASS_RDMA*/
	{DISP_MODULE_OVL0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,
	 DISP_MODULE_GAMMA, DISP_MODULE_OD,
	 DISP_MODULE_DITHER, DISP_MODULE_UFOE, DISP_MODULE_PWM0, DISP_MODULE_DSI0, -1, -1, -1},
/*PRIMARY_OVL_MEMOUT*/
	{DISP_MODULE_OVL0, DISP_MODULE_WDMA0, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1},
/*PRIMARY_DITHER_MEMOUT*/
	{DISP_MODULE_OVL0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,
	 DISP_MODULE_GAMMA, DISP_MODULE_OD,
	 DISP_MODULE_DITHER, DISP_MODULE_WDMA0, -1, -1, -1, -1, -1},
/*PRIMARY_UFOE_MEMOUT*/
	{DISP_MODULE_OVL0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,
	 DISP_MODULE_GAMMA, DISP_MODULE_OD,
	 DISP_MODULE_DITHER, DISP_MODULE_RDMA0, DISP_MODULE_UFOE, DISP_MODULE_WDMA0, -1, -1, -1},
/*SUB_DISP*/
	{DISP_MODULE_OVL1, DISP_MODULE_RDMA1, DISP_MODULE_DPI, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1},
#ifndef CONFIG_FPGA_EARLY_PORTING
/*SUB_RDMA1_DISP*/
	{DISP_MODULE_RDMA1, DISP_MODULE_DPI, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1},
#else
/*SUB_RDMA1_DISP*/
	{DISP_MODULE_RDMA1, DISP_MODULE_DSI0, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1},
#endif
/*SUB_OVL_MEMOUT*/
	{DISP_MODULE_OVL1, DISP_MODULE_WDMA1, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1},
/*PRIMARY_ALL*/
	{DISP_MODULE_OVL0, DISP_MODULE_WDMA0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR,
	 DISP_MODULE_AAL, DISP_MODULE_GAMMA,
	 DISP_MODULE_OD, DISP_MODULE_DITHER, DISP_MODULE_RDMA0, DISP_MODULE_PWM0,
	 DISP_MODULE_DSI0, -1, -1},
/*SUB_ALL*/
	{DISP_MODULE_OVL1, DISP_MODULE_WDMA1, DISP_MODULE_RDMA1, DISP_MODULE_DPI, -1,
	 -1, -1, -1, -1, -1, -1, -1},
/*MULTIPLE_OVL*/
	{DISP_MODULE_OVL1, DISP_MODULE_OVL0, -1, -1, -1,
	 -1, -1, -1, -1, -1, -1, -1},
};
#endif


/* 1st para is mout's input, 2nd para is mout's output */
extern mout_t mout_map[DDP_MOUT_NUM];


extern sel_t sel_out_map[DDP_SEL_OUT_NUM];

/* 1st para is sout's output, 2nd para is sout's input */
extern sel_t sel_in_map[DDP_SEL_IN_NUM];


/* from DTS, for debug */
static const unsigned int ddp_reg_pa_base[DISP_REG_NUM] = {
	0x14007000, 0x14008000, 0x14009000, 0x1400A000,
	0x1400B000, 0x1400C000, 0x1400D000, 0x1400E000,
	0x1400F000, 0x14010000, 0, 0x1100E000,
	0, 0x14015000, 0x14013000, 0x14014000,
	0x14000000, 0x14016000, 0x14017000, 0x14018000,
	0x10206000, 0x10210000, 0x10211A70, 0x10211974,
	0x10211B70, 0x10206044, 0x10206514, 0x10206558,
	0x102100A0, 0x10209270, 0x10209274, 0x14012000,
	0x10209000
};

static const unsigned int ddp_irq_num[DISP_REG_NUM] = {
	193, 211, 194, 195,
	196, 197, 198, 199,
	200, 201, 0, 117,
	0, 186, 203, 204,
	205, 176, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 210,
	0
};

/* module bit in mutex */
static const module_map_t module_mutex_map[DISP_MODULE_NUM] = {
	{DISP_MODULE_OVL0, 6},
	{DISP_MODULE_OVL1, 7},
	{DISP_MODULE_RDMA0, 8},
	{DISP_MODULE_RDMA1, 9},
	{DISP_MODULE_WDMA0, 10},
	{DISP_MODULE_COLOR0, 12},
	{DISP_MODULE_CCORR, 11},
	{DISP_MODULE_AAL, 13},
	{DISP_MODULE_GAMMA, 14},
	{DISP_MODULE_DITHER, 15},
	{DISP_MODULE_UFOE, -1},
	{DISP_MODULE_PWM0, 17},
	{DISP_MODULE_WDMA1, -1},
	{DISP_MODULE_DSI0, -1},
	{DISP_MODULE_DPI, -1},
	{DISP_MODULE_SMI, -1},
	{DISP_MODULE_CONFIG, -1},
	{DISP_MODULE_CMDQ, -1},
	{DISP_MODULE_MUTEX, -1},
	{DISP_MODULE_COLOR1, -1},
	{DISP_MODULE_RDMA2, -1},
	{DISP_MODULE_PWM1, -1},
	{DISP_MODULE_OD, 18},
};


/* module can be connect if 1 */
static const module_map_t module_can_connect[DISP_MODULE_NUM] = {
	{DISP_MODULE_OVL0, 1},
	{DISP_MODULE_OVL1, 1},
	{DISP_MODULE_RDMA0, 1},
	{DISP_MODULE_RDMA1, 1},
	{DISP_MODULE_WDMA0, 1},
	{DISP_MODULE_COLOR0, 1},
	{DISP_MODULE_CCORR, 1},
	{DISP_MODULE_AAL, 1},
	{DISP_MODULE_GAMMA, 1},
	{DISP_MODULE_DITHER, 1},
	{DISP_MODULE_UFOE, 1},
	{DISP_MODULE_PWM0, 0},
	{DISP_MODULE_WDMA1, 0},
	{DISP_MODULE_DSI0, 1},
	{DISP_MODULE_DPI, 1},
	{DISP_MODULE_SMI, 0},
	{DISP_MODULE_CONFIG, 0},
	{DISP_MODULE_CMDQ, 0},
	{DISP_MODULE_MUTEX, 0},
	{DISP_MODULE_COLOR1, 0},
	{DISP_MODULE_RDMA2, 0},
	{DISP_MODULE_PWM1, 0},
	{DISP_MODULE_OD, 1},
	{DISP_MODULE_MERGE, 0},
	{DISP_MODULE_SPLIT0, 0},
	{DISP_MODULE_SPLIT1, 0},
	{DISP_MODULE_DSI1, 0},
	{DISP_MODULE_DSIDUAL, 0},
	{DISP_MODULE_SMI_LARB0, 0},
	{DISP_MODULE_SMI_COMMON, 0},
	{DISP_MODULE_UNKNOWN, 0},
};


struct MIPITX_DSI_CON_REG {
	unsigned RG_DSI_LDOCORE_EN:1;
	unsigned RG_DSI_CKG_LDOOUT_EN:1;
	unsigned RG_DSI_BCLK_SEL:2;
	unsigned RG_DSI_LD_IDX_SEL:3;
	unsigned rsv_7:1;
	unsigned RG_DSI_PHYCLK_SEL:2;
	unsigned RG_DSI_DSICLK_FREQ_SEL:1;
	unsigned RG_DSI_LPTX_CLMP_EN:1;
	unsigned rsv_12:20;
};


struct MIPITX_DSI_CLOCK_LANE_REG {
	unsigned RG_DSI_LNTC_LDOOUT_EN:1;
	unsigned RG_DSI_LNTC_LOOPBACK_EN:1;
	unsigned RG_DSI_LNTC_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNTC_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNTC_LPTX_IMINUS:1;
	unsigned RG_DSI_LNTC_PHY_SEL:1;
	unsigned rsv_6:2;
	unsigned RG_DSI_LNTC_RT_CODE:4;
	unsigned rsv_12:20;
};


struct MIPITX_DSI_DATA_LANE0_REG {
	unsigned RG_DSI_LNT0_LDOOUT_EN:1;
	unsigned RG_DSI_LNT0_LOOPBACK_EN:1;
	unsigned RG_DSI_LNT0_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT0_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT0_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT0_LPCD_IPLUS:1;
	unsigned RG_DSI_LNT0_LPCD_IMINUS:1;
	unsigned RG_DSI_LNT0_RT_CODE:4;
	unsigned rsv_11:21;
};


struct MIPITX_DSI_DATA_LANE1_REG {
	unsigned RG_DSI_LNT1_LDOOUT_EN:1;
	unsigned RG_DSI_LNT1_LOOPBACK_EN:1;
	unsigned RG_DSI_LNT1_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT1_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT1_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT1_RT_CODE:4;
	unsigned rsv_9:23;
};


struct MIPITX_DSI_DATA_LANE2_REG {
	unsigned RG_DSI_LNT2_LDOOUT_EN:1;
	unsigned RG_DSI_LNT2_LOOPBACK_EN:1;
	unsigned RG_DSI_LNT2_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT2_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT2_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT2_RT_CODE:4;
	unsigned rsv_9:23;
};

struct MIPITX_DSI_DATA_LANE3_REG {
	unsigned RG_DSI_LNT3_LDOOUT_EN:1;
	unsigned RG_DSI_LNT3_LOOPBACK_EN:1;
	unsigned RG_DSI_LNT3_LPTX_IPLUS1:1;
	unsigned RG_DSI_LNT3_LPTX_IPLUS2:1;
	unsigned RG_DSI_LNT3_LPTX_IMINUS:1;
	unsigned RG_DSI_LNT3_RT_CODE:4;
	unsigned rsv_9:23;
};

struct MIPITX_DSI_TOP_CON_REG {
	unsigned RG_DSI_LNT_INTR_EN:1;
	unsigned RG_DSI_LNT_HS_BIAS_EN:1;
	unsigned RG_DSI_LNT_IMP_CAL_EN:1;
	unsigned RG_DSI_LNT_TESTMODE_EN:1;
	unsigned RG_DSI_LNT_IMP_CAL_CODE:4;
	unsigned RG_DSI_LNT_AIO_SEL:3;
	unsigned RG_DSI_PAD_TIE_LOW_EN:1;
	unsigned RG_DSI_DEBUG_INPUT_EN:1;
	unsigned RG_DSI_PRESERVE:3;
	unsigned rsv_16:16;
};


struct MIPITX_DSI_BG_CON_REG {
	unsigned RG_DSI_BG_CORE_EN:1;
	unsigned RG_DSI_BG_CKEN:1;
	unsigned RG_DSI_BG_DIV:2;
	unsigned RG_DSI_BG_FAST_CHARGE:1;
	unsigned RG_DSI_V12_SEL:3;
	unsigned RG_DSI_V10_SEL:3;
	unsigned RG_DSI_V072_SEL:3;
	unsigned RG_DSI_V04_SEL:3;
	unsigned RG_DSI_V032_SEL:3;
	unsigned RG_DSI_V02_SEL:3;
	unsigned rsv_23:1;
	unsigned RG_DSI_BG_R1_TRIM:4;
	unsigned RG_DSI_BG_R2_TRIM:4;
};


struct MIPITX_DSI_PLL_CON0_REG {
	unsigned RG_DSI0_MPPLL_PLL_EN:1;
	unsigned RG_DSI0_MPPLL_PREDIV:2;
	unsigned RG_DSI0_MPPLL_TXDIV0:2;
	unsigned RG_DSI0_MPPLL_TXDIV1:2;
	unsigned RG_DSI0_MPPLL_POSDIV:3;
	unsigned RG_DSI0_MPPLL_MONVC_EN:1;
	unsigned RG_DSI0_MPPLL_MONREF_EN:1;
	unsigned RG_DSI0_MPPLL_VDO_EN:1;
	unsigned rsv_13:19;
};


struct MIPITX_DSI_PLL_CON1_REG {
	unsigned RG_DSI0_MPPLL_SDM_FRA_EN:1;
	unsigned RG_DSI0_MPPLL_SDM_SSC_PH_INIT:1;
	unsigned RG_DSI0_MPPLL_SDM_SSC_EN:1;
	unsigned rsv_3:13;
	unsigned RG_DSI0_MPPLL_SDM_SSC_PRD:16;
};

struct MIPITX_DSI_PLL_CON2_REG {
	unsigned RG_DSI0_MPPLL_SDM_PCW_0_7:8;
	unsigned RG_DSI0_MPPLL_SDM_PCW_8_15:8;
	unsigned RG_DSI0_MPPLL_SDM_PCW_16_23:8;
	unsigned RG_DSI0_MPPLL_SDM_PCW_H:7;
	unsigned rsv_31:1;
};


struct MIPITX_DSI_PLL_CON3_REG {
	unsigned RG_DSI0_MPPLL_SDM_SSC_DELTA1:16;
	unsigned RG_DSI0_MPPLL_SDM_SSC_DELTA:16;
};


struct MIPITX_DSI_PLL_CHG_REG {
	unsigned RG_DSI0_MPPLL_SDM_PCW_CHG:1;
	unsigned rsv_1:31;
};


struct MIPITX_DSI_PLL_TOP_REG {
	unsigned RG_MPPLL_TST_EN:1;
	unsigned RG_MPPLL_TSTCK_EN:1;
	unsigned RG_MPPLL_TSTSEL:2;
	unsigned RG_MPPLL_S2QDIV:2;
	unsigned RG_MPPLL_PLLOUT_EN:1;
	unsigned RG_MPPLL_PRESERVE:5;
	unsigned rsv_12:20;
};


struct MIPITX_DSI_PLL_PWR_REG {
	unsigned DA_DSI_MPPLL_SDM_PWR_ON:1;
	unsigned DA_DSI_MPPLL_SDM_ISO_EN:1;
	unsigned rsv_2:6;
	unsigned AD_DSI0_MPPLL_SDM_PWR_ACK:1;
	unsigned rsv_9:23;
};

struct MIPITX_DSI_RGS_REG {
	unsigned RGS_DSI_LNT_IMP_CAL_OUTPUT:1;
	unsigned rsv_1:31;
};


struct MIPITX_DSI_GPI_EN_REG {
	unsigned RG_DSI0_GPI0_EN:1;
	unsigned RG_DSI0_GPI1_EN:1;
	unsigned RG_DSI0_GPI2_EN:1;
	unsigned RG_DSI0_GPI3_EN:1;
	unsigned RG_DSI0_GPI4_EN:1;
	unsigned RG_DSI0_GPI5_EN:1;
	unsigned RG_DSI0_GPI6_EN:1;
	unsigned RG_DSI0_GPI7_EN:1;
	unsigned RG_DSI0_GPI8_EN:1;
	unsigned RG_DSI0_GPI9_EN:1;
	unsigned RG_DSI0_GPI_SMT_EN:1;
	unsigned RG_DSI0_GPI_DRIVE_EN:1;
	unsigned rsv_12:20;
};

struct MIPITX_DSI_GPI_PULL_REG {
	unsigned RG_DSI0_GPI0_PD:1;
	unsigned RG_DSI0_GPI1_PD:1;
	unsigned RG_DSI0_GPI2_PD:1;
	unsigned RG_DSI0_GPI3_PD:1;
	unsigned RG_DSI0_GPI4_PD:1;
	unsigned RG_DSI0_GPI5_PD:1;
	unsigned RG_DSI0_GPI6_PD:1;
	unsigned RG_DSI0_GPI7_PD:1;
	unsigned RG_DSI0_GPI8_PD:1;
	unsigned RG_DSI0_GPI9_PD:1;
	unsigned rsv_10:6;
	unsigned RG_DSI0_GPI0_PU:1;
	unsigned RG_DSI0_GPI1_PU:1;
	unsigned RG_DSI0_GPI2_PU:1;
	unsigned RG_DSI0_GPI3_PU:1;
	unsigned RG_DSI0_GPI4_PU:1;
	unsigned RG_DSI0_GPI5_PU:1;
	unsigned RG_DSI0_GPI6_PU:1;
	unsigned RG_DSI0_GPI7_PU:1;
	unsigned RG_DSI0_GPI8_PU:1;
	unsigned RG_DSI0_GPI9_PU:1;
	unsigned rsv_26:6;
};


struct MIPITX_DSI_PHY_SEL_REG {
	unsigned MIPI_TX_PHY0_SEL:3;
	unsigned rsv_3:1;
	unsigned MIPI_TX_PHY1_SEL:3;
	unsigned rsv_7:1;
	unsigned MIPI_TX_PHY2_SEL:3;
	unsigned rsv_11:1;
	unsigned MIPI_TX_PHY3_SEL:3;
	unsigned rsv_15:1;
	unsigned MIPI_TX_PHYC_SEL:3;
	unsigned rsv_19:1;
	unsigned MIPI_TX_LPRX_SEL:3;
	unsigned rsv_23:9;
};


struct MIPITX_DSI_SW_CTRL_REG {
	unsigned SW_CTRL_EN:1;
	unsigned rsv_1:31;
};


struct MIPITX_DSI_SW_CTRL_CON0_REG {
	unsigned SW_LNTC_LPTX_PRE_OE:1;
	unsigned SW_LNTC_LPTX_OE:1;
	unsigned SW_LNTC_LPTX_DP:1;
	unsigned SW_LNTC_LPTX_DN:1;
	unsigned SW_LNTC_HSTX_PRE_OE:1;
	unsigned SW_LNTC_HSTX_OE:1;
	unsigned SW_LNTC_HSTX_RDY:1;
	unsigned SW_LNTC_LPRX_EN:1;
	unsigned SW_LNTC_HSTX_DATA:8;
	unsigned rsv_16:16;
};


struct MIPITX_DSI_SW_CTRL_CON1_REG {
	unsigned SW_LNT0_LPTX_PRE_OE:1;
	unsigned SW_LNT0_LPTX_OE:1;
	unsigned SW_LNT0_LPTX_DP:1;
	unsigned SW_LNT0_LPTX_DN:1;
	unsigned SW_LNT0_HSTX_PRE_OE:1;
	unsigned SW_LNT0_HSTX_OE:1;
	unsigned SW_LNT0_HSTX_RDY:1;
	unsigned SW_LNT0_LPRX_EN:1;
	unsigned SW_LNT1_LPTX_PRE_OE:1;
	unsigned SW_LNT1_LPTX_OE:1;
	unsigned SW_LNT1_LPTX_DP:1;
	unsigned SW_LNT1_LPTX_DN:1;
	unsigned SW_LNT1_HSTX_PRE_OE:1;
	unsigned SW_LNT1_HSTX_OE:1;
	unsigned SW_LNT1_HSTX_RDY:1;
	unsigned SW_LNT1_LPRX_EN:1;
	unsigned SW_LNT2_LPTX_PRE_OE:1;
	unsigned SW_LNT2_LPTX_OE:1;
	unsigned SW_LNT2_LPTX_DP:1;
	unsigned SW_LNT2_LPTX_DN:1;
	unsigned SW_LNT2_HSTX_PRE_OE:1;
	unsigned SW_LNT2_HSTX_OE:1;
	unsigned SW_LNT2_HSTX_RDY:1;
	unsigned SW_LNT2_LPRX_EN:1;
	unsigned SW_LNT3_LPTX_PRE_OE:1;
	unsigned SW_LNT3_LPTX_OE:1;
	unsigned SW_LNT3_LPTX_DP:1;
	unsigned SW_LNT3_LPTX_DN:1;
	unsigned SW_LNT3_HSTX_PRE_OE:1;
	unsigned SW_LNT3_HSTX_OE:1;
	unsigned SW_LNT3_HSTX_RDY:1;
	unsigned SW_LNT3_LPRX_EN:1;
};

struct MIPITX_DSI_SW_CTRL_CON2_REG {
	unsigned SW_LNT_HSTX_DATA:8;
	unsigned rsv_8:24;
};

struct MIPITX_DSI_DBG_CON_REG {
	unsigned MIPI_TX_DBG_SEL:4;
	unsigned MIPI_TX_DBG_OUT_EN:1;
	unsigned MIPI_TX_GPIO_MODE_EN:1;
	unsigned MIPI_TX_APB_ASYNC_CNT_EN:1;
	unsigned rsv_7:25;
};

struct MIPITX_DSI_APB_ASYNC_STA_REG {
	unsigned MIPI_TX_APB_ASYNC_ERR:1;
	unsigned MIPI_TX_APB_ASYNC_ERR_ADDR:10;
	unsigned rsv_11:21;
};





struct DSI_START_REG {
	unsigned DSI_START:1;
	unsigned rsv_1:1;
	unsigned SLEEPOUT_START:1;
	unsigned SKEWCAL_START:1;
	unsigned rsv_4:12;
	unsigned VM_CMD_START:1;
	unsigned rsv_17:15;
};


struct DSI_STATUS_REG {
	unsigned rsv_0:1;
	unsigned BUF_UNDERRUN:1;
	unsigned rsv_2:2;
	unsigned ESC_ENTRY_ERR:1;
	unsigned ESC_SYNC_ERR:1;
	unsigned CTRL_ERR:1;
	unsigned CONTENT_ERR:1;
	unsigned rsv_8:24;
};


struct DSI_INT_ENABLE_REG {
	unsigned RD_RDY:1;
	unsigned CMD_DONE:1;
	unsigned TE_RDY:1;
	unsigned VM_DONE:1;
	unsigned FRAME_DONE_INT_EN:1;
	unsigned VM_CMD_DONE:1;
	unsigned SLEEPOUT_DONE:1;
	unsigned TE_TIMEOUT_INT_EN:1;
	unsigned VM_VBP_STR_INT_EN:1;
	unsigned VM_VACT_STR_INT_EN:1;
	unsigned VM_VFP_STR_INT_EN:1;
	unsigned SKEWCAL_DONE_INT_EN:1;
	unsigned rsv_12:20;
};


struct DSI_INT_STATUS_REG {
	unsigned RD_RDY:1;
	unsigned CMD_DONE:1;
	unsigned TE_RDY:1;
	unsigned VM_DONE:1;
	unsigned FRAME_DONE_INT_EN:1;
	unsigned VM_CMD_DONE:1;
	unsigned SLEEPOUT_DONE:1;
	unsigned TE_TIMEOUT_INT_EN:1;
	unsigned VM_VBP_STR_INT_EN:1;
	unsigned VM_VACT_STR_INT_EN:1;
	unsigned VM_VFP_STR_INT_EN:1;
	unsigned SKEWCAL_DONE_INT_EN:1;
	unsigned rsv_12:19;
	unsigned BUSY:1;
};


struct DSI_COM_CTRL_REG {
	unsigned DSI_RESET:1;
	unsigned rsv_1:1;
	unsigned DPHY_RESET:1;
	unsigned rsv_3:1;
	unsigned DSI_DUAL_EN:1;
	unsigned rsv_5:27;
};


enum DSI_MODE_CTRL {
	DSI_CMD_MODE = 0,
	DSI_SYNC_PULSE_VDO_MODE = 1,
	DSI_SYNC_EVENT_VDO_MODE = 2,
	DSI_BURST_VDO_MODE = 3
};


struct DSI_MODE_CTRL_REG {
	unsigned MODE:2;
	unsigned rsv_2:14;
	unsigned FRM_MODE:1;
	unsigned MIX_MODE:1;
	unsigned V2C_SWITCH_ON:1;
	unsigned C2V_SWITCH_ON:1;
	unsigned SLEEP_MODE:1;
	unsigned rsv_21:11;
};


enum DSI_LANE_NUM {
	ONE_LANE = 1,
	TWO_LANE = 2,
	THREE_LANE = 3,
	FOUR_LANE = 4
};


struct DSI_TXRX_CTRL_REG {
	unsigned VC_NUM:2;
	unsigned LANE_NUM:4;
	unsigned DIS_EOT:1;
	unsigned BLLP_EN:1;
	unsigned TE_FREERUN:1;
	unsigned EXT_TE_EN:1;
	unsigned EXT_TE_EDGE:1;
	unsigned TE_AUTO_SYNC:1;
	unsigned MAX_RTN_SIZE:4;
	unsigned HSTX_CKLP_EN:1;
	unsigned TYPE1_BTA_SEL:1;
	unsigned TE_WITH_CMD_EN:1;
	unsigned TE_TIMEOUT_CHK_EN:1;
	unsigned rsv_20:12;
};


enum DSI_PS_TYPE {
	PACKED_PS_16BIT_RGB565 = 0,
	LOOSELY_PS_18BIT_RGB666 = 1,
	PACKED_PS_24BIT_RGB888 = 2,
	PACKED_PS_18BIT_RGB666 = 3
};


struct DSI_PSCTRL_REG {
	unsigned DSI_PS_WC:14;
	unsigned rsv_14:2;
	unsigned DSI_PS_SEL:2;
	unsigned rsv_18:6;
	unsigned RGB_SWAP:1;
	unsigned BYTE_SWAP:1;
	unsigned rsv_26:6;
};


struct DSI_VSA_NL_REG {
	unsigned VSA_NL:10;
	unsigned rsv_11:22;
};


struct DSI_VBP_NL_REG {
	unsigned VBP_NL:10;
	unsigned rsv_11:22;
};


struct DSI_VFP_NL_REG {
	unsigned VFP_NL:10;
	unsigned rsv_11:22;
};


struct DSI_VACT_NL_REG {
	unsigned VACT_NL:12;
	unsigned rsv_12:20;
};

struct DSI_LFR_CON_REG {
	unsigned LFR_MODE:2;
	unsigned LFR_TYPE:2;
	unsigned LFR_EN:1;
	unsigned LFR_UPDATE:1;
	unsigned LFR_VSE_DIS:1;
	unsigned rsv_7:1;
	unsigned LFR_SKIP_NUM:6;
	unsigned rsv_14:18;

};

struct DSI_LFR_STA_REG {
	unsigned LFR_SKIP_STA:6;
	unsigned rsv_6:2;
	unsigned LFR_SKIP_CNT:24;
};


struct DSI_HSA_WC_REG {
	unsigned HSA_WC:12;
	unsigned rsv_12:20;
};


struct DSI_HBP_WC_REG {
	unsigned HBP_WC:12;
	unsigned rsv_12:20;
};


struct DSI_HFP_WC_REG {
	unsigned HFP_WC:12;
	unsigned rsv_12:20;
};

struct DSI_BLLP_WC_REG {
	unsigned BLLP_WC:12;
	unsigned rsv_12:20;
};

struct DSI_CMDQ_CTRL_REG {
	unsigned CMDQ_SIZE:8;
	unsigned rsv_8:24;
};

struct DSI_RX_DATA_REG {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
};


struct DSI_RACK_REG {
	unsigned DSI_RACK:1;
	unsigned DSI_RACK_BYPASS:1;
	unsigned rsv2:30;
};


struct DSI_TRIG_STA_REG {
	unsigned TRIG0:1;	/* remote rst */
	unsigned TRIG1:1;	/* TE */
	unsigned TRIG2:1;	/* ack */
	unsigned TRIG3:1;	/* rsv */
	unsigned RX_ULPS:1;
	unsigned DIRECTION:1;
	unsigned RX_LPDT:1;
	unsigned rsv7:1;
	unsigned RX_POINTER:4;
	unsigned rsv12:20;
};


struct DSI_MEM_CONTI_REG {
	unsigned RWMEM_CONTI:16;
	unsigned rsv16:16;
};


struct DSI_FRM_BC_REG {
	unsigned FRM_BC:21;
	unsigned rsv21:11;
};


struct DSI_PHY_CON_REG {
	unsigned PHY_RST:1;
	unsigned rsv1:4;
	unsigned HTXTO_RST:1;
	unsigned LRXTO_RST:1;
	unsigned BTATO_RST:1;
	unsigned rsv8:24;
};


struct DSI_PHY_LCCON_REG {
	unsigned LC_HS_TX_EN:1;
	unsigned LC_ULPM_EN:1;
	unsigned LC_WAKEUP_EN:1;
	unsigned rsv3:29;
};


struct DSI_PHY_LD0CON_REG {
	unsigned L0_RM_TRIG_EN:1;
	unsigned L0_ULPM_EN:1;
	unsigned L0_WAKEUP_EN:1;
	unsigned Lx_ULPM_AS_L0:1;
	unsigned L0_RX_FILTER_EN:1;
	unsigned rsv3:27;
};


struct DSI_PHY_TIMCON0_REG {
	unsigned char LPX;
	unsigned char HS_PRPR;
	unsigned char HS_ZERO;
	unsigned char HS_TRAIL;
};


struct DSI_PHY_TIMCON1_REG {
	unsigned char TA_GO;
	unsigned char TA_SURE;
	unsigned char TA_GET;
	unsigned char DA_HS_EXIT;
};


struct DSI_PHY_TIMCON2_REG {
	unsigned char CONT_DET;
	unsigned char DA_HS_SYNC;
	unsigned char CLK_ZERO;
	unsigned char CLK_TRAIL;
};


struct DSI_PHY_TIMCON3_REG {
	unsigned char CLK_HS_PRPR;
	unsigned char CLK_HS_POST;
	unsigned char CLK_HS_EXIT;
	unsigned rsv24:8;
};


struct DSI_PHY_TIMCON4_REG {
	unsigned ULPS_WAKEUP:20;
	unsigned rsv20:12;
};


struct DSI_PHY_TIMCON_REG {
	struct DSI_PHY_TIMCON0_REG CTRL0;
	struct DSI_PHY_TIMCON1_REG CTRL1;
	struct DSI_PHY_TIMCON2_REG CTRL2;
	struct DSI_PHY_TIMCON3_REG CTRL3;
};


struct DSI_CKSM_OUT_REG {
	unsigned PKT_CHECK_SUM:16;
	unsigned ACC_CHECK_SUM:16;
};


struct DSI_STATE_DBG0_REG {
	unsigned DPHY_CTL_STATE_C:9;
	unsigned rsv9:7;
	unsigned DPHY_HS_TX_STATE_C:5;
	unsigned rsv21:11;
};


struct DSI_STATE_DBG1_REG {
	unsigned CTL_STATE_C:15;
	unsigned rsv15:1;
	unsigned HS_TX_STATE_0:5;
	unsigned rsv21:3;
	unsigned ESC_STATE_0:8;
};


struct DSI_STATE_DBG2_REG {
	unsigned RX_ESC_STATE:10;
	unsigned rsv10:6;
	unsigned TA_T2R_STATE:5;
	unsigned rsv21:3;
	unsigned TA_R2T_STATE:5;
	unsigned rsv29:3;
};


struct DSI_STATE_DBG3_REG {
	unsigned CTL_STATE_1:5;
	unsigned rsv5:3;
	unsigned HS_TX_STATE_1:5;
	unsigned rsv13:3;
	unsigned CTL_STATE_2:5;
	unsigned rsv21:3;
	unsigned HS_TX_STATE_2:5;
	unsigned rsv29:3;
};


struct DSI_STATE_DBG4_REG {
	unsigned CTL_STATE_3:5;
	unsigned rsv5:3;
	unsigned HS_TX_STATE_3:5;
	unsigned rsv13:19;
};


struct DSI_STATE_DBG5_REG {
	unsigned TIMER_COUNTER:16;
	unsigned TIMER_BUSY:1;
	unsigned rsv17:11;
	unsigned WAKEUP_STATE:4;
};


struct DSI_STATE_DBG6_REG {
	unsigned CMTRL_STATE:14;
	unsigned rsv14:2;
	unsigned CMDQ_STATE:8;
	unsigned rsv24:8;
};


struct DSI_STATE_DBG7_REG {
	unsigned VMCTL_STATE:11;
	unsigned rsv11:1;
	unsigned VFP_PERIOD:1;
	unsigned VACT_PERIOD:1;
	unsigned VBP_PERIOD:1;
	unsigned VSA_PERIOD:1;
	unsigned rsv16:16;
};


struct DSI_STATE_DBG8_REG {
	unsigned WORD_COUNTER:14;
	unsigned rsv14:18;
};


struct DSI_STATE_DBG9_REG {
	unsigned LINE_COUNTER:22;
	unsigned rsv22:10;
};


struct DSI_DEBUG_SEL_REG {
	unsigned DEBUG_OUT_SEL:5;
	unsigned rsv5:3;
	unsigned CHKSUM_REC_EN:1;
	unsigned rsv9:23;
};


struct DSI_BIST_CON_REG {
	unsigned BIST_MODE:1;
	unsigned BIST_ENABLE:1;
	unsigned BIST_FIX_PATTERN:1;
	unsigned BIST_SPC_PATTERN:1;
	unsigned BIST_HS_FREE:1;
	unsigned rsv_05:1;
	unsigned SELF_PAT_MODE:1;
	unsigned rsv_07:1;
	unsigned BIST_LANE_NUM:4;
	unsigned rsv12:4;
	unsigned BIST_TIMING:8;
	unsigned rsv24:8;
};

struct DSI_VM_CMD_CON_REG {
	unsigned VM_CMD_EN:1;
	unsigned LONG_PKT:1;
	unsigned TIME_SEL:1;
	unsigned TS_VSA_EN:1;
	unsigned TS_VBP_EN:1;
	unsigned TS_VFP_EN:1;
	unsigned rsv6:2;
	unsigned CM_DATA_ID:8;
	unsigned CM_DATA_0:8;
	unsigned CM_DATA_1:8;
};

struct DSI_3D_CON_REG {
	unsigned _3D_MODE:2;
	unsigned _3D_FMT:2;
	unsigned _3D_VSYNC:1;
	unsigned _3D_LR:1;
	unsigned _3D_EN:1;
	unsigned rsv08:25;
};

struct DSI_TIME_CON0_REG {
	unsigned UPLS_WAKEUP_PRD:16;
	unsigned SKEWCALL_PRD:16;
};

struct DSI_TIME_CON1_REG {
	unsigned UPLS_WAKEUP_PRD:16;
	unsigned rsv16:16;
};

struct DSI_REGS {
	struct DSI_START_REG DSI_START;	/* 0000 */
	struct DSI_STATUS_REG DSI_STA;	/* 0004 */
	struct DSI_INT_ENABLE_REG DSI_INTEN;	/* 0008 */
	struct DSI_INT_STATUS_REG DSI_INTSTA;	/* 000C */
	struct DSI_COM_CTRL_REG DSI_COM_CTRL;	/* 0010 */
	struct DSI_MODE_CTRL_REG DSI_MODE_CTRL;	/* 0014 */
	struct DSI_TXRX_CTRL_REG DSI_TXRX_CTRL;	/* 0018 */
	struct DSI_PSCTRL_REG DSI_PSCTRL;	/* 001C */
	struct DSI_VSA_NL_REG DSI_VSA_NL;	/* 0020 */
	struct DSI_VBP_NL_REG DSI_VBP_NL;	/* 0024 */
	struct DSI_VFP_NL_REG DSI_VFP_NL;	/* 0028 */
	struct DSI_VACT_NL_REG DSI_VACT_NL;	/* 002C */
	struct DSI_LFR_CON_REG DSI_LFR_CON;	/* 0030 */
	struct DSI_LFR_STA_REG DSI_LFR_STA;	/* 0034 */
	uint32_t rsv_38[6];	/* 0038..004C */
	struct DSI_HSA_WC_REG DSI_HSA_WC;	/* 0050 */
	struct DSI_HBP_WC_REG DSI_HBP_WC;	/* 0054 */
	struct DSI_HFP_WC_REG DSI_HFP_WC;	/* 0058 */
	struct DSI_BLLP_WC_REG DSI_BLLP_WC;	/* 005C */

	struct DSI_CMDQ_CTRL_REG DSI_CMDQ_SIZE;	/* 0060 */
	uint32_t DSI_HSTX_CKL_WC;	/* 0064 */
	uint32_t DSI_HSTX_CKL_WC_AUTO_RESULT;	/* 0068 */
	uint32_t rsv_006C[2];	/* 006c..0070 */
	struct DSI_RX_DATA_REG DSI_RX_DATA0;	/* 0074 */
	struct DSI_RX_DATA_REG DSI_RX_DATA1;	/* 0078 */
	struct DSI_RX_DATA_REG DSI_RX_DATA2;	/* 007c */
	struct DSI_RX_DATA_REG DSI_RX_DATA3;	/* 0080 */
	struct DSI_RACK_REG DSI_RACK;	/* 0084 */
	struct DSI_TRIG_STA_REG DSI_TRIG_STA;	/* 0088 */
	uint32_t rsv_008C;	/* 008C */
	struct DSI_MEM_CONTI_REG DSI_MEM_CONTI;	/* 0090 */
	struct DSI_FRM_BC_REG DSI_FRM_BC;	/* 0094 */
	struct DSI_3D_CON_REG DSI_3D_CON;	/* 0098 */
	uint32_t rsv_009C;	/* 009c */
	struct DSI_TIME_CON0_REG DSI_TIME_CON0;	/* 00A0 */
	struct DSI_TIME_CON1_REG DSI_TIME_CON1;	/* 00A4 */

	uint32_t rsv_00A8[22];	/* 0A8..0FC */
	uint32_t DSI_PHY_PCPAT;	/* 00100 */

	struct DSI_PHY_LCCON_REG DSI_PHY_LCCON;	/* 0104 */
	struct DSI_PHY_LD0CON_REG DSI_PHY_LD0CON;	/* 0108 */
	uint32_t rsv_010C;	/* 010C */
	struct DSI_PHY_TIMCON0_REG DSI_PHY_TIMECON0;	/* 0110 */
	struct DSI_PHY_TIMCON1_REG DSI_PHY_TIMECON1;	/* 0114 */
	struct DSI_PHY_TIMCON2_REG DSI_PHY_TIMECON2;	/* 0118 */
	struct DSI_PHY_TIMCON3_REG DSI_PHY_TIMECON3;	/* 011C */
	struct DSI_PHY_TIMCON4_REG DSI_PHY_TIMECON4;	/* 0120 */
	uint32_t rsv_0124[3];	/* 0124..012c */
	struct DSI_VM_CMD_CON_REG DSI_VM_CMD_CON;	/* 0130 */
	uint32_t DSI_VM_CMD_DATA0;	/* 0134 */
	uint32_t DSI_VM_CMD_DATA4;	/* 0138 */
	uint32_t DSI_VM_CMD_DATA8;	/* 013C */
	uint32_t DSI_VM_CMD_DATAC;	/* 0140 */
	struct DSI_CKSM_OUT_REG DSI_CKSM_OUT;	/* 0144 */
	struct DSI_STATE_DBG0_REG DSI_STATE_DBG0;	/* 0148 */
	struct DSI_STATE_DBG1_REG DSI_STATE_DBG1;	/* 014C */
	struct DSI_STATE_DBG2_REG DSI_STATE_DBG2;	/* 0150 */
	struct DSI_STATE_DBG3_REG DSI_STATE_DBG3;	/* 0154 */
	struct DSI_STATE_DBG4_REG DSI_STATE_DBG4;	/* 0158 */
	struct DSI_STATE_DBG5_REG DSI_STATE_DBG5;	/* 015C */
	struct DSI_STATE_DBG6_REG DSI_STATE_DBG6;	/* 0160 */
	struct DSI_STATE_DBG7_REG DSI_STATE_DBG7;	/* 0164 */
	struct DSI_STATE_DBG8_REG DSI_STATE_DBG8;	/* 0168 */
	struct DSI_STATE_DBG9_REG DSI_STATE_DBG9;	/* 016C */
	struct DSI_DEBUG_SEL_REG DSI_DEBUG_SEL;	/* 0170 */
	uint32_t rsv174;		/* 0174 */
	uint32_t DSI_BIST_PATTERN;	/* 0178 */
	struct DSI_BIST_CON_REG DSI_BIST_CON;	/* 017C */
};


struct DSI_CMDQ {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
};

struct DSI_CMDQ_REGS {
	struct DSI_CMDQ data[128];
};

struct DSI_VM_CMDQ {
	unsigned char byte0;
	unsigned char byte1;
	unsigned char byte2;
	unsigned char byte3;
};

struct DSI_VM_CMDQ_REGS {
	struct DSI_VM_CMDQ data[4];
};

struct DSI_PHY_REGS {
	struct MIPITX_DSI_CON_REG MIPITX_DSI_CON;	/* 0000 */
	struct MIPITX_DSI_CLOCK_LANE_REG MIPITX_DSI_CLOCK_LANE;	/* 0004 */
	struct MIPITX_DSI_DATA_LANE0_REG MIPITX_DSI_DATA_LANE0;	/* 0008 */
	struct MIPITX_DSI_DATA_LANE1_REG MIPITX_DSI_DATA_LANE1;	/* 000C */
	struct MIPITX_DSI_DATA_LANE2_REG MIPITX_DSI_DATA_LANE2;	/* 0010 */
	struct MIPITX_DSI_DATA_LANE3_REG MIPITX_DSI_DATA_LANE3;	/* 0014 */
	uint32_t rsv_18[10];	/* 0018..003C */

	struct MIPITX_DSI_TOP_CON_REG MIPITX_DSI_TOP_CON;	/* 0040 */
	struct MIPITX_DSI_BG_CON_REG MIPITX_DSI_BG_CON;	/* 0044 */
	uint32_t rsv_48[2];	/* 0048..004C */
	struct MIPITX_DSI_PLL_CON0_REG MIPITX_DSI_PLL_CON0;	/* 0050 */
	struct MIPITX_DSI_PLL_CON1_REG MIPITX_DSI_PLL_CON1;	/* 0054 */
	struct MIPITX_DSI_PLL_CON2_REG MIPITX_DSI_PLL_CON2;	/* 0058 */
	struct MIPITX_DSI_PLL_CON3_REG MIPITX_DSI_PLL_CON3;	/* 005C */
	struct MIPITX_DSI_PLL_CHG_REG MIPITX_DSI_PLL_CHG;	/* 0060 */
	struct MIPITX_DSI_PLL_TOP_REG MIPITX_DSI_PLL_TOP;	/* 0064 */
	struct MIPITX_DSI_PLL_PWR_REG MIPITX_DSI_PLL_PWR;	/* 0068 */
	uint32_t rsv_6C;		/* 006C */
	struct MIPITX_DSI_RGS_REG MIPITX_DSI_RGS;	/* 0070 */
	struct MIPITX_DSI_GPI_EN_REG MIPITX_DSI_GPI_EN;	/* 0074 */
	struct MIPITX_DSI_GPI_PULL_REG MIPITX_DSI_GPI_PULL;	/* 0078 */
	struct MIPITX_DSI_PHY_SEL_REG MIPITX_DSI_PHY_SEL;	/* 007C */

	struct MIPITX_DSI_SW_CTRL_REG MIPITX_DSI_SW_CTRL_EN;	/* 0080 */
	struct MIPITX_DSI_SW_CTRL_CON0_REG MIPITX_DSI_SW_CTRL_CON0;	/* 0084 */
	struct MIPITX_DSI_SW_CTRL_CON1_REG MIPITX_DSI_SW_CTRL_CON1;	/* 0088 */
	struct MIPITX_DSI_SW_CTRL_CON2_REG MIPITX_DSI_SW_CTRL_CON2;	/* 008C */
	struct MIPITX_DSI_DBG_CON_REG MIPITX_DSI_DBG_CON;	/* 0090 */
	uint32_t MIPITX_DSI_DBG_OUT;	/* 0084 */
	struct MIPITX_DSI_APB_ASYNC_STA_REG MIPITX_DSI_APB_ASYNC_STA;	/* 0098 */

};

#ifndef BUILD_LK
/*
STATIC_ASSERT(0x0050 == offsetof(struct DSI_PHY_REGS, MIPITX_DSI_PLL_CON0));
STATIC_ASSERT(0x0070 == offsetof(struct DSI_PHY_REGS, MIPITX_DSI_RGS));
STATIC_ASSERT(0x0080 == offsetof(struct DSI_PHY_REGS, MIPITX_DSI_SW_CTRL_EN));
STATIC_ASSERT(0x0090 == offsetof(struct DSI_PHY_REGS, MIPITX_DSI_DBG_CON));
STATIC_ASSERT(0x002C == offsetof(struct DSI_REGS, DSI_VACT_NL));
STATIC_ASSERT(0x0104 == offsetof(struct DSI_REGS, DSI_PHY_LCCON));
STATIC_ASSERT(0x011C == offsetof(struct DSI_REGS, DSI_PHY_TIMECON3));
STATIC_ASSERT(0x017C == offsetof(struct DSI_REGS, DSI_BIST_CON));
STATIC_ASSERT(0x0100 == offsetof(struct DSI_REGS, DSI_PHY_PCPAT));


STATIC_ASSERT(0x0098 == offsetof(struct DSI_REGS, DSI_3D_CON));
*/
#endif


extern unsigned long dispsys_reg[DISP_REG_NUM];
extern unsigned long mipi_tx_reg;
extern unsigned long dsi_reg_va;

/* DTS will assign reigister address dynamically, so can not define to 0x1000 */
/* #define DISP_INDEX_OFFSET 0x1000 */
#define DISP_RDMA_INDEX_OFFSET (dispsys_reg[DISP_REG_RDMA1] - dispsys_reg[DISP_REG_RDMA0])
#define DISP_OVL_INDEX_OFFSET  (dispsys_reg[DISP_REG_OVL1] - dispsys_reg[DISP_REG_OVL0])
#define DISP_WDMA_INDEX_OFFSET (0)

#define DDP_REG_BASE_MMSYS_CONFIG  dispsys_reg[DISP_REG_CONFIG]
#define DDP_REG_BASE_DISP_OVL0     dispsys_reg[DISP_REG_OVL0]
#define DDP_REG_BASE_DISP_OVL1     dispsys_reg[DISP_REG_OVL1]
#define DDP_REG_BASE_DISP_RDMA0    dispsys_reg[DISP_REG_RDMA0]
#define DDP_REG_BASE_DISP_RDMA1    dispsys_reg[DISP_REG_RDMA1]
#define DDP_REG_BASE_DISP_WDMA0    dispsys_reg[DISP_REG_WDMA0]
#define DDP_REG_BASE_DISP_WDMA1    0
#define DDP_REG_BASE_DISP_COLOR0   dispsys_reg[DISP_REG_COLOR]
#define DDP_REG_BASE_DISP_COLOR1   0
#define DDP_REG_BASE_DISP_AAL      dispsys_reg[DISP_REG_AAL]
#define DDP_REG_BASE_DISP_GAMMA    dispsys_reg[DISP_REG_GAMMA]
#define DDP_REG_BASE_DISP_MERGE    0
#define DDP_REG_BASE_DISP_SPLIT0   0
#define DDP_REG_BASE_DISP_SPLIT1   0
#define DDP_REG_BASE_DISP_UFOE     0
#define DDP_REG_BASE_DSI0          dispsys_reg[DISP_REG_DSI0]
#define DDP_REG_BASE_DSI1          0
#define DDP_REG_BASE_DPI           dispsys_reg[DISP_REG_DPI0]
#define DDP_REG_BASE_DISP_PWM0     dispsys_reg[DISP_REG_PWM]
#define DDP_REG_BASE_DISP_PWM1     0
#define DDP_REG_BASE_MM_MUTEX      dispsys_reg[DISP_REG_MUTEX]
#define DDP_REG_BASE_SMI_LARB0     dispsys_reg[DISP_REG_SMI_LARB0]
#define DDP_REG_BASE_SMI_COMMON    dispsys_reg[DISP_REG_SMI_COMMON]
#define DDP_REG_BASE_DISP_OD       dispsys_reg[DISP_REG_OD]
#define DDP_REG_BASE_DISP_CCORR    dispsys_reg[DISP_REG_CCORR]
#define DDP_REG_BASE_DISP_DITHER   dispsys_reg[DISP_REG_DITHER]
#define DDP_REG_BASE_MMSYS_CONFIG2 dispsys_reg[DISP_REG_CONFIG2]
#define DDP_REG_BASE_MMSYS_CONFIG3 dispsys_reg[DISP_REG_CONFIG3]
#define DDP_REG_IO_DRIVING1         dispsys_reg[DISP_REG_IO_DRIVING1]
#define DDP_REG_IO_DRIVING2         dispsys_reg[DISP_REG_IO_DRIVING2]
#define DDP_REG_IO_DRIVING3         dispsys_reg[DISP_REG_IO_DRIVING3]
#define DDP_REG_EFUSE               dispsys_reg[DISP_REG_EFUSE]
#define DDP_REG_EFUSE_PERMISSION    dispsys_reg[DISP_REG_EFUSE_PERMISSION]
#define DDP_REG_EFUSE_KEY           dispsys_reg[DISP_RGE_EFUSE_KEY]
#define DDP_REG_BASE_VENCPLL        dispsys_reg[DISP_RGE_VENCPLL]


#define DDP_REG_TVDPLL_CON6        dispsys_reg[DISP_TVDPLL_CFG6]
#define DDP_REG_TVDPLL_CON0        dispsys_reg[DISP_TVDPLL_CON0]
#define DDP_REG_TVDPLL_CON1        dispsys_reg[DISP_TVDPLL_CON1]

#define MIPI_TX_REG_BASE			(mipi_tx_reg)
#if 0
#define DDP_REG_BASE_MMSYS_CONFIG MMSYS_CONFIG_BASE	/* 0xf4000000 */
#define DDP_REG_BASE_DISP_OVL0    OVL0_BASE	/* 0xf400C000 */
#define DDP_REG_BASE_DISP_OVL1    OVL1_BASE	/* 0xf400D000 */
#define DDP_REG_BASE_DISP_RDMA0   DISP_RDMA0_BASE	/* 0xf400E000 */
#define DDP_REG_BASE_DISP_RDMA1   DISP_RDMA1_BASE	/* 0xf400F000 */
#define DDP_REG_BASE_DISP_RDMA2   DISP_RDMA2_BASE	/* 0xf4010000 */
#define DDP_REG_BASE_DISP_WDMA0   DISP_WDMA0_BASE	/* 0xf4011000 */
#define DDP_REG_BASE_DISP_WDMA1   DISP_WDMA1_BASE	/* 0xf4012000 */
#define DDP_REG_BASE_DISP_COLOR0  COLOR0_BASE	/* 0xf4013000 */
#define DDP_REG_BASE_DISP_COLOR1  COLOR1_BASE	/* 0xf4014000 */
#define DDP_REG_BASE_DISP_AAL     DISP_AAL_BASE	/* 0xf4015000 */
#define DDP_REG_BASE_DISP_GAMMA   DISP_GAMMA_BASE	/* 0xf4016000 */
#define DDP_REG_BASE_DISP_MERGE   DISP_MERGE_BASE	/* 0xf4017000 */
#define DDP_REG_BASE_DISP_SPLIT0  DISP_SPLIT0_BASE	/* 0xf4018000 */
#define DDP_REG_BASE_DISP_SPLIT1  DISP_SPLIT1_BASE	/* 0xf4019000 */
#define DDP_REG_BASE_DISP_UFOE    DISP_UFOE_BASE	/* 0xf401A000 */
#define DDP_REG_BASE_DSI0         DSI0_BASE	/* 0xf401B000 */
#define DDP_REG_BASE_DSI1         DSI1_BASE	/* 0xf401C000 */
#define DDP_REG_BASE_DPI          DPI_BASE	/* 0xf401D000 */
#define DDP_REG_BASE_DISP_PWM0    DISP_PWM0_BASE	/* 0xf401E000 */
#define DDP_REG_BASE_DISP_PWM1    DISP_PWM1_BASE	/* 0xf401F000 */
#define DDP_REG_BASE_MM_MUTEX     MM_MUTEX_BASE	/* 0xf4020000 */
#define DDP_REG_BASE_SMI_LARB0    SMI_LARB0_BASE	/* 0xf4021000 */
#define DDP_REG_BASE_SMI_COMMON   SMI_COMMON_BASE	/* 0xf4022000 */
#define DDP_REG_BASE_DISP_OD      DISP_OD_BASE	/* 0xf4023000 */
#endif

#define DISPSYS_REG_ADDR_MIN 0	/* dispsys_reg[0] */
#define DISPSYS_REG_ADDR_MAX 0xffffffffffffffff	/* (dispsys_reg[0]+0x200000) */

#define DISPSYS_CONFIG_BASE			DDP_REG_BASE_MMSYS_CONFIG
#define DISPSYS_OVL0_BASE				DDP_REG_BASE_DISP_OVL0
#define DISPSYS_OVL1_BASE				DDP_REG_BASE_DISP_OVL1
#define DISPSYS_RDMA0_BASE		       DDP_REG_BASE_DISP_RDMA0
#define DISPSYS_RDMA1_BASE		       DDP_REG_BASE_DISP_RDMA1
#define DISPSYS_WDMA0_BASE		       DDP_REG_BASE_DISP_WDMA0
#define DISPSYS_WDMA1_BASE		       DDP_REG_BASE_DISP_WDMA1
#define DISPSYS_COLOR0_BASE		       DDP_REG_BASE_DISP_COLOR0
#define DISPSYS_COLOR1_BASE		       DDP_REG_BASE_DISP_COLOR1
#define DISPSYS_AAL_BASE				DDP_REG_BASE_DISP_AAL
#define DISPSYS_GAMMA_BASE		       DDP_REG_BASE_DISP_GAMMA
#define DISPSYS_MERGE_BASE				DDP_REG_BASE_DISP_MERGE
#define DISPSYS_SPLIT0_BASE		       DDP_REG_BASE_DISP_SPLIT0
#define DISPSYS_SPLIT1_BASE		       DDP_REG_BASE_DISP_SPLIT1
#define DISPSYS_UFOE_BASE				DDP_REG_BASE_DISP_UFOE
#define DISPSYS_DSI0_BASE				DDP_REG_BASE_DSI0
#define DISPSYS_DSI1_BASE				DDP_REG_BASE_DSI1
#define DISPSYS_DPI_BASE				DDP_REG_BASE_DPI
#define DISPSYS_PWM0_BASE				DDP_REG_BASE_DISP_PWM0
#define DISPSYS_PWM1_BASE				DDP_REG_BASE_DISP_PWM1
#define DISPSYS_MUTEX_BASE			DDP_REG_BASE_MM_MUTEX
#define DISPSYS_SMI_LARB0_BASE		DDP_REG_BASE_SMI_LARB0
#define DISPSYS_SMI_COMMON_BASE		DDP_REG_BASE_SMI_COMMON
#define DISPSYS_OD_BASE             DDP_REG_BASE_DISP_OD
#define DISPSYS_CCORR_BASE		       DDP_REG_BASE_DISP_CCORR
#define DISPSYS_DITHER_BASE		       DDP_REG_BASE_DISP_DITHER
#define DISPSYS_CONFIG2_BASE		    DDP_REG_BASE_MMSYS_CONFIG2
#define DISPSYS_CONFIG3_BASE		    DDP_REG_BASE_MMSYS_CONFIG3
#define DISPSYS_IO_DRIVING1		      DDP_REG_IO_DRIVING1
#define DISPSYS_IO_DRIVING2		      DDP_REG_IO_DRIVING2
#define DISPSYS_IO_DRIVING3		      DDP_REG_IO_DRIVING3
#define DISPSYS_EFUSE                 DDP_REG_EFUSE
#define DISPSYS_EFUSE_PERMISSION      DDP_REG_EFUSE_PERMISSION
#define DISPSYS_EFUSE_KEY             DDP_REG_EFUSE_KEY
#define DISPSYS_VENCPLL_BASE          DDP_REG_BASE_VENCPLL

#define MIPITX_BASE					MIPI_TX_REG_BASE
/* --------------------------------------------------------------------------- */
/* Type Casting */
/* --------------------------------------------------------------------------- */
#if 0
#define AS_INT32(x)     (*(INT32 *)(x))
#define AS_INT16(x)     (*(INT16 *)(x))
#define AS_INT8(x)      (*(INT8  *)(x))

#define AS_UINT32(x)    __raw_readl(x)
#define AS_UINT16(x)    (*(uint16_t *)(x))
#define AS_UINT8(x)     (*(uint8_t  *)(x))


/* --------------------------------------------------------------------------- */
/* Register Manipulations */
/* --------------------------------------------------------------------------- */

#define READ_REGISTER_UINT32(reg) \
	(*(volatile unsigned long * const)(reg))

#define WRITE_REGISTER_UINT32(reg, val) \
	((*(volatile unsigned long * const)(reg)) = (val))

#define READ_REGISTER_UINT16(reg) \
	(*(volatile uint16_t * const)(reg))

#define WRITE_REGISTER_UINT16(reg, val) \
	((*(volatile uint16_t * const)(reg)) = (val))

#define READ_REGISTER_UINT8(reg) \
	(*(volatile uint8_t * const)(reg))

#define WRITE_REGISTER_UINT8(reg, val) \
	((*(volatile uint8_t * const)(reg)) = (val))
#endif

#if 0
#define INREG8(x)           READ_REGISTER_UINT8((uint8_t *)(x))
#define OUTREG8(x, y)       WRITE_REGISTER_UINT8((uint8_t *)(x), (uint8_t)(y))
#define SETREG8(x, y)       OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)       OUTREG8(x, INREG8(x)&~(y))
#define MASKREG8(x, y, z)   OUTREG8(x, (INREG8(x)&~(y))|(z))

#define INREG16(x)          READ_REGISTER_UINT16((uint16_t *)(x))
#define OUTREG16(x, y)      WRITE_REGISTER_UINT16((uint16_t *)(x), (uint16_t)(y))
#define SETREG16(x, y)      OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)      OUTREG16(x, INREG16(x)&~(y))
#define MASKREG16(x, y, z)  OUTREG16(x, (INREG16(x)&~(y))|(z))

#define INREG32(x)          (__raw_readl((unsigned long *)(x)))
#define OUTREG32(x, y)      WRITE_REGISTER_UINT32((unsigned long *)(x), (uint32_t)(y))
#define SETREG32(x, y)      OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)      OUTREG32(x, INREG32(x)&~(y))
#define MASKREG32(x, y, z)  OUTREG32(x, (INREG32(x)&~(y))|(z))
#endif
#ifdef INREG32
#undef INREG32
#define INREG32(x)          (__raw_readl((unsigned long *)(x)))
#endif
/* --------------------------------------------------------------------------- */
/* Register Field Access */
/* --------------------------------------------------------------------------- */
#define READ_REGISTER_UINT32(reg)	(*(volatile uint32_t *const)(reg))
#define INREG32(x)	READ_REGISTER_UINT32((uint32_t *)((void *)(x)))

#define WRITE_REGISTER_UINT32(reg, val)	((*(volatile uint32_t *const)(reg)) = (val))
#define OUTREG32(x, y)	WRITE_REGISTER_UINT32((uint32_t *)((void *)(x)), (uint32_t)(y))

#define AS_UINT32(x)	(*(uint32_t *)((void *)x))

#define REG_FLD(width, shift) \
	((unsigned int)((((width) & 0xFF) << 16) | ((shift) & 0xFF)))

#define REG_FLD_WIDTH(field) \
	((unsigned int)(((field) >> 16) & 0xFF))

#define REG_FLD_SHIFT(field) \
	((unsigned int)((field) & 0xFF))

#define REG_FLD_MASK(field) \
	(((unsigned int)(1 << REG_FLD_WIDTH(field)) - 1) << REG_FLD_SHIFT(field))

#define REG_FLD_VAL(field, val) \
	(((val) << REG_FLD_SHIFT(field)) & REG_FLD_MASK(field))


#define DISP_REG_GET(reg32) __raw_readl((unsigned long *)(reg32))
#define DISP_REG_GET_FIELD(field, reg32) ((__raw_readl((void *)(reg32)) & REG_FLD_MASK(field)) >> REG_FLD_SHIFT(field))

/* polling register until masked bit is 1 */
#define DDP_REG_POLLING(reg32, mask) \
	do { \
		while (!((DISP_REG_GET(reg32))&mask)) \
			; \
	} while (0)

/* Polling register until masked bit is 0 */
#define DDP_REG_POLLING_NEG(reg32, mask) \
	do { \
		while ((DISP_REG_GET(reg32))&mask) \
			;			\
	} while (0)

#define DISP_CPU_REG_SET(reg32, val) \
	do {\
		if (0)	\
			dprec_reg_op(NULL, reg32, val, 0x00000000);\
		mt_reg_sync_writel(val, (volatile unsigned long *)(reg32));\
	} while (0)

#define DISP_CPU_REG_SET_FIELD(field, reg32, val)  \
	do {                                \
		if (0) \
			dprec_reg_op(NULL, (unsigned long)(reg32),\
				val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
		mt_reg_sync_writel(((unsigned int)(*(volatile unsigned long *)(reg32)) & ~REG_FLD_MASK(field))\
			| REG_FLD_VAL((field), (val)), (volatile unsigned long *)reg32);  \
	} while (0)

/* after apply device tree va/pa is not mapped by a fixed offset */
static inline unsigned long disp_addr_convert(unsigned long va)
{
	unsigned int i = 0;

	for (i = 0; i < DISP_REG_NUM; i++) {
		if (dispsys_reg[i] == (va & (~0xfffl))) {
			/* pr_err("DDP " "va=0x%x, pa=0x%x!\n", va,  (ddp_reg_pa_base[i]+(va&0xfffl))); */
			return ddp_reg_pa_base[i] + (va & 0xfffl);
		}
	}
	DDPERR("can not find reg addr for va=0x%lx!\n", va);
	return 0;
}


#define DISP_REG_MASK(handle, reg32, val, mask)	\
	do { \
		dprec_reg_op((void *)handle, (unsigned long)reg32, val, mask);\
		 if (handle == NULL) \
			mt_reg_sync_writel((unsigned int)(INREG32(reg32)&~(mask))|(val), \
				(volatile unsigned long *)(reg32));\
		 else \
			cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), val, mask); \
	} while (0)

/* #define DISP_ENABLE_TOP_CLK_CLOSE */

#ifdef DISP_ENABLE_TOP_CLK_CLOSE
extern unsigned int isIdlePowerOff;
extern void _disp_primary_path_exit_idle(const char *caller, unsigned int need_primary_lock);
#define DISP_REG_SET(handle, reg32, val) \
	do { \
		if (isIdlePowerOff == 1) { \
			DDPAEE("ddp clk error");\
			_disp_primary_path_exit_idle("ddp_reg", 0); \
		} \
		if (handle == 0) {\
			mt_reg_sync_writel(val, (volatile unsigned long *)(reg32));\
		} \
		else {\
			dprec_reg_op((void *)handle, reg32, val, 0x00000000);\
			cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert(reg32), val, ~0); \
		}  \
	} while (0)

#define DISP_REG_SET_FIELD(handle, field, reg32, val)  \
	do {  \
		if (isIdlePowerOff == 1) {\
			DDPAEE("ddp clk error");\
			_disp_primary_path_exit_idle("ddp_reg", 0); \
		} \
		if (handle == 0) { \
			mt_reg_sync_writel((__raw_readl(reg32) & ~REG_FLD_MASK(field)) | REG_FLD_VAL((field), (val)),\
				(reg32));  \
		} \
		else { \
			dprec_reg_op((void *)handle, (unsigned long)(reg32), val << REG_FLD_SHIFT(field),\
				     REG_FLD_MASK(field));\
			cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), \
				val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field)); \
		} \
	} while (0)

#else
#define DISP_REG_SET_PA(handle, reg32, val) \
	do { \
		if (handle == 0) {\
			mt_reg_sync_writel(val, (volatile unsigned long *)(reg32));\
		} \
		else {\
			cmdqRecWrite((cmdqRecHandle)handle, reg32, val, ~0); \
		}  \
	} while (0)

#define DISP_REG_SET(handle, reg32, val) \
	do { \
		if (handle == 0) {\
			mt_reg_sync_writel(val, (volatile unsigned long *)(reg32));\
		} \
		else {\
			dprec_reg_op((void *)handle, (unsigned long)reg32, val, 0x00000000);\
			cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), val, ~0); \
		}  \
	} while (0)

#define DISP_REG_SET_FIELD(handle, field, reg32, val)  \
	do {  \
		if (handle == 0) { \
			mt_reg_sync_writel((__raw_readl((void *)(reg32)) & ~REG_FLD_MASK(field))|REG_FLD_VAL((field), \
				(val)), (reg32)); \
		} \
		else{ \
			dprec_reg_op((void *)handle, (unsigned long)(reg32), val << REG_FLD_SHIFT(field),\
				     REG_FLD_MASK(field));\
			cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32),\
				     val << REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
		} \
	} while (0)
#endif

#define DISP_REG_CMDQ_POLLING(handle, reg32, val, mask) \
	do { \
		if (handle == 0) {\
			while (!((DISP_REG_GET(reg32))&val))\
				; \
		} \
		else {\
			dprec_reg_op((void *)handle, (unsigned long)reg32, val, 0x00000000);\
			cmdqRecPoll((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), val, mask); \
		}  \
	} while (0)

/* write to reg only if the value is different from reg value */
extern unsigned int gEnableReduceRegWrite;
#define DISP_REG_SET_DIRTY(handle, reg32, val) \
	do { \
		if (gEnableReduceRegWrite == 0 || DISP_REG_GET(reg32) != (val)) { \
			if (handle == 0) { \
				mt_reg_sync_writel(val, (volatile unsigned long *)(reg32));\
			} \
			else { \
				if (0)\
					dprec_reg_op((void *)handle, (unsigned long)reg32, val, 0x00000000);\
				cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), val, ~0); \
			}  \
		} \
	} while (0)


#define DISP_REG_SET_CMDQ(handle, reg32, val) \
	do { \
		dprec_reg_op((void *)handle, (unsigned long)reg32, val, 0x00000000);\
		cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32), val, ~0); \
	} while (0)
#define DISP_REG_SET_FIELD_CMDQ(handle, field, reg32, val)  \
	do {  \
		dprec_reg_op((void *)handle, (unsigned long)(reg32), val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
		cmdqRecWrite((cmdqRecHandle)handle, disp_addr_convert((unsigned long)reg32),\
			     val << REG_FLD_SHIFT(field), REG_FLD_MASK(field)); \
	} while (0)

#define DISP_REG_SET_CMDQ_MODULE(handle, reg32, val, module) \
	do { \
		dprec_reg_op((void *)handle, (unsigned long)reg32, val, 0x00000000);\
		cmdqRecWrite((cmdqRecHandle)handle, ddp_reg_pa_base[module]+(reg32&0xfffl), val, ~0); \
	} while (0)
#define DISP_REG_SET_FIELD_CMDQ_MODULE(handle, field, reg32, val, module)  \
	do {  \
		dprec_reg_op((void *)handle, (unsigned long)(reg32), val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field));\
		cmdqRecWrite((cmdqRecHandle)handle, ddp_reg_pa_base[module]+(reg32&0xfffl), \
			val<<REG_FLD_SHIFT(field), REG_FLD_MASK(field)); \
	} while (0)

#define DISP_REG_BACKUP(handle, hSlot, idx, reg32) \
	do { \
		if (handle != 0) { \
			if (hSlot)\
				cmdqRecBackupRegisterToSlot(handle, hSlot, \
				idx, disp_addr_convert((unsigned long)reg32));\
		}  \
	} while (0)

/* TODO: waiting for CMDQ enabled */
#if 1
/* Helper macros for local command queue */
#define DISP_CMDQ_BEGIN(__cmdq, scenario) \
	do { \
		cmdqRecCreate(scenario, &__cmdq);\
		cmdqRecReset(__cmdq);\
		ddp_insert_config_allow_rec(__cmdq); \
	} while (0)


#define DISP_CMDQ_REG_SET(__cmdq, reg32, val, mask) DISP_REG_MASK(__cmdq, reg32, val, mask)

#define DISP_CMDQ_CONFIG_STREAM_DIRTY(__cmdq) ddp_insert_config_dirty_rec(__cmdq)

#define DISP_CMDQ_END(__cmdq) \
	do {					\
		cmdqRecFlush(__cmdq); \
		cmdqRecDestroy(__cmdq); \
	} while (0)
#endif

/* field definition */
/* ------------------------------------------------------------- */
/* AAL */
#define DISP_AAL_EN                             (DISPSYS_AAL_BASE + 0x000)
#define DISP_AAL_RESET                             (DISPSYS_AAL_BASE + 0x004)
#define DISP_AAL_INTEN                          (DISPSYS_AAL_BASE + 0x008)
#define DISP_AAL_INTSTA                         (DISPSYS_AAL_BASE + 0x00c)
#define DISP_AAL_STATUS                         (DISPSYS_AAL_BASE + 0x010)
#define DISP_AAL_CFG                            (DISPSYS_AAL_BASE + 0x020)
#define DISP_AAL_IN_CNT                         (DISPSYS_AAL_BASE + 0x024)
#define DISP_AAL_OUT_CNT                        (DISPSYS_AAL_BASE + 0x028)
#define DISP_AAL_CHKSUM                        (DISPSYS_AAL_BASE + 0x02c)
#define DISP_AAL_SIZE                           (DISPSYS_AAL_BASE + 0x030)
#define DISP_AAL_DUMMY_REG                           (DISPSYS_AAL_BASE + 0x0c0)
#define DISP_AAL_MAX_HIST_CONFIG_00                        (DISPSYS_AAL_BASE + 0x204)
#define DISP_AAL_CABC_00                        (DISPSYS_AAL_BASE + 0x20c)
#define DISP_AAL_CABC_02                        (DISPSYS_AAL_BASE + 0x214)
#define DISP_AAL_CABC_04                        (DISPSYS_AAL_BASE + 0x21c)
#define DISP_AAL_STATUS_00                      (DISPSYS_AAL_BASE + 0x224)
/* 00 ~ 32: max histogram */
#define DISP_AAL_STATUS_32                      (DISPSYS_AAL_BASE + 0x2a4)
/* bit 8: dre_gain_force_en */
#define DISP_AAL_DRE_FLT_FORCE(idx)             (DISPSYS_AAL_BASE + 0x358 + (idx) * 4)
#define DISP_AAL_DRE_MAPPING_00                 (DISPSYS_AAL_BASE + 0x3b0)
#define DISP_AAL_CABC_GAINLMT_TBL(idx)          (DISPSYS_AAL_BASE + 0x40c + (idx) * 4)


#define DISP_PWM_EN_OFF                         (0x00)
#define DISP_PWM_COMMIT_OFF                     (0x08)
#define DISP_PWM_CON_0_OFF                      (0x10)
#define DISP_PWM_CON_1_OFF                      (0x14)


/* field definition */
/* ------------------------------------------------------------- */
/* DISP OD */
#define DISP_OD_EN                              (DISPSYS_OD_BASE + 0x000)
#define DISP_OD_RESET                           (DISPSYS_OD_BASE + 0x004)
#define DISP_OD_INTEN                           (DISPSYS_OD_BASE + 0x008)
#define DISP_OD_INTS                            (DISPSYS_OD_BASE + 0x00C)
#define DISP_OD_STATUS                          (DISPSYS_OD_BASE + 0x010)
#define DISP_OD_CFG                             (DISPSYS_OD_BASE + 0x020)
#define DISP_OD_INPUT_COUNT                     (DISPSYS_OD_BASE + 0x024)
#define DISP_OD_OUTPUT_COUNT                    (DISPSYS_OD_BASE + 0x028)
#define DISP_OD_CHKS_UM                         (DISPSYS_OD_BASE + 0x02c)
#define DISP_OD_SIZE                            (DISPSYS_OD_BASE + 0x030)
#define DISP_OD_HSYNC_WIDTH                     (DISPSYS_OD_BASE + 0x040)
#define DISP_OD_VSYNC_WIDTH                     (DISPSYS_OD_BASE + 0x044)
#define DISP_OD_MISC                            (DISPSYS_OD_BASE + 0x048)

/* ------------------------------------------------------------- */
/* COLOR */
#define CFG_MAIN_FLD_M_REG_RESET						REG_FLD(1, 31)
#define CFG_MAIN_FLD_M_DISP_RESET					REG_FLD(1, 30)
#define CFG_MAIN_FLD_COLOR_DBUF_EN					REG_FLD(1, 29)
#define CFG_MAIN_FLD_C_PP_CM_DBG_SEL					REG_FLD(4, 16)
#define CFG_MAIN_FLD_SEQ_SEL							REG_FLD(1, 13)
#define CFG_MAIN_FLD_ALLBP								REG_FLD(1, 7)
#define CFG_MAIN_FLD_HEBP								REG_FLD(1, 4)
#define CFG_MAIN_FLD_SEBP								REG_FLD(1, 3)
#define CFG_MAIN_FLD_YEBP								REG_FLD(1, 2)
#define CFG_MAIN_FLD_P2CBP								REG_FLD(1, 1)
#define CFG_MAIN_FLD_C2PBP								REG_FLD(1, 0)
#define START_FLD_DISP_COLOR_START					REG_FLD(1, 0)

#define DISP_COLOR_CFG_MAIN             (DISPSYS_COLOR0_BASE+0x400)
#define DISP_COLOR_PXL_CNT_MAIN         (DISPSYS_COLOR0_BASE+0x404)
#define DISP_COLOR_LINE_CNT_MAIN        (DISPSYS_COLOR0_BASE+0x408)
#define DISP_COLOR_WIN_X_MAIN           (DISPSYS_COLOR0_BASE+0x40C)
#define DISP_COLOR_WIN_Y_MAIN           (DISPSYS_COLOR0_BASE+0x410)
#define DISP_COLOR_TIMING_DETECTION_0           (DISPSYS_COLOR0_BASE+0x418)
#define DISP_COLOR_TIMING_DETECTION_1           (DISPSYS_COLOR0_BASE+0x41c)
#define DISP_COLOR_DBG_CFG_MAIN         (DISPSYS_COLOR0_BASE+0x420)
#define DISP_COLOR_C_BOOST_MAIN         (DISPSYS_COLOR0_BASE+0x428)
#define DISP_COLOR_C_BOOST_MAIN_2       (DISPSYS_COLOR0_BASE+0x42C)
#define DISP_COLOR_LUMA_ADJ     (DISPSYS_COLOR0_BASE+0x430)
#define DISP_COLOR_G_PIC_ADJ_MAIN_1     (DISPSYS_COLOR0_BASE+0x434)
#define DISP_COLOR_G_PIC_ADJ_MAIN_2     (DISPSYS_COLOR0_BASE+0x438)
#define DISP_COLOR_POS_MAIN     (DISPSYS_COLOR0_BASE+0x484)
#define DISP_COLOR_INK_DATA_MAIN     (DISPSYS_COLOR0_BASE+0x488)
#define DISP_COLOR_INK_DATA_MAIN_CR     (DISPSYS_COLOR0_BASE+0x48c)
#define DISP_COLOR_CAP_IN_DATA_MAIN     (DISPSYS_COLOR0_BASE+0x490)
#define DISP_COLOR_CAP_IN_DATA_MAIN_CR     (DISPSYS_COLOR0_BASE+0x494)
#define DISP_COLOR_CAP_OUT_DATA_MAIN     (DISPSYS_COLOR0_BASE+0x498)
#define DISP_COLOR_CAP_OUT_DATA_MAIN_CR     (DISPSYS_COLOR0_BASE+0x49c)
#define DISP_COLOR_Y_SLOPE_1_0_MAIN     (DISPSYS_COLOR0_BASE+0x4A0)
#define DISP_COLOR_LOCAL_HUE_CD_0       (DISPSYS_COLOR0_BASE+0x620)
#define DISP_COLOR_TWO_D_WINDOW_1       (DISPSYS_COLOR0_BASE+0x740)
#define DISP_COLOR_TWO_D_W1_RESULT      (DISPSYS_COLOR0_BASE+0x74C)
#define DISP_COLOR_SAT_HIST_X_CFG_MAIN      (DISPSYS_COLOR0_BASE+0x768)
#define DISP_COLOR_SAT_HIST_Y_CFG_MAIN      (DISPSYS_COLOR0_BASE+0x76c)
#define DISP_COLOR_BWS_2      (DISPSYS_COLOR0_BASE+0x79c)
#define DISP_COLOR_CRC_0      (DISPSYS_COLOR0_BASE+0x7e0)
#define DISP_COLOR_PART_SAT_GAIN1_0     (DISPSYS_COLOR0_BASE+0x7FC)
#define DISP_COLOR_PART_SAT_GAIN1_1     (DISPSYS_COLOR0_BASE+0x800)
#define DISP_COLOR_PART_SAT_GAIN1_2     (DISPSYS_COLOR0_BASE+0x804)
#define DISP_COLOR_PART_SAT_GAIN1_3     (DISPSYS_COLOR0_BASE+0x808)
#define DISP_COLOR_PART_SAT_GAIN1_4     (DISPSYS_COLOR0_BASE+0x80C)
#define DISP_COLOR_PART_SAT_GAIN2_0     (DISPSYS_COLOR0_BASE+0x810)
#define DISP_COLOR_PART_SAT_GAIN2_1     (DISPSYS_COLOR0_BASE+0x814)
#define DISP_COLOR_PART_SAT_GAIN2_2     (DISPSYS_COLOR0_BASE+0x818)
#define DISP_COLOR_PART_SAT_GAIN2_3	    (DISPSYS_COLOR0_BASE+0x81C)
#define DISP_COLOR_PART_SAT_GAIN2_4     (DISPSYS_COLOR0_BASE+0x820)
#define DISP_COLOR_PART_SAT_GAIN3_0     (DISPSYS_COLOR0_BASE+0x824)
#define DISP_COLOR_PART_SAT_GAIN3_1     (DISPSYS_COLOR0_BASE+0x828)
#define DISP_COLOR_PART_SAT_GAIN3_2     (DISPSYS_COLOR0_BASE+0x82C)
#define DISP_COLOR_PART_SAT_GAIN3_3     (DISPSYS_COLOR0_BASE+0x830)
#define DISP_COLOR_PART_SAT_GAIN3_4     (DISPSYS_COLOR0_BASE+0x834)
#define DISP_COLOR_PART_SAT_POINT1_0    (DISPSYS_COLOR0_BASE+0x838)
#define DISP_COLOR_PART_SAT_POINT1_1    (DISPSYS_COLOR0_BASE+0x83C)
#define DISP_COLOR_PART_SAT_POINT1_2    (DISPSYS_COLOR0_BASE+0x840)
#define DISP_COLOR_PART_SAT_POINT1_3    (DISPSYS_COLOR0_BASE+0x844)
#define DISP_COLOR_PART_SAT_POINT1_4    (DISPSYS_COLOR0_BASE+0x848)
#define DISP_COLOR_PART_SAT_POINT2_0    (DISPSYS_COLOR0_BASE+0x84C)
#define DISP_COLOR_PART_SAT_POINT2_1    (DISPSYS_COLOR0_BASE+0x850)
#define DISP_COLOR_PART_SAT_POINT2_2    (DISPSYS_COLOR0_BASE+0x854)
#define DISP_COLOR_PART_SAT_POINT2_3    (DISPSYS_COLOR0_BASE+0x858)
#define DISP_COLOR_PART_SAT_POINT2_4    (DISPSYS_COLOR0_BASE+0x85C)

#define DISP_COLOR_START        (DISPSYS_COLOR0_BASE+0xC00)
#define DISP_COLOR_INTEN        (DISPSYS_COLOR0_BASE+0xC04)
#define DISP_COLOR_OUT_SEL      (DISPSYS_COLOR0_BASE+0xC08)
#define DISP_COLOR_FRAME_DONE_DEL      (DISPSYS_COLOR0_BASE+0xC10)
#define DISP_COLOR_CRC      (DISPSYS_COLOR0_BASE+0xC14)
#define DISP_COLOR_SW_SCRATCH      (DISPSYS_COLOR0_BASE+0xC18)
#define DISP_COLOR_CK_ON        (DISPSYS_COLOR0_BASE+0xC28)
#define DISP_COLOR_INTERNAL_IP_WIDTH    (DISPSYS_COLOR0_BASE+0xC50)
#define DISP_COLOR_INTERNAL_IP_HEIGHT   (DISPSYS_COLOR0_BASE+0xC54)
#define DISP_COLOR_CM1_EN       (DISPSYS_COLOR0_BASE+0xC60)
#define DISP_COLOR_CM2_EN       (DISPSYS_COLOR0_BASE+0xCA0)



/* ------------------------------------------------------------- */
/* Config */
#define DISP_REG_CONFIG_MMSYS_INTEN				(DISPSYS_CONFIG_BASE + 0x0)
#define DISP_REG_CONFIG_MMSYS_INTSTA				(DISPSYS_CONFIG_BASE + 0x4)
#define DISP_REG_CONFIG_PWM_APB_ERR_ADDR				(DISPSYS_CONFIG_BASE + 0xc)
#define DISP_REG_CONFIG_ISP_MOUT_EN				(DISPSYS_CONFIG_BASE + 0x01c)
#define DISP_REG_CONFIG_MDP_RDMA0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x020)
#define DISP_REG_CONFIG_MDP_PRZ0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x024)
#define DISP_REG_CONFIG_MDP_PRZ1_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x028)
#define DISP_REG_CONFIG_MDP_TDSHP0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x02C)
#define DISP_REG_CONFIG_DISP_OVL0_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x030)
#define DISP_REG_CONFIG_DISP_OVL1_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x034)
#define DISP_REG_CONFIG_DISP_DITHER_MOUT_EN                     (DISPSYS_CONFIG_BASE + 0x038)
#define DISP_REG_CONFIG_DISP_UFOE_MOUT_EN			(DISPSYS_CONFIG_BASE + 0x03C)
#define DISP_REG_CONFIG_MMSYS_MOUT_RST			(DISPSYS_CONFIG_BASE + 0x040)
#define DISP_REG_CONFIG_MDP_PRZ0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x044)
#define DISP_REG_CONFIG_MDP_PRZ1_SEL_IN			(DISPSYS_CONFIG_BASE + 0x048)
#define DISP_REG_CONFIG_MDP_TDSHP0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x04C)
#define DISP_REG_CONFIG_MDP_WDMA0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x050)
#define DISP_REG_CONFIG_MDP_WROT0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x054)
#define DISP_REG_CONFIG_DISP_COLOR0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x058)
#define DISP_REG_CONFIG_DISP_WDMA0_SEL_IN			(DISPSYS_CONFIG_BASE + 0x05C)
#define DISP_REG_CONFIG_DISP_UFOE_SEL_IN			(DISPSYS_CONFIG_BASE + 0x060)
#define DISP_REG_CONFIG_DSI0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x064)
#define DISP_REG_CONFIG_DPI0_SEL_IN				(DISPSYS_CONFIG_BASE + 0x068)
#define DISP_REG_CONFIG_DISP_RDMA0_SOUT_SEL_IN                  (DISPSYS_CONFIG_BASE + 0x06C)
#define DISP_REG_CONFIG_DISP_RDMA1_SOUT_SEL_IN		(DISPSYS_CONFIG_BASE + 0x070)

#define DISP_REG_CONFIG_MMSYS_MISC					(DISPSYS_CONFIG_BASE + 0x0F0)
#define DISP_REG_CONFIG_MMSYS_CG_CON0					(DISPSYS_CONFIG_BASE + 0x100)
#define DISP_REG_CONFIG_MMSYS_CG_SET0					(DISPSYS_CONFIG_BASE + 0x104)
#define DISP_REG_CONFIG_MMSYS_CG_CLR0					(DISPSYS_CONFIG_BASE + 0x108)
#define DISP_REG_CONFIG_MMSYS_CG_CON1					(DISPSYS_CONFIG_BASE + 0x110)
#define DISP_REG_CONFIG_MMSYS_CG_SET1					(DISPSYS_CONFIG_BASE + 0x114)
#define DISP_REG_CONFIG_MMSYS_CG_CLR1					(DISPSYS_CONFIG_BASE + 0x118)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS0			(DISPSYS_CONFIG_BASE + 0x120)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_SET0                (DISPSYS_CONFIG_BASE + 0x124)
#define DISP_REG_CONFIG_MMSYS_HW_DCM_DIS_CLR0                (DISPSYS_CONFIG_BASE + 0x128)
#define DISP_REG_CONFIG_MMSYS_SW0_RST_B			(DISPSYS_CONFIG_BASE + 0x140)
#define DISP_REG_CONFIG_MMSYS_SW1_RST_B			(DISPSYS_CONFIG_BASE + 0x144)
#define DISP_REG_CONFIG_MMSYS_LCM_RST_B				(DISPSYS_CONFIG_BASE + 0x150)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_EN				(DISPSYS_CONFIG_BASE + 0x200)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RST			(DISPSYS_CONFIG_BASE + 0x204)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON0				(DISPSYS_CONFIG_BASE + 0x208)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_CON1				(DISPSYS_CONFIG_BASE + 0x20c)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_RD_ADDR		    (DISPSYS_CONFIG_BASE + 0x210)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_WR_ADDR		    (DISPSYS_CONFIG_BASE + 0x214)
#define DISP_REG_CONFIG_DISP_FAKE_ENG_STATE				(DISPSYS_CONFIG_BASE + 0x218)
#define DISP_REG_CONFIG_MMSYS_MBIST_CON			(DISPSYS_CONFIG_BASE + 0x800)
#define DISP_REG_CONFIG_MMSYS_MBIST_DONE			(DISPSYS_CONFIG_BASE + 0x804)
#define DISP_REG_CONFIG_MMSYS_MBIST_HOLDB			(DISPSYS_CONFIG_BASE + 0x808)
#define DISP_REG_CONFIG_MMSYS_MBIST_MODE			(DISPSYS_CONFIG_BASE + 0x80c)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL0			(DISPSYS_CONFIG_BASE + 0x810)
#define DISP_REG_CONFIG_MMSYS_MBIST_FAIL1			(DISPSYS_CONFIG_BASE + 0x814)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL0				(DISPSYS_CONFIG_BASE + 0x820)
#define DISP_REG_CONFIG_MMSYS_MBIST_BSEL1				(DISPSYS_CONFIG_BASE + 0x824)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL0			(DISPSYS_CONFIG_BASE + 0x830)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL1			(DISPSYS_CONFIG_BASE + 0x834)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL2			(DISPSYS_CONFIG_BASE + 0x838)
#define DISP_REG_CONFIG_MMSYS_MEM_DELSEL3			(DISPSYS_CONFIG_BASE + 0x83c)
#define DISP_REG_CONFIG_MMSYS_DEBUG_OUT_SEL			(DISPSYS_CONFIG_BASE + 0x880)
#define DISP_REG_CONFIG_MMSYS_DUMMY					(DISPSYS_CONFIG_BASE + 0x890)
#define DISP_REG_CONFIG_DISP_DL_VALID_0				(DISPSYS_CONFIG_BASE + 0x8a0)
#define DISP_REG_CONFIG_DISP_DL_READY_0				(DISPSYS_CONFIG_BASE + 0x8a4)
#define DISP_REG_CONFIG_MDP_DL_VALID_0				(DISPSYS_CONFIG_BASE + 0x8a8)
#define DISP_REG_CONFIG_MDP_DL_READY_0				(DISPSYS_CONFIG_BASE + 0x8ac)
#define DISP_REG_CONFIG_SMI_LARB0_GREQ				(DISPSYS_CONFIG_BASE + 0x8d0)
#define DISP_REG_CONFIG_C08                         (DISPSYS_CONFIG_BASE + 0xc08)
#define DISP_REG_CONFIG_C09                         (DISPSYS_CONFIG2_BASE + 0x40)	/* 0x10206040 */
#define DISP_REG_CONFIG_C10                         (DISPSYS_CONFIG2_BASE + 0x44)	/* 0x10206044 */
#define DISP_REG_CONFIG_C11                         (DISPSYS_CONFIG3_BASE + 0x40)	/* 0x10210040 */
#define DISP_REG_CONFIG_C12                         (DISPSYS_CONFIG3_BASE + 0x44)	/* 0x10210044 */
#define DISP_REG_CONFIG_C13                         (DISPSYS_CONFIG3_BASE + 0x48)	/* 0x10210048 */
#define DISP_REG_CLK_CFG_0_MM_CLK                   (DISPSYS_CONFIG_BASE+0x40)	/* BIT31 0 */
#define DISP_REG_CLK_CFG_0_CLR                      (DISPSYS_CONFIG_BASE+0x48)	/* BIT31  1 clear for mms */
#define DISP_REG_CLK_CFG_1_CLR                      (DISPSYS_CONFIG_BASE+0x58)	/* bit7 1 clear for pwm */
#define DISP_REG_CLK_CFG_6_DPI                      (DISPSYS_CONFIG_BASE+0xA0)	/* bit7 0 */
#define DISP_REG_CLK_CFG_6_CLR                      (DISPSYS_CONFIG_BASE+0xA8)	/* bit7 1 clear for dpi */
/* #define DISP_REG_VENCPLL_CON0                                        0xf0209260  // bit0 1 */
#define DISP_REG_VENCPLL_CON1                       (DISPSYS_VENCPLL_BASE + 0x254)	/* 0x10209254 */

#define MMSYS_INTEN_FLD_MMSYS_INTEN				REG_FLD(8, 0)
#define MMSYS_INSTA_FLD_MMSYS_INSTA				REG_FLD(2, 0)
#define MFG_APB_TX_CON_FLD_MFG_APB_COUNTER_EN				REG_FLD(1, 31)

#define MJC_APB_TX_CON_FLD_MJC_APB_COUNTER_EN				REG_FLD(1, 31)
#define MMSYS_TO_MFG_TX_ERROR                                   REG_FLD(1, 0)
#define MMSYS_TO_MJC_TX_ERROR                                   REG_FLD(1, 1)
#define PWM0_APB_TX_ERROR                                       REG_FLD(1, 2)

#define MFG_APB_TX_CON_FLD_MFG_APB_ERR_ADDR				REG_FLD(16, 0)
#define PWM_APB_ERR_ADDR_FLD_PWM0_APB_ERR_ADDR				REG_FLD(12, 0)
#define ISP_MOUT_EN_FLD_ISP_MOUT_EN				REG_FLD(2, 0)
#define MDP_RDMA0_MOUT_EN_FLD_MDP_RDMA0_MOUT_EN				REG_FLD(2, 0)
#define MDP_PRZ0_MOUT_EN_FLD_MDP_PRZ0_MOUT_EN		REG_FLD(3, 0)
#define MDP_PRZ1_MOUT_EN_FLD_MDP_PRZ1_MOUT_EN		REG_FLD(3, 0)
#define MDP_TDSHP0_MOUT_EN_FLD_MDP_TDSHP0_MOUT_EN		    REG_FLD(2, 0)
#define DISP_OVL0_MOUT_EN_FLD_DISP_OVL0_MOUT_EN                 REG_FLD(2, 0)
#define DISP_OVL1_MOUT_EN_FLD_DISP_OVL1_MOUT_EN				REG_FLD(2, 0)
#define DISP_DITHER_MOUT_EN_FLD_DISP_DITHER_MOUT_EN             REG_FLD(3, 0)
#define DISP_UFOE_MOUT_EN_FLD_DISP_UFOE_MOUT_EN				REG_FLD(3, 0)
#define MMSYS_MOUT_RST_FLD_MMSYS_MOUT_RST				REG_FLD(9, 0)
#define MDP_PRZ0_SEL_IN_FLD_MDP_PRZ0_SEL_IN			REG_FLD(1, 0)
#define MDP_PRZ1_SEL_IN_FLD_MDP_PRZ1_SEL_IN			REG_FLD(2, 0)
#define MDP_TDSHP0_SEL_IN_FLD_MDP_TDSHP0_SEL_IN                 REG_FLD(1, 0)
#define MDP_WDMA0_SEL_IN_FLD_MDP_WDMA0_SEL_IN			REG_FLD(2, 0)
#define MDP_WROT0_SEL_IN_FLD_MDP_WROT0_SEL_IN			REG_FLD(2, 0)
#define DISP_COLOR0_SEL_IN_FLD_DISP_COLOR0_SEL_IN               REG_FLD(1, 0)
#define DISP_WDMA0_SEL_IN_FLD_DISP_WDMA0_SEL_IN		REG_FLD(2, 0)
#define DISP_UFO_SEL_IN_FLD_DISP_UFO_SEL_IN			REG_FLD(1, 0)
#define DSI0_SEL_IN_FLD_DSI0_SEL_IN				REG_FLD(1, 0)
#define DPI0_SEL_IN_FLD_DPI0_SEL_IN                             REG_FLD(1, 0)
#define DISP_RDMA0_SOUT_SEL_IN_FLD_DISP_RDMA0_SOUT_SEL_IN       REG_FLD(1, 0)
#define DISP_RDMA1_SOUT_SEL_IN_FLD_DISP_RDMA1_SOUT_SEL_IN       REG_FLD(1, 0)
#define MMSYS_MISC_FLD_SMI_LARB0_TEST_MODE					    REG_FLD(1, 0)
#define MMSYS_CG_CON0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_SET0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_CLR0_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_CG_CON1_FLD_CG1                                   REG_FLD(32, 0)
#define MMSYS_CG_SET1_FLD_CG1                                   REG_FLD(32, 0)
#define MMSYS_CG_CLR1_FLD_CG0                                   REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS0_FLD_DCM_DIS0                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET0_FLD_DCM_DIS0                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR0_FLD_DCM_DIS0                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS1_FLD_DCM_DIS1                          REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_SET0_FLD_DCM_DIS1                      REG_FLD(32, 0)
#define MMSYS_HW_DCM_DIS_CLR0_FLD_DCM_DIS1                      REG_FLD(32, 0)
#define MMSYS_SW0_RST_B_FLD_SW0_RST_B                           REG_FLD(32, 0)
#define MMSYS_SW1_RST_B_FLD_SW1_RST_B                           REG_FLD(32, 0)
#define MMSYS_LCM_RST_B_FLD_LCM_RST_B                           REG_FLD(1, 0)
#define MM_VDEC_SLICE_CFG_FLD_MM_VDEC_SLICE_CFG_WD              REG_FLD(3, 0)
#define MM_VDEC_SLICE_CFG_FLD_MM_VDEC_SLICE_CFG_RD              REG_FLD(4, 4)
#define SMI_N21MUX_CFG_WR_FLD_SMI_N21MUX_CFG_WR                 REG_FLD(32, 0)
#define SMI_N21MUX_CFG_RD_FLD_SMI_N21MUX_CFG_RD                 REG_FLD(32, 0)
#define ELA2GMC_BASE_ADDR_FLD_ELA2GMC_BASE_ADDR		REG_FLD(32, 0)
#define ELA2GMC_BASE_ADDR_END_FLD_ELA2GMC_BASE_ADDR_END		    REG_FLD(32, 0)
#define ELA2GMC_FINAL_ADDR_FLD_ELA2GMC_FINAL_ADDR		REG_FLD(32, 0)
#define ELA2GMC_STATUS_FLD_ELA2GMC_STATUS				REG_FLD(1, 0)
#define LARB4_AXI_ASIF_CFG_WD_FLD_LARB4_AXI_ASIF_CFG_WD         REG_FLD(32, 0)
#define LARB4_AXI_ASIF_CFG_RD_FLD_LARB4_AXI_ASIF_CFG_RD         REG_FLD(32, 0)
#define DISP_FAKE_ENG_EN_FLD_DFE_START				REG_FLD(1, 1)
#define DISP_FAKE_ENG_EN_FLD_DFE_MUX_SEL			REG_FLD(1, 0)
#define DISP_FAKE_ENG_RST_FLD_DISP_FAKE_ENG_RST			REG_FLD(1, 0)
#define DISP_FAKE_ENG_CON0_FLD_DFE_DRE_EN			REG_FLD(1, 23)
#define DISP_FAKE_ENG_CON0_FLD_DFE_LOOP_MODE				REG_FLD(1, 22)
#define DISP_FAKE_ENG_CON0_FLD_DFE_TEST_LEN			REG_FLD(20, 0)
#define DISP_FAKE_ENG_CON1_FLD_DFE_WR_DIS			REG_FLD(1, 11)
#define DISP_FAKE_ENG_CON1_FLD_DFE_RD_DIS			REG_FLD(1, 10)
#define DISP_FAKE_ENG_CON1_FLD_DFE_SLOW_DOWN			REG_FLD(10, 0)
#define DISP_FAKE_ENG_RD_ADDR_FLD_DISP_FAKE_ENG_RD_ADDR			REG_FLD(32, 0)
#define DISP_FAKE_ENG_WR_ADDR_FLD_DISP_FAKE_ENG_WR_ADDR		REG_FLD(32, 0)
#define DISP_FAKE_ENG_STATE_FLD_DFE_RD_ST			REG_FLD(3, 12)
#define DISP_FAKE_ENG_STATE_FLD_DFE_WR_ST			REG_FLD(4, 8)
#define DISP_FAKE_ENG_STATE_FLD_DFE_BUSY			REG_FLD(1, 0)

#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_BACKGROUND			REG_FLD(3, 16)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_RSTB			REG_FLD(1, 15)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_SCANOUT_SEL			REG_FLD(4, 8)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_DEBUG			REG_FLD(1, 7)
#define MMSYS_MBIST_CON_FLD_MMSYS_MBIST_FAILOUT_SEL			REG_FLD(6, 0)
#define MMSYS_MBIST_DONE_FLD_MMSYS_MBIST_DONE			REG_FLD(14, 0)
#define MMSYS_MBIST_HOLDB_FLD_MMSYS_MBIST_HOLDB		REG_FLD(14, 0)
#define MMSYS_MBIST_MODE_FLD_MMSYS_MBIST_MODE			REG_FLD(14, 0)
#define MMSYS_MBIST_FAIL0_FLD_MMSYS_MBIST_FAIL0			REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL1_FLD_MMSYS_MBIST_FAIL1			REG_FLD(32, 0)
#define MMSYS_MBIST_FAIL2_FLD_MMSYS_MBIST_FAIL2			REG_FLD(24, 0)
#define MMSYS_MBIST_BSEL0_FLD_MDP_TDSHP_MBIST_BSEL			REG_FLD(16, 0)
#define MMSYS_MBIST_BSEL0_FLD_MDP_RDMA_MBIST_BSEL		REG_FLD(8, 16)
#define MMSYS_MBIST_BSEL0_FLD_MDP_WROT_MBIST_BSEL		REG_FLD(8, 24)
#define MMSYS_MBIST_BSEL1_FLD_MDP_RSZ0_MBIST_BSEL			REG_FLD(12, 0)
#define MMSYS_MBIST_BSEL1_FLD_MDP_RSZ1_MBIST_BSEL			REG_FLD(12, 12)
#define MMSYS_MBIST_BSEL1_FLD_DISP_UFOE_MBIST_BSEL		REG_FLD(6, 24)
#define MMSYS_MEM_DELSEL0_FLD_MDP_RSZ_MEM_DELSEL                REG_FLD(16, 0)
#define MMSYS_MEM_DELSEL0_FLD_MDP_RDMA_MEM_DELSEL               REG_FLD(14, 16)
#define MMSYS_MEM_DELSEL1_FLD_MDP_WROT_MEM_DELSEL				REG_FLD(22, 0)
#define MMSYS_MEM_DELSEL1_FLD_MDP_WDMA_MEM_DELSEL               REG_FLD(6, 22)
#define MMSYS_MEM_DELSEL1_FLD_MDP_TDSHP_MEM_DELSEL              REG_FLD(4, 28)


#define MMSYS_MEM_DELSEL2_FLD_DISP_OVL_MEM_DELSEL               REG_FLD(4, 0)
#define MMSYS_MEM_DELSEL2_FLD_DISP_RDMA_MEM_DELSEL              REG_FLD(4, 4)
#define MMSYS_MEM_DELSEL2_FLD_DISP_WDMA_MEM_DELSEL              REG_FLD(6, 8)
#define MMSYS_MEM_DELSEL2_FLD_DISP_GAMM_MEM_DELSEL              REG_FLD(4, 14)
#define MMSYS_MEM_DELSEL2_FLD_DISP_UFOE_MEM_DELSEL              REG_FLD(12, 18)

#define MMSYS_MEM_DELSEL3_FLD_DSI_MEM_DELSEL                    REG_FLD(4, 0)

#define MMSYS_DEBUG_OUT_SEL_FLD_MMSYS_DEBUG_OUT_SEL             REG_FLD(5, 0)
#define MMSYS_DUMMY0_FLD_MMSYS_DUMMY0                           REG_FLD(32, 0)
#define MMSYS_DUMMY1_FLD_MMSYS_DUMMY1                           REG_FLD(32, 0)
#define MMSYS_DUMMY2_FLD_MMSYS_DUMMY2                           REG_FLD(32, 0)
#define MMSYS_DUMMY3_FLD_MMSYS_DUMMY3                           REG_FLD(32, 0)
#define DISP_DL_VALID_0_FLD_DISP_DL_VALID_0                     REG_FLD(32, 0)
#define DISP_DL_READY_0_FLD_DISP_DL_READY_0                     REG_FLD(32, 0)
#define MDP_DL_VALID_0_FLD_MDP_DL_VALID_0                       REG_FLD(32, 0)
#define MDP_DL_READY_0_FLD_MDP_DL_READY_0                       REG_FLD(32, 0)
#define SMI_LARB0_GREQ_FLD_SMI_LARB0_GREQ                       REG_FLD(14, 0)

/* ------------------------------------------------------------- */
/* SMI_LARB */
#define DISP_REG_SMI_LARB_MMU_EN							(DISPSYS_SMI_LARB0_BASE+0xfc0)

/* ------------------------------------------------------------- */
/* DSI */
#define DISP_REG_DSI_STATE								(DISPSYS_DSI0_BASE + 0x004)

/* ------------------------------------------------------------- */
/* DPI */
#define DISP_REG_DPI_EN								(DISPSYS_DPI_BASE + 0x000)
#define DISP_REG_DPI_RST								(DISPSYS_DPI_BASE + 0x004)
#define DISP_REG_DPI_INTEN								(DISPSYS_DPI_BASE + 0x008)
#define DISP_REG_DPI_INSTA								(DISPSYS_DPI_BASE + 0x00C)
#define DISP_REG_DPI_CON								(DISPSYS_DPI_BASE + 0x010)
#define DISP_REG_DPI_OUTPUT_SETTING					(DISPSYS_DPI_BASE + 0x014)
#define DISP_REG_DPI_SIZE								(DISPSYS_DPI_BASE + 0x018)
#define DISP_REG_DPI_DDR_SETTING						(DISPSYS_DPI_BASE + 0x01c)
#define DISP_REG_DPI_TGEN_HWIDTH						(DISPSYS_DPI_BASE + 0x020)
#define DISP_REG_DPI_TGEN_HPORCH						(DISPSYS_DPI_BASE + 0x024)
#define DISP_REG_DPI_TGEN_VWIDTH						(DISPSYS_DPI_BASE + 0x028)
#define DISP_REG_DPI_TGEN_VPORCH						(DISPSYS_DPI_BASE + 0x02C)
#define DISP_REG_DPI_BG_HCNTL							(DISPSYS_DPI_BASE + 0x030)
#define DISP_REG_DPI_BG_VCNTL							(DISPSYS_DPI_BASE + 0x034)
#define DISP_REG_DPI_BG_COLOR							(DISPSYS_DPI_BASE + 0x038)
#define DISP_REG_DPI_FIFO_CTL							(DISPSYS_DPI_BASE + 0x03C)
#define DISP_REG_DPI_STATUS							(DISPSYS_DPI_BASE + 0x040)
#define DISP_REG_DPI_TMODE								(DISPSYS_DPI_BASE + 0x044)
#define DISP_REG_DPI_CHKSUM							(DISPSYS_DPI_BASE + 0x048)
#define DISP_REG_DPI_DUMMY							(DISPSYS_DPI_BASE + 0x050)
#define DISP_REG_DPI_TGEN_VWIDTH_LEVEN				(DISPSYS_DPI_BASE + 0x068)
#define DISP_REG_DPI_TGEN_VPORCH_LEVEN				(DISPSYS_DPI_BASE + 0x06c)
#define DISP_REG_DPI_TGEN_VWIDTH_RODD				(DISPSYS_DPI_BASE + 0x070)
#define DISP_REG_DPI_TGEN_VPORCH_RODD				(DISPSYS_DPI_BASE + 0x074)
#define DISP_REG_DPI_TGEN_VWIDTH_REVEN				(DISPSYS_DPI_BASE + 0x078)
#define DISP_REG_DPI_TGEN_VPORCH_REVEN				(DISPSYS_DPI_BASE + 0x07c)
#define DISP_REG_DPI_ESAV_VTIM_LODD					(DISPSYS_DPI_BASE + 0x080)
#define DISP_REG_DPI_ESAV_VTIM_LEVEN				(DISPSYS_DPI_BASE + 0x084)
#define DISP_REG_DPI_ESAV_VTIM_RODD					(DISPSYS_DPI_BASE + 0x088)
#define DISP_REG_DPI_ESAV_VTIM_REVEN				(DISPSYS_DPI_BASE + 0x08C)
#define DISP_REG_DPI_ESAV_FTIM						(DISPSYS_DPI_BASE + 0x090)
#define DISP_REG_DPI_CLPF_SETTING					(DISPSYS_DPI_BASE + 0x094)
#define DISP_REG_DPI_Y_LIMIT						(DISPSYS_DPI_BASE + 0x098)
#define DISP_REG_DPI_C_LIMIT						(DISPSYS_DPI_BASE + 0x09C)
#define DISP_REG_DPI_YUV422_SETTING					(DISPSYS_DPI_BASE + 0x0A0)
#define DISP_REG_DPI_EMBSYNC_SETTING				(DISPSYS_DPI_BASE + 0x0A4)
#define DISP_REG_DPI_ESAV_CODE_SET0					(DISPSYS_DPI_BASE + 0x0A8)
#define DISP_REG_DPI_ESAV_CODE_SET1					(DISPSYS_DPI_BASE + 0x0AC)

#define EN_FLD_EN									REG_FLD(1, 0)
#define RST_FLD_RST								REG_FLD(1, 0)
#define INTEN_FLD_INT_UNDERFLOW_EN				REG_FLD(1, 2)
#define INTEN_FLD_INT_VDE_EN						REG_FLD(1, 1)
#define INTEN_FLD_INT_VSYNC_EN					REG_FLD(1, 0)
#define INSTA_FLD_INTSTA_UNDERFLOW_EN				REG_FLD(1, 2)
#define INSTA_FLD_INTSTA_VDE_EN					REG_FLD(1, 1)
#define INSTA_FLD_INTSTA_VSYNC_EN					REG_FLD(1, 0)
#define CON_FLD_IN_RB_SWAP							REG_FLD(1, 1)
#define CON_FLD_BG_ENABLE							REG_FLD(1, 0)
#define OUTPUT_SETTING_FLD_EDGE_SEL					REG_FLD(1, 17)
#define OUTPUT_SETTING_FLD_OEN_OFF				REG_FLD(1, 16)
#define OUTPUT_SETTING_FLD_CK_POL					REG_FLD(1, 15)
#define OUTPUT_SETTING_FLD_VSYNC_POL				REG_FLD(1, 14)
#define OUTPUT_SETTING_FLD_HSYNC_POL				REG_FLD(1, 13)
#define OUTPUT_SETTING_FLD_DE_POL					REG_FLD(1, 12)
#define OUTPUT_SETTING_FLD_VS_MASK					REG_FLD(1, 10)
#define OUTPUT_SETTING_FLD_HS_MASK				REG_FLD(1, 9)
#define OUTPUT_SETTING_FLD_DE_MASK				REG_FLD(1, 8)
#define OUTPUT_SETTING_FLD_R_MASK					REG_FLD(1, 6)
#define OUTPUT_SETTING_FLD_G_MASK				REG_FLD(1, 5)
#define OUTPUT_SETTING_FLD_B_MASK				REG_FLD(1, 4)
#define OUTPUT_SETTING_FLD_BIT_SWAP				REG_FLD(1, 3)
#define OUTPUT_SETTING_FLD_CH_SWAP					REG_FLD(3, 0)
#define DPI_SIZE_FLD_HSIZE							REG_FLD(11, 16)
#define DPI_SIZE_FLD_VSIZE								REG_FLD(11, 0)
#define TGEN_HWIDTH_FLD_HPW							REG_FLD(12, 0)
#define TGEN_HPORCH_FLD_HFP							REG_FLD(12, 16)
#define TGEN_HPORCH_FLD_HBP							REG_FLD(12, 0)
#define TGEN_VWIDTH_FLD_VPW							REG_FLD(8, 0)
#define TGEN_VPORCH_FLD_VFP							REG_FLD(8, 16)
#define TGEN_VPORCH_FLD_VBP							REG_FLD(8, 0)
#define BG_HCNTL_FLD_BG_LEFT							REG_FLD(11, 16)
#define BG_HCNTL_FLD_BG_RIGHT						REG_FLD(11, 0)
#define BG_VCNTL_FLD_BG_TOP							REG_FLD(11, 16)
#define BG_VCNTL_FLD_BG_BOT							REG_FLD(11, 0)
#define BG_COLOR_FLD_BG_R								REG_FLD(8, 16)
#define BG_COLOR_FLD_BG_G							REG_FLD(8, 8)
#define BG_COLOR_FLD_BG_B								REG_FLD(8, 0)
#define FIFO_CTL_FLD_FIFO_RST_SEL					REG_FLD(1, 8)
#define FIFO_CTL_FLD_FIFO_VALID_SET					REG_FLD(5, 0)
#define STATUS_FLD_OUTEN								REG_FLD(1, 17)
#define STATUS_FLD_DPI_BUSY							REG_FLD(1, 16)
#define STATUS_FLD_V_COUNTER							REG_FLD(13, 0)
#define TMODE_FLD_DPI_OEN_ON							REG_FLD(1, 0)
#define CHKSUM_FLD_DPI_CHKSUM_EN					REG_FLD(1, 31)
#define CHKSUM_FLD_DPI_CHKSUM_READY				REG_FLD(1, 30)
#define CHKSUM_FLD_DPI_CHKSUM					REG_FLD(24, 0)
#define PATTERN_FLD_PAT_R_MAN						REG_FLD(8, 24)
#define PATTERN_FLD_PAT_G_MAN					REG_FLD(8, 16)
#define PATTERN_FLD_PAT_B_MAN						REG_FLD(8, 8)
#define PATTERN_FLD_PAT_SEL						REG_FLD(3, 4)
#define PATTERN_FLD_PAT_EN						REG_FLD(1, 0)

/* CCORR */
#define DISP_REG_CCORR_EN                                     (DISPSYS_CCORR_BASE + 0x000)
#define DISP_REG_CCORR_RESET                                     (DISPSYS_CCORR_BASE + 0x004)
#define DISP_REG_CCORR_INTEN                                     (DISPSYS_CCORR_BASE + 0x008)
#define DISP_REG_CCORR_INTSTA                                     (DISPSYS_CCORR_BASE + 0x00c)
#define DISP_REG_CCORR_STATUS                                     (DISPSYS_CCORR_BASE + 0x010)
#define DISP_REG_CCORR_CFG                                    (DISPSYS_CCORR_BASE + 0x020)
#define DISP_REG_CCORR_IN_CNT                                 (DISPSYS_CCORR_BASE + 0x024)
#define DISP_REG_CCORR_OUT_CNT                                (DISPSYS_CCORR_BASE + 0x028)
#define DISP_REG_CCORR_CHKSUM                                (DISPSYS_CCORR_BASE + 0x02c)
#define DISP_REG_CCORR_SIZE                                   (DISPSYS_CCORR_BASE + 0x030)
#define DISP_REG_CCORR_COEF_0                                   (DISPSYS_CCORR_BASE + 0x080)
#define DISP_REG_CCORR_DUMMY_REG                                   (DISPSYS_CCORR_BASE + 0x0c0)

#define CCORR_SIZE_FLD_HSIZE                                  REG_FLD(13, 16)
#define CCORR_SIZE_FLD_VSIZE                                  REG_FLD(13, 0)
#define CCORR_CFG_FLD_CHKSUM_SEL                              REG_FLD(3, 29)
#define CCORR_CFG_FLD_CHKSUM_EN                               REG_FLD(1, 28)
#define CCORR_CFG_FLD_CCORR_GAMMA_OFF                         REG_FLD(1, 2)
#define CCORR_CFG_FLD_CCORR_ENGINE_EN                         REG_FLD(1, 1)

/* ------------------------------------------------------------- */
/* GAMMA */
#define DISP_REG_GAMMA_EN								(DISPSYS_GAMMA_BASE + 0x000)
#define DISP_REG_GAMMA_RESET							(DISPSYS_GAMMA_BASE + 0x004)
#define DISP_REG_GAMMA_INTEN							(DISPSYS_GAMMA_BASE + 0x008)
#define DISP_REG_GAMMA_INTSTA							(DISPSYS_GAMMA_BASE + 0x00c)
#define DISP_REG_GAMMA_STATUS						(DISPSYS_GAMMA_BASE + 0x010)
#define DISP_REG_GAMMA_CFG							(DISPSYS_GAMMA_BASE + 0x020)
#define DISP_REG_GAMMA_INPUT_COUNT					(DISPSYS_GAMMA_BASE + 0x024)
#define DISP_REG_GAMMA_OUTPUT_COUNT					(DISPSYS_GAMMA_BASE + 0x028)
#define DISP_REG_GAMMA_CHKSUM						(DISPSYS_GAMMA_BASE + 0x02c)
#define DISP_REG_GAMMA_SIZE							(DISPSYS_GAMMA_BASE + 0x030)
#define DISP_REG_GAMMA_DUMMY_REG					(DISPSYS_GAMMA_BASE + 0x0c0)
#define DISP_REG_GAMMA_LUT							(DISPSYS_GAMMA_BASE + 0x700)

#define EN_FLD_GAMMA_EN					REG_FLD(1, 0)
#define RESET_FLD_GAMMA_RESET			REG_FLD(1, 0)
#define INTEN_FLD_OF_END_INT_EN			REG_FLD(1, 1)
#define INTEN_FLD_IF_END_INT_EN			REG_FLD(1, 0)
#define INTSTA_FLD_OF_END_INT				REG_FLD(1, 1)
#define INTSTA_FLD_IF_END_INT				REG_FLD(1, 0)
#define STATUS_FLD_IN_VALID					REG_FLD(1, 7)
#define STATUS_FLD_IN_READY					REG_FLD(1, 6)
#define STATUS_FLD_OUT_VALID					REG_FLD(1, 5)
#define STATUS_FLD_OUT_READY					REG_FLD(1, 4)
#define STATUS_FLD_OF_UNFINISH				REG_FLD(1, 1)
#define STATUS_FLD_IF_UNFINISH	                     REG_FLD(1, 0)
#define CFG_FLD_CHKSUM_SEL					REG_FLD(2, 29)
#define CFG_FLD_CHKSUM_EN						REG_FLD(1, 28)
#define CFG_FLD_CCORR_GAMMA_OFF			REG_FLD(1, 5)
#define CFG_FLD_CCORR_EN						REG_FLD(1, 4)
#define CFG_FLD_DITHER_EN						REG_FLD(1, 2)
#define CFG_FLD_GAMMA_LUT_EN				REG_FLD(1, 1)
#define CFG_FLD_RELAY_MODE					REG_FLD(1, 0)
#define INPUT_COUNT_FLD_INP_LINE_CNT			REG_FLD(13, 16)
#define INPUT_COUNT_FLD_INP_PIX_CNT			REG_FLD(13, 0)
#define OUTPUT_COUNT_FLD_OUTP_LINE_CNT		REG_FLD(13, 16)
#define OUTPUT_COUNT_FLD_OUTP_PIX_CNT		REG_FLD(13, 0)
#define CHKSUM_FLD_CHKSUM					REG_FLD(30, 0)
#define SIZE_FLD_HSIZE							REG_FLD(13, 16)
#define SIZE_FLD_VSIZE							REG_FLD(13, 0)
#define CCORR_0_FLD_CCORR_C00					REG_FLD(12, 16)
#define CCORR_0_FLD_CCORR_C01					REG_FLD(12, 0)
#define CCORR_1_FLD_CCORR_C02					REG_FLD(12, 16)
#define CCORR_1_FLD_CCORR_C10					REG_FLD(12, 0)
#define CCORR_2_FLD_CCORR_C11					REG_FLD(12, 16)
#define CCORR_2_FLD_CCORR_C12					REG_FLD(12, 0)
#define CCORR_3_FLD_CCORR_C20					REG_FLD(12, 16)
#define CCORR_3_FLD_CCORR_C21					REG_FLD(12, 0)
#define CCORR_4_FLD_CCORR_C22					REG_FLD(12, 16)
#define DUMMY_REG_FLD_DUMMY_REG				REG_FLD(32, 0)
#define DITHER_0_FLD_CRC_CLR					REG_FLD(1, 24)
#define DITHER_0_FLD_CRC_START				REG_FLD(1, 20)
#define DITHER_0_FLD_CRC_CEN					REG_FLD(1, 16)
#define DITHER_0_FLD_FRAME_DONE_DEL			REG_FLD(8, 8)
#define DITHER_0_FLD_OUT_SEL					REG_FLD(1, 4)
#define DITHER_5_FLD_W_DEMO					REG_FLD(16, 0)
#define DITHER_6_FLD_WRAP_MODE				REG_FLD(1, 16)
#define DITHER_6_FLD_LEFT_EN					REG_FLD(2, 14)
#define DITHER_6_FLD_FPHASE_R					REG_FLD(1, 13)
#define DITHER_6_FLD_FPHASE_EN				REG_FLD(1, 12)
#define DITHER_6_FLD_FPHASE					REG_FLD(6, 4)
#define DITHER_6_FLD_ROUND_EN					REG_FLD(1, 3)
#define DITHER_6_FLD_RDITHER_EN				REG_FLD(1, 2)
#define DITHER_6_FLD_LFSR_EN					REG_FLD(1, 1)
#define DITHER_6_FLD_EDITHER_EN				REG_FLD(1, 0)
#define DITHER_7_FLD_DRMOD_B					REG_FLD(2, 8)
#define DITHER_7_FLD_DRMOD_G					REG_FLD(2, 4)
#define DITHER_7_FLD_DRMOD_R					REG_FLD(2, 0)
#define GAMMA_DITHER_8_FLD_INK_DATA_R		REG_FLD(12, 16)
#define DITHER_8_FLD_INK						REG_FLD(1, 0)
#define GAMMA_DITHER_9_FLD_INK_DATA_B		REG_FLD(12, 16)
#define GAMMA_DITHER_9_FLD_INK_DATA_G		REG_FLD(12, 0)
#define DITHER_10_FLD_FPHASE_BIT				REG_FLD(3, 8)
#define DITHER_10_FLD_FPHASE_SEL				REG_FLD(2, 4)
#define DITHER_10_FLD_FPHASE_CTRL				REG_FLD(2, 0)
#define DITHER_11_FLD_SUB_B					REG_FLD(2, 12)
#define DITHER_11_FLD_SUB_G					REG_FLD(2, 8)
#define DITHER_11_FLD_SUB_R					REG_FLD(2, 4)
#define DITHER_11_FLD_SUBPIX_EN				REG_FLD(1, 0)
#define DITHER_12_FLD_H_ACTIVE				REG_FLD(16, 16)
#define DITHER_12_FLD_TABLE_EN				REG_FLD(2, 4)
#define DITHER_12_FLD_LSB_OFF					REG_FLD(1, 0)
#define DITHER_13_FLD_RSHIFT_B					REG_FLD(3, 8)
#define DITHER_13_FLD_RSHIFT_G				REG_FLD(3, 4)
#define DITHER_13_FLD_RSHIFT_R					REG_FLD(3, 0)
#define DITHER_14_FLD_DEBUG_MODE				REG_FLD(2, 8)
#define DITHER_14_FLD_DIFF_SHIFT				REG_FLD(3, 4)
#define DITHER_14_FLD_TESTPIN_EN				REG_FLD(1, 0)
#define DITHER_15_FLD_LSB_ERR_SHIFT_R			REG_FLD(3, 28)
#define DITHER_15_FLD_LSB_OVFLW_BIT_R		REG_FLD(3, 24)
#define DITHER_15_FLD_LSB_ADD_LSHIFT_R		REG_FLD(3, 20)
#define DITHER_15_FLD_LSB_INPUT_RSHIFT_R		REG_FLD(3, 16)
#define DITHER_15_FLD_LSB_NEW_BIT_MODE		REG_FLD(1, 0)
#define DITHER_16_FLD_LSB_ERR_SHIFT_B			REG_FLD(3, 28)
#define DITHER_16_FLD_OVFLW_BIT_B				REG_FLD(3, 24)
#define DITHER_16_FLD_ADD_LSHIFT_B			REG_FLD(3, 20)
#define DITHER_16_FLD_INPUT_RSHIFT_B			REG_FLD(3, 16)
#define DITHER_16_FLD_LSB_ERR_SHIFT_G			REG_FLD(3, 12)
#define DITHER_16_FLD_OVFLW_BIT_G				REG_FLD(3, 8)
#define DITHER_16_FLD_ADD_LSHIFT_G			REG_FLD(3, 4)
#define DITHER_16_FLD_INPUT_RSHIFT_G			REG_FLD(3, 0)
#define DITHER_17_FLD_CRC_RDY					REG_FLD(1, 16)
#define DITHER_17_FLD_CRC_OUT					REG_FLD(16, 0)
#define LUT_FLD_GAMMA_LUT_R					REG_FLD(10, 20)
#define LUT_FLD_GAMMA_LUT_G					REG_FLD(10, 10)
#define LUT_FLD_GAMMA_LUT_B					REG_FLD(10, 0)

/* ------------------------------------------------------------- */
/* Dither */
#define DISP_REG_DITHER_EN                                          (DISPSYS_DITHER_BASE + 0x000)
#define DISP_REG_DITHER_RESET                                          (DISPSYS_DITHER_BASE + 0x004)
#define DISP_REG_DITHER_INTEN                                          (DISPSYS_DITHER_BASE + 0x008)
#define DISP_REG_DITHER_INTSTA                                          (DISPSYS_DITHER_BASE + 0x00c)
#define DISP_REG_DITHER_STATUS                                          (DISPSYS_DITHER_BASE + 0x010)
#define DISP_REG_DITHER_CFG                                         (DISPSYS_DITHER_BASE + 0x020)
#define DISP_REG_DITHER_IN_CNT                                      (DISPSYS_DITHER_BASE + 0x024)
#define DISP_REG_DITHER_OUT_CNT                                     (DISPSYS_DITHER_BASE + 0x028)
#define DISP_REG_DITHER_CHKSUM                                     (DISPSYS_DITHER_BASE + 0x02c)
#define DISP_REG_DITHER_SIZE                                        (DISPSYS_DITHER_BASE + 0x030)
#define DISP_REG_DITHER_DUMMY_REG                                        (DISPSYS_DITHER_BASE + 0x0c0)
#define DISP_REG_DITHER_0                                        (DISPSYS_DITHER_BASE + 0x100)
#define DISP_REG_DITHER_5                                        (DISPSYS_DITHER_BASE + 0x114)
#define DISP_REG_DITHER_6                                        (DISPSYS_DITHER_BASE + 0x118)
#define DISP_REG_DITHER_7                                        (DISPSYS_DITHER_BASE + 0x11c)
#define DISP_REG_DITHER_8                                        (DISPSYS_DITHER_BASE + 0x120)
#define DISP_REG_DITHER_9                                        (DISPSYS_DITHER_BASE + 0x124)
#define DISP_REG_DITHER_10                                        (DISPSYS_DITHER_BASE + 0x128)
#define DISP_REG_DITHER_11                                        (DISPSYS_DITHER_BASE + 0x12c)
#define DISP_REG_DITHER_12                                        (DISPSYS_DITHER_BASE + 0x130)
#define DISP_REG_DITHER_13                                        (DISPSYS_DITHER_BASE + 0x134)
#define DISP_REG_DITHER_14                                        (DISPSYS_DITHER_BASE + 0x138)
#define DISP_REG_DITHER_15                                        (DISPSYS_DITHER_BASE + 0x13c)
#define DISP_REG_DITHER_16                                        (DISPSYS_DITHER_BASE + 0x140)
#define DISP_REG_DITHER_17                                        (DISPSYS_DITHER_BASE + 0x144)

#define DITHER_CFG_FLD_CHKSUM_SEL                       REG_FLD(2, 29)
#define DITHER_CFG_FLD_CHKSUM_EN                        REG_FLD(1, 28)
#define DITHER_CFG_FLD_DITHER_ENGINE_EN                 REG_FLD(1, 1)
#define DITHER_CFG_FLD_RELAY_MODE                       REG_FLD(1, 0)
#define DITHER_SIZE_FLD_HSIZE                            REG_FLD(13, 16)
#define DITHER_SIZE_FLD_VSIZE                            REG_FLD(13, 0)

/* ------------------------------------------------------------- */
/* MERGE */
#define DISP_REG_MERGE_ENABLE						(DISPSYS_MERGE_BASE + 0x000)
#define DISP_REG_MERGE_SW_RESET					(DISPSYS_MERGE_BASE + 0x004)
#define DISP_REG_MERGE_DEBUG						(DISPSYS_MERGE_BASE + 0x008)

#define ENABLE_FLD_MERGE_EN						REG_FLD(1, 0)
#define SW_RESET_FLD_MERGE_SW_RST				REG_FLD(1, 0)
#define DEBUG_FLD_MERGE_FSM						REG_FLD(3, 29)
#define DEBUG_FLD_OUT_PIXEL_CNT						REG_FLD(24, 0)

/* ------------------------------------------------------------- */
/* MUTEX */
#define DISP_OVL_SEPARATE_MUTEX_ID (DISP_MUTEX_DDP_LAST+1)	/* other disp will not see mutex 4 */
#define DISP_REG_CONFIG_MUTEX_INTEN				(DISPSYS_MUTEX_BASE + 0x000)
#define DISP_REG_CONFIG_MUTEX_INTSTA                             (DISPSYS_MUTEX_BASE + 0x004)
#define DISP_REG_CONFIG_MUTEX_HW_DCM                           (DISPSYS_MUTEX_BASE + 0x008)
#define DISP_REG_CONFIG_MUTEX0_EN					(DISPSYS_MUTEX_BASE + 0x020)
#define DISP_REG_CONFIG_MUTEX0_RST				(DISPSYS_MUTEX_BASE + 0x028)
#define DISP_REG_CONFIG_MUTEX0_MOD				(DISPSYS_MUTEX_BASE + 0x02C)
#define DISP_REG_CONFIG_MUTEX0_SOF				(DISPSYS_MUTEX_BASE + 0x030)
#define DISP_REG_CONFIG_MUTEX1_EN					(DISPSYS_MUTEX_BASE + 0x040)
#define DISP_REG_CONFIG_MUTEX1_RST				(DISPSYS_MUTEX_BASE + 0x048)
#define DISP_REG_CONFIG_MUTEX1_MOD				(DISPSYS_MUTEX_BASE + 0x04C)
#define DISP_REG_CONFIG_MUTEX1_SOF				(DISPSYS_MUTEX_BASE + 0x050)
#define DISP_REG_CONFIG_MUTEX2_EN					(DISPSYS_MUTEX_BASE + 0x060)
#define DISP_REG_CONFIG_MUTEX2_RST				(DISPSYS_MUTEX_BASE + 0x068)
#define DISP_REG_CONFIG_MUTEX2_MOD				(DISPSYS_MUTEX_BASE + 0x06C)
#define DISP_REG_CONFIG_MUTEX2_SOF				(DISPSYS_MUTEX_BASE + 0x070)
#define DISP_REG_CONFIG_MUTEX3_EN					(DISPSYS_MUTEX_BASE + 0x080)
#define DISP_REG_CONFIG_MUTEX3_RST				(DISPSYS_MUTEX_BASE + 0x088)
#define DISP_REG_CONFIG_MUTEX3_MOD				(DISPSYS_MUTEX_BASE + 0x08C)
#define DISP_REG_CONFIG_MUTEX3_SOF				(DISPSYS_MUTEX_BASE + 0x090)
#define DISP_REG_CONFIG_MUTEX4_EN					(DISPSYS_MUTEX_BASE + 0x0A0)
#define DISP_REG_CONFIG_MUTEX4_RST				(DISPSYS_MUTEX_BASE + 0x0A8)
#define DISP_REG_CONFIG_MUTEX4_MOD				(DISPSYS_MUTEX_BASE + 0x0AC)
#define DISP_REG_CONFIG_MUTEX4_SOF				(DISPSYS_MUTEX_BASE + 0x0B0)
#define DISP_REG_CONFIG_MUTEX5_EN					(DISPSYS_MUTEX_BASE + 0x0C0)
#define DISP_REG_CONFIG_MUTEX5_RST				(DISPSYS_MUTEX_BASE + 0x0C8)
#define DISP_REG_CONFIG_MUTEX5_MOD				(DISPSYS_MUTEX_BASE + 0x0CC)
#define DISP_REG_CONFIG_MUTEX5_SOF				(DISPSYS_MUTEX_BASE + 0x0D0)
#define DISP_REG_CONFIG_DEBUG_OUT_SEL				(DISPSYS_MUTEX_BASE + 0x200)

#define DISP_REG_CONFIG_MUTEX_EN(n)             (DISP_REG_CONFIG_MUTEX0_EN + (0x20 * n))
#define DISP_REG_CONFIG_MUTEX_RST(n)            (DISP_REG_CONFIG_MUTEX0_RST + (0x20 * n))
#define DISP_REG_CONFIG_MUTEX_MOD(n)            (DISP_REG_CONFIG_MUTEX0_MOD + (0x20 * n))
#define DISP_REG_CONFIG_MUTEX_SOF(n)            (DISP_REG_CONFIG_MUTEX0_SOF + (0x20 * n))

#define INTEN_FLD_MUTEX_INTEN						REG_FLD(12, 0)
#define INTSTA_FLD_MUTEX_INTSTA						REG_FLD(12, 0)
#define EN_FLD_MUTEX0_EN							REG_FLD(1, 0)
#define RST_FLD_MUTEX0_RST							REG_FLD(1, 0)
#define MOD_FLD_MUTEX0_MOD						REG_FLD(26, 0)
#define SOF_FLD_MUTEX0_SOF_TIMING				REG_FLD(1, 3)
#define SOF_FLD_MUTEX0_SOF						REG_FLD(3, 0)
#define EN_FLD_MUTEX1_EN							REG_FLD(1, 0)
#define RST_FLD_MUTEX1_RST							REG_FLD(1, 0)
#define MOD_FLD_MUTEX0_MOD							REG_FLD(26, 0)
#define SOF_FLD_MUTEX1_SOF_TIMING				REG_FLD(1, 3)
#define SOF_FLD_MUTEX1_SOF						REG_FLD(3, 0)
#define EN_FLD_MUTEX2_EN							REG_FLD(1, 0)
#define RST_FLD_MUTEX2_RST							REG_FLD(1, 0)
#define MOD_FLD_MUTEX0_MOD						REG_FLD(26, 0)
#define SOF_FLD_MUTEX2_SOF_TIMING					REG_FLD(1, 3)
#define SOF_FLD_MUTEX2_SOF						REG_FLD(3, 0)
#define EN_FLD_MUTEX3_EN							REG_FLD(1, 0)
#define RST_FLD_MUTEX3_RST							REG_FLD(1, 0)
#define MOD_FLD_MUTEX0_MOD							REG_FLD(26, 0)
#define SOF_FLD_MUTEX3_SOF_TIMING					REG_FLD(1, 3)
#define SOF_FLD_MUTEX3_SOF						REG_FLD(3, 0)
#define EN_FLD_MUTEX4_EN							REG_FLD(1, 0)
#define RST_FLD_MUTEX4_RST							REG_FLD(1, 0)
#define MOD_FLD_MUTEX0_MOD							REG_FLD(26, 0)
#define SOF_FLD_MUTEX4_SOF_TIMING					REG_FLD(1, 3)
#define SOF_FLD_MUTEX4_SOF						REG_FLD(3, 0)
#define EN_FLD_MUTEX5_EN							REG_FLD(1, 0)
#define RST_FLD_MUTEX5_RST							REG_FLD(1, 0)
#define MOD_FLD_MUTEX0_MOD							REG_FLD(26, 0)
#define SOF_FLD_MUTEX5_SOF_TIMING					REG_FLD(1, 3)
#define SOF_FLD_MUTEX5_SOF							REG_FLD(3, 0)
#define DEBUG_OUT_SEL_FLD_DEBUG_OUT_SEL			REG_FLD(2, 0)

/* ------------------------------------------------------------- */
/* OD */
#define DISP_REG_OD_EN           (DISPSYS_OD_BASE+0x000)
#define DISP_REG_OD_RESET        (DISPSYS_OD_BASE+0x004)
#define DISP_REG_OD_INTEN        (DISPSYS_OD_BASE+0x008)
#define DISP_REG_OD_INTSTA       (DISPSYS_OD_BASE+0x00C)
#define DISP_REG_OD_STATUS       (DISPSYS_OD_BASE+0x010)
#define DISP_REG_OD_CFG          (DISPSYS_OD_BASE+0x020)
#define DISP_REG_OD_INPUT_COUNT	 (DISPSYS_OD_BASE+0x024)
#define DISP_REG_OD_OUTPUT_COUNT (DISPSYS_OD_BASE+0x028)
#define DISP_REG_OD_CHKSUM       (DISPSYS_OD_BASE+0x02C)
#define DISP_REG_OD_SIZE	     (DISPSYS_OD_BASE+0x030)
#define DISP_REG_OD_HSYNC_WIDTH  (DISPSYS_OD_BASE+0x040)
#define DISP_REG_OD_VSYNC_WIDTH	 (DISPSYS_OD_BASE+0x044)
#define DISP_REG_OD_MISC         (DISPSYS_OD_BASE+0x048)
#define DISP_REG_OD_DUMMY_REG    (DISPSYS_OD_BASE+0x0C0)
#define DISP_REG_OD_DITHER_0	   (DISPSYS_OD_BASE+0x100)
#define DISP_REG_OD_DITHER_5     (DISPSYS_OD_BASE+0x114)
#define DISP_REG_OD_DITHER_6     (DISPSYS_OD_BASE+0x118)
#define DISP_REG_OD_DITHER_7	   (DISPSYS_OD_BASE+0x11C)
#define DISP_REG_OD_DITHER_8	   (DISPSYS_OD_BASE+0x120)
#define DISP_REG_OD_DITHER_9	   (DISPSYS_OD_BASE+0x124)
#define DISP_REG_OD_DITHER_10	   (DISPSYS_OD_BASE+0x128)
#define DISP_REG_OD_DITHER_11	   (DISPSYS_OD_BASE+0x12C)
#define DISP_REG_OD_DITHER_12	   (DISPSYS_OD_BASE+0x130)
#define DISP_REG_OD_DITHER_13	   (DISPSYS_OD_BASE+0x134)
#define DISP_REG_OD_DITHER_14	   (DISPSYS_OD_BASE+0x138)
#define DISP_REG_OD_DITHER_15	   (DISPSYS_OD_BASE+0x13C)
#define DISP_REG_OD_DITHER_16	   (DISPSYS_OD_BASE+0x140)
#define DISP_REG_OD_DITHER_17    (DISPSYS_OD_BASE+0x144)

/* ------------------------------------------------------------- */
/* OVL */
#define OVL_STATUS_IDLE 0
#define DISP_REG_OVL0_STATE_PA 0x14007240
#define DISP_REG_OVL1_STATE_PA 0x14008240
#define DISP_REG_OVL0_STATUS_PA 0x14007000
#define DISP_REG_OVL1_STATUS_PA 0x14008000
#define DISP_REG_OVL_STA							(DISPSYS_OVL0_BASE + 0x000)
#define DISP_REG_OVL_INTEN						(DISPSYS_OVL0_BASE + 0x004)
#define DISP_REG_OVL_INTSTA						(DISPSYS_OVL0_BASE + 0x008)
#define DISP_REG_OVL_EN							(DISPSYS_OVL0_BASE + 0x00C)
#define DISP_REG_OVL_TRIG							(DISPSYS_OVL0_BASE + 0x010)
#define DISP_REG_OVL_RST							(DISPSYS_OVL0_BASE + 0x014)
#define DISP_REG_OVL_ROI_SIZE						(DISPSYS_OVL0_BASE + 0x020)
#define DISP_REG_OVL_DATAPATH_CON				(DISPSYS_OVL0_BASE + 0x024)
#define DISP_REG_OVL_ROI_BGCLR						(DISPSYS_OVL0_BASE + 0x028)
#define DISP_REG_OVL_SRC_CON						(DISPSYS_OVL0_BASE + 0x02C)
#define DISP_REG_OVL_L0_CON						(DISPSYS_OVL0_BASE + 0x030)
#define DISP_REG_OVL_L0_SRCKEY						(DISPSYS_OVL0_BASE + 0x034)
#define DISP_REG_OVL_L0_SRC_SIZE					(DISPSYS_OVL0_BASE + 0x038)
#define DISP_REG_OVL_L0_OFFSET						(DISPSYS_OVL0_BASE + 0x03C)
#define DISP_REG_OVL_L0_ADDR						(DISPSYS_OVL0_BASE + 0xf40)
#define DISP_REG_OVL_L0_PITCH						(DISPSYS_OVL0_BASE + 0x044)
#define DISP_REG_OVL_L0_TILE						(DISPSYS_OVL0_BASE + 0x048)
#define DISP_REG_OVL_L1_CON						(DISPSYS_OVL0_BASE + 0x050)
#define DISP_REG_OVL_L1_SRCKEY						(DISPSYS_OVL0_BASE + 0x054)
#define DISP_REG_OVL_L1_SRC_SIZE					(DISPSYS_OVL0_BASE + 0x058)
#define DISP_REG_OVL_L1_OFFSET						(DISPSYS_OVL0_BASE + 0x05C)
#define DISP_REG_OVL_L1_ADDR						(DISPSYS_OVL0_BASE + 0xf60)
#define DISP_REG_OVL_L1_PITCH						(DISPSYS_OVL0_BASE + 0x064)
#define DISP_REG_OVL_L1_TILE						(DISPSYS_OVL0_BASE + 0x068)
#define DISP_REG_OVL_L2_CON						(DISPSYS_OVL0_BASE + 0x070)
#define DISP_REG_OVL_L2_SRCKEY						(DISPSYS_OVL0_BASE + 0x074)
#define DISP_REG_OVL_L2_SRC_SIZE					(DISPSYS_OVL0_BASE + 0x078)
#define DISP_REG_OVL_L2_OFFSET						(DISPSYS_OVL0_BASE + 0x07C)
#define DISP_REG_OVL_L2_ADDR						(DISPSYS_OVL0_BASE + 0xf80)
#define DISP_REG_OVL_L2_PITCH						(DISPSYS_OVL0_BASE + 0x084)
#define DISP_REG_OVL_L2_TILE						(DISPSYS_OVL0_BASE + 0x088)
#define DISP_REG_OVL_L3_CON						(DISPSYS_OVL0_BASE + 0x090)
#define DISP_REG_OVL_L3_SRCKEY						(DISPSYS_OVL0_BASE + 0x094)
#define DISP_REG_OVL_L3_SRC_SIZE					(DISPSYS_OVL0_BASE + 0x098)
#define DISP_REG_OVL_L3_OFFSET						(DISPSYS_OVL0_BASE + 0x09C)
#define DISP_REG_OVL_L3_ADDR						(DISPSYS_OVL0_BASE + 0xfA0)
#define DISP_REG_OVL_L3_PITCH						(DISPSYS_OVL0_BASE + 0x0A4)
#define DISP_REG_OVL_L3_TILE						(DISPSYS_OVL0_BASE + 0x0A8)
#define DISP_REG_OVL_RDMA0_CTRL						(DISPSYS_OVL0_BASE + 0x0C0)
#define DISP_REG_OVL_RDMA0_MEM_GMC_SETTING		(DISPSYS_OVL0_BASE + 0x0C8)
#define DISP_REG_OVL_RDMA0_MEM_SLOW_CON		(DISPSYS_OVL0_BASE + 0x0CC)
#define DISP_REG_OVL_RDMA0_FIFO_CTRL				(DISPSYS_OVL0_BASE + 0x0D0)
#define DISP_REG_OVL_RDMA1_CTRL						(DISPSYS_OVL0_BASE + 0x0E0)
#define DISP_REG_OVL_RDMA1_MEM_GMC_SETTING		(DISPSYS_OVL0_BASE + 0x0E8)
#define DISP_REG_OVL_RDMA1_MEM_SLOW_CON			(DISPSYS_OVL0_BASE + 0x0EC)
#define DISP_REG_OVL_RDMA1_FIFO_CTRL				(DISPSYS_OVL0_BASE + 0x0F0)
#define DISP_REG_OVL_RDMA2_CTRL						(DISPSYS_OVL0_BASE + 0x100)
#define DISP_REG_OVL_RDMA2_MEM_GMC_SETTING		(DISPSYS_OVL0_BASE + 0x108)
#define DISP_REG_OVL_RDMA2_MEM_SLOW_CON			(DISPSYS_OVL0_BASE + 0x10C)
#define DISP_REG_OVL_RDMA2_FIFO_CTRL				(DISPSYS_OVL0_BASE + 0x110)
#define DISP_REG_OVL_RDMA3_CTRL						(DISPSYS_OVL0_BASE + 0x120)
#define DISP_REG_OVL_RDMA3_MEM_GMC_SETTING		(DISPSYS_OVL0_BASE + 0x128)
#define DISP_REG_OVL_RDMA3_MEM_SLOW_CON			(DISPSYS_OVL0_BASE + 0x12C)
#define DISP_REG_OVL_RDMA3_FIFO_CTRL				(DISPSYS_OVL0_BASE + 0x130)
#define DISP_REG_OVL_L0_Y2R_PARA_R0				(DISPSYS_OVL0_BASE + 0x134)
#define DISP_REG_OVL_L0_Y2R_PARA_R1				(DISPSYS_OVL0_BASE + 0x138)
#define DISP_REG_OVL_L0_Y2R_PARA_G0				(DISPSYS_OVL0_BASE + 0x13C)
#define DISP_REG_OVL_L0_Y2R_PARA_G1				(DISPSYS_OVL0_BASE + 0x140)
#define DISP_REG_OVL_L0_Y2R_PARA_B0				(DISPSYS_OVL0_BASE + 0x144)
#define DISP_REG_OVL_L0_Y2R_PARA_B1				(DISPSYS_OVL0_BASE + 0x148)
#define DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0			(DISPSYS_OVL0_BASE + 0x14C)
#define DISP_REG_OVL_L0_Y2R_PARA_YUV_A_1			(DISPSYS_OVL0_BASE + 0x150)
#define DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0			(DISPSYS_OVL0_BASE + 0x154)
#define DISP_REG_OVL_L0_Y2R_PARA_RGB_A_1			(DISPSYS_OVL0_BASE + 0x158)
#define DISP_REG_OVL_L1_Y2R_PARA_R0				(DISPSYS_OVL0_BASE + 0x15C)
#define DISP_REG_OVL_L1_Y2R_PARA_R1				(DISPSYS_OVL0_BASE + 0x160)
#define DISP_REG_OVL_L1_Y2R_PARA_G0				(DISPSYS_OVL0_BASE + 0x164)
#define DISP_REG_OVL_L1_Y2R_PARA_G1				(DISPSYS_OVL0_BASE + 0x168)
#define DISP_REG_OVL_L1_Y2R_PARA_B0				(DISPSYS_OVL0_BASE + 0x16C)
#define DISP_REG_OVL_L1_Y2R_PARA_B1				(DISPSYS_OVL0_BASE + 0x170)
#define DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0			(DISPSYS_OVL0_BASE + 0x174)
#define DISP_REG_OVL_L1_Y2R_PARA_YUV_A_1			(DISPSYS_OVL0_BASE + 0x178)
#define DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0			(DISPSYS_OVL0_BASE + 0x17C)
#define DISP_REG_OVL_L1_Y2R_PARA_RGB_A_1			(DISPSYS_OVL0_BASE + 0x180)
#define DISP_REG_OVL_L2_Y2R_PARA_R0				(DISPSYS_OVL0_BASE + 0x184)
#define DISP_REG_OVL_L2_Y2R_PARA_R1				(DISPSYS_OVL0_BASE + 0x188)
#define DISP_REG_OVL_L2_Y2R_PARA_G0				(DISPSYS_OVL0_BASE + 0x18C)
#define DISP_REG_OVL_L2_Y2R_PARA_G1				(DISPSYS_OVL0_BASE + 0x190)
#define DISP_REG_OVL_L2_Y2R_PARA_B0				(DISPSYS_OVL0_BASE + 0x194)
#define DISP_REG_OVL_L2_Y2R_PARA_B1				(DISPSYS_OVL0_BASE + 0x198)
#define DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0			(DISPSYS_OVL0_BASE + 0x19C)
#define DISP_REG_OVL_L2_Y2R_PARA_YUV_A_1			(DISPSYS_OVL0_BASE + 0x1A0)
#define DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0			(DISPSYS_OVL0_BASE + 0x1A4)
#define DISP_REG_OVL_L2_Y2R_PARA_RGB_A_1			(DISPSYS_OVL0_BASE + 0x1A8)
#define DISP_REG_OVL_L3_Y2R_PARA_R0				(DISPSYS_OVL0_BASE + 0x1AC)
#define DISP_REG_OVL_L3_Y2R_PARA_R1				(DISPSYS_OVL0_BASE + 0x1B0)
#define DISP_REG_OVL_L3_Y2R_PARA_G0				(DISPSYS_OVL0_BASE + 0x1B4)
#define DISP_REG_OVL_L3_Y2R_PARA_G1				(DISPSYS_OVL0_BASE + 0x1B8)
#define DISP_REG_OVL_L3_Y2R_PARA_B0				(DISPSYS_OVL0_BASE + 0x1BC)
#define DISP_REG_OVL_L3_Y2R_PARA_B1				(DISPSYS_OVL0_BASE + 0x1C0)
#define DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0			(DISPSYS_OVL0_BASE + 0x1C4)
#define DISP_REG_OVL_L3_Y2R_PARA_YUV_A_1			(DISPSYS_OVL0_BASE + 0x1C8)
#define DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0			(DISPSYS_OVL0_BASE + 0x1CC)
#define DISP_REG_OVL_L3_Y2R_PARA_RGB_A_1			(DISPSYS_OVL0_BASE + 0x1D0)
#define DISP_REG_OVL_DEBUG_MON_SEL				(DISPSYS_OVL0_BASE + 0x1D4)
#define DISP_REG_OVL_RDMA0_MEM_GMC_S2		(DISPSYS_OVL0_BASE + 0x1E0)
#define DISP_REG_OVL_RDMA1_MEM_GMC_S2		(DISPSYS_OVL0_BASE + 0x1E4)
#define DISP_REG_OVL_RDMA2_MEM_GMC_S2		(DISPSYS_OVL0_BASE + 0x1E8)
#define DISP_REG_OVL_RDMA3_MEM_GMC_S2		(DISPSYS_OVL0_BASE + 0x1EC)
#define DISP_REG_OVL_DUMMY_REG						(DISPSYS_OVL0_BASE + 0x200)
#define DISP_REG_OVL_SMI_DBG                                    (DISPSYS_OVL0_BASE + 0x230)
#define DISP_REG_OVL_GREQ_LAYER_CNT                             (DISPSYS_OVL0_BASE + 0x234)
#define DISP_REG_OVL_FLOW_CTRL_DBG				(DISPSYS_OVL0_BASE + 0x240)
#define DISP_REG_OVL_ADDCON_DBG					(DISPSYS_OVL0_BASE + 0x244)
#define DISP_REG_OVL_RDMA0_DBG						(DISPSYS_OVL0_BASE + 0x24C)
#define DISP_REG_OVL_RDMA1_DBG						(DISPSYS_OVL0_BASE + 0x250)
#define DISP_REG_OVL_RDMA2_DBG						(DISPSYS_OVL0_BASE + 0x254)
#define DISP_REG_OVL_RDMA3_DBG						(DISPSYS_OVL0_BASE + 0x258)
#define DISP_REG_OVL_L0_CLR					(DISPSYS_OVL0_BASE + 0x25c)
#define DISP_REG_OVL_L1_CLR					(DISPSYS_OVL0_BASE + 0x260)
#define DISP_REG_OVL_L2_CLR					(DISPSYS_OVL0_BASE + 0x264)
#define DISP_REG_OVL_L3_CLR					(DISPSYS_OVL0_BASE + 0x268)

#define STA_FLD_RDMA3_IDLE							REG_FLD(1, 4)
#define STA_FLD_RDMA2_IDLE							REG_FLD(1, 3)
#define STA_FLD_RDMA1_IDLE							REG_FLD(1, 2)
#define STA_FLD_RDMA0_IDLE							REG_FLD(1, 1)
#define STA_FLD_RUN									REG_FLD(1, 0)
#define INTEN_FLD_RDMA3_FIFO_UNDERFLOW_INTEN		REG_FLD(1, 12)
#define INTEN_FLD_RDMA2_FIFO_UNDERFLOW_INTEN	REG_FLD(1, 11)
#define INTEN_FLD_RDMA1_FIFO_UNDERFLOW_INTEN		REG_FLD(1, 10)
#define INTEN_FLD_RDMA0_FIFO_UNDERFLOW_INTEN		REG_FLD(1, 9)
#define INTEN_FLD_RDMA3_EOF_ABNORMAL_INTEN		REG_FLD(1, 8)
#define INTEN_FLD_RDMA2_EOF_ABNORMAL_INTEN		REG_FLD(1, 7)
#define INTEN_FLD_RDMA1_EOF_ABNORMAL_INTEN		REG_FLD(1, 6)
#define INTEN_FLD_RDMA0_EOF_ABNORMAL_INTEN		REG_FLD(1, 5)
#define INTEN_FLD_FME_HWRST_DONE_INTEN			REG_FLD(1, 4)
#define INTEN_FLD_FME_SWRST_DONE_INTEN			REG_FLD(1, 3)
#define INTEN_FLD_FME_UND_INTEN						REG_FLD(1, 2)
#define INTEN_FLD_FME_CPL_INTEN						REG_FLD(1, 1)
#define INTEN_FLD_REG_CMT_INTEN						REG_FLD(1, 0)
#define INTSTA_FLD_RDMA3_FIFO_UNDERFLOW_INTSTA	REG_FLD(1, 12)
#define INTSTA_FLD_RDMA2_FIFO_UNDERFLOW_INTSTA	REG_FLD(1, 11)
#define INTSTA_FLD_RDMA1_FIFO_UNDERFLOW_INTSTA	REG_FLD(1, 10)
#define INTSTA_FLD_RDMA0_FIFO_UNDERFLOW_INTSTA	REG_FLD(1, 9)
#define INTSTA_FLD_RDMA3_EOF_ABNORMAL_INTSTA		REG_FLD(1, 8)
#define INTSTA_FLD_RDMA2_EOF_ABNORMAL_INTSTA		REG_FLD(1, 7)
#define INTSTA_FLD_RDMA1_EOF_ABNORMAL_INTSTA		REG_FLD(1, 6)
#define INTSTA_FLD_RDMA0_EOF_ABNORMAL_INTSTA		REG_FLD(1, 5)
#define INTSTA_FLD_FME_HWRST_DONE_INTSTA		REG_FLD(1, 4)
#define INTSTA_FLD_FME_SWRST_DONE_INTSTA		REG_FLD(1, 3)
#define INTSTA_FLD_FME_UND_INTSTA				REG_FLD(1, 2)
#define INTSTA_FLD_FME_CPL_INTSTA				REG_FLD(1, 1)
#define INTSTA_FLD_REG_CMT_INTSTA				REG_FLD(1, 0)
#define EN_FLD_EN									REG_FLD(1, 0)
#define TRIG_FLD_SW_TRIG							REG_FLD(1, 0)
#define RST_FLD_RST									REG_FLD(1, 0)
#define RST_FLD_FORCE_RST									REG_FLD(1, 31)
#define ROI_SIZE_FLD_ROI_H							REG_FLD(12, 16)
#define ROI_SIZE_FLD_ROI_W							REG_FLD(12, 0)
#define DATAPATH_CON_FLD_LAYER_GREQ_NUM			REG_FLD(3, 29)
#define DATAPATH_CON_FLD_RDMA3_OUT_SEL			REG_FLD(1, 23)
#define DATAPATH_CON_FLD_RDMA2_OUT_SEL			REG_FLD(1, 22)
#define DATAPATH_CON_FLD_RDMA1_OUT_SEL			REG_FLD(1, 21)
#define DATAPATH_CON_FLD_RDMA0_OUT_SEL			REG_FLD(1, 20)
#define DATAPATH_CON_FLD_PQ_OUT_SEL				REG_FLD(2, 16)
#define DATAPATH_CON_FLD_OVL_GAMMA_OUT		REG_FLD(1, 15)
#define DATAPATH_CON_FLD_ADOBE_LAYER			REG_FLD(2, 13)
#define DATAPATH_CON_FLD_ADOBE_MODE			REG_FLD(1, 12)
#define DATAPATH_CON_FLD_L3_GPU_MODE			REG_FLD(1, 11)
#define DATAPATH_CON_FLD_L2_GPU_MODE			REG_FLD(1, 10)
#define DATAPATH_CON_FLD_L1_GPU_MODE			REG_FLD(1, 9)
#define DATAPATH_CON_FLD_L0_GPU_MODE			REG_FLD(1, 8)
#define DATAPATH_CON_FLD_BGCLR_IN_SEL                                   REG_FLD(1, 2)
#define DATAPATH_CON_FLD_RANDOM_BGCLR_EN		REG_FLD(1, 1)
#define DATAPATH_CON_FLD_LAYER_SMI_ID_EN			REG_FLD(1, 0)
#define ROI_BGCLR_FLD_ALPHA							REG_FLD(8, 24)
#define ROI_BGCLR_FLD_RED							REG_FLD(8, 16)
#define ROI_BGCLR_FLD_GREEN							REG_FLD(8, 8)
#define ROI_BGCLR_FLD_BLUE							REG_FLD(8, 0)
#define SRC_CON_FLD_L3_EN			REG_FLD(1, 3)
#define SRC_CON_FLD_L2_EN			REG_FLD(1, 2)
#define SRC_CON_FLD_L1_EN			REG_FLD(1, 1)
#define SRC_CON_FLD_L0_EN			REG_FLD(1, 0)

#define L_CON_FLD_SKEN				REG_FLD(1, 30)
#define L_CON_FLD_LARC				REG_FLD(2, 28)	/* layer source 0:mem, 1:constant color */
#define L_CON_FLD_RGB_SWAP		REG_FLD(1, 25)
#define L_CON_FLD_BTSW				REG_FLD(1, 24)
#define L_CON_FLD_MTX				REG_FLD(4, 16)
#define L_CON_FLD_CFMT				REG_FLD(4, 12)
#define L_CON_FLD_HORI_FLIP	        REG_FLD(1, 10)
#define L_CON_FLD_VIRTICAL_FLIP	    REG_FLD(1, 9)
#define L_CON_FLD_AEN				REG_FLD(1, 8)
#define L_CON_FLD_APHA				REG_FLD(8, 0)

#define L_PITCH_FLD_LSP				REG_FLD(16, 0)
#define L_PITCH_FLD_SUR_ALFA		REG_FLD(16, 16)

#define ADDCON_DBG_FLD_L3_WIN_HIT		REG_FLD(1, 31)
#define ADDCON_DBG_FLD_L2_WIN_HIT		REG_FLD(1, 30)
#define ADDCON_DBG_FLD_ROI_Y			REG_FLD(13, 16)
#define ADDCON_DBG_FLD_L1_WIN_HIT		REG_FLD(1, 15)
#define ADDCON_DBG_FLD_L0_WIN_HIT		REG_FLD(1, 14)
#define ADDCON_DBG_FLD_ROI_X			REG_FLD(13, 0)

#define RDMA0_DBG_FLD_RDMA0_SMI_GREQ			REG_FLD(1, 29)
#define RDMA0_DBG_FLD_RDMA0_SMI_BUSY			REG_FLD(1, 28)
#define RDMA0_DBG_FLD_RDMA0_OUT_VALID			REG_FLD(1, 29)
#define RDMA0_DBG_FLD_RDMA0_OUT_READY			REG_FLD(1, 28)
#define RDMA0_DBG_FLD_RDMA0_OUT_DATA			REG_FLD(24, 4)
#define RDMA0_DBG_FLD_RDMA0_WRAM_RST_CS			REG_FLD(3, 0)
#define RDMA0_DBG_FLD_RDMA0_LAYER_GREQ			REG_FLD(1, 29)


/* ------------------------------------------------------------- */
/* RDMA */
#define DISP_REG_RDMA_INT_ENABLE				(DISPSYS_RDMA0_BASE+0x000)
#define DISP_REG_RDMA_INT_STATUS					(DISPSYS_RDMA0_BASE+0x004)
#define DISP_REG_RDMA_GLOBAL_CON				(DISPSYS_RDMA0_BASE+0x010)
#define DISP_REG_RDMA_SIZE_CON_0					(DISPSYS_RDMA0_BASE+0x014)
#define DISP_REG_RDMA_SIZE_CON_1					(DISPSYS_RDMA0_BASE+0x018)
#define DISP_REG_RDMA_TARGET_LINE					(DISPSYS_RDMA0_BASE+0x01C)
#define DISP_REG_RDMA_MEM_CON					(DISPSYS_RDMA0_BASE+0x024)
#define DISP_REG_RDMA_MEM_SRC_PITCH				(DISPSYS_RDMA0_BASE+0x02C)
#define DISP_REG_RDMA_MEM_GMC_SETTING_0			(DISPSYS_RDMA0_BASE+0x030)
#define DISP_REG_RDMA_MEM_SLOW_CON				(DISPSYS_RDMA0_BASE+0x034)
#define DISP_REG_RDMA_MEM_GMC_SETTING_1			(DISPSYS_RDMA0_BASE+0x038)
#define DISP_REG_RDMA_FIFO_CON					(DISPSYS_RDMA0_BASE+0x040)
#define DISP_REG_RDMA_FIFO_LOG					(DISPSYS_RDMA0_BASE+0x044)
#define DISP_REG_RDMA_C00							(DISPSYS_RDMA0_BASE+0x054)
#define DISP_REG_RDMA_C01							(DISPSYS_RDMA0_BASE+0x058)
#define DISP_REG_RDMA_C02							(DISPSYS_RDMA0_BASE+0x05C)
#define DISP_REG_RDMA_C10							(DISPSYS_RDMA0_BASE+0x060)
#define DISP_REG_RDMA_C11							(DISPSYS_RDMA0_BASE+0x064)
#define DISP_REG_RDMA_C12							(DISPSYS_RDMA0_BASE+0x068)
#define DISP_REG_RDMA_C20							(DISPSYS_RDMA0_BASE+0x06C)
#define DISP_REG_RDMA_C21							(DISPSYS_RDMA0_BASE+0x070)
#define DISP_REG_RDMA_C22							(DISPSYS_RDMA0_BASE+0x074)
#define DISP_REG_RDMA_PRE_ADD_0					(DISPSYS_RDMA0_BASE+0x078)
#define DISP_REG_RDMA_PRE_ADD_1					(DISPSYS_RDMA0_BASE+0x07C)
#define DISP_REG_RDMA_PRE_ADD_2					(DISPSYS_RDMA0_BASE+0x080)
#define DISP_REG_RDMA_POST_ADD_0				(DISPSYS_RDMA0_BASE+0x084)
#define DISP_REG_RDMA_POST_ADD_1					(DISPSYS_RDMA0_BASE+0x088)
#define DISP_REG_RDMA_POST_ADD_2					(DISPSYS_RDMA0_BASE+0x08C)
#define DISP_REG_RDMA_DUMMY						(DISPSYS_RDMA0_BASE+0x090)
#define DISP_REG_RDMA_IN_P_CNT						(DISPSYS_RDMA0_BASE+0x0f0)
#define DISP_REG_RDMA_IN_LINE_CNT						(DISPSYS_RDMA0_BASE+0x0f4)
#define DISP_REG_RDMA_OUT_P_CNT						(DISPSYS_RDMA0_BASE+0x0f8)
#define DISP_REG_RDMA_OUT_LINE_CNT						(DISPSYS_RDMA0_BASE+0x0fc)
#define DISP_REG_RDMA_DEBUG_OUT_SEL				(DISPSYS_RDMA0_BASE+0x094)
#define DISP_REG_RDMA_THRESHOLD_FOR_SODI                    (DISPSYS_RDMA0_BASE+0x0a8)
#define DISP_REG_RDMA_MEM_START_ADDR			(DISPSYS_RDMA0_BASE+0xf00)

#define INT_ENABLE_FLD_TARGET_LINE_INT_EN		REG_FLD(1, 5)
#define INT_ENABLE_FLD_FIFO_UNDERFLOW_INT_EN		REG_FLD(1, 4)
#define INT_ENABLE_FLD_EOF_ABNORMAL_INT_EN		REG_FLD(1, 3)
#define INT_ENABLE_FLD_FRAME_END_INT_EN			REG_FLD(1, 2)
#define INT_ENABLE_FLD_FRAME_START_INT_EN			REG_FLD(1, 1)
#define INT_ENABLE_FLD_REG_UPDATE_INT_EN			REG_FLD(1, 0)
#define INT_STATUS_FLD_TARGET_LINE_INT_FLAG		REG_FLD(1, 5)
#define INT_STATUS_FLD_FIFO_UNDERFLOW_INT_FLAG	REG_FLD(1, 4)
#define INT_STATUS_FLD_EOF_ABNORMAL_INT_FLAG		REG_FLD(1, 3)
#define INT_STATUS_FLD_FRAME_END_INT_FLAG			REG_FLD(1, 2)
#define INT_STATUS_FLD_FRAME_START_INT_FLAG		REG_FLD(1, 1)
#define INT_STATUS_FLD_REG_UPDATE_INT_FLAG		REG_FLD(1, 0)
#define GLOBAL_CON_FLD_SMI_BUSY					REG_FLD(1, 12)
#define GLOBAL_CON_FLD_RESET_STATE				REG_FLD(3, 8)
#define GLOBAL_CON_FLD_SOFT_RESET				REG_FLD(1, 4)
#define GLOBAL_CON_FLD_MODE_SEL					REG_FLD(1, 1)
#define GLOBAL_CON_FLD_ENGINE_EN				REG_FLD(1, 0)
#define SIZE_CON_0_FLD_MATRIX_INT_MTX_SEL			REG_FLD(4, 20)
#define SIZE_CON_0_FLD_MATRIX_WIDE_GAMUT_EN		REG_FLD(1, 18)
#define SIZE_CON_0_FLD_MATRIX_ENABLE				REG_FLD(1, 17)
#define SIZE_CON_0_FLD_MATRIX_EXT_MTX_EN			REG_FLD(1, 16)
#define SIZE_CON_0_FLD_OUTPUT_FRAME_WIDTH		REG_FLD(13, 0)
#define SIZE_CON_1_FLD_OUTPUT_FRAME_HEIGHT		REG_FLD(20, 0)
#define TARGET_LINE_FLD_TARGET_LINE				REG_FLD(20, 0)
#define MEM_CON_FLD_MEM_MODE_HORI_BLOCK_NUM	REG_FLD(8, 24)
#define MEM_CON_FLD_MEM_MODE_INPUT_COSITE		REG_FLD(1, 13)
#define MEM_CON_FLD_MEM_MODE_INPUT_UPSAMPLE		REG_FLD(1, 12)
#define MEM_CON_FLD_MEM_MODE_INPUT_SWAP			REG_FLD(1, 8)
#define MEM_CON_FLD_MEM_MODE_INPUT_FORMAT		REG_FLD(4, 4)
#define MEM_CON_FLD_MEM_MODE_TILE_INTERLACE		REG_FLD(1, 1)
#define MEM_CON_FLD_MEM_MODE_TILE_EN                           REG_FLD(1, 0)
#define MEM_SRC_PITCH_FLD_MEM_MODE_SRC_PITCH		REG_FLD(16, 0)
#define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_HIGH_OFS	REG_FLD(8, 24)
#define MEM_GMC_SETTING_0_FLD_ULTRA_THRESHOLD_HIGH_OFS		REG_FLD(8, 16)
#define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_LOW_OFS	REG_FLD(8, 8)
#define MEM_GMC_SETTING_0_FLD_ULTRA_THRESHOLD_LOW			REG_FLD(8, 0)
#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_COUNT				REG_FLD(16, 16)
#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_EN					REG_FLD(1, 0)
#define MEM_GMC_SETTING_1_FLD_ISSUE_REQ_THRESHOLD			REG_FLD(8, 0)
#define FIFO_CON_FLD_FIFO_UNDERFLOW_EN						REG_FLD(1, 31)
#define FIFO_CON_FLD_FIFO_PSEUDO_SIZE							REG_FLD(10, 16)
#define FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD			REG_FLD(10, 0)
#define FIFO_LOG_FLD_RDMA_FIFO_LOG					REG_FLD(10, 0)
#define C00_FLD_DISP_RDMA_C00							REG_FLD(13, 0)
#define C01_FLD_DISP_RDMA_C01							REG_FLD(13, 0)
#define C02_FLD_DISP_RDMA_C02							REG_FLD(13, 0)
#define C10_FLD_DISP_RDMA_C10							REG_FLD(13, 0)
#define C11_FLD_DISP_RDMA_C11							REG_FLD(13, 0)
#define C12_FLD_DISP_RDMA_C12							REG_FLD(13, 0)
#define C20_FLD_DISP_RDMA_C20							REG_FLD(13, 0)
#define C21_FLD_DISP_RDMA_C21							REG_FLD(13, 0)
#define C22_FLD_DISP_RDMA_C22							REG_FLD(13, 0)
#define PRE_ADD_0_FLD_DISP_RDMA_PRE_ADD_0				REG_FLD(9, 0)
#define PRE_ADD_1_FLD_DISP_RDMA_PRE_ADD_1				REG_FLD(9, 0)
#define PRE_ADD_2_FLD_DISP_RDMA_PRE_ADD_2				REG_FLD(9, 0)
#define POST_ADD_0_FLD_DISP_RDMA_POST_ADD_0			REG_FLD(9, 0)
#define POST_ADD_1_FLD_DISP_RDMA_POST_ADD_1			REG_FLD(9, 0)
#define POST_ADD_2_FLD_DISP_RDMA_POST_ADD_2			REG_FLD(9, 0)
#define DUMMY_FLD_DISP_RDMA_DUMMY					REG_FLD(32, 0)
#define DEBUG_OUT_SEL_FLD_DISP_RDMA_DEBUG_OUT_SEL	REG_FLD(4, 0)
#define MEM_START_ADDR_FLD_MEM_MODE_START_ADDR		REG_FLD(32, 0)

/* ------------------------------------------------------------- */
/* SPLIT */
#define DISP_REG_SPLIT_ENABLE							(DISPSYS_SPLIT0_BASE+0x00)
#define DISP_REG_SPLIT_SW_RESET						(DISPSYS_SPLIT0_BASE+0x04)
#define DISP_REG_SPLIT_DEBUG							(DISPSYS_SPLIT0_BASE+0x08)

#define ENABLE_FLD_SPLIT_EN							REG_FLD(1, 0)
#define W_RESET_FLD_SPLIT_SW_RST						REG_FLD(1, 0)
#define DEBUG_FLD_SPLIT_FSM							REG_FLD(3, 29)
#define DEBUG_FLD_IN_PIXEL_CNT							REG_FLD(24, 0)

/* ------------------------------------------------------------- */
/* UFO */
#define DISP_REG_UFO_START							0

/* ------------------------------------------------------------- */
/* WDMA */
#define DISP_REG_WDMA_INTEN							(DISPSYS_WDMA0_BASE+0x000)
#define DISP_REG_WDMA_INTSTA							(DISPSYS_WDMA0_BASE+0x004)
#define DISP_REG_WDMA_EN							(DISPSYS_WDMA0_BASE+0x008)
#define DISP_REG_WDMA_RST							(DISPSYS_WDMA0_BASE+0x00C)
#define DISP_REG_WDMA_SMI_CON							(DISPSYS_WDMA0_BASE+0x010)
#define DISP_REG_WDMA_CFG							(DISPSYS_WDMA0_BASE+0x014)
#define DISP_REG_WDMA_SRC_SIZE							(DISPSYS_WDMA0_BASE+0x018)
#define DISP_REG_WDMA_CLIP_SIZE						(DISPSYS_WDMA0_BASE+0x01C)
#define DISP_REG_WDMA_CLIP_COORD					(DISPSYS_WDMA0_BASE+0x020)
#define DISP_REG_WDMA_DST_W_IN_BYTE					(DISPSYS_WDMA0_BASE+0x028)
#define DISP_REG_WDMA_ALPHA							(DISPSYS_WDMA0_BASE+0x02C)
#define DISP_REG_WDMA_BUF_CON1							(DISPSYS_WDMA0_BASE+0x038)
#define DISP_REG_WDMA_BUF_CON2							(DISPSYS_WDMA0_BASE+0x03C)
#define DISP_REG_WDMA_C00							(DISPSYS_WDMA0_BASE+0x040)
#define DISP_REG_WDMA_C02							(DISPSYS_WDMA0_BASE+0x044)
#define DISP_REG_WDMA_C10							(DISPSYS_WDMA0_BASE+0x048)
#define DISP_REG_WDMA_C12							(DISPSYS_WDMA0_BASE+0x04C)
#define DISP_REG_WDMA_C20							(DISPSYS_WDMA0_BASE+0x050)
#define DISP_REG_WDMA_C22							(DISPSYS_WDMA0_BASE+0x054)
#define DISP_REG_WDMA_PRE_ADD0							(DISPSYS_WDMA0_BASE+0x058)
#define DISP_REG_WDMA_PRE_ADD2							(DISPSYS_WDMA0_BASE+0x05C)
#define DISP_REG_WDMA_POST_ADD0					(DISPSYS_WDMA0_BASE+0x060)
#define DISP_REG_WDMA_POST_ADD2					(DISPSYS_WDMA0_BASE+0x064)
#define DISP_REG_WDMA_DST_UV_PITCH					(DISPSYS_WDMA0_BASE+0x078)
#define DISP_REG_WDMA_DST_ADDR_OFFSET0				(DISPSYS_WDMA0_BASE+0x080)
#define DISP_REG_WDMA_DST_ADDR_OFFSET1				(DISPSYS_WDMA0_BASE+0x084)
#define DISP_REG_WDMA_DST_ADDR_OFFSET2				(DISPSYS_WDMA0_BASE+0x088)
#define DISP_REG_WDMA_PROC_TRACK_CON_0				(DISPSYS_WDMA0_BASE+0x090)
#define DISP_REG_WDMA_PROC_TRACK_CON_1				(DISPSYS_WDMA0_BASE+0x094)
#define DISP_REG_WDMA_PROC_TRACK_CON_2				(DISPSYS_WDMA0_BASE+0x098)
#define DISP_REG_WDMA_FLOW_CTRL_DBG					(DISPSYS_WDMA0_BASE+0x0A0)
#define DISP_REG_WDMA_EXEC_DBG							(DISPSYS_WDMA0_BASE+0x0A4)
#define DISP_REG_WDMA_CT_DBG							(DISPSYS_WDMA0_BASE+0x0A8)
#define DISP_REG_WDMA_SMI_TRAFFIC_DBG							(DISPSYS_WDMA0_BASE+0x0AC)
#define DISP_REG_WDMA_PROC_TRACK_DBG_0							(DISPSYS_WDMA0_BASE+0x0b0)
#define DISP_REG_WDMA_PROC_TRACK_DBG_1							(DISPSYS_WDMA0_BASE+0x0b4)
#define DISP_REG_WDMA_DEBUG							(DISPSYS_WDMA0_BASE+0x0b8)
#define DISP_REG_WDMA_DUMMY							(DISPSYS_WDMA0_BASE+0x100)
#define DISP_REG_WDMA_DITHER_0							(DISPSYS_WDMA0_BASE+0xE00)
#define DISP_REG_WDMA_DITHER_5							(DISPSYS_WDMA0_BASE+0xE14)
#define DISP_REG_WDMA_DITHER_6							(DISPSYS_WDMA0_BASE+0xE18)
#define DISP_REG_WDMA_DITHER_7							(DISPSYS_WDMA0_BASE+0xE1C)
#define DISP_REG_WDMA_DITHER_8							(DISPSYS_WDMA0_BASE+0xE20)
#define DISP_REG_WDMA_DITHER_9							(DISPSYS_WDMA0_BASE+0xE24)
#define DISP_REG_WDMA_DITHER_10					(DISPSYS_WDMA0_BASE+0xE28)
#define DISP_REG_WDMA_DITHER_11					(DISPSYS_WDMA0_BASE+0xE2C)
#define DISP_REG_WDMA_DITHER_12					(DISPSYS_WDMA0_BASE+0xE30)
#define DISP_REG_WDMA_DITHER_13					(DISPSYS_WDMA0_BASE+0xE34)
#define DISP_REG_WDMA_DITHER_14					(DISPSYS_WDMA0_BASE+0xE38)
#define DISP_REG_WDMA_DITHER_15					(DISPSYS_WDMA0_BASE+0xE3C)
#define DISP_REG_WDMA_DITHER_16					(DISPSYS_WDMA0_BASE+0xE40)
#define DISP_REG_WDMA_DITHER_17					(DISPSYS_WDMA0_BASE+0xE44)
#define DISP_REG_WDMA_DST_ADDR0					(DISPSYS_WDMA0_BASE+0xF00)
#define DISP_REG_WDMA_DST_ADDR1					(DISPSYS_WDMA0_BASE+0xF04)
#define DISP_REG_WDMA_DST_ADDR2					(DISPSYS_WDMA0_BASE+0xF08)

#define INTEN_FLD_FRAME_UNDERRUN						REG_FLD(1, 1)
#define INTEN_FLD_FRAME_COMPLETE						REG_FLD(1, 0)
#define INTSTA_FLD_FRAME_UNDERRUN						REG_FLD(1, 1)
#define INTSTA_FLD_FRAME_COMPLETE						REG_FLD(1, 0)
#define EN_FLD_ENABLE									REG_FLD(1, 0)
#define RST_FLD_SOFT_RESET								REG_FLD(1, 0)
#define SMI_CON_FLD_SMI_V_REPEAT_NUM					REG_FLD(4, 24)
#define SMI_CON_FLD_SMI_U_REPEAT_NUM					REG_FLD(4, 20)
#define SMI_CON_FLD_SMI_Y_REPEAT_NUM					REG_FLD(4, 16)
#define SMI_CON_FLD_SLOW_COUNT						REG_FLD(8, 8)
#define SMI_CON_FLD_SLOW_LEVEL							REG_FLD(3, 5)
#define SMI_CON_FLD_SLOW_ENABLE						REG_FLD(1, 4)
#define SMI_CON_FLD_THRESHOLD							REG_FLD(4, 0)
#define CFG_FLD_DEBUG_SEL								REG_FLD(4, 28)
#define CFG_FLD_INT_MTX_SEL							REG_FLD(4, 24)
#define CFG_FLD_SWAP									REG_FLD(1, 16)
#define CFG_FLD_DNSP_SEL								REG_FLD(1, 15)
#define CFG_FLD_EXT_MTX_EN								REG_FLD(1, 13)
#define CFG_FLD_VERTICAL_AVG							REG_FLD(1, 12)
#define CFG_FLD_CT_EN									REG_FLD(1, 11)
#define CFG_FLD_OUT_FORMAT								REG_FLD(4, 4)
#define SRC_SIZE_FLD_HEIGHT							REG_FLD(14, 16)
#define SRC_SIZE_FLD_WIDTH								REG_FLD(14, 0)
#define CLIP_SIZE_FLD_HEIGHT							REG_FLD(14, 16)
#define CLIP_SIZE_FLD_WIDTH							REG_FLD(14, 0)
#define CLIP_COORD_FLD_Y_COORD						REG_FLD(14, 16)
#define CLIP_COORD_FLD_X_COORD						REG_FLD(14, 0)
#define DST_W_IN_BYTE_FLD_DST_W_IN_BYTE				REG_FLD(16, 0)
#define ALPHA_FLD_A_SEL								REG_FLD(1, 31)
#define ALPHA_FLD_A_VALUE								REG_FLD(8, 0)
#define BUF_CON1_FLD_ULTRA_ENABLE						REG_FLD(1, 31)
#define BUF_CON1_FLD_FRAME_END_ULTRA				REG_FLD(1, 28)
#define BUF_CON1_FLD_ISSUE_REQ_TH						REG_FLD(9, 16)
#define BUF_CON1_FLD_FIFO_PSEUDO_SIZE				REG_FLD(9, 0)
#define BUF_CON2_FLD_ULTRA_TH_HIGH_OFS				REG_FLD(8, 24)
#define BUF_CON2_FLD_PRE_ULTRA_TH_HIGH_OFS			REG_FLD(8, 16)
#define BUF_CON2_FLD_ULTRA_TH_LOW_OFS				REG_FLD(8, 8)
#define BUF_CON2_FLD_PRE_ULTRA_TH_LOW				REG_FLD(8, 0)
#define C00_FLD_C01									REG_FLD(13, 16)
#define C00_FLD_C00									REG_FLD(13, 0)
#define C02_FLD_C02									REG_FLD(13, 0)
#define C10_FLD_C11									REG_FLD(13, 16)
#define C10_FLD_C10									REG_FLD(13, 0)
#define C12_FLD_C12									REG_FLD(13, 0)
#define C20_FLD_C21									REG_FLD(13, 16)
#define C20_FLD_C20									REG_FLD(13, 0)
#define C22_FLD_C22									REG_FLD(13, 0)
#define PRE_ADD0_FLD_PRE_ADD_1						REG_FLD(9, 16)
#define PRE_ADD0_FLD_PRE_ADD_0						REG_FLD(9, 0)
#define PRE_ADD2_FLD_PRE_ADD_2						REG_FLD(9, 0)
#define POST_ADD0_FLD_POST_ADD_1						REG_FLD(9, 16)
#define POST_ADD0_FLD_POST_ADD_0						REG_FLD(9, 0)
#define POST_ADD2_FLD_POST_ADD_2						REG_FLD(9, 0)
#define DST_UV_PITCH_FLD_UV_DST_W_IN_BYTE			REG_FLD(16, 0)
#define DST_ADDR_OFFSET0_FLD_WDMA_DESTINATION_ADDRESS_OFFSET0	REG_FLD(28, 0)
#define DST_ADDR_OFFSET1_FLD_WDMA_DESTINATION_ADDRESS_OFFSET1	REG_FLD(28, 0)
#define DST_ADDR_OFFSET2_FLD_WDMA_DESTINATION_ADDRESS_OFFSET2	REG_FLD(28, 0)

#define FLOW_CTRL_DBG_FLD_WDMA_STA_FLOW_CTRL					REG_FLD(10, 0)
#define EXEC_DBG_FLD_WDMA_IN_REQ							REG_FLD(1, 15)
#define EXEC_DBG_FLD_WDMA_IN_ACK							REG_FLD(1, 14)

#define EXEC_DBG_FLD_WDMA_STA_EXEC				REG_FLD(32, 0)
#define CT_DBG_FLD_WDMA_STA_CT				REG_FLD(32, 0)
#define DEBUG_FLD_WDMA_STA_DEBUG				REG_FLD(32, 0)
#define DUMMY_FLD_WDMA_DUMMY				REG_FLD(32, 0)
#define DITHER_0_FLD_CRC_CLR					REG_FLD(1, 24)
#define DITHER_0_FLD_CRC_START				REG_FLD(1, 20)
#define DITHER_0_FLD_CRC_CEN					REG_FLD(1, 16)
#define DITHER_0_FLD_FRAME_DONE_DEL			REG_FLD(8, 8)
#define DITHER_0_FLD_OUT_SEL					REG_FLD(1, 4)
#define DITHER_0_FLD_START						REG_FLD(1, 0)
#define DITHER_5_FLD_W_DEMO					REG_FLD(16, 0)
#define DITHER_6_FLD_WRAP_MODE				REG_FLD(1, 16)
#define DITHER_6_FLD_LEFT_EN					REG_FLD(2, 14)
#define DITHER_6_FLD_FPHASE_R					REG_FLD(1, 13)
#define DITHER_6_FLD_FPHASE_EN				REG_FLD(1, 12)
#define DITHER_6_FLD_FPHASE					REG_FLD(6, 4)
#define DITHER_6_FLD_ROUND_EN				REG_FLD(1, 3)
#define DITHER_6_FLD_RDITHER_EN				REG_FLD(1, 2)
#define DITHER_6_FLD_LFSR_EN					REG_FLD(1, 1)
#define DITHER_6_FLD_EDITHER_EN				REG_FLD(1, 0)
#define DITHER_7_FLD_DRMOD_B					REG_FLD(2, 8)
#define DITHER_7_FLD_DRMOD_G					REG_FLD(2, 4)
#define DITHER_7_FLD_DRMOD_R					REG_FLD(2, 0)
#define DITHER_8_FLD_INK_DATA_R				REG_FLD(10, 16)
#define DITHER_8_FLD_INK						REG_FLD(1, 0)
#define DITHER_9_FLD_INK_DATA_B				 REG_FLD(10, 16)
#define DITHER_9_FLD_INK_DATA_G				REG_FLD(10, 0)
#define DITHER_10_FLD_FPHASE_BIT				REG_FLD(3, 8)
#define DITHER_10_FLD_FPHASE_SEL				REG_FLD(2, 4)
#define DITHER_10_FLD_FPHASE_CTRL				REG_FLD(2, 0)
#define DITHER_11_FLD_SUB_B					REG_FLD(2, 12)
#define DITHER_11_FLD_SUB_G					REG_FLD(2, 8)
#define DITHER_11_FLD_SUB_R					REG_FLD(2, 4)
#define DITHER_11_FLD_SUBPIX_EN				REG_FLD(1, 0)
#define DITHER_12_FLD_H_ACTIVE				REG_FLD(16, 16)
#define DITHER_12_FLD_TABLE_EN				REG_FLD(2, 4)
#define DITHER_12_FLD_LSB_OFF					REG_FLD(1, 0)
#define DITHER_13_FLD_RSHIFT_B					REG_FLD(3, 8)
#define DITHER_13_FLD_RSHIFT_G				REG_FLD(3, 4)
#define DITHER_13_FLD_RSHIFT_R					REG_FLD(3, 0)
#define DITHER_14_FLD_DEBUG_MODE				REG_FLD(2, 8)
#define DITHER_14_FLD_DIFF_SHIFT				REG_FLD(3, 4)
#define DITHER_14_FLD_TESTPIN_EN				REG_FLD(1, 0)
#define DITHER_15_FLD_LSB_ERR_SHIFT_R			REG_FLD(3, 28)
#define DITHER_15_FLD_OVFLW_BIT_R				REG_FLD(3, 24)
#define DITHER_15_FLD_ADD_lSHIFT_R				REG_FLD(3, 20)
#define DITHER_15_FLD_INPUT_RSHIFT_R			REG_FLD(3, 16)
#define DITHER_15_FLD_NEW_BIT_MODE				REG_FLD(1, 0)
#define DITHER_16_FLD_LSB_ERR_SHIFT_B			REG_FLD(3, 28)
#define DITHER_16_FLD_OVFLW_BIT_B				REG_FLD(3, 24)
#define DITHER_16_FLD_ADD_lSHIFT_B				REG_FLD(3, 20)
#define DITHER_16_FLD_INPUT_RSHIFT_B			REG_FLD(3, 16)
#define DITHER_16_FLD_lSB_ERR_SHIFT_G			REG_FLD(3, 12)
#define DITHER_16_FLD_OVFLW_BIT_G				REG_FLD(3, 8)
#define DITHER_16_FLD_ADD_lSHIFT_G				REG_FLD(3, 4)
#define DITHER_16_FLD_INPUT_RSHIFT_G			REG_FLD(3, 0)
#define DITHER_17_FLD_CRC_RDY					REG_FLD(1, 16)
#define DITHER_17_FLD_CRC_OUT					REG_FLD(16, 0)
#define DST_ADDR0_FLD_ADDRESS0				REG_FLD(32, 0)
#define DST_ADDR1_FLD_ADDRESS1				REG_FLD(32, 0)
#define DST_ADDR2_FLD_ADDRESS2				REG_FLD(32, 0)
#endif
