/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Filename: ap3xx6.c
 *
 * Summary:
 *	ap3xx6 sensor dirver.
 *
 * Modification History:
 * Date	By		Summary
 * -------- -------- -------------------------------------------------------
 * 05/11/12 YC		Original Creation (Test version:1.0)
 * 05/30/12 YC		Modify AP3216C_check_and_clear_intr return value and exchange
 *					AP3216C_get_ps_value return value to meet our spec.
 * 05/30/12 YC		Correct shift number in AP3216C_read_ps.
 * 05/30/12 YC		Correct ps data formula.
 * 05/31/12 YC		1. Change the reg in clear int function from low byte to high byte
 *						and modify the return value.
 *					2. Modify the eint_work function to filter als int.
 * 06/04/12 YC		Add PS high/low threshold instead of using the same value.
 * 07/12/12 YC		Add wakelock to prevent entering suspend when early suspending.
 *
 *
 *29/5/14	ansun modify code to mt6582 add ap3426
 *
 */

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include "cust_alsps.h"
#include "ap3xx.h"
#include "alsps.h"


/*-------------------------------flag---------------------------------------------*/
/*
#define APS_TAG					"[ALS/PS] "
#define APS_FUN(f)				printk(KERN_INFO APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)	printk(KERN_ERR APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)	printk(KERN_INFO APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)	printk(KERN_INFO APS_TAG fmt, ##args)
*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               pr_debug(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    pr_debug(APS_TAG fmt, ##args)

/*---------------------------user define-------------------------------------------------*/
//#define POWER_NONE_MACRO MT65XX_POWER_NONE



#define DELAYED_WORK 0	/*1*/
#define AP3426
#ifdef AP3426
#define DI_AUTO_CAL
#ifdef DI_AUTO_CAL
		#define DI_PS_CAL_THR 255
		u16 prev_cal_data = 0;
		
#endif
	#define AP3XX6_DEV_NAME	"AP3426"
#else
	#define AP3XX6_DEV_NAME	"AP3216"
#endif

/*----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
static struct i2c_client *ap3xx6_i2c_client;

static struct ap3xx6_priv *ap3xx6_obj;

/*----------------------------------------------------------------------------*/
static const struct i2c_device_id ap3xx6_i2c_id[] = {{AP3XX6_DEV_NAME, 0}, {} };
/*satic struct i2c_board_info i2c_ap3xx6 __initdata = { I2C_BOARD_INFO(AP3XX6_DEV_NAME, (0x3C >> 1))};*/

/*----------------------------------------------------------------------------*/
static int ap3xx6_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ap3xx6_i2c_remove(struct i2c_client *client);
static int ap3xx6_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
/*----------------------------------------------------------------------------*/
static int ap3xx6_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ap3xx6_i2c_resume(struct i2c_client *client);
static int ap3xx6_ps_get_data(int *value, int *status);
static int ap3xx6_als_get_data(int *value, int *status);

/*----------------------------------------------------------------------------*/

static struct wake_lock chrg_lock;

/*----------------------------------------------------------------------------*/
typedef enum {
	CMC_BIT_ALS	= 1,
	CMC_BIT_PS	= 2,

} CMC_BIT;
typedef enum {
	TRACE_DEBUG = 0x1,
	TRACE_ALS = 0x2,
	TRACE_PS = 0x4,
} TRACE_BIT;
/*----------------------------------------------------------------------------*/
struct ap3xx6_i2c_addr {	/*define a series of i2c slave address*/
	u8 write_addr;
	u8 ps_thd;	/*PS INT threshold*/
};
/*----------------------------------------------------------------------------*/
/* Maintain alsps cust info here */
struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
struct platform_device *alspsPltFmDev;
/* For alsp driver get cust info */
struct alsps_hw *get_cust_alsps(void)
{
	return &alsps_cust;
}
/*----------------------------------------------------------------------------*/

struct ap3xx6_priv {
	struct alsps_hw *hw;
	struct i2c_client *client;
#if DELAYED_WORK
	struct delayed_work eint_work;
#else
	struct work_struct eint_work;
#endif
	struct mutex lock;
	/*i2c address group*/
	struct ap3xx6_i2c_addr addr;
	struct device_node *irq_node;
	int irq;
	/*misc*/
	u16			als_modulus;
	atomic_t	i2c_retry;
	atomic_t	als_suspend;
	atomic_t	als_debounce;	/*debounce time after enabling als*/
	atomic_t	als_deb_on;	/*indicates if the debounce is on*/
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_mask;		/*mask ps: always return far away*/
	atomic_t	ps_debounce;	/*debounce time after enabling ps*/
	atomic_t	ps_deb_on;		/*indicates if the debounce is on*/
	atomic_t	ps_deb_end;	/*the jiffies representing the end of debounce*/
	atomic_t	ps_suspend;
	atomic_t	trace;

	/*data*/
	u16		als;
	u16		ps;
	u8		_align;
	u16		als_level_num;
	u16		als_value_num;
	u32		als_level[C_CUST_ALS_LEVEL-1];
	u32		als_value[C_CUST_ALS_LEVEL];

	atomic_t	als_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_cmd_val;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_h;	/*the cmd value can't be read, stored in ram*/
	atomic_t	ps_thd_val_l;	/*the cmd value can't be read, stored in ram*/

	ulong		enable;		/*enable mask*/
	ulong		pending_intr;	/*pending interrupt*/

