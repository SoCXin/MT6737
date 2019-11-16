/*******************************************************************************
* mt_pwm.c PWM Drvier
*
* Copyright (c) 2012, Media Teck.inc
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public Licence,
* version 2, as publish by the Free Software Foundation.
*
* This program is distributed and in hope it will be useful, but WITHOUT
* ANY WARRNTY; without even the implied warranty of MERCHANTABITLITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*/

#include <linux/types.h>

#include <mach/mt_pwm_hal_pub.h>
#include <mach/mt_pwm_hal.h>
#include <mach/mt_pwm_prv.h>
#if !defined(CONFIG_MTK_CLKMGR)
#include <linux/clk.h>
#else
#include <mach/mt_clkmgr.h>
#endif


/**********************************
* Global  data
***********************************/
enum {
	PWM_CON,
	PWM_HDURATION,
	PWM_LDURATION,
	PWM_GDURATION,
	PWM_BUF0_BASE_ADDR,
	PWM_BUF0_SIZE,
	PWM_BUF1_BASE_ADDR,
	PWM_BUF1_SIZE,
	PWM_SEND_DATA0,
	PWM_SEND_DATA1,
	PWM_WAVE_NUM,
	PWM_DATA_WIDTH,
	PWM_THRESH,
	PWM_SEND_WAVENUM,
	PWM_VALID
} PWM_REG_OFF;


#if defined(CONFIG_MTK_CLKMGR)
int pwm_power_id[] = {
		MT_CG_PERI_PWM1,
		MT_CG_PERI_PWM2,
		MT_CG_PERI_PWM3,
		MT_CG_PERI_PWM4,
		MT_CG_PERI_PWM5,
		MT_CG_PERI_PWM
	};

#define PWM_CG		5

#endif
#ifdef CONFIG_OF
unsigned long PWM_register[PWM_NUM] = {};
#else
unsigned long PWM_register[PWM_NUM] = {
	(PWM_BASE+0x0010),	   /* PWM1 register base,   15 registers */
	(PWM_BASE+0x0050),	   /* PWM2 register base    15 registers */
	(PWM_BASE+0x0090),	   /* PWM3 register base    15 registers */
	(PWM_BASE+0x00d0),	   /* PWM4 register base    13 registers */
	(PWM_BASE+0x0110),	   /* PWM5 register base    13 registers */
};
#endif
/**************************************************************/

#if !defined(CONFIG_MTK_CLKMGR)
enum {
	PWM1_CLK,
	PWM2_CLK,
	PWM3_CLK,
	PWM4_CLK,
	PWM5_CLK,
	PWM_CLK,
	PWM_CLK_NUM,
};

const char *pwm_clk_name[] = {
	"PWM1-main", "PWM2-main", "PWM3-main", "PWM4-main", "PWM5-main", "PWM-main"
};

struct clk *pwm_clk[PWM_CLK_NUM];

void mt_pwm_power_on_hal(u32 pwm_no, bool pmic_pad, unsigned long *power_flag)
{
	int clk_en_ret;
	/*Set pwm_main , pwm_hclk_main(for memory and random mode) */
	if (0 == (*power_flag)) {
		pr_warn("[PWM][CCF]enable clk PWM_CLK:%p\n", pwm_clk[PWM_CLK]);
		clk_en_ret = clk_prepare_enable(pwm_clk[PWM_CLK]);
		if (clk_en_ret) {
			pr_err("[PWM][CCF]enable clk PWM_CLK failed. ret:%d, clk_pwm_main:%p\n",
			clk_en_ret, pwm_clk[PWM_CLK]);
		} else
			set_bit(PWM_CLK, power_flag);

	}
	/* Set pwm_no clk */
	if (!test_bit(pwm_no, power_flag)) {
		pr_warn("[PWM][CCF]enable clk_pwm%d :%p\n", pwm_no, pwm_clk[pwm_no]);
		clk_en_ret = clk_prepare_enable(pwm_clk[pwm_no]);
		if (clk_en_ret) {
			pr_err("[PWM][CCF]enable clk_pwm_main failed. ret:%d, clk_pwm%d :%p\n",
			clk_en_ret, pwm_no, pwm_clk[pwm_no]);
		} else
			set_bit(pwm_no, power_flag);
	}

}

