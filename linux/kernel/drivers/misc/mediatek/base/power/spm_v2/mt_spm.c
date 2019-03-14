#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include "mt_spm_idle.h"
#include <mach/irqs.h>
#include <mt-plat/upmu_common.h>
#include "mt_spm_vcore_dvfs.h"
#include "mt_vcorefs_governor.h"
#include "mt_spm_internal.h"
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/irqchip/mt-eic.h>
/* #include <mach/eint.h> */
/* #include <mach/mt_boot.h> */
#ifdef CONFIG_MTK_WD_KICKER
#include <mach/wd_api.h>
#endif

#define ENABLE_DYNA_LOAD_PCM
#ifdef ENABLE_DYNA_LOAD_PCM	/* for dyna_load_pcm */
/* for request_firmware */
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/dcache.h>
#include <asm/cacheflush.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include "mt_spm_misc.h"
#if defined(CONFIG_MTK_LEGACY)
#include <cust_gpio_usage.h>
#endif
#ifndef dmac_map_area
#define dmac_map_area __dma_map_area
#endif

static struct dentry *spm_dir;
static struct dentry *spm_file;
struct platform_device *pspmdev;
static int dyna_load_pcm_done;
static char *dyna_load_pcm_path[] = {
	[DYNA_LOAD_PCM_SUSPEND] = "pcm_suspend.bin",
	[DYNA_LOAD_PCM_SUSPEND_BY_MP1] = "pcm_suspend_by_mp1.bin",

#if defined(CONFIG_ARCH_MT6797)
	[DYNA_LOAD_PCM_SODI_LPM] = "pcm_sodi_ddrdfs_lpm.bin",
	[DYNA_LOAD_PCM_SODI_BY_MP1_LPM] = "pcm_sodi_ddrdfs_by_mp1_lpm.bin",
	[DYNA_LOAD_PCM_SODI_HPM] = "pcm_sodi_ddrdfs_hpm.bin",
	[DYNA_LOAD_PCM_SODI_BY_MP1_HPM] = "pcm_sodi_ddrdfs_by_mp1_hpm.bin",
	[DYNA_LOAD_PCM_SODI_ULTRA] = "pcm_sodi_ddrdfs_ultra.bin",
	[DYNA_LOAD_PCM_SODI_BY_MP1_ULTRA] = "pcm_sodi_ddrdfs_by_mp1_ultra.bin",
#else
	[DYNA_LOAD_PCM_SODI] = "pcm_sodi_ddrdfs.bin",
	[DYNA_LOAD_PCM_SODI_BY_MP1] = "pcm_sodi_ddrdfs_by_mp1.bin",
	[DYNA_LOAD_PCM_MCDI] = "pcm_mcdi_ddrdfs.bin",
#endif
	[DYNA_LOAD_PCM_DEEPIDLE] = "pcm_deepidle.bin",
	[DYNA_LOAD_PCM_DEEPIDLE_BY_MP1] = "pcm_deepidle_by_mp1.bin",
	[DYNA_LOAD_PCM_MAX] = "pcm_path_max",
};

MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SUSPEND]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SUSPEND_BY_MP1]);

#if defined(CONFIG_ARCH_MT6797)
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI_LPM]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI_BY_MP1_LPM]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI_HPM]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI_BY_MP1_HPM]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI_ULTRA]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI_BY_MP1_ULTRA]);
#else
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_SODI_BY_MP1]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_MCDI]);
#endif
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_DEEPIDLE]);
MODULE_FIRMWARE(dyna_load_pcm_path[DYNA_LOAD_PCM_DEEPIDLE_BY_MP1]);


struct dyna_load_pcm_t dyna_load_pcm[DYNA_LOAD_PCM_MAX];
#if defined(CONFIG_ARCH_MT6797)
static unsigned int sodi_fw_mode;
#endif

/* add char device for spm */
#include <linux/cdev.h>
#define SPM_DETECT_MAJOR 159	/* FIXME */
#define SPM_DETECT_DEV_NUM 1
#define SPM_DETECT_DRVIER_NAME "spm"
#define SPM_DETECT_DEVICE_NAME "spm"

struct class *pspmDetectClass = NULL;
struct device *pspmDetectDev = NULL;
static int gSPMDetectMajor = SPM_DETECT_MAJOR;
static struct cdev gSPMDetectCdev;

#endif				/* ENABLE_DYNA_LOAD_PCM */

void __iomem *spm_base;
void __iomem *spm_infracfg_ao_base;
void __iomem *spm_ddrphy_base;
void __iomem *spm_cksys_base;
void __iomem *spm_mcucfg;
#if defined(CONFIG_ARCH_MT6755)
void __iomem *spm_bsi1cfg;
#endif
u32 gpio_base_addr;
struct clk *i2c3_clk_main;
#if defined(CONFIG_ARCH_MT6755)
u32 spm_irq_0 = 197;
u32 spm_irq_1 = 198;
u32 spm_irq_2 = 199;
u32 spm_irq_3 = 200;
u32 spm_irq_4 = 201;
u32 spm_irq_5 = 202;
u32 spm_irq_6 = 203;
u32 spm_irq_7 = 204;
#else
u32 spm_irq_0 = 180;
#endif

#ifdef SPM_VCORE_EN_MT6755
u32 spm_vcorefs_start_irq = 152;
u32 spm_vcorefs_end_irq = 153;
#endif

/**************************************
 * Config and Parameter
 **************************************/
#define SPM_MD_DDR_EN_OUT	0


/**************************************
 * Define and Declare
 **************************************/
struct spm_irq_desc {
	unsigned int irq;
	irq_handler_t handler;
};

static twam_handler_t spm_twam_handler;

#ifdef SPM_VCORE_EN_MT6755
static vcorefs_handler_t vcorefs_handler;
static vcorefs_start_handler_t vcorefs_start_handler;
#endif

void __attribute__((weak)) spm_sodi3_init(void)
{

}

void __attribute__((weak)) spm_sodi_init(void)
{

}

void __attribute__((weak)) spm_mcdi_init(void)
{

}

void __attribute__((weak)) spm_deepidle_init(void)
{

}

void __attribute__((weak)) mt_power_gs_dump_suspend(void)
{

}

int __attribute__((weak)) spm_fs_init(void)
{
	return 0;
}

/**************************************
 * Init and IRQ Function
 **************************************/
