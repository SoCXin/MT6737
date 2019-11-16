#ifndef _SPM_
#define _SPM_

/* 
//XXX: only in kernel
#include <linux/kernel.h>
#include <linux/io.h>

#include <mach/mt_reg_base.h>
#include <mach/mt_irq.h>
#include <mach/sync_write.h>
*/
#include <platform.h>
#include <typedefs.h>
#include <sync_write.h>

/**************************************
 * Config and Parameter
 **************************************/
//#define SPM_BASE		SLEEP_BASE
#define SPM_IRQ0_ID		SLEEP_IRQ_BIT0_ID
#define SPM_IRQ1_ID		SLEEP_IRQ_BIT1_ID
#define SPM_IRQ2_ID		SLEEP_IRQ_BIT2_ID
#define SPM_IRQ3_ID		SLEEP_IRQ_BIT3_ID
#define SPM_IRQ4_ID		SLEEP_IRQ_BIT4_ID
#define SPM_IRQ5_ID		SLEEP_IRQ_BIT5_ID
#define SPM_IRQ6_ID		SLEEP_IRQ_BIT6_ID
#define SPM_IRQ7_ID		SLEEP_IRQ_BIT7_ID


/**************************************
 * Define and Declare
 **************************************/
#define SPM_POWERON_CONFIG_SET		(SPM_BASE + 0x000)
#define SPM_POWER_ON_VAL0		(SPM_BASE + 0x010)
#define SPM_POWER_ON_VAL1		(SPM_BASE + 0x014)
#define SPM_CLK_SETTLE			(SPM_BASE + 0x100)
#define SPM_CA7_CPU0_PWR_CON		(SPM_BASE + 0x200)
#define SPM_CA7_DBG_PWR_CON		(SPM_BASE + 0x204)
#define SPM_CA7_CPUTOP_PWR_CON		(SPM_BASE + 0x208)
#define SPM_VDE_PWR_CON			(SPM_BASE + 0x210)
#define SPM_MFG_PWR_CON			(SPM_BASE + 0x214)
#define SPM_CA7_CPU1_PWR_CON		(SPM_BASE + 0x218)
#define SPM_CA7_CPU2_PWR_CON		(SPM_BASE + 0x21c)
#define SPM_CA7_CPU3_PWR_CON		(SPM_BASE + 0x220)
#define SPM_VEN_PWR_CON			(SPM_BASE + 0x230)
#define SPM_IFR_PWR_CON			(SPM_BASE + 0x234)
#define SPM_ISP_PWR_CON			(SPM_BASE + 0x238)
#define SPM_DIS_PWR_CON			(SPM_BASE + 0x23c)
#define SPM_DPY_PWR_CON			(SPM_BASE + 0x240)
#define SPM_CA7_CPUTOP_L2_PDN		(SPM_BASE + 0x244)
#define SPM_CA7_CPUTOP_L2_SLEEP		(SPM_BASE + 0x248)
#define SPM_CA7_CPU0_L1_PDN		(SPM_BASE + 0x25c)
#define SPM_CA7_CPU1_L1_PDN		(SPM_BASE + 0x264)
#define SPM_CA7_CPU2_L1_PDN		(SPM_BASE + 0x26c)
#define SPM_CA7_CPU3_L1_PDN		(SPM_BASE + 0x274)
#define SPM_GCPU_SRAM_CON		(SPM_BASE + 0x27c)
#define SPM_CONN_PWR_CON		(SPM_BASE + 0x280)
#define SPM_MD_PWR_CON			(SPM_BASE + 0x284)
#define SPM_MCU_PWR_CON			(SPM_BASE + 0x290)
#define SPM_IFR_SRAMROM_CON		(SPM_BASE + 0x294)
#define SPM_MJC_PWR_CON			(SPM_BASE + 0x298)
#define SPM_AUDIO_PWR_CON		(SPM_BASE + 0x29c)
#define SPM_CA15_CPU0_PWR_CON		(SPM_BASE + 0x2a0)
#define SPM_CA15_CPU1_PWR_CON		(SPM_BASE + 0x2a4)
#define SPM_CA15_CPU2_PWR_CON		(SPM_BASE + 0x2a8)
#define SPM_CA15_CPU3_PWR_CON		(SPM_BASE + 0x2ac)
#define SPM_CA15_CPUTOP_PWR_CON		(SPM_BASE + 0x2b0)
#define SPM_CA15_L1_PWR_CON		(SPM_BASE + 0x2b4)
#define SPM_CA15_L2_PWR_CON		(SPM_BASE + 0x2b8)
#define SPM_MFG_2D_PWR_CON		(SPM_BASE + 0x2c0)
#define SPM_MFG_ASYNC_PWR_CON		(SPM_BASE + 0x2c4)
#define SPM_MD32_SRAM_CON		(SPM_BASE + 0x2c8)
#define SPM_ARMPLL_DIV_PWR_CON  (SPM_BASE + 0x2cc)
#define SPM_MD2_PWR_CON         (SPM_BASE + 0x2d0)    
#define SPM_C2K_PWR_CON         (SPM_BASE + 0x2d4)//mt6735  
#define SPM_INFRA_MD_PWR_CON    (SPM_BASE + 0x2d8)//mt6735 
#define SPM_CPU_EXT_ISO         (SPM_BASE + 0x2dc)//mt6735
#define SPM_PCM_CON0			(SPM_BASE + 0x310)
#define SPM_PCM_CON1			(SPM_BASE + 0x314)
#define SPM_PCM_IM_PTR			(SPM_BASE + 0x318)
#define SPM_PCM_IM_LEN			(SPM_BASE + 0x31c)
#define SPM_PCM_REG_DATA_INI		(SPM_BASE + 0x320)
#define SPM_PCM_EVENT_VECTOR0		(SPM_BASE + 0x340)
#define SPM_PCM_EVENT_VECTOR1		(SPM_BASE + 0x344)
#define SPM_PCM_EVENT_VECTOR2		(SPM_BASE + 0x348)
#define SPM_PCM_EVENT_VECTOR3		(SPM_BASE + 0x34c)
#define SPM_PCM_MAS_PAUSE_MASK		(SPM_BASE + 0x354)
#define SPM_PCM_PWR_IO_EN		(SPM_BASE + 0x358)
#define SPM_PCM_TIMER_VAL		(SPM_BASE + 0x35c)
#define SPM_PCM_TIMER_OUT		(SPM_BASE + 0x360)
#define SPM_PCM_REG0_DATA		(SPM_BASE + 0x380)
#define SPM_PCM_REG1_DATA		(SPM_BASE + 0x384)
#define SPM_PCM_REG2_DATA		(SPM_BASE + 0x388)
#define SPM_PCM_REG3_DATA		(SPM_BASE + 0x38c)
#define SPM_PCM_REG4_DATA		(SPM_BASE + 0x390)
#define SPM_PCM_REG5_DATA		(SPM_BASE + 0x394)
#define SPM_PCM_REG6_DATA		(SPM_BASE + 0x398)
#define SPM_PCM_REG7_DATA		(SPM_BASE + 0x39c)
#define SPM_PCM_REG8_DATA		(SPM_BASE + 0x3a0)
#define SPM_PCM_REG9_DATA		(SPM_BASE + 0x3a4)
#define SPM_PCM_REG10_DATA		(SPM_BASE + 0x3a8)
#define SPM_PCM_REG11_DATA		(SPM_BASE + 0x3ac)
#define SPM_PCM_REG12_DATA		(SPM_BASE + 0x3b0)
#define SPM_PCM_REG13_DATA		(SPM_BASE + 0x3b4)
#define SPM_PCM_REG14_DATA		(SPM_BASE + 0x3b8)
#define SPM_PCM_REG15_DATA		(SPM_BASE + 0x3bc)
#define SPM_PCM_EVENT_REG_STA		(SPM_BASE + 0x3c0)
#define SPM_PCM_FSM_STA			(SPM_BASE + 0x3c4)
#define SPM_PCM_IM_HOST_RW_PTR		(SPM_BASE + 0x3c8)
#define SPM_PCM_IM_HOST_RW_DAT		(SPM_BASE + 0x3cc)
#define SPM_PCM_EVENT_VECTOR4		(SPM_BASE + 0x3d0)
#define SPM_PCM_EVENT_VECTOR5		(SPM_BASE + 0x3d4)
#define SPM_PCM_EVENT_VECTOR6		(SPM_BASE + 0x3d8)
#define SPM_PCM_EVENT_VECTOR7		(SPM_BASE + 0x3dc)
#define SPM_PCM_SW_INT_SET		(SPM_BASE + 0x3e0)
#define SPM_PCM_SW_INT_CLEAR		(SPM_BASE + 0x3e4)
#define SPM_CLK_CON			(SPM_BASE + 0x400)
#define SPM_SLEEP_DUAL_VCORE_PWR_CON	(SPM_BASE + 0x404)
#define SPM_SLEEP_PTPOD2_CON		(SPM_BASE + 0x408)
#define SPM_APMCU_PWRCTL		(SPM_BASE + 0x600)
#define SPM_AP_DVFS_CON_SET		(SPM_BASE + 0x604)
#define SPM_AP_STANBY_CON		(SPM_BASE + 0x608)
#define SPM_PWR_STATUS			(SPM_BASE + 0x60c)
#define SPM_PWR_STATUS_2ND		(SPM_BASE + 0x610)
//#define SPM_AP_BSI_REQ			(SPM_BASE + 0x614)     
#define SPM_SLEEP_MDBSI_CON		(SPM_BASE + 0x614)//mt6735 
#define SPM_BSI_GEN		        (SPM_BASE + 0x620)//mt6735
#define SPM_BSI_EN_SR		      (SPM_BASE + 0x624)//mt6735
#define SPM_BSI_CLK_SR		    (SPM_BASE + 0x628)//mt6735
#define SPM_BSI_DO_SR		      (SPM_BASE + 0x62c)//mt6735
#define SPM_BSI_D1_SR		      (SPM_BASE + 0x630)//mt6735
#define SPM_BSI_D2_SR		      (SPM_BASE + 0x634)//mt6735
#define SPM_AP_SEMA		        (SPM_BASE + 0x638)//mt6735
#define SPM_SPM_SEMA		      (SPM_BASE + 0x63c)//mt6735                    
#define SPM_SLEEP_TIMER_STA		(SPM_BASE + 0x720)
#define SPM_SLEEP_TWAM_CON		(SPM_BASE + 0x760)
#define SPM_SLEEP_TWAM_STATUS0		(SPM_BASE + 0x764)
#define SPM_SLEEP_TWAM_STATUS1		(SPM_BASE + 0x768)
#define SPM_SLEEP_TWAM_STATUS2		(SPM_BASE + 0x76c)
#define SPM_SLEEP_TWAM_STATUS3		(SPM_BASE + 0x770)
#define SPM_SLEEP_TWAM_CURR_STATUS0	(SPM_BASE + 0x774)
#define SPM_SLEEP_TWAM_CURR_STATUS1	(SPM_BASE + 0x778)
#define SPM_SLEEP_TWAM_CURR_STATUS2	(SPM_BASE + 0x77C)
#define SPM_SLEEP_TWAM_CURR_STATUS3	(SPM_BASE + 0x780)
#define SPM_SLEEP_TWAM_TIMER_OUT	(SPM_BASE + 0x784)
#define SPM_SLEEP_TWAM_WINDOW_LEN	(SPM_BASE + 0x788)
#define SPM_SLEEP_IDLE_SEL      	(SPM_BASE + 0x78C)
#define SPM_SLEEP_WAKEUP_EVENT_MASK	(SPM_BASE + 0x810)
#define SPM_SLEEP_CPU_WAKEUP_EVENT	(SPM_BASE + 0x814)
#define SPM_SLEEP_MD32_WAKEUP_EVENT_MASK	(SPM_BASE + 0x818)
#define SPM_PCM_WDT_TIMER_VAL		(SPM_BASE + 0x824)
#define SPM_PCM_WDT_TIMER_OUT		(SPM_BASE + 0x828)
#define SPM_PCM_MD32_MAILBOX		(SPM_BASE + 0x830)
#define SPM_PCM_MD32_IRQ		(SPM_BASE + 0x834)
#define SPM_SLEEP_ISR_MASK		(SPM_BASE + 0x900)
#define SPM_SLEEP_ISR_STATUS		(SPM_BASE + 0x904)
#define SPM_SLEEP_ISR_RAW_STA		(SPM_BASE + 0x910)
#define SPM_SLEEP_MD32_ISR_RAW_STA	(SPM_BASE + 0x914)
#define SPM_SLEEP_WAKEUP_MISC		(SPM_BASE + 0x918)
#define SPM_SLEEP_BUS_PROTECT_RDY	(SPM_BASE + 0x91c)
#define SPM_SLEEP_SUBSYS_IDLE_STA	(SPM_BASE + 0x920)
#define SPM_PCM_RESERVE			(SPM_BASE + 0xb00)
#define SPM_PCM_RESERVE2		(SPM_BASE + 0xb04)
#define SPM_PCM_FLAGS			(SPM_BASE + 0xb08)
#define SPM_PCM_SRC_REQ			(SPM_BASE + 0xb0c)
#define SPM_PCM_DEBUG_CON		(SPM_BASE + 0xb20)
#define SPM_CA7_CPU0_IRQ_MASK		(SPM_BASE + 0xb30)
#define SPM_CA7_CPU1_IRQ_MASK		(SPM_BASE + 0xb34)
#define SPM_CA7_CPU2_IRQ_MASK		(SPM_BASE + 0xb38)
#define SPM_CA7_CPU3_IRQ_MASK		(SPM_BASE + 0xb3c)
#define SPM_CA15_CPU0_IRQ_MASK		(SPM_BASE + 0xb40)
#define SPM_CA15_CPU1_IRQ_MASK		(SPM_BASE + 0xb44)
#define SPM_CA15_CPU2_IRQ_MASK		(SPM_BASE + 0xb48)
#define SPM_CA15_CPU3_IRQ_MASK		(SPM_BASE + 0xb4c)
#define SPM_PCM_PASR_DPD_0		(SPM_BASE + 0xb60)
#define SPM_PCM_PASR_DPD_1		(SPM_BASE + 0xb64)
#define SPM_PCM_PASR_DPD_2		(SPM_BASE + 0xb68)
#define SPM_PCM_PASR_DPD_3		(SPM_BASE + 0xb6c)
#define SPM_SLEEP_CA7_WFI0_EN		(SPM_BASE + 0xf00)
#define SPM_SLEEP_CA7_WFI1_EN		(SPM_BASE + 0xf04)
#define SPM_SLEEP_CA7_WFI2_EN		(SPM_BASE + 0xf08)
#define SPM_SLEEP_CA7_WFI3_EN		(SPM_BASE + 0xf0c)
#define SPM_SLEEP_CA15_WFI0_EN		(SPM_BASE + 0xf10)
#define SPM_SLEEP_CA15_WFI1_EN		(SPM_BASE + 0xf14)
#define SPM_SLEEP_CA15_WFI2_EN		(SPM_BASE + 0xf18)
#define SPM_SLEEP_CA15_WFI3_EN		(SPM_BASE + 0xf1c)