	/*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend	early_drv;
#endif
};
#ifdef CONFIG_OF
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif
/*----------------------------------------------------------------------------*/
static struct i2c_driver ap3xx6_i2c_driver = {
	.probe		= ap3xx6_i2c_probe,
	.remove	= ap3xx6_i2c_remove,
	.detect = ap3xx6_i2c_detect,
	.suspend	= ap3xx6_i2c_suspend,
	.resume	= ap3xx6_i2c_resume,
	.id_table	= ap3xx6_i2c_id,
	.driver = {
/* .owner			= THIS_MODULE, */
		.name			= AP3XX6_DEV_NAME,
#ifdef CONFIG_OF
		.of_match_table = alsps_of_match,
#endif
	},
};

static int ap3xx6_local_init(void);
static int ap3xx6_remove(void);

static int ap3xx6_init_flag = -1; /* 0<==>OK -1 <==> fail */

static struct alsps_init_info ap3xx6_init_info = {
	.name = "ap3xx6",
	.init = ap3xx6_local_init,
	.uninit = ap3xx6_remove,
};

static int ap3xx6_read_reg(struct i2c_client *client,
		char reg, u8 mask, u8 shift)
{
	int ret = 0;
	char tmp[1];
	tmp[0] = reg;
	mutex_lock(&ap3xx6_obj->lock);

	ret = i2c_master_send(client, tmp, 0x01);
	if (ret <= 0) {
		APS_ERR("ap3xx6_read_reg 1 ret=%x\n", ret);
		goto EXIT_ERR;
	}
	ret = i2c_master_recv(client, tmp, 0x01);
	if (ret <= 0) {
		APS_ERR("ap3xx6_read_reg 2 ret=%d\n", ret);
		goto EXIT_ERR;
	}

	mutex_unlock(&ap3xx6_obj->lock);
	return (tmp[0] & mask) >> shift;

EXIT_ERR:
		APS_ERR("ap3xx6_read_reg fail\n");
	mutex_unlock(&ap3xx6_obj->lock);
		return ret;
}

static int ap3xx6_write_reg(struct i2c_client *client,
		char reg, u8 mask, u8 shift, u8 val)
{
	int ret = 0x00;
	char tmp[2];

	mutex_lock(&ap3xx6_obj->lock);

	tmp[0] = reg;
	tmp[1] = val;
	ret = i2c_master_send(client, tmp, 0x02);
	if (ret <= 0) {
		APS_ERR("ap3xx6_write_reg ret=%d\n", ret);
		goto EXIT_ERR;
	}

	mutex_unlock(&ap3xx6_obj->lock);
	return ret;

EXIT_ERR:
		APS_ERR("ap3xx6_write_reg fail\n");
	mutex_unlock(&ap3xx6_obj->lock);
		return ret;
}
/*----------------------------------------------------------------------------*/
int ap3xx6_get_addr(struct alsps_hw *hw, struct ap3xx6_i2c_addr *addr)
{
	if (!hw || !addr) {
		return -EFAULT;
	}
	addr->write_addr = hw->i2c_addr[0];
	return 0;
}
/*----------------------------------------------------------------------------*/
static void ap3xx6_power(struct alsps_hw *hw, unsigned int on)
{
	static unsigned int power_on;
#if 0
#ifdef __USE_LINUX_REGULATOR_FRAMEWORK__

#else
	if (hw->power_id != POWER_NONE_MACRO) {
		if (power_on == on) {
			APS_LOG("ignore power control: %d\n", on);
		} else if (on) {
			if (!hwPowerOn(hw->power_id, hw->power_vol, AP3XX6_DEV_NAME)) {
				APS_ERR("power on fails!!\n");
			}
		} else{
			if (!hwPowerDown(hw->power_id, AP3XX6_DEV_NAME)) {
				APS_ERR("power off fail!!\n");
			}
		}
	}
#endif
#endif
	power_on = on;
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_enable_als(struct i2c_client *client, int enable)
{
		struct ap3xx6_priv *obj = i2c_get_clientdata(client);
		u8 databuf[2];
		int res = 0;
		u8 buffer[1];
		int reg_value[1];

		if (client == NULL) {
			APS_DBG("CLIENT CANN'T EQUAL NULL\n");
			return -1;
		}

		buffer[0] = AP3xx6_ENABLE;
		reg_value[0] = ap3xx6_read_reg(client, buffer[0], 0xFF, 0x00);
		if (res < 0) {
			goto EXIT_ERR;
		}

		if (enable) {
			databuf[0] = AP3xx6_ENABLE;
			databuf[1] = reg_value[0] | 0x01;
			res = ap3xx6_write_reg(client, databuf[0], 0xFF, 0x00, databuf[1]);
			if (res <= 0) {
				goto EXIT_ERR;
			}
			atomic_set(&obj->als_deb_on, 1);
			atomic_set(&obj->als_deb_end, jiffies+atomic_read(&obj->als_debounce)/(1000/HZ));
			APS_DBG("ap3xx6_ ALS enable\n");
		#if DELAYED_WORK
			schedule_delayed_work(&obj->eint_work, 1100*HZ/1000);
		#endif
		} else{
			databuf[0] = AP3xx6_ENABLE;
			databuf[1] = reg_value[0] & 0xFE;
			res = ap3xx6_write_reg(client, databuf[0], 0xFF, 0x00, databuf[1]);
			if (res <= 0) {
				goto EXIT_ERR;
			}
			atomic_set(&obj->als_deb_on, 0);
			APS_DBG("ap3xx6_ ALS disable\n");
		}
		msleep(30);
		return 0;

EXIT_ERR:
		APS_ERR("ap3xx6__enable_als fail\n");
		return res;
}

/*----------------------------------------------------------------------------*/
static int ap3xx6_enable_ps(struct i2c_client *client, int enable)
{
	struct ap3xx6_priv *obj = i2c_get_clientdata(client);
	u8 databuf[2];
	int res = 0;
	u8 buffer[1];
	u8 reg_value[1];

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL\n");
		return -1;
	}


	buffer[0] = AP3xx6_ENABLE;
	reg_value[0] = ap3xx6_read_reg(client, buffer[0], 0xFF, 0x00);
	if (res < 0) {
		goto EXIT_ERR;
	}

	if (enable) {
		databuf[0] = AP3xx6_ENABLE;
		databuf[1] = reg_value[0] | 0x02;
		res = ap3xx6_write_reg(client, databuf[0], 0xFF, 0x00, databuf[1]);
		if (res <= 0) {
			goto EXIT_ERR;
		}
		if (0 == obj->hw->polling_mode_ps) 
		{
		
		} else{
			wake_lock(&chrg_lock);
			atomic_set(&obj->ps_deb_on, 1);
			atomic_set(&obj->ps_deb_end, jiffies+atomic_read(&obj->ps_debounce)/(1000/HZ));
		}

	#if DELAYED_WORK
		schedule_delayed_work(&obj->eint_work, 110*HZ/1000);
	#endif
		APS_DBG("ap3xx6_ PS enable\n");
	} else{
		databuf[0] = AP3xx6_ENABLE;
		databuf[1] = reg_value[0] & 0xfd;
		res = ap3xx6_write_reg(client, databuf[0], 0xFF, 0x00, databuf[1]);
		if (res <= 0) {
			goto EXIT_ERR;
		}
		atomic_set(&obj->ps_deb_on, 0);
		APS_DBG("ap3xx6_ PS disable\n");

		if (0 == obj->hw->polling_mode_ps) {
	#if (!DELAYED_WORK)
			cancel_work_sync(&obj->eint_work);
	#endif
		} else{
			wake_unlock(&chrg_lock);
		}
	}
	msleep(30);
	return 0;

EXIT_ERR:
	APS_ERR("ap3xx6__enable_ps fail\n");
	return res;
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_check_and_clear_intr(struct i2c_client *client)
{
/*	struct ap3xx6_priv *obj = i2c_get_clientdata(client); */
	int res;
	u8 ints[1];

	/* Get Int status */
	ints[0] = ap3xx6_read_reg(client, AP3xx6_INT_STATUS, 0xFF, 0x00);
	if (ints[0] < 0) {
		goto EXIT_ERR;
	}

	/* Clear ALS int flag */
	res = ap3xx6_read_reg(client, AP3xx6_ADATA_H, 0xFF, 0x00);
	if (res < 0) {
		goto EXIT_ERR;
	}

	/* Clear PS int flag */
	res = ap3xx6_read_reg(client, AP3xx6_PDATA_H, 0xFF, 0x00);
	if (res < 0) {
		goto EXIT_ERR;
	}

	return ints[0];

EXIT_ERR:
	APS_ERR("ap3xx6_check_and_clear_intr fail\n");
	return -1;
}
/* als */
static int ap3xx6_set_ALSGain(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3426
/*val=0x00~0x3F*/
		re_val = val << 4;
	err = ap3xx6_write_reg(client, 0x10,
		0xFF, 0x00, re_val);
#else
/*val=0x00~0xF*/
	re_val = ap3xx6_read_reg(client, 0x20, 0xFF, 0x00);
	re_val = (re_val&0xF)|(val << 4);
#endif



	return err;
}

/* ps */
static int ap3xx6_set_plthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

#ifdef AP3426
	msb = val >> 8;
	lsb = val & 0xFF;
#else
	msb = val >> 2;
	lsb = val & 0x03;
#endif

	err = ap3xx6_write_reg(client, 0x2A,
			0xFF, 0x00, lsb);
	if (err <= 0)
		return err;

	err = ap3xx6_write_reg(client, 0x2B,
			0xFF, 0x00, msb);

	return err;
}

static int ap3xx6_set_phthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

#ifdef AP3426
	msb = val >> 8;
	lsb = val & 0xFF;
#else
	msb = val >> 2;
	lsb = val & 0x03;
#endif

	err = ap3xx6_write_reg(client, 0x2C,
			0xFF, 0x00, lsb);
	if (err <= 0)
		return err;

	err = ap3xx6_write_reg(client, 0x2D,
			0xFF, 0x00, msb);

	return err;
}

static int ap3xx6_set_pcrosstalk(struct i2c_client *client, int val)
{
	int lsb, msb, err;
	
#ifdef AP3426
	msb = val >> 8;
	lsb = val & 0xFF;
#else
	msb = val >> 1;
	lsb = val & 0x01;
#endif
	err = ap3xx6_write_reg(client, 0x28,
		0xFF, 0x00, lsb);
	if (err <= 0)
		return err;
	err = ap3xx6_write_reg(client, 0x29,
		0xFF, 0x00, msb);

	return err;
}