static irqreturn_t spm_irq0_handler(int irq, void *dev_id)
{
	u32 isr;
	unsigned long flags;
	struct twam_sig twamsig;

	spin_lock_irqsave(&__spm_lock, flags);
	/* get ISR status */
	isr = spm_read(SPM_IRQ_STA);
	if (isr & ISRS_TWAM) {
		twamsig.sig0 = spm_read(SPM_TWAM_LAST_STA0);
		twamsig.sig1 = spm_read(SPM_TWAM_LAST_STA1);
		twamsig.sig2 = spm_read(SPM_TWAM_LAST_STA2);
		twamsig.sig3 = spm_read(SPM_TWAM_LAST_STA3);
	}

	/* clean ISR status */
	spm_write(SPM_IRQ_MASK, spm_read(SPM_IRQ_MASK) | ISRM_ALL_EXC_TWAM);
	spm_write(SPM_IRQ_STA, isr);
	if (isr & ISRS_TWAM)
		udelay(100);	/* need 3T TWAM clock (32K/26M) */
	spm_write(SPM_SW_INT_CLEAR, PCM_SW_INT0);
	spin_unlock_irqrestore(&__spm_lock, flags);

	if ((isr & ISRS_TWAM) && spm_twam_handler)
		spm_twam_handler(&twamsig);

	if (isr & (ISRS_SW_INT0 | ISRS_PCM_RETURN))
		spm_err("IRQ0 HANDLER SHOULD NOT BE EXECUTED (0x%x)\n", isr);

	return IRQ_HANDLED;
}

#if defined(CONFIG_ARCH_MT6755)
static irqreturn_t spm_irq_aux_handler(u32 irq_id)
{
	u32 isr;
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	isr = spm_read(SPM_IRQ_STA);
	spm_write(SPM_SW_INT_CLEAR, (1U << irq_id));
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_err("IRQ%u HANDLER SHOULD NOT BE EXECUTED (0x%x)\n", irq_id, isr);

	return IRQ_HANDLED;
}

static irqreturn_t spm_irq1_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(1);
}

static irqreturn_t spm_irq2_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(2);
}

static irqreturn_t spm_irq3_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(3);
}

static irqreturn_t spm_irq4_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(4);
}

static irqreturn_t spm_irq5_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(5);
}

static irqreturn_t spm_irq6_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(6);
}

static irqreturn_t spm_irq7_handler(int irq, void *dev_id)
{
	return spm_irq_aux_handler(7);
}
#endif

static int spm_irq_register(void)
{
	int i, err, r = 0;
#if defined(CONFIG_ARCH_MT6755)
	struct spm_irq_desc irqdesc[] = {
		{.irq = 0, .handler = spm_irq0_handler,},
		{.irq = 0, .handler = spm_irq1_handler,},
		{.irq = 0, .handler = spm_irq2_handler,},
		{.irq = 0, .handler = spm_irq3_handler,},
		{.irq = 0, .handler = spm_irq4_handler,},
		{.irq = 0, .handler = spm_irq5_handler,},
		{.irq = 0, .handler = spm_irq6_handler,},
		{.irq = 0, .handler = spm_irq7_handler,}
	};
#elif defined(CONFIG_ARCH_MT6797)
	struct spm_irq_desc irqdesc[] = {
		{.irq = 0, .handler = spm_irq0_handler,}
	};
#endif
	irqdesc[0].irq = SPM_IRQ0_ID;
#if defined(CONFIG_ARCH_MT6755)
	irqdesc[1].irq = SPM_IRQ1_ID;
	irqdesc[2].irq = SPM_IRQ2_ID;
	irqdesc[3].irq = SPM_IRQ3_ID;
	irqdesc[4].irq = SPM_IRQ4_ID;
	irqdesc[5].irq = SPM_IRQ5_ID;
	irqdesc[6].irq = SPM_IRQ6_ID;
	irqdesc[7].irq = SPM_IRQ7_ID;
#endif
	for (i = 0; i < ARRAY_SIZE(irqdesc); i++) {
		err = request_irq(irqdesc[i].irq, irqdesc[i].handler,
				  IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND | IRQF_PERCPU, "SPM", NULL);
		if (err) {
			spm_err("FAILED TO REQUEST IRQ%d (%d)\n", i, err);
			r = -EPERM;
		}
	}
	return r;
}

#ifdef SPM_VCORE_EN_MT6755
void spm_vcorefs_register_handler(vcorefs_handler_t handler, vcorefs_start_handler_t start_handler)
{
	vcorefs_handler = handler;
	vcorefs_start_handler = start_handler;
}
EXPORT_SYMBOL(spm_vcorefs_register_handler);

static irqreturn_t spm_vcorefs_start_handler(int irq, void *dev_id)
{
	if (vcorefs_start_handler)
		vcorefs_start_handler();

	mt_eint_virq_soft_clr(irq);
	return IRQ_HANDLED;
}

static irqreturn_t spm_vcorefs_end_handler(int irq, void *dev_id)
{
	u32 opp = 0;

	if (vcorefs_handler) {
		opp = spm_read(SPM_SW_RSV_5) & SPM_SW_RSV_5_LSB;
		vcorefs_handler(opp);
	}

	mt_eint_virq_soft_clr(irq);
	return IRQ_HANDLED;
}
#endif

static void spm_register_init(void)
{
	unsigned long flags;

	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "mediatek,sleep");
	if (!node)
		spm_err("find SLEEP node failed\n");
	spm_base = of_iomap(node, 0);
	if (!spm_base)
		spm_err("base spm_base failed\n");

	spm_irq_0 = irq_of_parse_and_map(node, 0);
	if (!spm_irq_0)
		spm_err("get spm_irq_0 failed\n");
#if defined(CONFIG_ARCH_MT6755)
	spm_irq_1 = irq_of_parse_and_map(node, 1);
	if (!spm_irq_1)
		spm_err("get spm_irq_1 failed\n");
	spm_irq_2 = irq_of_parse_and_map(node, 2);
	if (!spm_irq_2)
		spm_err("get spm_irq_2 failed\n");
	spm_irq_3 = irq_of_parse_and_map(node, 3);
	if (!spm_irq_3)
		spm_err("get spm_irq_3 failed\n");
	spm_irq_4 = irq_of_parse_and_map(node, 4);
	if (!spm_irq_4)
		spm_err("get spm_irq_4 failed\n");
	spm_irq_5 = irq_of_parse_and_map(node, 5);
	if (!spm_irq_5)
		spm_err("get spm_irq_5 failed\n");
	spm_irq_6 = irq_of_parse_and_map(node, 6);
	if (!spm_irq_6)
		spm_err("get spm_irq_6 failed\n");
	spm_irq_7 = irq_of_parse_and_map(node, 7);
	if (!spm_irq_7)
		spm_err("get spm_irq_7 failed\n");