void mt_pwm_power_off_hal(u32 pwm_no, bool pmic_pad, unsigned long *power_flag)
{
	if (test_bit(pwm_no, power_flag)) {
		pr_debug("[PWM][CCF]disable clk_pwm%d :%p\n", pwm_no, pwm_clk[pwm_no]);
		clk_disable_unprepare(pwm_clk[pwm_no]);
		clear_bit(pwm_no, power_flag);
	}

	/*Disable PWM-main, PWM-HCLK-main */
	pr_debug("[PWM][CCF]disable clk_pwm :%p\n", pwm_clk[PWM_CLK]);

	if (test_bit(PWM_CLK, power_flag)) {
		clk_disable_unprepare(pwm_clk[PWM_CLK]);
		clear_bit(PWM_CLK, power_flag);
	}
}

#else
void mt_pwm_power_on_hal(u32 pwm_no, bool pmic_pad, unsigned long *power_flag)
{
	if (0 == (*power_flag)) {
		PWMDBG("enable_clock: main\n");
		enable_clock(pwm_power_id[PWM_CG], "PWM");
		set_bit(PWM_CG, power_flag);
	}
	if (!test_bit(pwm_no, power_flag)) {
		PWMDBG("enable_clock: %d\n", pwm_no);

			enable_clock(pwm_power_id[pwm_no], "PWM");

		set_bit(pwm_no, power_flag);
		PWMDBG("enable_clock PWM%d\n", pwm_no+1);
	}
}

void mt_pwm_power_off_hal(u32 pwm_no, bool pmic_pad, unsigned long *power_flag)
{
	if (test_bit(pwm_no, power_flag)) {
		PWMDBG("disable_clock: %d\n", pwm_no);
		disable_clock(pwm_power_id[pwm_no], "PWM");
		clear_bit(pwm_no, power_flag);
		PWMDBG("disable_clock PWM%d\n", pwm_no+1);
	}
	if (BIT(PWM_CG) == (*power_flag)) {
		PWMDBG("disable_clock: main\n");
		disable_clock(pwm_power_id[PWM_CG], "PWM");
		clear_bit(PWM_CG, power_flag);
	}
}
#endif

void mt_pwm_init_power_flag(unsigned long *power_flag)
{
#ifdef CONFIG_OF
	PWM_register[PWM1] = (unsigned long)pwm_base + 0x0010;
	PWM_register[PWM2] = (unsigned long)pwm_base + 0x0050;
	PWM_register[PWM3] = (unsigned long)pwm_base + 0x0090;
	PWM_register[PWM4] = (unsigned long)pwm_base + 0x00d0;
	PWM_register[PWM5] = (unsigned long)pwm_base + 0x0110;
#endif
}

s32 mt_pwm_sel_pmic_hal(u32 pwm_no)
{
	PWMDBG("mt_pwm_sel_pmic\n");
	return -EINVALID;
}

s32 mt_pwm_sel_ap_hal(u32 pwm_no)
{
	PWMDBG("mt_pwm_sel_ap\n");
	return -EINVALID;
}

void mt_set_pwm_enable_hal(u32 pwm_no)
{
	SETREG32(PWM_ENABLE, 1 << pwm_no);
}

void mt_set_pwm_disable_hal(u32 pwm_no)
{
	CLRREG32(PWM_ENABLE, 1 << pwm_no);
}

void mt_set_pwm_enable_seqmode_hal(void)
{
}

void mt_set_pwm_disable_seqmode_hal(void)
{
}

s32 mt_set_pwm_test_sel_hal(u32 val)
{
	return 0;
}

void mt_set_pwm_clk_hal(u32 pwm_no, u32 clksrc, u32 div)
{
	unsigned long reg_con;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;
	MASKREG32(reg_con, PWM_CON_CLKDIV_MASK, div);
	if ((clksrc & 0x80000000) != 0) {
		clksrc &= ~(0x80000000);
		if (clksrc == CLK_BLOCK_BY_1625_OR_32K) {	/* old mode: 26M/1625 = 16KHz */
			CLRREG32(reg_con, 1 << PWM_CON_CLKSEL_OLD_OFFSET);	/* bit 4: 0 */
			SETREG32(reg_con, 1 << PWM_CON_CLKSEL_OFFSET);	/* bit 3: 1 */
		} else {	/* old mode 32k clk */
			SETREG32(reg_con, 1 << PWM_CON_CLKSEL_OLD_OFFSET);
			SETREG32(reg_con, 1 << PWM_CON_CLKSEL_OFFSET);
		}
	} else {
		CLRREG32(reg_con, 1 << PWM_CON_CLKSEL_OLD_OFFSET);
		if (clksrc == CLK_BLOCK)
			CLRREG32(reg_con, 1 << PWM_CON_CLKSEL_OFFSET);
		else if (clksrc == CLK_BLOCK_BY_1625_OR_32K)
			SETREG32(reg_con, 1 << PWM_CON_CLKSEL_OFFSET);
	}
}