static int ap3xx6_set_PSTtime(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3426
/*val=0x00~0x3F*/
	re_val = val&0x3F;
	err = ap3xx6_write_reg(client, 0x25, 0xFF, 0x00, re_val);
#else
/*val=0x00~0xF*/
	re_val = ap3xx6_read_reg(client, 0x20, 0xFF, 0x00);
	re_val = (re_val&0xF)|(val << 4);
#endif



	return err;
}

/*val=0x00~0x03*/
static int ap3xx6_set_PSgain(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3426
	re_val = val << 2;
	err = ap3xx6_write_reg(client, 0x20, 0xFF, 0x00, re_val);
#else
	re_val = ap3xx6_read_reg(client, 0x20, 0xFF, 0x00);
	re_val = (re_val&0xF3)|(val << 2);
#endif



	return err;
}
#if 0
static int ap3xx6_set_PSTPersistence(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3426
/*val=0x00~0x3f*/
		re_val = val&0x3F;
	err = ap3xx6_write_reg(client, 0x26,
		0xFF, 0x00, re_val);
#else
/*val=0x00~0x03*/
	re_val = ap3xx6_read_reg(client, 0x20, 0xFF, 0x00);
	re_val = (re_val&0xFC)|val;
#endif



	return err;
}
#endif



static int ap3xx6_set_PSpulse(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3426
	re_val = ap3xx6_read_reg(client, 0x21, 0xFF, 0x00);
	re_val = ((re_val&0xFC)|val);


	err = ap3xx6_write_reg(client, 0x21,
		0xFF, 0x00, re_val);
#else
/*val=0x00~0x03*/
	re_val = ap3xx6_read_reg(client, 0x21, 0xFF, 0x00);
	re_val = (re_val&0xCF)|(val<<4);


	err = ap3xx6_write_reg(client, 0x21,
		0xFF, 0x00, re_val);
#endif

	return err;
}

#if 0
static int ap3xx6_set_intform(struct i2c_client *client, int val)
{
	int re_val, err;

	re_val = val&0x1;
	err = ap3xx6_write_reg(client, 0x22,
		0xFF, 0x00, re_val);

	return err;
}
#endif

static int ap3xx6_set_meantime(struct i2c_client *client, int val)
{
	int re_val, err;
/*val=0x00~0x03*/
	re_val = val&0x3;
	err = ap3xx6_write_reg(client, 0x23,
		0xFF, 0x00, re_val);

	return err;
}

#if 0
static int ap3xx6_set_waittime(struct i2c_client *client, int val)
{
	int re_val, err;

#ifdef AP3426
/*val=0x00~0xff*/
	re_val = val&0xFF;
	err = ap3xx6_write_reg(client, 0x06,
		0xFF, 0x00, re_val);
#else
/*val=0x00~0x3f*/
	re_val = val&0x3F;
	err = ap3xx6_write_reg(client, 0x24, 0xFF, 0x00, re_val);
#endif

	return err;
}
#endif
/*----------------------------------------------------------------------------*/

void ap3xx6_eint_func(void)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	if (unlikely(obj == NULL)) {
		APS_ERR("%s--%d ap3xx6_obj is NULL!\n", __func__, __LINE__);
		return;
	}

	if (atomic_read(&obj->trace) & TRACE_DEBUG) {
		APS_LOG("%s--%d\n", __func__, __LINE__);
	}

#if DELAYED_WORK
	schedule_delayed_work(&obj->eint_work, 0);
#else
	schedule_work(&obj->eint_work);
#endif
}
#if defined(CONFIG_OF)
static irqreturn_t ap3xx6_eint_handler(int irq, void *desc)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	if (unlikely(obj == NULL)) {
		APS_ERR("%s--%d ap3xx6_obj is NULL!\n", __func__, __LINE__);
		return IRQ_HANDLED;
	}

	if (atomic_read(&obj->trace) & TRACE_DEBUG) {
		APS_LOG("%s--%d\n", __func__, __LINE__);
	}

	disable_irq_nosync(obj->irq);
	ap3xx6_eint_func();

	return IRQ_HANDLED;
}

#endif

/*----------------------------------------------------------------------------*/
/* This function depends the real hw setting, customers should modify it. 2012/5/10 YC. */
int ap3xx6_setup_eint(struct i2c_client *client)
{
#if defined(CONFIG_OF)
	u32 ints[2] = {0};
	int err = 0;
	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
#endif

	if (!ap3xx6_obj) {
		APS_ERR("ap3xx6_obj is null!!\n");
		return 0;
	}

#if defined(CONFIG_OF)
	alspsPltFmDev = get_alsps_platformdev();
	
	/* gpio setting */
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
		return ret;
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");
	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");
		return ret;
	}
#else
	mt_set_gpio_dir(GPIO_ALS_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_ALS_EINT_PIN, GPIO_ALS_EINT_PIN_M_EINT);
	mt_set_gpio_pull_enable(GPIO_ALS_EINT_PIN, GPIO_PULL_ENABLE);
	mt_set_gpio_pull_select(GPIO_ALS_EINT_PIN, GPIO_PULL_UP);
#endif
	