#endif
	/* cksys_base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,topckgen");
	if (!node)
		spm_err("[CLK_CKSYS] find node failed\n");
	spm_cksys_base = of_iomap(node, 0);
	if (!spm_cksys_base)
		spm_err("[CLK_CKSYS] base failed\n");

	node = of_find_compatible_node(NULL, NULL, "mediatek,infracfg_ao");
	if (!node)
		spm_err("[CLK_INFRACFG_AO] find node failed\n");
	spm_infracfg_ao_base = of_iomap(node, 0);
	if (!spm_infracfg_ao_base)
		spm_err("[CLK_INFRACFG_AO] base failed\n");

	/* mcucfg */
	node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg");
	if (!node)
		spm_err("[MCUCFG] find node failed\n");
	spm_mcucfg = of_iomap(node, 0);
	if (!spm_mcucfg)
		spm_err("[MCUCFG] base failed\n");
#if defined(CONFIG_ARCH_MT6755)
	/* bsi1cfg */
	node = of_find_compatible_node(NULL, NULL, "mediatek,bpi_bsi_slv1");
	if (!node)
		spm_err("[bsi1] find node failed\n");
	spm_bsi1cfg = of_iomap(node, 0);
	if (!spm_bsi1cfg)
		spm_err("[bsi1] base failed\n");
#endif
	node = of_find_compatible_node(NULL, NULL, "mediatek,ddrphy");
	if (!node)
		spm_err("find DDRPHY node failed\n");
	spm_ddrphy_base = of_iomap(node, 0);
	if (!spm_ddrphy_base)
		spm_err("[DDRPHY] base failed\n");

#ifdef SPM_VCORE_EN_MT6755
	node = of_find_compatible_node(NULL, NULL, "mediatek,spm_vcorefs_start_eint");
	if (!node) {
		spm_err("find spm_vcorefs_start_eint failed\n");
	} else {
		int ret;
		u32 ints[2];

		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		mt_gpio_set_debounce(ints[0], ints[1]);
		spm_vcorefs_start_irq = irq_of_parse_and_map(node, 0);
		ret =
		    request_irq(spm_vcorefs_start_irq, spm_vcorefs_start_handler,
				IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND, "spm_vcorefs_start_eint",
				NULL);
	}

	node = of_find_compatible_node(NULL, NULL, "mediatek,spm_vcorefs_end_eint");
	if (!node) {
		spm_err("find spm_vcorefs_end_eint failed\n");
	} else {
		int ret;
		u32 ints[2];

		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		mt_gpio_set_debounce(ints[0], ints[1]);
		spm_vcorefs_end_irq = irq_of_parse_and_map(node, 0);
		ret =
		    request_irq(spm_vcorefs_end_irq, spm_vcorefs_end_handler,
				IRQF_TRIGGER_HIGH | IRQF_NO_SUSPEND, "spm_vcorefs_end_eint", NULL);
	}
	spm_err("spm_vcorefs_start_irq = %d, spm_vcorefs_end_irq = %d\n", spm_vcorefs_start_irq,
		spm_vcorefs_end_irq);
#endif
	spm_err("spm_base = %p, spm_irq_0 = %d\n", spm_base, spm_irq_0);
#if defined(CONFIG_ARCH_MT6755)
	spm_err("spm_irq_1 = %d, spm_irq_2 = %d, spm_irq_3 = %d\n",
		spm_irq_1, spm_irq_2, spm_irq_3);
	spm_err("spm_irq_4 = %d, spm_irq_5 = %d, spm_irq_6 = %d, spm_irq_7 = %d\n", spm_irq_4,
		spm_irq_5, spm_irq_6, spm_irq_7);
#endif
	spm_err("cksys_base = %p, infracfg_ao_base = %p, spm_mcucfg = %p\n", spm_cksys_base,
		spm_infracfg_ao_base, spm_mcucfg);

	/* GPIO */
	node = of_find_compatible_node(NULL, NULL, "mediatek,gpio");
	if (!node)
		spm_err("find mediatek,GPIO failed\n");
	if (of_property_read_u32_array(node, "reg", &gpio_base_addr, 1))
		spm_err("mediatek,GPIO base addr can NOT found!\n");

	spin_lock_irqsave(&__spm_lock, flags);

	/* md resource request selection */

#ifdef CONFIG_MTK_C2K_SUPPORT
	spm_write(SPM_INFRA_MISC, (spm_read(SPM_INFRA_MISC) &
		~(0xff << MD_SRC_REQ_BIT)) | (0x6d << MD_SRC_REQ_BIT));
#else
	spm_write(SPM_INFRA_MISC, (spm_read(SPM_INFRA_MISC) &
		~(0xff << MD_SRC_REQ_BIT)) | (0x29 << MD_SRC_REQ_BIT));
#endif

	/* enable register control */
	spm_write(POWERON_CONFIG_EN, SPM_REGWR_CFG_KEY | BCLK_CG_EN_LSB);

	/* init power control register */
	/* dram will set this register */
	/* spm_write(SPM_POWER_ON_VAL0, 0); */
	spm_write(SPM_POWER_ON_VAL1, POWER_ON_VAL1_DEF);
	spm_write(PCM_PWR_IO_EN, 0);

	/* reset PCM */
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | PCM_SW_RESET_LSB);
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB);
	BUG_ON((spm_read(PCM_FSM_STA) & 0x7fffff) != PCM_FSM_STA_DEF);	/* PCM reset failed */

	/* init PCM control register */
	spm_write(PCM_CON0, SPM_REGWR_CFG_KEY | PCM_CK_EN_LSB | EN_IM_SLEEP_DVS_LSB);
	spm_write(PCM_CON1, SPM_REGWR_CFG_KEY | EVENT_LOCK_EN_LSB |
		  SPM_SRAM_ISOINT_B_LSB | SPM_SRAM_SLEEP_B_LSB | MIF_APBEN_LSB);
	spm_write(PCM_IM_PTR, 0);
	spm_write(PCM_IM_LEN, 0);

	/*
	 * SRCLKENA0: POWER_ON_VAL1 (PWR_IO_EN[7]=0) or
	 *            E1: r7|SRCLKENAI0|SRCLKENAI1|MD1_SRCLKENA (PWR_IO_EN[7]=1)
	 *            E2: r7|SRCLKENAI0 (PWR_IO_EN[7]=1)
	 * CLKSQ0_OFF: POWER_ON_VAL0 (PWR_IO_EN[0]=0) or r0 (PWR_IO_EN[0]=1)
	 * SRCLKENA1: MD2_SRCLKENA
	 * CLKSQ1_OFF: !MD2_SRCLKENA
	 */
	/* bit 2, 3, 6, 12, 27 = 1 */
	spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) | CC_SYSCLK1_EN_0 |
		  CC_SYSCLK1_EN_1 | CC_SRCLKENA_MASK_0 |
		  CLKSQ1_SEL_CTRL_LSB | SYSCLK0_SRC_MASK_B_LSB |
		  CC_SYSCLK1_SRC_MASK_B_MD2_SRCCLKENA);

	/* clean wakeup event raw status */
	spm_write(SPM_WAKEUP_EVENT_MASK, SPM_WAKEUP_EVENT_MASK_DEF);

	/* clean ISR status */
	spm_write(SPM_IRQ_MASK, ISRM_ALL);
	spm_write(SPM_IRQ_STA, ISRC_ALL);
	spm_write(SPM_SW_INT_CLEAR, PCM_SW_INT_ALL);

	/* output md_ddr_en if needed for debug */