#define SPM_PROJECT_CODE	0xb16

#define SPM_REGWR_EN		(1U << 0)
#define SPM_REGWR_CFG_KEY	(SPM_PROJECT_CODE << 16)

#if 1
/* PCM Flags store in PCM_RESERVE4(0xB18)*/
#define SPM_CPU_PDN_DIS		  (1U << 0)
#define SPM_INFRA_PDN_DIS	  (1U << 1)
#define SPM_DDRPHY_PDN_DIS	(1U << 2)
#define SPM_VCORE_DVS_DIS	  (1U << 3)//mt6735 no use
#define SPM_PASR_DIS		    (1U << 4)
#define SPM_MD_VRF18_DIS		(1U << 5)//mt6735  
#define SPM_CMD_LCM_EN		  (1U << 6)//mt6735
#define SPM_MEMPLL_RESET	  (1U << 7)
#define SPM_VCORE_DVFS_EN	  (1U << 8)//mt6735
#define SPM_CPU_DVS_DIS		    (1U << 9)   
#define SPM_IFRA_MD_PDN_DIS		(1U << 10)//mt6735
#define SPM_EXT_VSEL_GPIO103	    (1U << 11)//mt6735 no use
#define SPM_DDR_HIGH_SPEED	      (1U << 12)//mt6735 no use
#define SPM_SCREEN_OFF		        (1U << 13)//mt6735 no use
#define SPM_MEMPLL_1PLL_3PLL_SEL  (1U << 16)//mt6735 no use
#define SPM_VCORE_DVS_POSITION	  (1U << 17)//mt6735 no use
#define SPM_BUCK_SEL				      (1U << 18)//mt6735 no use
#define SPM_DRAM_RANK1_ADDR_SEL0	(1U << 19)//0x60000000
#define SPM_DRAM_RANK1_ADDR_SEL1	(1U << 20)//0x80000000
#define SPM_DRAM_RANK1_ADDR_SEL2	(1U << 21)//0xc0000000
#else
#define SPM_CPU_PDN_DIS		  (1U << 0)                               
#define SPM_INFRA_PDN_DIS	  (1U << 1)                               
#define SPM_DDRPHY_PDN_DIS	(1U << 2)                               
#define SPM_VCORE_DVS_DIS	  (1U << 3)                
#define SPM_PASR_DIS		    (1U << 4)                               
#define SPM_DPD_DIS		      (1U << 5)                               
#define SPM_SODI_DIS		    (1U << 6)                                                     
#define SPM_MEMPLL_RESET	  (1U << 7)                               
#define SPM_MAINPLL_PDN_DIS	(1U << 8)                                                  
#define SPM_CPU_DVS_DIS		  (1U << 9)                             
#define SPM_CPU_DORMANT		  (1U << 10)                                                                                                       
#define SPM_EXT_VSEL_GPIO103	    (1U << 11)       
#define SPM_DDR_HIGH_SPEED	      (1U << 12)       
#define SPM_SCREEN_OFF		        (1U << 13)               
#endif                                                                    