#if defined(CONFIG_OF)
	if (ap3xx6_obj->irq_node) {
		of_property_read_u32_array(ap3xx6_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		
		gpio_request(ints[0], "p-sensor");	
		gpio_set_debounce(ints[0], ints[1]);
		pinctrl_select_state(pinctrl, pins_cfg);
		APS_LOG("ints[0]=%d, ints[1]=%d\n", ints[0], ints[1]);
		

		ap3xx6_obj->irq = irq_of_parse_and_map(ap3xx6_obj->irq_node, 0);
		APS_LOG("ap3xx6_obj->irq = %d\n", ap3xx6_obj->irq);
		if (!ap3xx6_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		err = request_irq(ap3xx6_obj->irq, ap3xx6_eint_handler, IRQF_TRIGGER_NONE, "ALS-eint", NULL);

		if (err != 0) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		enable_irq(ap3xx6_obj->irq);
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}
#else
	mt_eint_set_hw_debounce(CUST_EINT_ALS_NUM, CUST_EINT_ALS_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_ALS_NUM, CUST_EINT_ALS_TYPE, ap3xx6_eint_func, 0);

	mt_eint_mask(CUST_EINT_ALS_NUM);
#endif
	return 0;
}

char *apdev_name = AP3XX6_DEV_NAME;
/*----------------------------------------------------------------------------*/
static int ap3xx6_init_client(struct i2c_client *client)
{
	struct ap3xx6_priv *obj = i2c_get_clientdata(client);
	u8 databuf[2];
	int res = 0;

	databuf[0] = AP3xx6_ENABLE;
	databuf[1] = 0x00;
	res = ap3xx6_write_reg(client, databuf[0], 0xFF, 0x00, databuf[1]);
	if (res <= 0) {
		goto EXIT_ERR;
	}
	msleep(50);
	
	ap3xx6_set_plthres(client, atomic_read(&obj->ps_thd_val_l));
	ap3xx6_set_phthres(client, atomic_read(&obj->ps_thd_val_h));
	ap3xx6_set_ALSGain(client, 0x02);
	ap3xx6_set_PSTtime(client, 0x08);
	ap3xx6_set_PSgain(client, 0x00);
	ap3xx6_set_PSpulse(client, 0x03);
	ap3xx6_set_meantime(client, 0x02);
	ap3xx6_set_pcrosstalk(client,0x00); 

	return AP3xx6_SUCCESS;

EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;
}



/******************************************************************************
 * Function Configuration
******************************************************************************/
int ap3xx6_read_als(struct i2c_client *client, u16 *data)
{

	/*struct ap3xx6_priv *obj = i2c_get_clientdata(client);	*/
	u8 als_value_low[1], als_value_high[1];

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL\n");
		return -1;
	}
/***************for test crostalk   bengin  2015.05.14************
	als_value_low[0] = ap3xx6_read_reg(client, 0x28, 0xFF, 0x00);
	if (als_value_low[0] < 0) {
		goto EXIT_ERR;
	}

	als_value_high[0] = ap3xx6_read_reg(client, 0x29, 0xFF, 0x00);
	if (als_value_high[0] < 0) {
		goto EXIT_ERR;
	}

	*data = als_value_low[0] | (als_value_high[0]<<8);
	printk("--------------Sam Calibrtion Value = %d\n",*data);

	als_value_low[0] = ap3xx6_read_reg(client, AP3xx6_PDATA_L, 0xFF, 0x00);
	if (als_value_low[0] < 0) {
		goto EXIT_ERR;
	}

	als_value_high[0] = ap3xx6_read_reg(client, AP3xx6_PDATA_H, 0xFF, 0x00);
	if (als_value_high[0] < 0) {
		goto EXIT_ERR;
	}

	*data = als_value_low[0] | (als_value_high[0]<<8);
	printk("--------------Sam PS Value = %d\n",*data);
**************for test crostalk  end  2015.05.14******************/

	/* get ALS adc count */
	als_value_low[0] = ap3xx6_read_reg(client, AP3xx6_ADATA_L, 0xFF, 0x00);
	if (als_value_low[0] < 0) {
		goto EXIT_ERR;
	}

	als_value_high[0] = ap3xx6_read_reg(client, AP3xx6_ADATA_H, 0xFF, 0x00);
	if (als_value_high[0] < 0) {
		goto EXIT_ERR;
	}

	*data = als_value_low[0] | (als_value_high[0]<<8);
	printk("--------------kevin first data = %d\n",*data);
	if (*data < 0) {
		*data = 0;
		APS_DBG("als_value is invalid!!\n");
		goto EXIT_ERR;
	}
//		*data = (*data / 8)*8;   //kevindang20150513 modifey for als not report
	return 0;

EXIT_ERR:
	APS_ERR("ap3xx6__read_als fail\n");
	return -1;
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_get_als_value(struct ap3xx6_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;
	for (idx = 0; idx < obj->als_level_num; idx++) {
		if (als < obj->hw->als_level[idx]) {
			break;
		}
	}

	if (idx >= obj->als_value_num) {
		APS_ERR("exceed range\n");
		idx = obj->als_value_num - 1;
	}

	if (1 == atomic_read(&obj->als_deb_on)) {
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if (time_after(jiffies, endt)) {
			atomic_set(&obj->als_deb_on, 0);
		}

		if (1 == atomic_read(&obj->als_deb_on)) {
			//invalid = 1;
		}
	}

	if (!invalid) {
		if (atomic_read(&obj->trace) & TRACE_DEBUG) {
			APS_DBG("ALS: %05d => %05d\n", als, obj->hw->als_value[idx]);
		}
		return obj->hw->als_value[idx];
	} else{
		APS_ERR("ALS: %05d => %05d (-1)\n", als, obj->hw->als_value[idx]);
		return -1;
	}
}
/*----------------------------------------------------------------------------*/
int ap3xx6_read_ps(struct i2c_client *client, u16 *data)
{
	/*struct ap3xx6_priv *obj = i2c_get_clientdata(client); */
	u8 ps_value_low[1], ps_value_high[1];

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL\n");
		return -1;
	}
	ps_value_low[0] = ap3xx6_read_reg(client, AP3xx6_PDATA_L, 0xFF, 0x00);
	if (ps_value_low[0] < 0) {
		goto EXIT_ERR;
	}

	ps_value_high[0] = ap3xx6_read_reg(client, AP3xx6_PDATA_H, 0xFF, 0x00);
	if (ps_value_high[0] < 0) {
		goto EXIT_ERR;
	}

#ifdef AP3426
	*data = (ps_value_low[0] & 0xFF) | ((ps_value_high[0] & 0x03) << 8);
#else
	*data = (ps_value_low[0] & 0x0f) | ((ps_value_high[0] & 0x3f) << 4);
#endif

	return 0;

EXIT_ERR:
	APS_ERR("ap3xx6_read_ps fail\n");
	return -1;
}
/*----------------------------------------------------------------------------*/
/*
	for ap3xx6_get_ps_value:
	return 1 = object close,
	return 0 = object far away. 2012/5/10 YC	// exchange 0 and 1 2012/5/30 YC
*/
static int ap3xx6_get_ps_value(struct ap3xx6_priv *obj, u16 ps)
{
	int val=1;
	int invalid = 0;

	if (ps > atomic_read(&obj->ps_thd_val_h))
		val = 0;	/*close*/
	else if (ps < atomic_read(&obj->ps_thd_val_l))
		val = 1;	/*far away*/

	if (atomic_read(&obj->ps_suspend)) {
		invalid = 1;
	} else if (1 == atomic_read(&obj->ps_deb_on)) {
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if (time_after(jiffies, endt)) {
			atomic_set(&obj->ps_deb_on, 0);
		}

		if (1 == atomic_read(&obj->ps_deb_on)) {
			invalid = 1;
		}
	}

	if (!invalid) {
		if (atomic_read(&obj->trace) & TRACE_DEBUG) {
			APS_DBG("PS: %05d => %05d\n", ps, val);
		}
		return val;
	} else{
		return -1;
	}
}

static int ap3xx6_get_OBJ(struct i2c_client *client)
{

	u8 ps_value_high[1];

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUAL NULL\n");
		return -1;
	}
#ifdef AP3426
	ps_value_high[0] = ap3xx6_read_reg(client, AP3xx6_INT_STATUS, 0xFF, 0x00);
	if (ps_value_high[0] < 0) {
		goto EXIT_ERR;
	}
	return !((ps_value_high[0]&0x10)>>4);
#else
	ps_value_high[0] = ap3xx6_read_reg(client, AP3xx6_PDATA_H, 0xFF, 0x00);
	if (ps_value_high[0] < 0) {
		goto EXIT_ERR;
	}
	/* APS_LOG("the ps_value_h>>7 is %d\n",ps_value_high[0]>>7); */
	return !(ps_value_high[0]>>7);
#endif

EXIT_ERR:
	APS_ERR("ap3xx6_get_obj fail\n");
	return 0;
}

#ifdef DI_AUTO_CAL
u8 Calibration_Flag = 0;
int ap3xx6_Calibration(struct i2c_client *client)
{
	int err;
	int i = 0;
	u16 ps_data = 0;
	/* struct i2c_client *client = (struct i2c_client*)file->private_data; */
	struct ap3xx6_priv *obj = i2c_get_clientdata(client);
	if (atomic_read(&obj->trace) & TRACE_DEBUG) {
		APS_LOG("AP3426 C_F =%d\n", Calibration_Flag);
	}
	if (Calibration_Flag == 0) {
		for (i = 0; i < 4; i++) {
			err = ap3xx6_read_ps(obj->client, &obj->ps);
			if (err != 0) {
				goto err_out;
			}
			if (atomic_read(&obj->trace) & TRACE_DEBUG) {
				APS_LOG("AP3426 ps =%d\n", obj->ps);
			}
			if ((obj->ps) > DI_PS_CAL_THR) {
				Calibration_Flag = 0;
				goto err_out;
			} else{
				ps_data += obj->ps;
			}
			msleep(100);
		}
		Calibration_Flag = 1;
		if (atomic_read(&obj->trace) & TRACE_DEBUG) {
			APS_LOG("AP3426 ps_data1 =%d\n", ps_data);
		}

		ps_data = ps_data/4;

		if (atomic_read(&obj->trace) & TRACE_DEBUG) {
			APS_LOG("AP3426 ps_data2 =%d\n", ps_data);
		}
		ap3xx6_set_pcrosstalk(obj->client, ps_data);
		prev_cal_data =ps_data;
	}
	return 1;
err_out:
	APS_ERR("ap3xx6_read_ps fail\n");
	return -1;
}