#if SPM_MD_DDR_EN_OUT
	__spm_dbgout_md_ddr_en(true);
#endif

#if defined(CONFIG_ARCH_MT6755)
	/* init r7 with POWER_ON_VAL1 */
	spm_write(PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL1));
	spm_write(PCM_PWR_IO_EN, PCM_RF_SYNC_R7);
	spm_write(PCM_PWR_IO_EN, 0);
#endif

	spin_unlock_irqrestore(&__spm_lock, flags);
}

int spm_module_init(void)
{
	int r = 0;
#if defined(CONFIG_ARCH_MT6755)
	u32 reg_val;
#endif
#if 0
#ifdef CONFIG_MTK_WD_KICKER
	struct wd_api *wd_api;
#endif
#endif
	spm_register_init();
	if (spm_irq_register() != 0)
		r = -EPERM;
#if defined(CONFIG_PM)
	if (spm_fs_init() != 0)
		r = -EPERM;
#endif

#if 0
#ifdef CONFIG_MTK_WD_KICKER
	get_wd_api(&wd_api);
	if (wd_api->wd_spmwdt_mode_config) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	} else {
		spm_err("FAILED TO GET WD API\n");
		r = -ENODEV;
	}
#endif
#endif
	spm_sodi3_init();
	spm_sodi_init();
	spm_mcdi_init();
	spm_deepidle_init();
#if 1				/* FIXME: wait for DRAMC golden setting enable */
	if (spm_golden_setting_cmp(1) != 0) {
		/* r = -EPERM; */
		/* aee_kernel_warning("SPM Warring", "dram golden setting mismach"); */
	}
#endif

	spm_set_dummy_read_addr();

#if defined(CONFIG_ARCH_MT6755)
	/* debug code */
	r = pmic_read_interface_nolock(MT6351_WDTDBG_CON1, &reg_val, 0xffff, 0);
	spm_crit("[PMIC]wdtdbg_con1 : 0x%x\n", reg_val);
	r = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON0, &reg_val, 0xffff, 0);
	spm_crit("[PMIC]vcore vosel_ctrl=0x%x\n", reg_val);
	r = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON4, &reg_val, 0xffff, 0);
	spm_crit("[PMIC]vcore vosel=0x%x\n", reg_val);
	r = pmic_read_interface_nolock(MT6351_BUCK_VCORE_CON5, &reg_val, 0xffff, 0);
	spm_crit("[PMIC]vcore vosel_on=0x%x\n", reg_val);
	r = pmic_read_interface_nolock(MT6351_WDTDBG_CON1, &reg_val, 0xffff, 0);
	spm_crit("[PMIC]wdtdbg_con1-after : 0x%x\n", reg_val);
#endif
/* set Vcore DVFS bootup opp by ddr shuffle opp */
#if defined(CONFIG_ARCH_MT6755)
	if (spm_read(SPM_POWER_ON_VAL0) & (1 << 14)) {
		spm_write(SPM_SW_RSV_5, spm_read(SPM_SW_RSV_5) & ~1);
		spm_crit2("SPM_SW_RSV5(0x%x) 1pll init(val0=0x%x)\n", spm_read(SPM_SW_RSV_5),
			spm_read(SPM_POWER_ON_VAL0));
	} else {
		spm_write(SPM_SW_RSV_5, spm_read(SPM_SW_RSV_5) | 1);
		spm_crit2("SPM_SW_RSV5(0x%x) 3pll init(val0=0x%x)\n", spm_read(SPM_SW_RSV_5),
			spm_read(SPM_POWER_ON_VAL0));
	}
#elif defined(CONFIG_ARCH_MT6797)
	if (spm_read(spm_ddrphy_base + SPM_SHUFFLE_ADDR) == 0)
		spm_write(SPM_SW_RSV_5, (spm_read(SPM_SW_RSV_5) & ~(0x3)) | (SPM_SCREEN_ON_HPM >> 23));
	else if (spm_read(spm_ddrphy_base + SPM_SHUFFLE_ADDR) == 1)
		spm_write(SPM_SW_RSV_5, (spm_read(SPM_SW_RSV_5) & ~(0x3)) | (SPM_SCREEN_ON_LPM >> 23));
	else
		spm_write(SPM_SW_RSV_5, (spm_read(SPM_SW_RSV_5) & ~(0x3)) | (SPM_SCREEN_OFF_LPM >> 23));

	spm_crit2("[VcoreFS] SPM_SW_RSV_5: 0x%x, dramc shuf addr: %p, val: 0x%x\n",
							spm_read(SPM_SW_RSV_5),
							spm_ddrphy_base + SPM_SHUFFLE_ADDR,
							spm_read(spm_ddrphy_base + SPM_SHUFFLE_ADDR));
#endif

	return r;
}

/* arch_initcall(spm_module_init); */

#ifdef ENABLE_DYNA_LOAD_PCM	/* for dyna_load_pcm */
static char *local_buf;
static dma_addr_t local_buf_dma;