/* Wakeup Source*/
#if 1
#define SPM_WAKE_SRC_LIST	{	\
	SPM_WAKE_SRC(0, SPM_MERGE),	/* PCM timer, TWAM or CPU */	\
	SPM_WAKE_SRC(1, LTE_PTP),	\
	SPM_WAKE_SRC(2, KP),		\
	SPM_WAKE_SRC(3, WDT),		\
	SPM_WAKE_SRC(4, GPT),		\
	SPM_WAKE_SRC(5, EINT),	\
	SPM_WAKE_SRC(6, CONN_WDT),		\
	SPM_WAKE_SRC(7, CCIF0_MD),	\
	SPM_WAKE_SRC(8, CCIF1_MD),	\
	SPM_WAKE_SRC(9, LOW_BAT),	\
	SPM_WAKE_SRC(10, CONN2AP),	\
	SPM_WAKE_SRC(11, F26M_WAKE),	\
	SPM_WAKE_SRC(12, F26M_SLEEP),	\
	SPM_WAKE_SRC(13, PCM_WDT),	\
	SPM_WAKE_SRC(14, USB_CD),	\
	SPM_WAKE_SRC(15, USB_PDN),	\
	SPM_WAKE_SRC(16, LTE_WAKE),	\
	SPM_WAKE_SRC(17, LTE_SLEEP),	\
	SPM_WAKE_SRC(18, SEJ),	\
	SPM_WAKE_SRC(19, UART0),	\
	SPM_WAKE_SRC(20, AFE),		\
	SPM_WAKE_SRC(21, THERM),	\
	SPM_WAKE_SRC(22, CIRQ),		\
	SPM_WAKE_SRC(23, MD1_VRF18_WAKE),	\
	SPM_WAKE_SRC(24, SYSPWREQ),	\
	SPM_WAKE_SRC(25, MD_WDT),	\
	SPM_WAKE_SRC(26, C2K_WDT),	\
	SPM_WAKE_SRC(27, CLDMA_MD),		\
	SPM_WAKE_SRC(28, MD1_VRF18_SLEEP),	\
	SPM_WAKE_SRC(29, CPU_IRQ),	\
	SPM_WAKE_SRC(30, APSRC_WAKE),	\
	SPM_WAKE_SRC(31, APSRC_SLEEP)	\
}
#else
#define SPM_WAKE_SRC_LIST	{	\                                   
	SPM_WAKE_SRC(0, SPM_MERGE),	/* PCM timer, TWAM or CPU */	\   
	SPM_WAKE_SRC(1, MD32_WDT),	\                                 
	SPM_WAKE_SRC(2, KP),		\                                     
	SPM_WAKE_SRC(3, WDT),		\                                     
	SPM_WAKE_SRC(4, GPT),		\                                     
	SPM_WAKE_SRC(5, CONN2AP),	\                                   
	SPM_WAKE_SRC(6, EINT),		\                                   
	SPM_WAKE_SRC(7, CONN_WDT),	\                                 
	SPM_WAKE_SRC(8, CCIF0_MD),	\                                 
	SPM_WAKE_SRC(9, LOW_BAT),	\                                   
	SPM_WAKE_SRC(10, MD32_SPM),	\                                 
	SPM_WAKE_SRC(11, F26M_WAKE),	\                               
	SPM_WAKE_SRC(12, F26M_SLEEP),	\                               
	SPM_WAKE_SRC(13, PCM_WDT),	\                                 
	SPM_WAKE_SRC(14, USB_CD),	\                                   
	SPM_WAKE_SRC(15, USB_PDN),	\                                 
	SPM_WAKE_SRC(16, LTE_WAKE),	\                                 
	SPM_WAKE_SRC(17, LTE_SLEEP),	\                               
	SPM_WAKE_SRC(18, CCIF1_MD),	\                                 
	SPM_WAKE_SRC(19, UART0),	\                                   
	SPM_WAKE_SRC(20, AFE),		\                                   
	SPM_WAKE_SRC(21, THERM),	\                                   
	SPM_WAKE_SRC(22, CIRQ),		\                                   
	SPM_WAKE_SRC(23, MD2_WDT),	\                                 
	SPM_WAKE_SRC(24, SYSPWREQ),	\                                 
	SPM_WAKE_SRC(25, MD_WDT),	\                                   
	SPM_WAKE_SRC(26, CLDMA_MD),	\                                 
	SPM_WAKE_SRC(27, SEJ),		\                                   
	SPM_WAKE_SRC(28, ALL_MD32),	\                                 
	SPM_WAKE_SRC(29, CPU_IRQ),	\                                 
	SPM_WAKE_SRC(30, APSRC_WAKE),	\                               
	SPM_WAKE_SRC(31, APSRC_SLEEP)	\                               
}                                                               
#endif
/* define WAKE_SRC_XXX */
#undef SPM_WAKE_SRC
#define SPM_WAKE_SRC(id, name)	\
	WAKE_SRC_##name = (1U << (id))