static int ap3xx6_Calibration_every_time(struct i2c_client *client)
{
	int err;
	int i = 0;
	u16 ui_ps_data = 0;
	u16 ps_als_status = 0;
	/* struct i2c_client *client = (struct i2c_client*)file->private_data; */
	struct ap3xx6_priv *obj = i2c_get_clientdata(client);

	u16 ps_crosstalk[5] = {0,0,0,0,0};
	u16 ps_data_avg = 0;
	

	ps_als_status = ap3xx6_read_reg(client, AP3xx6_ENABLE, 0xFF, 0x00);
	if (ps_als_status < 0) 
	{
		goto err_out;
	}

	err = ap3xx6_write_reg(client, AP3xx6_ENABLE, 0xFF, 0x00, 0x00);
	if (err<= 0) {
		goto err_out;
	}
	msleep(100);
	
	ap3xx6_set_pcrosstalk(client, 0);
	ap3xx6_enable_ps(client, 1);
	
	for (i = 0; i < 5; i++) 
	{
		msleep(30);
		err = ap3xx6_read_ps(obj->client, &obj->ps);
		if (err != 0) 
		{
			goto err_out;
		}
		if (atomic_read(&obj->trace) & TRACE_DEBUG) 
		{
			APS_LOG("AP3426 ps =%d\n", obj->ps);
		}
		
		ps_crosstalk[i] = obj->ps;
	}
	
	for(i = 1;i < 5;i ++)
	{
		ui_ps_data += ps_crosstalk[i];
	}

	if (atomic_read(&obj->trace) & TRACE_DEBUG) 
	{
		APS_LOG("AP3426 ps_data1 =%d\n", ui_ps_data);
	}
	ps_data_avg = ui_ps_data/4;

	if (atomic_read(&obj->trace) & TRACE_DEBUG) 
	{
		APS_LOG("AP3426 ps_data2 =%d\n", ps_data_avg);
	}

	if(ps_data_avg > 511)
	{
		ap3xx6_set_pcrosstalk(obj->client, 511);
	}
	else
	{
		ap3xx6_set_pcrosstalk(obj->client, ps_data_avg);
	}
	msleep(70);

	if(ps_als_status & 0x01)
	{
		ap3xx6_enable_als(client,1);
	}
	
	return 1;

err_out:
	if(ps_als_status & 0x01)
	{
		ap3xx6_enable_als(client,1);
	}
	
	APS_ERR("ap3xx6_read_ps fail\n");
	return -1;
}


static int ap3xx6_enable_ps_interrupt(struct i2c_client *client, int enable)
{
//	struct ap3xx6_priv *obj = i2c_get_clientdata(client);
	u8 buffer[1];
	int res = 0;
	u8 reg_value[1];
	u8 databuf[2];

	if (client == NULL) 
	{
		APS_DBG("CLIENT CANN'T EQUAL NULL\n");
		return -1;
	}

	buffer[0] = AP3xx6_INT_CTL;
	reg_value[0] = ap3xx6_read_reg(client, buffer[0], 0xFF, 0x00);
	if (res < 0) 
	{
		goto EXIT_ERR;
	}

	if(enable)
	{
		databuf[0] = AP3xx6_INT_CTL;
		databuf[1] = reg_value[0] | 0x80;
		res = ap3xx6_write_reg(client, databuf[0], 0xFF, 0x00, databuf[1]);
		if (res <= 0) 
		{
			goto EXIT_ERR;
		}
	}
	else
	{
		databuf[0] = AP3xx6_INT_CTL;
		databuf[1] = reg_value[0] & 0x7f;
		res = ap3xx6_write_reg(client, databuf[0], 0xFF, 0x00, databuf[1]);
		if (res <= 0) 
		{
			goto EXIT_ERR;
		}
	}

	return 0;
	
EXIT_ERR:
	APS_ERR("ap3xx6_enable_ps_interrupt fail\n");
	return res;

	
}

#endif
/*----------------------------------------------------------------------------*/
static void ap3xx6_eint_work(struct work_struct *work)
{
	struct ap3xx6_priv *obj = (struct ap3xx6_priv *)container_of(work, struct ap3xx6_priv, eint_work);
	int err;
	int ps_value;
	//hwm_sensor_data sensor_data;

	if (atomic_read(&obj->trace) & TRACE_DEBUG) {
		APS_LOG("%s--%d\n", __func__, __LINE__);
	}

	err = ap3xx6_check_and_clear_intr(obj->client);
	//if ((err < 0) || err & 0x20)
	if(err < 0)
	{
		APS_ERR("ap3xx6_eint_work check intrs: %d\n", err);
	} else if (err & 0x01) {
		/* ALS interrupt. User should add their code here if they want to handle ALS Int. */
	} else{

	/* ap3xx6_read_ps(obj->client, &obj->ps); */
	/* sensor_data.values[0] = ap3xx6_get_ps_value(obj, obj->ps); */
	//	sensor_data.values[0] = ap3xx6_get_OBJ(obj->client);
	//	sensor_data.value_divide = 1;
	//	sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	
		ps_value = ap3xx6_get_OBJ(obj->client);
		err = ps_report_interrupt_data(ps_value);
		/* let up layer to know */
		/* if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data))) */
		if (err < 0) {
			APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
	}
#if defined(CONFIG_OF)
	enable_irq(obj->irq);
#else
	mt_eint_unmask(CUST_EINT_ALS_NUM);
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t ap3xx6_ps_rawdata_show(struct device_driver *ddri, char *buf)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int err = 0;
	u16 ps = -1;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}

	err = ap3xx6_read_ps(obj->client, &ps);
	if (err < 0) {
		APS_ERR("ap3xx6_read_ps failed\n");
		return -1;
	}
	return sprintf(buf, "ps raw dat= %d\n", ps);
}
static ssize_t ap3xx6_ps_data_show(struct device_driver *ddri, char *buf)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int err = 0;
	int value = -1;
	int status = -1;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}

	err = ap3xx6_ps_get_data(&value, &status);
	if (err < 0) {
		APS_ERR("ap3xx6_ps_get_data failed\n");
		return -1;
	}
	return sprintf(buf, "ps dat= %d,,status=%d\n", value, status);
}
static ssize_t ap3xx6_als_rawdata_show(struct device_driver *ddri, char *buf)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int err = 0;
	u16 als = -1;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}

	err = ap3xx6_read_als(obj->client, &als);
	if (err < 0) {
		APS_ERR("ap3xx6_read_als failed\n");
		return -1;
	}
	return sprintf(buf, "als raw dat= %d\n", als);
}
static ssize_t ap3xx6_als_data_show(struct device_driver *ddri, char *buf)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int err = 0;
	int value = -1;
	int status = -1;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}

	err = ap3xx6_als_get_data(&value, &status);
	if (err < 0) {
		APS_ERR("ap3xx6_als_get_data failed\n");
		return -1;
	}
	return sprintf(buf, "als dat= %d,,status=%d\n", value, status);
}
#if 0
static ssize_t ap3xx6_trace_show(struct device_driver *ddri, char *buf)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int ret = 0;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}
	ret = sprintf(buf, "obj->trace = %d,\n", atomic_read(&obj->trace));

	return ret;
}
static ssize_t ap3xx6_trace_store(struct device_driver *ddri, const char *buf, size_t count)
{

	int val = 0;
	int ret = 0;

	if (!ap3xx6_obj) {
		APS_ERR("ap3xx6_obj is null!!\n");
		return -1;
	}
	ret = sscanf(buf, "0x%x", &val);
	if (ret == 1) {
		atomic_set(&ap3xx6_obj->trace, val);
	}

	return count;
}
#endif
static ssize_t ap3xx6_status_show(struct device_driver *ddri, char *buf)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int ret = 0;
	//int value = -1;
	//int status = -1;
	int i = 0;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}
	ret += sprintf(buf+ret, "obj->irq = %d,\n", obj->irq);