int spm_load_pcm_firmware(struct platform_device *pdev)
{
	const struct firmware *fw;
	int err = 0;
	int i;
	int offset = 0;
	int addr_2nd = 0;

	if (!pdev)
		return err;

	if (dyna_load_pcm_done)
		return err;

	if (NULL == local_buf) {
		local_buf = dma_alloc_coherent(&pdev->dev, PCM_FIRMWARE_SIZE * DYNA_LOAD_PCM_MAX,
					       &local_buf_dma, GFP_KERNEL);
		if (!local_buf) {
			pr_debug("Failed to dma_alloc_coherent(), %d.\n", err);
			return -ENOMEM;
		}
	}

	for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++) {
		u16 firmware_size = 0;
		int copy_size = 0;
		struct pcm_desc *pdesc = &(dyna_load_pcm[i].desc);

		err = request_firmware(&fw, dyna_load_pcm_path[i], &pdev->dev);
		if (err) {
			pr_debug("Failed to load %s, %d.\n", dyna_load_pcm_path[i], err);
			continue;
			/* return -EINVAL; */
		}

		/* Do whatever it takes to load firmware into device. */
		/* start of binary size */
		offset = 0;
		copy_size = 2;
		memcpy(&firmware_size, fw->data, copy_size);

		/* start of binary */
		offset += copy_size;
		copy_size = firmware_size * 4;
		dyna_load_pcm[i].buf = local_buf + i * PCM_FIRMWARE_SIZE;
		dyna_load_pcm[i].buf_dma = local_buf_dma + i * PCM_FIRMWARE_SIZE;
		memcpy(dyna_load_pcm[i].buf, fw->data + offset, copy_size);
		/* dmac_map_area((void *)dyna_load_pcm[i].buf, PCM_FIRMWARE_SIZE, DMA_TO_DEVICE); */

		/* start of pcm_desc without pointer */
		offset += copy_size;
		copy_size = sizeof(struct pcm_desc) - offsetof(struct pcm_desc, size);
		memcpy((void *)&(dyna_load_pcm[i].desc.size), fw->data + offset, copy_size);
		/* get minimum addr_2nd */
		if (pdesc->addr_2nd) {
			if (addr_2nd)
				addr_2nd = min_t(int, (int)pdesc->addr_2nd, (int)addr_2nd);
			else
				addr_2nd = pdesc->addr_2nd;
		}

		/* start of pcm_desc version */
		offset += copy_size;
		copy_size = fw->size - offset;
		snprintf(dyna_load_pcm[i].version, PCM_FIRMWARE_VERSION_SIZE - 1,
				"%s", fw->data + offset);
		pdesc->version = dyna_load_pcm[i].version;
		pdesc->base = (u32 *) dyna_load_pcm[i].buf;
		pdesc->base_dma = dyna_load_pcm[i].buf_dma;

		release_firmware(fw);

		dyna_load_pcm[i].ready = 1;
		dyna_load_pcm_done = 1;
	}

	/* check addr_2nd */
	if (dyna_load_pcm_done) {
		for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++) {
			struct pcm_desc *pdesc = &(dyna_load_pcm[i].desc);

			if (!pdesc->version)
				continue;

			if (pdesc->addr_2nd == 0) {
				if (addr_2nd == (pdesc->size - 3))
					*(u16 *) &pdesc->size = addr_2nd;
#if defined(CONFIG_ARCH_MT6755)
				else
					BUG();
#endif
			}
		}
	}

#if defined(SPM_VCORE_EN_MT6797)
	if (dyna_load_pcm_done)
		vcorefs_late_init_dvfs();
#endif

	return err;
}

int spm_load_pcm_firmware_nodev(void)
{
	spm_load_pcm_firmware(pspmdev);
	return 0;
}

int spm_load_firmware_status(void)
{
	return dyna_load_pcm_done;
}

static int spm_dbg_show_firmware(struct seq_file *s, void *unused)
{
	int i;
	struct pcm_desc *pdesc = NULL;

	for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++) {
		pdesc = &(dyna_load_pcm[i].desc);
		seq_printf(s, "#@# %s\n", dyna_load_pcm_path[i]);

		if (pdesc->version) {
			seq_printf(s, "#@#  version = %s\n", pdesc->version);
			seq_printf(s, "#@#  base = 0x%p\n", pdesc->base);
			seq_printf(s, "#@#  size = %u\n", pdesc->size);
			seq_printf(s, "#@#  sess = %u\n", pdesc->sess);
			seq_printf(s, "#@#  replace = %u\n", pdesc->replace);
			seq_printf(s, "#@#  addr_2nd = %u\n", pdesc->addr_2nd);
			seq_printf(s, "#@#  vec0 = 0x%x\n", pdesc->vec0);
			seq_printf(s, "#@#  vec1 = 0x%x\n", pdesc->vec1);
			seq_printf(s, "#@#  vec2 = 0x%x\n", pdesc->vec2);
			seq_printf(s, "#@#  vec3 = 0x%x\n", pdesc->vec3);
			seq_printf(s, "#@#  vec4 = 0x%x\n", pdesc->vec4);
			seq_printf(s, "#@#  vec5 = 0x%x\n", pdesc->vec5);
			seq_printf(s, "#@#  vec6 = 0x%x\n", pdesc->vec6);
			seq_printf(s, "#@#  vec7 = 0x%x\n", pdesc->vec7);
			seq_printf(s, "#@#  vec8 = 0x%x\n", pdesc->vec8);
			seq_printf(s, "#@#  vec9 = 0x%x\n", pdesc->vec9);
			seq_printf(s, "#@#  vec10 = 0x%x\n", pdesc->vec10);
			seq_printf(s, "#@#  vec11 = 0x%x\n", pdesc->vec11);
			seq_printf(s, "#@#  vec12 = 0x%x\n", pdesc->vec12);
			seq_printf(s, "#@#  vec13 = 0x%x\n", pdesc->vec13);
			seq_printf(s, "#@#  vec14 = 0x%x\n", pdesc->vec14);
			seq_printf(s, "#@#  vec15 = 0x%x\n", pdesc->vec15);
		}
	}
	seq_puts(s, "\n\n");

	return 0;
}

static int spm_dbg_open(struct inode *inode, struct file *file)
{
	return single_open(file, spm_dbg_show_firmware, &inode->i_private);
}

static const struct file_operations spm_debug_fops = {
	.open = spm_dbg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int SPM_detect_open(struct inode *inode, struct file *file)
{
	pr_debug("open major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);
	spm_load_pcm_firmware_nodev();

	return 0;
}

static int SPM_detect_close(struct inode *inode, struct file *file)
{
	pr_debug("close major %d minor %d (pid %d)\n", imajor(inode), iminor(inode), current->pid);

	return 0;
}

static ssize_t SPM_detect_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	pr_debug(" ++\n");
	pr_debug(" --\n");

	return 0;
}

ssize_t SPM_detect_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	pr_debug(" ++\n");
	pr_debug(" --\n");

	return 0;
}