s32 mt_get_pwm_clk_hal(u32 pwm_no)
{
	s32 clk, clksrc, clkdiv;
	unsigned long reg_con, reg_val, reg_en;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;

	reg_val = INREG32(reg_con);
	reg_en = INREG32(PWM_ENABLE);

	if (((reg_val & PWM_CON_CLKSEL_MASK) >> PWM_CON_CLKSEL_OFFSET) == 1)
		if (((reg_en & PWM_CON_OLD_MODE_MASK) >> PWM_CON_OLD_MODE_OFFSET) == 1)
			clksrc = 32 * 1024;
		else
			clksrc = BLOCK_CLK;
	else
		clksrc = BLOCK_CLK / 1625;

	clkdiv = 2 << (reg_val & PWM_CON_CLKDIV_MASK);
	if (clkdiv <= 0) {
		PWMDBG("clkdiv less zero, not valid\n");
		return -ERROR;
	}

	clk = clksrc / clkdiv;
	PWMDBG("CLK is :%d\n", clk);
	return clk;
}

s32 mt_set_pwm_con_datasrc_hal(u32 pwm_no, u32 val)
{
	unsigned long reg_con;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;
	if (val == PWM_FIFO)
		CLRREG32(reg_con, 1 << PWM_CON_SRCSEL_OFFSET);
	else if (val == MEMORY)
		SETREG32(reg_con, 1 << PWM_CON_SRCSEL_OFFSET);
	else
		return 1;
	return 0;
}

s32 mt_set_pwm_con_mode_hal(u32 pwm_no, u32 val)
{
	unsigned long reg_con;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;
	if (val == PERIOD)
		CLRREG32(reg_con, 1 << PWM_CON_MODE_OFFSET);
	else if (val == RAND)
		SETREG32(reg_con, 1 << PWM_CON_MODE_OFFSET);
	else
		return 1;
	return 0;
}

s32 mt_set_pwm_con_idleval_hal(u32 pwm_no, uint16_t val)
{
	unsigned long reg_con;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;
	if (val == IDLE_TRUE)
		SETREG32(reg_con, 1 << PWM_CON_IDLE_VALUE_OFFSET);
	else if (val == IDLE_FALSE)
		CLRREG32(reg_con, 1 << PWM_CON_IDLE_VALUE_OFFSET);
	else
		return 1;
	return 0;
}

s32 mt_set_pwm_con_guardval_hal(u32 pwm_no, uint16_t val)
{
	unsigned long reg_con;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;
	if (val == GUARD_TRUE)
		SETREG32(reg_con, 1 << PWM_CON_GUARD_VALUE_OFFSET);
	else if (val == GUARD_FALSE)
		CLRREG32(reg_con, 1 << PWM_CON_GUARD_VALUE_OFFSET);
	else
		return 1;
	return 0;
}

void mt_set_pwm_con_stpbit_hal(u32 pwm_no, u32 stpbit, u32 srcsel)
{
	unsigned long reg_con;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;
	if (srcsel == PWM_FIFO)
		MASKREG32(reg_con, PWM_CON_STOP_BITS_MASK, stpbit << PWM_CON_STOP_BITS_OFFSET);
	if (srcsel == MEMORY)
		MASKREG32(reg_con, PWM_CON_STOP_BITS_MASK & (0x1f << PWM_CON_STOP_BITS_OFFSET),
			  stpbit << PWM_CON_STOP_BITS_OFFSET);
}

s32 mt_set_pwm_con_oldmode_hal(u32 pwm_no, u32 val)
{
	unsigned long reg_con;

	reg_con = PWM_register[pwm_no] + 4 * PWM_CON;
	if (val == OLDMODE_DISABLE)
		CLRREG32(reg_con, 1 << PWM_CON_OLD_MODE_OFFSET);
	else if (val == OLDMODE_ENABLE)
		SETREG32(reg_con, 1 << PWM_CON_OLD_MODE_OFFSET);
	else
		return 1;
	return 0;
}

void mt_set_pwm_HiDur_hal(u32 pwm_no, uint16_t DurVal)
{				/* only low 16 bits are valid */
	unsigned long reg_HiDur;

	reg_HiDur = PWM_register[pwm_no] + 4 * PWM_HDURATION;
	OUTREG32(reg_HiDur, DurVal);
}