#ifdef CUST_EINT_ALS_NUM
	ret += sprintf(buf+ret, "CUST_EINT_ALS_NUM = %d,\n", CUST_EINT_ALS_NUM);
#endif
	ret += sprintf(buf+ret, "als_level:");
	for (i = 0; i < sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]); i++) {
		ret += sprintf(buf+ret, "%d, ", obj->hw->als_level[i]);
	}
	ret += sprintf(buf+ret, "\n als_value:");
	for (i = 0; i < sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]); i++) {
		ret += sprintf(buf+ret, "%d, ", obj->hw->als_value[i]);
	}

	ret += sprintf(buf+ret, "\n ps_thd_val_h= %d,,ps_thd_val_l=%d, i2c_num = %d\n",
		atomic_read(&obj->ps_thd_val_h), atomic_read(&obj->ps_thd_val_l), obj->hw->i2c_num);
	return ret;
}

static ssize_t ap3xx6_em_read(struct device_driver *ddri, char *buf)
{
	u16 idx = 0;
	int count = 0;
	int reg_value[1];
#ifdef AP3426
		#define ap3xx6_NUM_CACHABLE_REGS	29
		u8 ap3xx6_reg[ap3xx6_NUM_CACHABLE_REGS] = {
		0x00, 0x01, 0x02, 0x06, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x14, 0x1a, 0x1b, 0x1c, 0x1d,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d};
#else
		#define ap3xx6_NUM_CACHABLE_REGS	26
		u8 ap3xx6_reg[ap3xx6_NUM_CACHABLE_REGS] = {
		0x00, 0x01, 0x02, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d};
#endif
	if (!ap3xx6_obj) {
		APS_ERR("ap3xx6_obj is null!!\n");
		count += sprintf(buf+count, "ap3xx6_obj is null!!\n");
		return count;
	}
	for (idx = 0; idx < ap3xx6_NUM_CACHABLE_REGS; idx++) {

			reg_value[0] = ap3xx6_read_reg(ap3xx6_obj->client, ap3xx6_reg[idx], 0xFF, 0x00);
		if (reg_value[0] < 0) {
			count += sprintf(buf+count, "i2c read_reg err\n");
			return count;
		}
		count += sprintf(buf+count, "[%x]=0x%x\n", ap3xx6_reg[idx], reg_value[0]);
	}
	ap3xx6_read_ps(ap3xx6_obj->client, &idx);
	count += sprintf(buf+count, "[ps]=%d\n", idx);
	return count;
}

static ssize_t ap3xx6_em_write(struct device_driver *ddri, const char *buf, size_t count)
{

	int addr, val;
	int ret = 0;

	if (!ap3xx6_obj) {
		APS_ERR("ap3xx6_obj is null!!\n");
		return -1;
	}

	ret = sscanf(buf, "%x %x", &addr, &val);

	APS_LOG("Reg[%x].Write [%x]..\n", addr, val);

	ret = ap3xx6_write_reg(ap3xx6_obj->client, addr, 0xFF, 0x00, val);

	return count;
}
//static DRIVER_ATTR(em,	S_IWUGO | S_IRUGO, ap3xx6_em_read, ap3xx6_em_write);
static DRIVER_ATTR(em,	S_IWUSR | S_IRUGO, ap3xx6_em_read, ap3xx6_em_write);
static DRIVER_ATTR(ps_rawdata,	S_IRUGO, ap3xx6_ps_rawdata_show, NULL);
static DRIVER_ATTR(ps_data,	S_IRUGO, ap3xx6_ps_data_show, NULL);
static DRIVER_ATTR(als_rawdata,	S_IRUGO, ap3xx6_als_rawdata_show, NULL);
static DRIVER_ATTR(als_data,	S_IRUGO, ap3xx6_als_data_show, NULL);
static DRIVER_ATTR(status,	S_IRUGO, ap3xx6_status_show, NULL);
//static DRIVER_ATTR(trace,	S_IRUGO | S_IWUGO, ap3xx6_trace_show, ap3xx6_trace_store);
//static DRIVER_ATTR(trace,	S_IWUSR | S_IWUGO, ap3xx6_trace_show, ap3xx6_trace_store);




static struct driver_attribute *ap3xx6_attr_list[] = {
	&driver_attr_em,
	&driver_attr_ps_rawdata,
	&driver_attr_ps_data,
	&driver_attr_als_rawdata,
	&driver_attr_als_data,
	&driver_attr_status,
	//&driver_attr_trace,
};