const struct file_operations gSPMDetectFops = {
	.open = SPM_detect_open,
	.release = SPM_detect_close,
	.read = SPM_detect_read,
	.write = SPM_detect_write,
};

int spm_module_late_init(void)
{
	int i = 0;
	dev_t devID = MKDEV(gSPMDetectMajor, 0);
	int cdevErr = -1;
	int ret = -1;

	pspmdev = platform_device_register_simple("spm", 0, NULL, 0);
	if (IS_ERR(pspmdev)) {
		pr_debug("Failed to register platform device.\n");
		return -EINVAL;
	}

	ret = register_chrdev_region(devID, SPM_DETECT_DEV_NUM, SPM_DETECT_DRVIER_NAME);
	if (ret) {
		pr_debug("fail to register chrdev\n");
		return ret;
	}

	cdev_init(&gSPMDetectCdev, &gSPMDetectFops);
	gSPMDetectCdev.owner = THIS_MODULE;

	cdevErr = cdev_add(&gSPMDetectCdev, devID, SPM_DETECT_DEV_NUM);
	if (cdevErr) {
		pr_debug("cdev_add() fails (%d)\n", cdevErr);
		goto err1;
	}

	pspmDetectClass = class_create(THIS_MODULE, SPM_DETECT_DEVICE_NAME);
	if (IS_ERR(pspmDetectClass)) {
		pr_debug("class create fail, error code(%ld)\n", PTR_ERR(pspmDetectClass));
		goto err1;
	}

	pspmDetectDev = device_create(pspmDetectClass, NULL, devID, NULL, SPM_DETECT_DEVICE_NAME);
	if (IS_ERR(pspmDetectDev)) {
		pr_debug("device create fail, error code(%ld)\n", PTR_ERR(pspmDetectDev));
		goto err2;
	}

	pr_debug("driver(major %d) installed success\n", gSPMDetectMajor);

	spm_dir = debugfs_create_dir("spm", NULL);
	if (spm_dir == NULL) {
		pr_debug("Failed to create spm dir in debugfs.\n");
		return -EINVAL;
	}

	spm_file = debugfs_create_file("firmware", S_IRUGO, spm_dir, NULL, &spm_debug_fops);

	for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++)
		dyna_load_pcm[i].ready = 0;

	return 0;

err2:

	if (pspmDetectClass) {
		class_destroy(pspmDetectClass);
		pspmDetectClass = NULL;
	}

err1:

	if (cdevErr == 0)
		cdev_del(&gSPMDetectCdev);

	if (ret == 0) {
		unregister_chrdev_region(devID, SPM_DETECT_DEV_NUM);
		gSPMDetectMajor = -1;
	}

	pr_debug("fail\n");

	return -1;

}
late_initcall(spm_module_late_init);
#endif				/* ENABLE_DYNA_LOAD_PCM */

/**************************************
 * PLL Request API
 **************************************/
void spm_mainpll_on_request(const char *drv_name)
{
	int req;

	req = atomic_inc_return(&__spm_mainpll_req);
	spm_debug("%s request MAINPLL on (%d)\n", drv_name, req);
}
EXPORT_SYMBOL(spm_mainpll_on_request);

void spm_mainpll_on_unrequest(const char *drv_name)
{
	int req;

	req = atomic_dec_return(&__spm_mainpll_req);
	spm_debug("%s unrequest MAINPLL on (%d)\n", drv_name, req);
}
EXPORT_SYMBOL(spm_mainpll_on_unrequest);


/**************************************
 * TWAM Control API
 **************************************/
static unsigned int idle_sel;
void spm_twam_set_idle_select(unsigned int sel)
{
	idle_sel = sel & 0x3;
}
EXPORT_SYMBOL(spm_twam_set_idle_select)

static unsigned int window_len;
void spm_twam_set_window_length(unsigned int len)
{
	window_len = len;
}
EXPORT_SYMBOL(spm_twam_set_window_length)

static struct twam_sig mon_type;
void spm_twam_set_mon_type(struct twam_sig *mon)
{
	if (mon) {
		mon_type.sig0 = mon->sig0 & 0x3;
		mon_type.sig1 = mon->sig1 & 0x3;
		mon_type.sig2 = mon->sig2 & 0x3;
		mon_type.sig3 = mon->sig3 & 0x3;
	}
}
EXPORT_SYMBOL(spm_twam_set_mon_type)

void spm_twam_register_handler(twam_handler_t handler)
{
	spm_twam_handler = handler;
}
EXPORT_SYMBOL(spm_twam_register_handler);

void spm_twam_enable_monitor(const struct twam_sig *twamsig, bool speed_mode)
{
	u32 sig0 = 0, sig1 = 0, sig2 = 0, sig3 = 0;
	u32 mon0 = 0, mon1 = 0, mon2 = 0, mon3 = 0;
	unsigned int sel;
	unsigned int length;
	unsigned long flags;

	if (twamsig) {
		sig0 = twamsig->sig0 & 0x1f;
		sig1 = twamsig->sig1 & 0x1f;
		sig2 = twamsig->sig2 & 0x1f;
		sig3 = twamsig->sig3 & 0x1f;
	}

	/* Idle selection */
	sel = idle_sel;
	/* Window length */
	length = window_len;
	/* Monitor type */
	mon0 = mon_type.sig0 & 0x3;
	mon1 = mon_type.sig1 & 0x3;
	mon2 = mon_type.sig2 & 0x3;
	mon3 = mon_type.sig3 & 0x3;

	spin_lock_irqsave(&__spm_lock, flags);
	spm_write(SPM_IRQ_MASK, spm_read(SPM_IRQ_MASK) & ~ISRM_TWAM);
	/* Signal Select */
	spm_write(SPM_TWAM_IDLE_SEL, sel);
	/* Monitor Control */
	spm_write(SPM_TWAM_CON,
		  (sig3 << 27) |
		  (sig2 << 22) |
		  (sig1 << 17) |
		  (sig0 << 12) |
		  (mon3 << 10) |
		  (mon2 << 8) |
		  (mon1 << 6) |
		  (mon0 << 4) | (speed_mode ? TWAM_SPEED_MODE_ENABLE_LSB : 0) | TWAM_ENABLE_LSB);
	/* Window Length */
	/* 0x13DDF0 for 50ms, 0x65B8 for 1ms, 0x1458 for 200us, 0xA2C for 100us */
	/* in speed mode (26 MHz) */
	spm_write(SPM_TWAM_WINDOW_LEN, length);
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_debug("enable TWAM for signal %u, %u, %u, %u (%u)\n",
		  sig0, sig1, sig2, sig3, speed_mode);
}
EXPORT_SYMBOL(spm_twam_enable_monitor);