void mt_set_pwm_LowDur_hal(u32 pwm_no, uint16_t DurVal)
{
	unsigned long reg_LowDur;

	reg_LowDur = PWM_register[pwm_no] + 4 * PWM_LDURATION;
	OUTREG32(reg_LowDur, DurVal);
}

void mt_set_pwm_GuardDur_hal(u32 pwm_no, uint16_t DurVal)
{
	unsigned long reg_GuardDur;

	reg_GuardDur = PWM_register[pwm_no] + 4 * PWM_GDURATION;
	OUTREG32(reg_GuardDur, DurVal);
}

void mt_set_pwm_send_data0_hal(u32 pwm_no, u32 data)
{
	unsigned long reg_data0;

	reg_data0 = PWM_register[pwm_no] + 4 * PWM_SEND_DATA0;
	OUTREG32(reg_data0, data);
}

void mt_set_pwm_send_data1_hal(u32 pwm_no, u32 data)
{
	unsigned long reg_data1;

	reg_data1 = PWM_register[pwm_no] + 4 * PWM_SEND_DATA1;
	OUTREG32(reg_data1, data);
}

void mt_set_pwm_wave_num_hal(u32 pwm_no, uint16_t num)
{
	unsigned long reg_wave_num;

	reg_wave_num = PWM_register[pwm_no] + 4 * PWM_WAVE_NUM;
	OUTREG32(reg_wave_num, num);
}

void mt_set_pwm_data_width_hal(u32 pwm_no, uint16_t width)
{
	unsigned long reg_data_width;

	reg_data_width = PWM_register[pwm_no] + 4 * PWM_DATA_WIDTH;
	OUTREG32(reg_data_width, width);
}

void mt_set_pwm_thresh_hal(u32 pwm_no, uint16_t thresh)
{
	unsigned long reg_thresh;

	reg_thresh = PWM_register[pwm_no] + 4 * PWM_THRESH;
	OUTREG32(reg_thresh, thresh);
}

s32 mt_get_pwm_send_wavenum_hal(u32 pwm_no)
{
	unsigned long reg_send_wavenum;

	reg_send_wavenum = PWM_register[pwm_no] + 4 * PWM_SEND_WAVENUM;
	return INREG32(reg_send_wavenum);
}

void mt_set_intr_enable_hal(u32 pwm_intr_enable_bit)
{
	SETREG32(PWM_INT_ENABLE, 1 << pwm_intr_enable_bit);
}

s32 mt_get_intr_status_hal(u32 pwm_intr_status_bit)
{
	int ret;

	ret = INREG32(PWM_INT_STATUS);
	ret = (ret >> pwm_intr_status_bit) & 0x01;
	return ret;
}

void mt_set_intr_ack_hal(u32 pwm_intr_ack_bit)
{
	SETREG32(PWM_INT_ACK, 1 << pwm_intr_ack_bit);
}

void mt_set_pwm_buf0_addr_hal(u32 pwm_no, u32 *addr)
{
	unsigned long reg_buff0_addr;

	reg_buff0_addr = PWM_register[pwm_no] + 4 * PWM_BUF0_BASE_ADDR;
	/*OUTREG32(reg_buff0_addr, addr);*/
	OUTREG32_DMA(reg_buff0_addr, addr);
}

void mt_set_pwm_buf0_size_hal(u32 pwm_no, uint16_t size)
{
	unsigned long reg_buff0_size;

	reg_buff0_size = PWM_register[pwm_no] + 4 * PWM_BUF0_SIZE;
	OUTREG32(reg_buff0_size, size);
}