static int ap3xx6_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(ap3xx6_attr_list)/sizeof(ap3xx6_attr_list[0]));
	if (driver == NULL) {
		return -EINVAL;
	}

	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, ap3xx6_attr_list[idx]);
		if (err != 0) {
			APS_ERR("driver_create_file (%s) = %d\n", ap3xx6_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

/*----------------------------------------------------------------------------*/
static int ap3xx6_delete_attr(struct device_driver *driver)
{
	int idx , err = 0;
	int num = (int)(sizeof(ap3xx6_attr_list)/sizeof(ap3xx6_attr_list[0]));

	if (!driver)
		return -EINVAL;

	for (idx = 0; idx < num; idx++) {
		driver_remove_file(driver, ap3xx6_attr_list[idx]);
	}

	return err;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
static int ap3xx6_open(struct inode *inode, struct file *file)
{
	file->private_data = ap3xx6_i2c_client;

	if (!file->private_data) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}
/*----------------------------------------------------------------------------*/
static long ap3xx6_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct ap3xx6_priv *obj = i2c_get_clientdata(client);
	long err = 0;
	void __user *ptr = (void __user *) arg;
	int dat;
	uint32_t enable;

	switch (cmd) {
	case ALSPS_SET_PS_MODE:
			if (copy_from_user(&enable, ptr, sizeof(enable))) {
				err = -EFAULT;
				goto err_out;
			}
			if (enable) {
				err = ap3xx6_enable_ps(obj->client, 1);
				if (err != 0) {
					APS_ERR("enable ps fail: %ld\n", err);
					goto err_out;
				}
				msleep(100);

				set_bit(CMC_BIT_PS, &obj->enable);

			} else{
				err = ap3xx6_enable_ps(obj->client, 0);
				if (err != 0) {
					APS_ERR("disable ps fail: %ld\n", err);
					goto err_out;
				}


				clear_bit(CMC_BIT_PS, &obj->enable);

			}
			break;

	case ALSPS_GET_PS_MODE:

			enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);

			if (copy_to_user(ptr, &enable, sizeof(enable))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_GET_PS_DATA:
			err = ap3xx6_read_ps(obj->client, &obj->ps);
			if (err != 0) {
				goto err_out;
			}

			dat = ap3xx6_get_ps_value(obj, obj->ps);
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_GET_PS_RAW_DATA:
			err = ap3xx6_read_ps(obj->client, &obj->ps);
			if (err != 0) {
				goto err_out;
			}

			dat = obj->ps;
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_SET_ALS_MODE:
			if (copy_from_user(&enable, ptr, sizeof(enable))) {
				err = -EFAULT;
				goto err_out;
			}
			if (enable) {
				err = ap3xx6_enable_als(obj->client, 1);
				if (err != 0) {
					APS_ERR("enable als fail: %ld\n", err);
					goto err_out;
				}

				set_bit(CMC_BIT_ALS, &obj->enable);

			} else{
				err = ap3xx6_enable_als(obj->client, 0);
				if (err != 0) {
					APS_ERR("disable als fail: %ld\n", err);
					goto err_out;
				}

				clear_bit(CMC_BIT_ALS, &obj->enable);

			}
			break;

	case ALSPS_GET_ALS_MODE:

			enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);

			if (copy_to_user(ptr, &enable, sizeof(enable)))	{
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_GET_ALS_DATA:
			err = ap3xx6_read_als(obj->client, &obj->als);
			if (err != 0) {
				goto err_out;
			}

			dat = ap3xx6_get_als_value(obj, obj->als);
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	case ALSPS_GET_ALS_RAW_DATA:
			err = ap3xx6_read_als(obj->client, &obj->als);
			if (err != 0) {
				goto err_out;
			}

			dat = obj->als;
			if (copy_to_user(ptr, &dat, sizeof(dat))) {
				err = -EFAULT;
				goto err_out;
			}
			break;

	default:
			APS_ERR("%s not supported = 0x%04x", __func__, cmd);
			err = -ENOIOCTLCMD;
			break;
	}

err_out:
	return err;
}
#if 0
#ifdef CONFIG_COMPAT
static long ap3xx6_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long err = 0;
	void __user *arg32 = compat_ptr(arg);

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_ALSPS_SET_PS_MODE:
		err = file->f_op->unlocked_ioctl(file, ALSPS_SET_PS_MODE, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_SET_PS_MODE unlocked_ioctl failed.");
		}

		break;

	case COMPAT_ALSPS_GET_PS_MODE:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_PS_MODE, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_GET_PS_MODE unlocked_ioctl failed.");
		}

		break;

	case COMPAT_ALSPS_GET_PS_DATA:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_PS_DATA, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_GET_PS_DATA unlocked_ioctl failed.");
		}

		break;

	case COMPAT_ALSPS_GET_PS_RAW_DATA:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_PS_RAW_DATA, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_GET_PS_RAW_DATA unlocked_ioctl failed.");
		}

		break;

	case COMPAT_ALSPS_SET_ALS_MODE:
		err = file->f_op->unlocked_ioctl(file, ALSPS_SET_ALS_MODE, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_SET_ALS_MODE unlocked_ioctl failed.");
		}

		break;

	case COMPAT_ALSPS_GET_ALS_MODE:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_ALS_MODE, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_GET_ALS_MODE unlocked_ioctl failed.");
		}

		break;

	case COMPAT_ALSPS_GET_ALS_DATA:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_ALS_DATA, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_GET_ALS_DATA unlocked_ioctl failed.");
		}

		break;

	case COMPAT_ALSPS_GET_ALS_RAW_DATA:
		err = file->f_op->unlocked_ioctl(file, ALSPS_GET_ALS_RAW_DATA, (unsigned long)arg32);
		if (err) {
			APS_ERR("ALSPS_GET_ALS_RAW_DATA unlocked_ioctl failed.");
		}

		break;

	default:
		APS_ERR("%s not supported = 0x%04x", __func__, cmd);
		err = -ENOIOCTLCMD;
		break;

	}
	return err;
}
#endif
#endif
/*----------------------------------------------------------------------------*/
static const struct file_operations ap3xx6_fops = {
/* .owner = THIS_MODULE, */
	.open = ap3xx6_open,
	.release = ap3xx6_release,
	.unlocked_ioctl = ap3xx6_ioctl,
#if 0
#ifdef CONFIG_COMPAT
	.compat_ioctl = ap3xx6_compat_ioctl,
#endif
#endif
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ap3xx6_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ap3xx6_fops,
};
/*----------------------------------------------------------------------------*/
static int ap3xx6_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
	APS_FUN();
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_i2c_resume(struct i2c_client *client)
{
	APS_FUN();
	return 0;
}
static int ap3xx6_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, AP3XX6_DEV_NAME);
	return 0;

}
#if defined(CONFIG_HAS_EARLYSUSPEND)
/*----------------------------------------------------------------------------*/
static void ap3xx6_early_suspend(struct early_suspend *h)
{
	struct ap3xx6_priv *obj = container_of(h, struct ap3xx6_priv, early_drv);
	int err;
	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 1);

	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = ap3xx6_enable_als(obj->client, 0);
		if (err != 0) {
			APS_ERR("disable als fail: %d\n", err);
		}
	}

}
/*----------------------------------------------------------------------------*/
static void ap3xx6_late_resume(struct early_suspend *h)
{
	struct ap3xx6_priv *obj = container_of(h, struct ap3xx6_priv, early_drv);
	int err;
	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return;
	}

	atomic_set(&obj->als_suspend, 0);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = ap3xx6_enable_als(obj->client, 1);
		if (err != 0) {
			APS_ERR("enable als fail: %d\n", err);

		}
	}

}
#endif

static int ap3xx6_als_open_report_data(int open)
{

	return 0;
}

static int ap3xx6_als_enable_nodata(int en)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int value = 0;
	int err = 0;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}

	value = en;
	if (value) {
		err = ap3xx6_enable_als(obj->client, 1);
		if (err != 0) {
			APS_ERR("enable als fail: %d\n", err);
			return -1;
		}
		set_bit(CMC_BIT_ALS, &obj->enable);
	} else{
		err = ap3xx6_enable_als(obj->client, 0);
		if (err != 0) {
			APS_ERR("disable als fail: %d\n", err);
			return -1;
		}
		clear_bit(CMC_BIT_ALS, &obj->enable);
	}
	return 0;
}

static int ap3xx6_als_set_delay(u64 ns)
{
	return 0;
}

static int ap3xx6_als_get_data(int *value, int *status)
{
	static int temp_als;

	struct ap3xx6_priv *obj = ap3xx6_obj;
	int temp_value = -1;
	int err = 0;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}

	ap3xx6_read_als(obj->client, &obj->als);
	if (atomic_read(&obj->trace) & TRACE_DEBUG) {
		APS_LOG("ap3xx6 ALS level=%d\n", obj->als);
	}
#if 0
	if (obj->als == 0) 
	{
		temp_value = temp_als;
		printk("ap3xx6_als_get_data als_en= 0 temp_als= %d\n",temp_als);
	} 
	else
#endif
	{
		u16 b[2];
		int i;
		for (i = 0; i < 2; i++) {
			ap3xx6_read_als(obj->client, &obj->als);
			b[i] = obj->als;
		}
		(b[1] > b[0])?(obj->als = b[0]):(obj->als = b[1]);
		temp_value = ap3xx6_get_als_value(obj, obj->als);
		temp_als = temp_value;
	}

	*value = temp_value;
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return err;
}

static int ap3xx6_ps_open_report_data(int open)
{
	return 0;
}

static int ap3xx6_ps_enable_nodata(int en)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int value = 0;
	int err = 0;
	int ps_value = -1;
	int ps_status = 0;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}
	value = en;
	if (value) {
	
/*
		err = ap3xx6_enable_ps(obj->client, 1);
		if (err != 0) {
			APS_ERR("enable ps fail: %d\n", err);
			return -1;
		}
		msleep(100);
		set_bit(CMC_BIT_PS, &obj->enable);

#ifdef DI_AUTO_CAL
		ap3xx6_Calibration(obj->client);
#endif
*/

#ifdef DI_AUTO_CAL
		ap3xx6_Calibration_every_time(obj->client);	
		err = ap3xx6_enable_ps_interrupt(obj->client, 1);
		if (err != 0)
		{
			APS_ERR(" ap3xx6 enable ps interrupt fail: %d\n", err);
			return -1;
		}
		msleep(50);
#else
	err = ap3xx6_enable_ps(obj->client, 1);
		if (err != 0) {
			APS_ERR("enable ps fail: %d\n", err);
			return -1;
		}
#endif
		set_bit(CMC_BIT_PS, &obj->enable);
		
		err = ap3xx6_ps_get_data(&ps_value, &ps_status);
		if ((err == 0) && (ps_value > 0)) {
			ps_report_interrupt_data(ps_value);
		}
	} else{
		err = ap3xx6_enable_ps(obj->client, 0);
		if (err != 0) {
			APS_ERR("disable ps fail: %d\n", err);
			return -1;
		}

		clear_bit(CMC_BIT_PS, &obj->enable);
#ifdef DI_AUTO_CAL
		err = ap3xx6_enable_ps_interrupt(obj->client, 0);
		if (err != 0)
		{
			APS_ERR(" ap3xx6 disable ps interrupt fail: %d\n", err);
			return -1;
		}
#endif

	}

	return 0;
}