void spm_twam_disable_monitor(void)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	spm_write(SPM_TWAM_CON, spm_read(SPM_TWAM_CON) & ~TWAM_ENABLE_LSB);
	spm_write(SPM_IRQ_MASK, spm_read(SPM_IRQ_MASK) | ISRM_TWAM);
	spm_write(SPM_IRQ_STA, ISRC_TWAM);
	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_debug("disable TWAM\n");
}
EXPORT_SYMBOL(spm_twam_disable_monitor);

/**************************************
 * SPM Golden Seting API(MEMPLL Control, DRAMC)
 **************************************/
struct ddrphy_golden_cfg {
	u32 addr;
	u32 value;
	u32 value1;
};

static struct ddrphy_golden_cfg ddrphy_setting[] = {
	{0x5c0, 0x21271b1b, 0xffffffff},
	{0x5c4, 0x5096001e, 0xffffffff},
	{0x5c8, 0x9010f010, 0xffffffff},
	{0x5cc, 0x50101010, 0xffffffff},
	{0x640, 0x000220b1, 0x00022091},
	{0x650, 0x00000018, 0xffffffff},
	{0x698, 0x00011e00, 0x00018030},
};

int spm_golden_setting_cmp(bool en)
{

	int i, ddrphy_num, r = 0;

	if (!en)
		return r;

	/*Compare Dramc Goldeing Setting */
	ddrphy_num = sizeof(ddrphy_setting) / sizeof(ddrphy_setting[0]);
	for (i = 0; i < ddrphy_num; i++) {
		if ((spm_read(spm_ddrphy_base + ddrphy_setting[i].addr) != ddrphy_setting[i].value)
		    && ((ddrphy_setting[i].value1 == 0xffffffff)
			|| (spm_read(spm_ddrphy_base + ddrphy_setting[i].addr) !=
			    ddrphy_setting[i].value1))) {
			spm_err("dramc setting mismatch addr: %p, val: 0x%x\n",
				spm_ddrphy_base + ddrphy_setting[i].addr,
				spm_read(spm_ddrphy_base + ddrphy_setting[i].addr));
			r = -EPERM;
		}
	}

	return r;

}

/* for PMIC power settings */
#define VCORE_VOSEL_SLEEP_0P7	0x10	/* 7'b0010000 */
#define VCORE_VOSEL_SLEEP_0P9	0x30	/* 7'b0110000 */
static void spm_pmic_set_vcore(int vcore, int lock)
{
	if (lock == 0) {
		pmic_config_interface_nolock(MT6351_BUCK_VCORE_CON6,
					     vcore,
					     MT6351_PMIC_BUCK_VCORE_VOSEL_SLEEP_MASK,
					     MT6351_PMIC_BUCK_VCORE_VOSEL_SLEEP_SHIFT);
	} else {
		pmic_config_interface(MT6351_BUCK_VCORE_CON6,
				      vcore,
				      MT6351_PMIC_BUCK_VCORE_VOSEL_SLEEP_MASK,
				      MT6351_PMIC_BUCK_VCORE_VOSEL_SLEEP_SHIFT);
	}
}

static void spm_pmic_set_buck(u32 addr, int en_ctrl, int en, int vsleep_en, int vosel_sel, int lock)
{
	if (lock == 0) {
		if (en_ctrl != -1)
			pmic_config_interface_nolock(addr,	/* EN_CTRL_ADDR:        CON0 */
						     en_ctrl, 0x1,	/* EN_CTRL_MASK */
						     0);	/* EN_CTRL_SHIFT */
		if (en != -1)
			pmic_config_interface_nolock(addr + 0x4,	/* EN_ADDR:             CON2 */
						     en, 0x1,	/* EN_MASK */
						     0);	/* EN_SHIFT */
		if (vsleep_en != -1)
			pmic_config_interface_nolock(addr + 0x12,	/* VSLEEP_EN_ADDR:      CON9 */
						     vsleep_en, 0x1,	/* VSLEEP_EN_MASK */
						     8);	/* VSLEEP_EN_SHIFT */
		if (vosel_sel != -1)
			pmic_config_interface_nolock(addr + 0x2,	/* VOSEL_SEL_ADDR       CON1 */
						     vosel_sel, 0x7,	/* VOSEL_SEL_MASK */
						     3);	/* VOSEL_SEL_SHIFT */
	} else {
		if (en_ctrl != -1)
			pmic_config_interface(addr,	/* EN_CTRL_ADDR:        CON0 */
					      en_ctrl, 0x1,	/* EN_CTRL_MASK */
					      0);	/* EN_CTRL_SHIFT */
		if (en != -1)
			pmic_config_interface(addr + 0x4,	/* EN_ADDR:             CON2 */
					      en, 0x1,	/* EN_MASK */
					      0);	/* EN_SHIFT */
		if (vsleep_en != -1)
			pmic_config_interface(addr + 0x12,	/* VSLEEP_EN_ADDR:      CON9 */
					      vsleep_en, 0x1,	/* VSLEEP_EN_MASK */
					      8);	/* VSLEEP_EN_SHIFT */
		if (vosel_sel != -1)
			pmic_config_interface(addr + 0x2,	/* VOSEL_SEL_ADDR       CON1 */
					      vosel_sel, 0x7,	/* VOSEL_SEL_MASK */
					      3);	/* VOSEL_SEL_SHIFT */
	}
}

static void spm_pmic_set_ldo(u32 addr, int on_ctrl, int en, int mode_ctrl,
			     int secclk_mode_sel, int lock)
{
	if (lock == 0) {
		if (on_ctrl != -1)
			pmic_config_interface_nolock(addr, on_ctrl, 0x1,	/* ON_CTRL_MSAK */
						     3);	/* ON_CTRL_SHIFT */
		if (en != -1)
			pmic_config_interface_nolock(addr, en, 0x1,	/* EN_MASK */
						     1);	/* EN_SHIFT */
		if (mode_ctrl != -1)
			pmic_config_interface_nolock(addr, mode_ctrl, 0x1,	/* MODE_CTRL_MASK */
						     2);	/* MODE_CTRL_SHIFT */
		if (secclk_mode_sel != -1)
			pmic_config_interface_nolock(addr, secclk_mode_sel, 0x7,	/* SRCLK_MODE_SEL_MASK */
						     5);	/* SRCLK_MODE_SEL_SHIFT */
	} else {
		if (on_ctrl != -1)
			pmic_config_interface(addr, on_ctrl, 0x1,	/* ON_CTRL_MSAK */
					      3);	/* ON_CTRL_SHIFT */
		if (en != -1)
			pmic_config_interface(addr, en, 0x1,	/* EN_MASK */
					      1);	/* EN_SHIFT */
		if (mode_ctrl != -1)
			pmic_config_interface(addr, mode_ctrl, 0x1,	/* MODE_CTRL_MASK */
					      2);	/* MODE_CTRL_SHIFT */
		if (secclk_mode_sel != -1)
			pmic_config_interface(addr, secclk_mode_sel, 0x7,	/* SRCLK_MODE_SEL_MASK */
					      5);	/* SRCLK_MODE_SEL_SHIFT */
	}
}