enum SPM_WAKE_SRC_LIST;

typedef enum {
	WR_NONE			= 0,
	WR_UART_BUSY		= 1,
	WR_PCM_ASSERT		= 2,
	WR_PCM_TIMER		= 3,
	WR_WAKE_SRC		= 4,
	WR_UNKNOWN		= 5,
} wake_reason_t;

struct twam_sig {
	u32 sig0;		/* signal 0: config or status */
	u32 sig1;		/* signal 1: config or status */
	u32 sig2;		/* signal 2: config or status */
	u32 sig3;		/* signal 3: config or status */
};

typedef void (*twam_handler_t)(struct twam_sig *twamsig);

/* for power management init */
extern int spm_module_init(void);

/* for ANC in talking */
extern void spm_mainpll_on_request(const char *drv_name);
extern void spm_mainpll_on_unrequest(const char *drv_name);

/* for TWAM in MET tool */
extern void spm_twam_register_handler(twam_handler_t handler);
extern void spm_twam_enable_monitor(struct twam_sig *twamsig, bool speed_mode);
extern void spm_twam_disable_monitor(void);


/**************************************
 * Macro and Inline
 **************************************/
/* 
//XXX: only in kernel
#define spm_read(addr)			__raw_readl(IOMEM(addr))
*/
#define spm_read(addr)			__raw_readl(addr)
#define spm_write(addr, val)		mt_reg_sync_writel(val, addr)

#define is_cpu_pdn(flags)		(!((flags) & SPM_CPU_PDN_DIS))
#define is_infra_pdn(flags)		(!((flags) & SPM_INFRA_PDN_DIS))
#define is_ddrphy_pdn(flags)		(!((flags) & SPM_DDRPHY_PDN_DIS))
#define is_dualvcore_pdn(flags)		(!((flags) & SPM_DUALVCORE_PDN_DIS))

#define get_high_cnt(sigsta)		((sigsta) & 0x3ff)
#define get_high_percent(sigsta)	(get_high_cnt(sigsta) * 100 / 1023)

#endif