static int ap3xx6_ps_set_delay(u64 ns)
{
	return 0;
}

static int ap3xx6_ps_get_data(int *value, int *status)
{
	struct ap3xx6_priv *obj = ap3xx6_obj;
	int err = 0;

	if (obj == NULL) {
		APS_ERR("ap3xx6_obj is null\n");
		return -1;
	}

	err = ap3xx6_read_ps(obj->client, &obj->ps);
	if (err != 0) {
		APS_ERR("ap3xx6_read_ps failed err=%d\n", err);
		*value = -1;
		return err;
	}
	*value = ap3xx6_get_ps_value(obj, obj->ps);
	*status = SENSOR_STATUS_ACCURACY_MEDIUM;

	return 0;
}

/*----------------------------------------------------------------------------*/
static int ap3xx6_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ap3xx6_priv *obj = NULL;
	struct als_control_path als_ctl = {0};
	struct als_data_path als_data = {0};
	struct ps_control_path ps_ctl = {0};
	struct ps_data_path ps_data = {0};
	int err = 0;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (obj == NULL) {
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ap3xx6_obj = obj;

	mutex_init(&obj->lock);
	obj->hw = get_cust_alsps();
	ap3xx6_get_addr(obj->hw, &obj->addr);
#if DELAYED_WORK
	INIT_DELAYED_WORK(&obj->eint_work, ap3xx6_eint_work);
#else
	INIT_WORK(&obj->eint_work, ap3xx6_eint_work);
#endif
	obj->client = client;
	i2c_set_clientdata(client, obj);
	atomic_set(&obj->als_debounce, 300);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 300);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val, 0xC1);
	atomic_set(&obj->ps_thd_val_h, obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_l, obj->hw->ps_threshold_low);
	atomic_set(&obj->trace, 0);

	obj->enable = 0;

#if defined(CONFIG_OF)
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, als-eint");
#endif

	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level)/sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value)/sizeof(obj->hw->als_value[0]);
	obj->als_modulus = (400*100*40)/(1*1500);

	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);

	ap3xx6_i2c_client = client;

	err = ap3xx6_init_client(client);
	if (err != 0) {
		APS_ERR("ap3xx6_init_client() ERROR!\n");
		goto exit_init_failed;
	}
	APS_LOG("ap3xx6_init_client() OK!\n");

#ifdef DI_AUTO_CAL
		ap3xx6_enable_ps(client, 1);
		ap3xx6_Calibration(client);
		ap3xx6_enable_ps(client, 0);
#endif

	err = misc_register(&ap3xx6_device);
	if (err != 0) {
		APS_ERR("ap3xx6_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err = ap3xx6_create_attr(&ap3xx6_init_info.platform_diver_addr->driver);
	if (err != 0) {
		APS_ERR("create attribute err = %d\n", err);
		goto exit_create_attr_failed;
	}

	if (1 == obj->hw->polling_mode_ps) {
		ps_ctl.is_report_input_direct = false;
		ps_ctl.is_polling_mode = true;
		wake_lock_init(&chrg_lock, WAKE_LOCK_SUSPEND, "ap3xx6_wake_lock");
	} else{
		ps_ctl.is_report_input_direct = true;
		ps_ctl.is_polling_mode = false;

		err = ap3xx6_setup_eint(client);
		if (err != 0) {
			APS_ERR("setup eint: %d\n", err);
			goto exit_create_attr_failed;
		}
		err = ap3xx6_check_and_clear_intr(client);
		if (err < 0) {
			APS_ERR("check/clear intr: %d\n", err);
		}
	}

	ps_ctl.open_report_data = ap3xx6_ps_open_report_data;
	ps_ctl.enable_nodata = ap3xx6_ps_enable_nodata;
	ps_ctl.set_delay = ap3xx6_ps_set_delay;
	ps_ctl.is_support_batch = obj->hw->is_batch_supported_ps;
	ps_ctl.is_use_common_factory = false;

	err = ps_register_control_path(&ps_ctl);
	if (err != 0) {
		APS_ERR("ps_register_control_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}

	ps_data.get_data = ap3xx6_ps_get_data;
	ps_data.vender_div = 1;

	err = ps_register_data_path(&ps_data);

	if (err != 0) {
		APS_ERR("ps_register_data_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}

	als_ctl.open_report_data = ap3xx6_als_open_report_data;
	als_ctl.enable_nodata = ap3xx6_als_enable_nodata;
	als_ctl.set_delay = ap3xx6_als_set_delay;
	als_ctl.is_support_batch = obj->hw->is_batch_supported_als;
	als_ctl.is_use_common_factory = false;
	als_ctl.is_report_input_direct = false;
	als_ctl.is_polling_mode = true;

	err = als_register_control_path(&als_ctl);
	if (err != 0) {
		APS_ERR("als_register_control_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}

	als_data.get_data = ap3xx6_als_get_data;
	als_data.vender_div = 1;

	err = als_register_data_path(&als_data);

	if (err != 0) {
		APS_ERR("als_register_data_path fail = %d\n", err);
		goto exit_create_attr_failed;
	}

APS_LOG("als_register_data_path OK.%s:\n", __func__);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level	= EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
	obj->early_drv.suspend = ap3xx6_early_suspend,
	obj->early_drv.resume	= ap3xx6_late_resume,
	register_early_suspend(&obj->early_drv);
#endif

	ap3xx6_init_flag = 0;
	APS_LOG("%s: OK\n", __func__);
	return 0;

exit_create_attr_failed:
	misc_deregister(&ap3xx6_device);
exit_misc_device_register_failed:
exit_init_failed:
	mutex_destroy(&ap3xx6_obj->lock);
//exit_kfree:
	kfree(obj);
exit:
	ap3xx6_i2c_client = NULL;
	ap3xx6_init_flag = -1;
	APS_ERR("%s: err = %d\n", __func__, err);
	return err;
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_i2c_remove(struct i2c_client *client)
{
	int err;

	ap3xx6_delete_attr(&ap3xx6_init_info.platform_diver_addr->driver);
	err = misc_deregister(&ap3xx6_device);
	if (err != 0) {
		APS_ERR("misc_deregister fail: %d\n", err);
	}

	if (1 == ap3xx6_obj->hw->polling_mode_ps) {
		wake_lock_destroy(&chrg_lock);
	}

	ap3xx6_i2c_client = NULL;
	i2c_unregister_device(client);
	mutex_destroy(&ap3xx6_obj->lock);
	kfree(i2c_get_clientdata(client));

	return 0;
	
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_local_init(void)
{

	ap3xx6_power(hw, 1);

	if (i2c_add_driver(&ap3xx6_i2c_driver)) {
		APS_ERR("add driver error\n");
		return -1;
	}
	if (-1 == ap3xx6_init_flag) {
		APS_ERR("add driver--ap3xx6_init_flag check error\n");
		return -1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int ap3xx6_remove(void)
{
	APS_FUN();
	ap3xx6_power(hw, 0);
	i2c_del_driver(&ap3xx6_i2c_driver);
	ap3xx6_init_flag = -1;

	return 0;
}
/*----------------------------------------------------------------------------*/
static int __init ap3xx6_init(void)
{
	const char *name = "mediatek,AP3426";
	APS_FUN();
	hw =   get_alsps_dts_func(name, hw);
	if (!hw)
		APS_ERR("get dts info fail\n");
	alsps_driver_add(&ap3xx6_init_info);

	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit ap3xx6_exit(void)
{
	APS_FUN();
}


module_init(ap3xx6_init);
module_exit(ap3xx6_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("YC Hou");
MODULE_DESCRIPTION("ap3xx6 driver");
MODULE_LICENSE("GPL");