#define PMIC_BUCK_SRCLKEN0	0
#define PMIC_BUCK_SRCLKEN2	4

#define PMIC_LDO_SRCLKEN_NA	-1
#define PMIC_LDO_SRCLKEN0	0
#define PMIC_LDO_SRCLKEN2	2

void spm_pmic_power_mode(int mode, int force, int lock)
{
	static int prev_mode = -1;

	if (mode < PMIC_PWR_NORMAL || mode >= PMIC_PWR_NUM) {
		pr_debug("wrong spm pmic power mode");
		return;
	}

	if (force == 0 && mode == prev_mode)
		return;

	switch (mode) {
	case PMIC_PWR_NORMAL:
		/* nothing */
		break;
	case PMIC_PWR_DEEPIDLE:
		spm_pmic_set_vcore(VCORE_VOSEL_SLEEP_0P9, lock);
		spm_pmic_set_buck(MT6351_BUCK_VCORE_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN2, lock);
		spm_pmic_set_buck(MT6351_BUCK_VS1_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN2, lock);
		spm_pmic_set_buck(MT6351_BUCK_VS2_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN2, lock);

		spm_pmic_set_ldo(MT6351_LDO_VDRAM_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN2, lock);
		spm_pmic_set_ldo(MT6351_LDO_VUSB33_CON0, 0, 1, 0, PMIC_LDO_SRCLKEN_NA, lock);	/* For Audio MP3 */
		spm_pmic_set_ldo(MT6351_LDO_VIO28_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN2, lock);
		spm_pmic_set_ldo(MT6351_LDO_VLDO28_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN2, lock);
		spm_pmic_set_ldo(MT6351_LDO_VIO18_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN2, lock);
		spm_pmic_set_ldo(MT6351_LDO_VA18_CON0, 0, 1, 0, PMIC_LDO_SRCLKEN_NA, lock);	/* For Audio MP3 */
		spm_pmic_set_ldo(MT6351_LDO_VA10_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN2, lock);
		break;
	case PMIC_PWR_SODI3:
		spm_pmic_set_vcore(VCORE_VOSEL_SLEEP_0P9, lock);
		spm_pmic_set_buck(MT6351_BUCK_VCORE_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN0, lock);
		spm_pmic_set_buck(MT6351_BUCK_VS1_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN0, lock);
		spm_pmic_set_buck(MT6351_BUCK_VS2_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN0, lock);

		spm_pmic_set_ldo(MT6351_LDO_VDRAM_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VUSB33_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VIO28_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VLDO28_CON0, 0, 1, 0, PMIC_LDO_SRCLKEN_NA, lock);	/* For Panel */
		spm_pmic_set_ldo(MT6351_LDO_VIO18_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VA18_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VA10_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		break;
	case PMIC_PWR_SODI:
		/* nothing */
		break;
	case PMIC_PWR_SUSPEND:
		spm_pmic_set_vcore(VCORE_VOSEL_SLEEP_0P7, lock);
		spm_pmic_set_buck(MT6351_BUCK_VCORE_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN0, lock);
		spm_pmic_set_buck(MT6351_BUCK_VS1_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN0, lock);
		spm_pmic_set_buck(MT6351_BUCK_VS2_CON0, 0, 1, 1, PMIC_BUCK_SRCLKEN0, lock);

		spm_pmic_set_ldo(MT6351_LDO_VDRAM_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VUSB33_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VIO28_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VLDO28_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VIO18_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VA18_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);
		spm_pmic_set_ldo(MT6351_LDO_VA10_CON0, 0, 1, 1, PMIC_LDO_SRCLKEN0, lock);

		mt_power_gs_dump_suspend();
		break;
	default:
		pr_debug("spm pmic power mode (%d) is not configured\n", mode);
	}

	prev_mode = mode;
}

void spm_bypass_boost_gpio_set(void)
{
#if 0
	u32 gpio_nf = 0;
	u32 gpio_dout_nf = 0;
	u32 gpio_dout_bit = 0;
	u32 gpio_dout_addr = 0;

	gpio_nf = 20;
#if 0
#if defined(CONFIG_MTK_LEGACY)
	/* TODO: get GPIO # from header */
	gpio_nf = (GPIO_BYPASS_BOOST_PIN & 0x0000FFFF);
#else
	/* TODO: get GPIO # from dtsi */
#endif
#endif

	gpio_dout_nf = gpio_nf / 32;
	gpio_dout_bit = gpio_nf % 32;
	gpio_dout_addr = gpio_base_addr + 0x100;
	gpio_dout_addr += gpio_dout_nf * 0x10;

#if 0
	pr_debug("bypass-boost: addr = 0x%x, bit = %d\n", gpio_dout_addr, gpio_dout_bit);
#endif

	spm_write(SPM_BSI_EN_SR, gpio_dout_addr);
	spm_write(SPM_BSI_CLK_SR, gpio_dout_bit);
#endif
}

u32 spm_get_register(void __force __iomem *offset)
{
	return spm_read(offset);
}

void spm_set_register(void __force __iomem *offset, u32 value)
{
	spm_write(offset, value);
}

#if defined(CONFIG_ARCH_MT6797)
void set_sodi_fw_mode(u32 sodi_fw)
{
	sodi_fw_mode = sodi_fw;
}

u32 get_sodi_fw_mode(void)
{
	return	sodi_fw_mode;
}

u32 spm_get_sodi_pcm_index(void)
{
	switch (get_sodi_fw_mode()) {
	case SODI_FW_LPM:	return DYNA_LOAD_PCM_SODI_LPM;
	case SODI_FW_HPM:	return DYNA_LOAD_PCM_SODI_HPM;
	case SODI_FW_ULTRA:	return DYNA_LOAD_PCM_SODI_ULTRA;
	default:
		break;
	}
	BUG();
}
#endif

MODULE_DESCRIPTION("SPM Driver v0.1");
