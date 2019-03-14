/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Xudong.chen <xudong.chen@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __I2C_MTK_H__
#define __I2C_MTK_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>

#define I2C_DEBUG_FS

#define I2C_HS_NACKERR			(1 << 2)
#define I2C_ACKERR			(1 << 1)
#define I2C_TRANSAC_COMP		(1 << 0)
#define I2C_TRANSAC_START		(1 << 0)
#define I2C_TIMING_STEP_DIV_MASK	(0x3f << 0)
#define I2C_TIMING_SAMPLE_COUNT_MASK	(0x7 << 0)
#define I2C_TIMING_SAMPLE_DIV_MASK	(0x7 << 8)
#define I2C_TIMING_DATA_READ_MASK	(0x7 << 12)
#define I2C_DCM_DISABLE			0x0000
#define I2C_IO_CONFIG_OPEN_DRAIN	0x0003
#define I2C_IO_CONFIG_PUSH_PULL		0x0000
#define I2C_SOFT_RST			0x0001
#define I2C_FIFO_ADDR_CLR		0x0001
#define I2C_DELAY_LEN			0x0002
#define I2C_ST_START_CON		0x8001
#define I2C_FS_START_CON		0x1800
#define I2C_TIME_CLR_VALUE		0x0000
#define I2C_TIME_DEFAULT_VALUE		0x0003

#define I2C_DMA_CON_TX			0x0000
#define I2C_DMA_CON_RX			0x0001
#define I2C_DMA_START_EN		0x0001
#define I2C_DMA_INT_FLAG_NONE		0x0000

#define I2C_DEFAUT_SPEED		100000	/* hz */
#define MAX_FS_MODE_SPEED		400000
#define MAX_HS_MODE_SPEED		3400000
#define MAX_DMA_TRANS_SIZE		4096	/* 255 */
#define MAX_SAMPLE_CNT_DIV		8
#define MAX_STEP_CNT_DIV		64
#define MAX_HS_STEP_CNT_DIV		8

#define I2C_CONTROL_RS                  (0x1 << 1)
#define I2C_CONTROL_DMA_EN              (0x1 << 2)
#define I2C_CONTROL_CLK_EXT_EN          (0x1 << 3)
#define I2C_CONTROL_DIR_CHANGE          (0x1 << 4)
#define I2C_CONTROL_ACKERR_DET_EN       (0x1 << 5)
#define I2C_CONTROL_TRANSFER_LEN_CHANGE (0x1 << 6)
#define I2C_CONTROL_WRAPPER             (0x1 << 0)

#define I2C_DRV_NAME		"mt-i2c"
#define I2CTAG          "[I2C]"

enum DMA_REGS_OFFSET {
	OFFSET_INT_FLAG = 0x0,
	OFFSET_INT_EN = 0x04,
	OFFSET_EN = 0x08,
	OFFSET_RST = 0x0C,
	OFFSET_STOP = 0x10,
	OFFSET_FLUSH = 0x14,
	OFFSET_CON = 0x18,
	OFFSET_TX_MEM_ADDR = 0x1C,
	OFFSET_RX_MEM_ADDR = 0x20,
	OFFSET_TX_LEN = 0x24,
	OFFSET_RX_LEN = 0x28,
	OFFSET_INT_BUF_SIZE = 0x38,
};

enum i2c_trans_st_rs {
	I2C_TRANS_STOP = 0,
	I2C_TRANS_REPEATED_START,
};

enum {
	FS_MODE,
	HS_MODE,
};

enum mt_trans_op {
	I2C_MASTER_WR = 1,
	I2C_MASTER_RD,
	I2C_MASTER_WRRD,
};

enum I2C_REGS_OFFSET {
	OFFSET_DATA_PORT = 0x0,
	OFFSET_SLAVE_ADDR = 0x04,
	OFFSET_INTR_MASK = 0x08,
	OFFSET_INTR_STAT = 0x0c,
	OFFSET_CONTROL = 0x10,
	OFFSET_TRANSFER_LEN = 0x14,
	OFFSET_TRANSAC_LEN = 0x18,
	OFFSET_DELAY_LEN = 0x1c,
	OFFSET_TIMING = 0x20,
	OFFSET_START = 0x24,
	OFFSET_EXT_CONF = 0x28,
	OFFSET_FIFO_STAT = 0x30,
	OFFSET_FIFO_THRESH = 0x34,
	OFFSET_FIFO_ADDR_CLR = 0x38,
	OFFSET_IO_CONFIG = 0x40,
	OFFSET_RSV_DEBUG = 0x44,
	OFFSET_HS = 0x48,
	OFFSET_SOFTRESET = 0x50,
	OFFSET_DCM_EN = 0x54,
	OFFSET_PATH_DIR = 0x60,
	OFFSET_DEBUGSTAT = 0x64,
	OFFSET_DEBUGCTRL = 0x68,
	OFFSET_TRANSFER_LEN_AUX = 0x6c,
};

enum PERICFG_OFFSET {
	OFFSET_PERI_I2C_MODE_ENABLE = 0x0410,
};

struct mt_i2c_data {
	unsigned int clk_frequency;	/* bus speed in Hz */
	unsigned int flags;
	unsigned int clk_src_div;
};

struct i2c_dma_buf {
	u8 *vaddr;
	dma_addr_t paddr;
};

struct mt_i2c_ext {
#define I2C_HWTRIG_FLAG		0x00000001
	bool isEnable;
	bool is_hw_trig;
	u32 timing;
};

struct mt_i2c {
	struct i2c_adapter adap;	/* i2c host adapter */
	struct device *dev;
	wait_queue_head_t wait;		/* i2c transfer wait queue */
	/* set in i2c probe */
	void __iomem *base;		/* i2c base addr */
	void __iomem *pdmabase;		/* dma base address*/
	int irqnr;			/* i2c interrupt number */
	int id;
	struct i2c_dma_buf dma_buf;	/* memory alloc for DMA mode */
	struct clk *clk_main;		/* main clock for i2c bus */
	struct clk *clk_dma;		/* DMA clock for i2c via DMA */
	struct clk *clk_pmic;		/* PMIC clock for i2c from PMIC */
	struct clk *clk_arb;		/* Arbitrator clock for i2c */
	bool have_pmic;			/* can use i2c pins form PMIC */
	bool have_dcm;			/* HW DCM function */
	bool use_push_pull;		/* IO config push-pull mode */
	/* set when doing the transfer */
	u16 irq_stat;			/* interrupt status */
	unsigned int speed_hz;		/* The speed in transfer */
	unsigned int clk_src_div;
	bool trans_stop;		/* i2c transfer stop */
	enum mt_trans_op op;
	u16 msg_len;
	u8 *msg_buf;			/* pointer to msg data */
	u16 msg_aux_len;		/* WRRD mode to set AUX_LEN register*/
	u16 addr;	/* 7bit slave address, without read/write bit */
	u16 timing_reg;
	u16 high_speed_reg;
	struct mutex i2c_mutex;
	struct mt_i2c_ext ext_data;
};

extern void i2c_dump_info(struct mt_i2c *i2c);

extern int mtk_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num,
					u32 ext_flag, u32 timing);

#endif