void mt_pwm_dump_regs_hal(void)
{
	int i;
	unsigned long reg_val;

	reg_val = INREG32(PWM_ENABLE);
	PWMMSG("\r\n[PWM_ENABLE is:%lx]\n\r ", reg_val);
	reg_val = INREG32(PWM_CK_26M_SEL);
	PWMMSG("\r\n[PWM_26M_SEL is:%lx]\n\r ", reg_val);
	/*PWMDBG("peri pdn0 clock: 0x%x\n", INREG32(INFRA_PDN_STA0));*/

	for (i = PWM1; i < PWM_NUM; i++) {
		reg_val = INREG32(PWM_register[i] + 4 * PWM_CON);
		PWMMSG("\r\n[PWM%d_CON is:%lx]\r\n", i + 1, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_HDURATION);
		PWMMSG("[PWM%d_HDURATION is:%lx]\r\n", i + 1, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_LDURATION);
		PWMMSG("[PWM%d_LDURATION is:%lx]\r\n", i + 1, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_GDURATION);
		PWMMSG("[PWM%d_GDURATION is:%lx]\r\n", i + 1, reg_val);

		reg_val = INREG32(PWM_register[i] + 4 * PWM_BUF0_BASE_ADDR);
		PWMMSG("\r\n[PWM%d_BUF0_BASE_ADDR is:%lx]\r\n", i, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_BUF0_SIZE);
		PWMMSG("\r\n[PWM%d_BUF0_SIZE is:%lx]\r\n", i, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_BUF1_BASE_ADDR);
		PWMMSG("\r\n[PWM%d_BUF1_BASE_ADDR is:%lx]\r\n", i, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_BUF1_SIZE);
		PWMMSG("\r\n[PWM%d_BUF1_SIZE is:%lx]\r\n", i + 1, reg_val);

		reg_val = INREG32(PWM_register[i] + 4 * PWM_SEND_DATA0);
		PWMMSG("[PWM%d_SEND_DATA0 is:%lx]\r\n", i + 1, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_SEND_DATA1);
		PWMMSG("[PWM%d_PWM_SEND_DATA1 is:%lx]\r\n", i + 1, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_WAVE_NUM);
		PWMMSG("[PWM%d_WAVE_NUM is:%lx]\r\n", i + 1, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_DATA_WIDTH);
		PWMMSG("[PWM%d_WIDTH is:%lx]\r\n", i + 1, reg_val);

		reg_val = INREG32(PWM_register[i] + 4 * PWM_THRESH);
		PWMMSG("[PWM%d_THRESH is:%lx]\r\n", i + 1, reg_val);
		reg_val = INREG32(PWM_register[i] + 4 * PWM_SEND_WAVENUM);
		PWMMSG("[PWM%d_SEND_WAVENUM is:%lx]\r\n", i + 1, reg_val);

	}
}

void pwm_debug_store_hal(void)
{
	/* dump clock status */
	/*PWMDBG("peri pdn0 clock: 0x%x\n", INREG32(INFRA_PDN_STA0));*/
}

void pwm_debug_show_hal(void)
{
	mt_pwm_dump_regs_hal();
}

/*----------3dLCM support-----------*/
/*
 base pwm2, select pwm3&4&5 same as pwm2 or inversion of pwm2
 */
void mt_set_pwm_3dlcm_enable_hal(u8 enable)
{
	SETREG32(PWM_3DLCM, 1 << PWM_3DLCM_ENABLE_OFFSET);
}

/*
 set "pwm_no" inversion of pwm base or not
 */
void mt_set_pwm_3dlcm_inv_hal(u32 pwm_no, u8 inv)
{
	/*set "pwm_no" as auxiliary first */
	SETREG32(PWM_3DLCM, 1 << (pwm_no + 16));

	if (inv)
		SETREG32(PWM_3DLCM, 1 << (pwm_no + 1));
	else
		CLRREG32(PWM_3DLCM, 1 << (pwm_no + 1));
}

void mt_set_pwm_3dlcm_base_hal(u32 pwm_no)
{
	CLRREG32(PWM_3DLCM, 0x7F << 8);
	SETREG32(PWM_3DLCM, 1 << (pwm_no + 8));
}

void mt_pwm_26M_clk_enable_hal(u32 enable)
{
	unsigned long reg_con;

	/* select 66M or 26M */
	reg_con = (unsigned long)PWM_CK_26M_SEL;
	if (enable)
		SETREG32(reg_con, 1 << PWM_CK_26M_SEL_OFFSET);
	else
		CLRREG32(reg_con, 1 << PWM_CK_26M_SEL_OFFSET);

}

#if !defined(CONFIG_MTK_CLKMGR)
int mt_get_pwm_clk_src(struct platform_device *pdev)
{
	int i;

	for (i = PWM1_CLK; i < PWM_CLK_NUM; i++) {
		pwm_clk[i] = devm_clk_get(&pdev->dev, pwm_clk_name[i]);
		pr_err("[PWM] get %s clock, %p\n", pwm_clk_name[i], pwm_clk[i]);
		if (IS_ERR(pwm_clk[i])) {
			PWMDBG("cannot get %s clock\n", pwm_clk_name[i]);
			return PTR_ERR(pwm_clk[i]);
		}
	}
	return 0;
}
#endif
