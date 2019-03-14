#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include "gfx1xm-spi.h"

unsigned char tmpbuf[8192];

static int write_reg(struct gfx1xm_dev* gfx1xm_dev, u16 addr, u8 value)
{  
    int status = 0;
    mutex_lock(&gfx1xm_dev->buf_lock);
    gfx1xm_dev->buffer[GFX1XM_WDATA_OFFSET] = value;
    status = gfx1xm_spi_write_bytes(gfx1xm_dev, addr, 1, gfx1xm_dev->buffer);
    mutex_unlock(&gfx1xm_dev->buf_lock);
    return status;
}
static int read_reg(struct gfx1xm_dev* gfx1xm_dev, u16 addr, u8* value)
{
    int status = 0;
    mutex_lock(&gfx1xm_dev->buf_lock);
    status = gfx1xm_spi_read_bytes(gfx1xm_dev, addr, 1, gfx1xm_dev->buffer);
    *value = gfx1xm_dev->buffer[GFX1XM_RDATA_OFFSET];
    mutex_unlock(&gfx1xm_dev->buf_lock);
    return status;
}
static void read_4k_bytes(struct gfx1xm_dev* gfx1xm_dev, u16 addr, u8* buf)
{
    mutex_lock(&gfx1xm_dev->buf_lock);
    gfx1xm_spi_read_bytes(gfx1xm_dev, addr, 2048, gfx1xm_dev->buffer);
    memcpy(buf, gfx1xm_dev->buffer+GFX1XM_RDATA_OFFSET, 2048);
    gfx1xm_spi_read_bytes(gfx1xm_dev, addr+2048, 2048, gfx1xm_dev->buffer);
    memcpy(buf+2048, gfx1xm_dev->buffer+GFX1XM_RDATA_OFFSET, 2048);
    mutex_unlock(&gfx1xm_dev->buf_lock);
}
static void write_4k_bytes(struct gfx1xm_dev* gfx1xm_dev, u16 addr, u8* buf)
{
    mutex_lock(&gfx1xm_dev->buf_lock);
    memcpy(gfx1xm_dev->buffer+GFX1XM_WDATA_OFFSET, buf, 2048);
    gfx1xm_spi_write_bytes(gfx1xm_dev, addr, 2048, gfx1xm_dev->buffer);
    memcpy(gfx1xm_dev->buffer+GFX1XM_WDATA_OFFSET, buf+2048, 2048);
    gfx1xm_spi_write_bytes(gfx1xm_dev, addr+2048, 2048, gfx1xm_dev->buffer);
    mutex_unlock(&gfx1xm_dev->buf_lock);
}

/*Whether the initialization is complete.
 * If it's completed. value of 0x5094 is 0xAA
 * return 1 if succeed. 0 if failed.
 * */
static int is_init_complete(struct gfx1xm_dev* gfx1xm_dev)
{
    int time_out = 0;
    unsigned char reg_value = 0;
    read_reg(gfx1xm_dev, 0x5094, &reg_value);
    while(reg_value != 0xAA) {
		time_out++;
		if (time_out > 5*1000) {
		    printk("timeout = %d\n", time_out);
		    return 1;			
		}

		mdelay(5);
		read_reg(gfx1xm_dev, 0x5094,&reg_value);
		if(time_out%100 == 99)
		    printk("%s time use %dms", __func__, time_out);
    }
    printk("timeout = %d\n", time_out);
    return 0;
}

/*Waiting DSP move the code to the right area of flash.
 * If it's ok. 0x5094==0x0
 * */
static int is_burn_complete(struct gfx1xm_dev* gfx1xm_dev)
{
    int time_out = 0;
    unsigned char reg_value = -1;
    read_reg(gfx1xm_dev ,0x5094, &reg_value);
    while (reg_value != 0x00){
		time_out++;
		if (time_out > 5*1000){
		    return 1;
		}

		mdelay(5);
		read_reg(gfx1xm_dev, 0x5094, &reg_value);
		if(time_out%100 == 99)
		    printk("%s time use %dms", __func__, time_out);
    }
    return 0;
}

void gfx1xm_run_ice(struct gfx1xm_dev* gfx1xm_dev)
{
    // update_init_9p(gfx1xm_dev);

    //0x2676->0x56,0x5d,0x08,0x1D
    write_reg(gfx1xm_dev, 0x4180, 0x0C);

    write_reg(gfx1xm_dev, 0x404C, 0x01); /*<DSP EN>*/
    write_reg(gfx1xm_dev, 0x404D, 0x01); /*<patch0 EN>*/
    write_reg(gfx1xm_dev, 0x404E, 0x01); /*<patch1 EN>*/

    write_reg(gfx1xm_dev, 0x4278, 0x01); /*<Flash Power-On,default:0>*/
    write_reg(gfx1xm_dev, 0x422E, 0x01); /*<Flash IO_EN,default:0>*/
    write_reg(gfx1xm_dev, 0x422C, 0x01); /*<Flash CTRL_EN,default:1>*/
    write_reg(gfx1xm_dev, 0x422D, 0x00); /*<Flash CTRL BY MicroP,default:0>*/

    /*<ICE ADDR0>*/
    write_reg(gfx1xm_dev, 0x42C4, 0x76);//use 20MHz clock
    write_reg(gfx1xm_dev, 0x42C5, 0x26);
    write_reg(gfx1xm_dev, 0x42C6, 0x76);
    write_reg(gfx1xm_dev, 0x42C7, 0x26);

    /*<ICE ADDR1>*/
    write_reg(gfx1xm_dev, 0x42C8, 0x2F);//flashless flag set to 0
    write_reg(gfx1xm_dev, 0x42C9, 0x2A);
    write_reg(gfx1xm_dev, 0x42CA, 0x2F);
    write_reg(gfx1xm_dev, 0x42CB, 0x2A);

    /*<ICE ADDR2>*/
    write_reg(gfx1xm_dev, 0x42CC, 0x30);//debug flag 0xA5
    write_reg(gfx1xm_dev, 0x42CD, 0x00);
    write_reg(gfx1xm_dev, 0x42CE, 0x30);
    write_reg(gfx1xm_dev, 0x42CF, 0x00);

    /*<ICE DATA0>*/
    write_reg(gfx1xm_dev, 0x42E4, 0x56);
    write_reg(gfx1xm_dev, 0x42E5, 0x5D);
    write_reg(gfx1xm_dev, 0x42E6, 0x08);
    write_reg(gfx1xm_dev, 0x42E7, 0x1D);

    /*<ICE DATA1>*/
    write_reg(gfx1xm_dev, 0x42E8, 0x33);//clock = 20M
    write_reg(gfx1xm_dev, 0x42E9, 0x21);
    write_reg(gfx1xm_dev, 0x42EA, 0x5B);
    write_reg(gfx1xm_dev, 0x42EB, 0x62);

    /*<ICE DATA2>*/
    write_reg(gfx1xm_dev, 0x42EC, 0x18);//debug flag 0x5094 = 0xA5
    write_reg(gfx1xm_dev, 0x42ED, 0x7B);
    write_reg(gfx1xm_dev, 0x42EE, 0xA9);
    write_reg(gfx1xm_dev, 0x42EF, 0x0A);

    write_reg(gfx1xm_dev, 0x4304, 0x07);/*ICE ADDR EN*/
    write_reg(gfx1xm_dev, 0x4305, 0x07);

    write_reg(gfx1xm_dev, 0x404F, 0x00);
    write_reg(gfx1xm_dev, 0x4180, 0x08);//Release CPU and hold DSP
}

/********************step2:burn DSP ISP(4KB)*****************
 * 1.Close watch-dog,  clear cache enable(write 0 to 0x40B0, write 0 to 0x404B)
 * 2.Confg to boot from SRAM(write 0x2 to 0x4190)
 * 3.software reset(write 0x1 to 0x4184)
 * 4.Enable bank3(write 0x3 to 0x4048)
 * 5.Enable accessing code(write 0x1 to 0x404c)
 * 6.Burn the DSP ISP code to the area begin from 0xC000
 * 7.Set scramble(write 0x0 to 0x4218)
 * 8.Disable accessing coe(write 0 to 0x404C)
 */
static int download_dsp_isp_9p(struct gfx1xm_dev* gfx1xm_dev, unsigned char *buf, u16 len)
{
    /*1.Close watch-dog,  clear cache enable(write 0 to 0x40B0)*/
    write_reg(gfx1xm_dev, 0x40B0, 0x0);
    write_reg(gfx1xm_dev, 0x404B, 0x0);

    /*Confg to boot from SRAM(write 0x2 to 0x4190)*/
    write_reg(gfx1xm_dev, 0x4190, 0x2);

    /*3.software reset(write 0x1 to 0x4184)*/
    write_reg(gfx1xm_dev, 0x4184, 0x01);

    /* 4.Enable bank3(write 0x3 to 0x4048)*/
    write_reg(gfx1xm_dev, 0x4048, 0x03);

    /*5.Enable accessing code(write 0x1 to 0x404c)*/
    write_reg(gfx1xm_dev, 0x404C, 0x01);

    /*6.Burn the DSP ISP code to the area begin from 0xC000*/
    // write_data(0xC000, len, buf);
    write_4k_bytes(gfx1xm_dev, 0xC000, buf);

    mdelay(1);
    // read_data(0xC000, len, tmpBuf);
    read_4k_bytes(gfx1xm_dev, 0xC000, tmpbuf);
    printk("tmpBuf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
	    tmpbuf[0],tmpbuf[1],tmpbuf[2],tmpbuf[3],tmpbuf[4],tmpbuf[5],tmpbuf[6],tmpbuf[7]);
    printk("-tmpBuf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
	    tmpbuf[len-8],tmpbuf[len-7],tmpbuf[len-6],tmpbuf[len-5],tmpbuf[len-4],tmpbuf[len-3],tmpbuf[len-2],tmpbuf[len-1]);
    if (memcmp(tmpbuf, buf, 4096)) {
		printk("download_dsp_isp_9p 111 Data read back is not the same as the write one.\n");
		printk("tmpBuf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			tmpbuf[0],tmpbuf[1],tmpbuf[2],tmpbuf[3],tmpbuf[4],tmpbuf[5],tmpbuf[6],tmpbuf[7]);
		printk("buf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
		printk("-tmpBuf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			tmpbuf[len-8],tmpbuf[len-7],tmpbuf[len-6],tmpbuf[len-5],tmpbuf[len-4],tmpbuf[len-3],tmpbuf[len-2],tmpbuf[len-1]);
		printk("-buf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			buf[len-8],buf[len-7],buf[len-6],buf[len-5],buf[len-4],buf[len-3],buf[len-2],buf[len-1]);
		return -1;
    }

    /* 7.Set scramble(write 0x0 to 0x4218)*/
    write_reg(gfx1xm_dev, 0x4218, 0x00);

    /*8.Disable accessing code (writ 0 to 0x404C)*/
    write_reg(gfx1xm_dev, 0x404C, 0x00);

    return 0;
}

/**********************update patch 0**********************************
 * 1.Hold SS51 and DSP(write 0x0C to 0x4180)
 * 2.Clean control flag(write 0x0 to 0x5094)
 * 3.Enbale bank4(write 0x4 to 0x4048)
 * 4.Enable accessing code(write 0x1 to 0x404d)
 * 5.Hold SS51, release DSP(write 0x4 to 0x4180). Waiting the initialization to complete(0x5094==0xAA)
 * 6.Burn the first 8KB of the firmware to the area begin from 0xC000.
 * 7.Send the command(move code[write 0x01 to 0x5094]). Waiting DSP move the code.
 * 8.Select bank4(write 0x4 to 0x4048)
 * 9.Enable accessing code(write 0x1 to 0x404D)
 * 10.Read 8KB data from 0xC000, check whether burn successfully.
 * 11.Disable accessing code(write 0x00 to 0x404D)
 * */
static int update_patch_block0(struct gfx1xm_dev* gfx1xm_dev, unsigned char *buf, unsigned short len)
{
    //struct timeval t1, t2, t3;

    /*1.Hold SS51 and DSP(write 0x0C to 0x4180)*/
    write_reg(gfx1xm_dev, 0x4180, 0x0C);
    /*2.Clean control flag(write 0x0 to 0x5094)*/
    write_reg(gfx1xm_dev, 0x5094, 0x00);
    /*3.Enbale bank4(write 0x4 to 0x4048)*/
    write_reg(gfx1xm_dev, 0x4048, 0x04);
    /*4.Enable accessing code(write 0x1 to 0x404d)*/
    write_reg(gfx1xm_dev, 0x404D, 0x01);

    /*5.Hold SS51, release DSP(write 0x4 to 0x4180).
     * Waiting the initialization to complete(0x5094==0xAA)*/
    write_reg(gfx1xm_dev, 0x4180, 0x04);

    if (is_init_complete(gfx1xm_dev)) {
		printk("Init is not complete.\n");
		return -1;
    }

    /*6.Burn the first 8KB of the firmware to the area begin from 0xC000.*/
    //  gettimeofday(&t1, NULL);
    // write_data(0xC000, len, buf);
    write_4k_bytes(gfx1xm_dev, 0xC000, buf);
    write_4k_bytes(gfx1xm_dev, 0xC000+4096, buf+4096);
    //gettimeofday(&t2, NULL);
    // printf("Write use: %ld us\n", 1000000*(t2.tv_sec - t1.tv_sec) + t2.tv_usec - t1.tv_usec);
    //read_data(0xC000, len, tmpBuf);
    read_4k_bytes(gfx1xm_dev, 0xC000, tmpbuf);
    read_4k_bytes(gfx1xm_dev, 0xC000+4096, tmpbuf+4096);
    // gettimeofday(&t3, NULL);
    // printf("Write use: %ld us\n", 1000000*(t3.tv_sec - t2.tv_sec) + t3.tv_usec - t2.tv_usec);

    if (memcmp(buf, tmpbuf, len)) {
		printk("update_patch_block0 111  Data read back is not the same as the write one.\n");
		return -1;
    }

    /*7.Send the command(move code[write 0x01 to 0x5094]). Waiting DSP move the code.*/
    write_reg(gfx1xm_dev, 0x5094, 0x01);
    printk("tmpBuf_2:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
	    tmpbuf[0],tmpbuf[1],tmpbuf[2],tmpbuf[3],tmpbuf[4],tmpbuf[5],tmpbuf[6],tmpbuf[7]);
    printk("-tmpBuf_2:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
	    tmpbuf[len-8],tmpbuf[len-7],tmpbuf[len-6],tmpbuf[len-5],tmpbuf[len-4],tmpbuf[len-3],tmpbuf[len-2],tmpbuf[len-1]);
    if (is_burn_complete(gfx1xm_dev)) {
		printk("Code is burnning, not completed.\n");
		return -1;
    }

    /*8.Select bank4(write 0x4 to 0x4048)*/
    write_reg(gfx1xm_dev, 0x4048, 0x04);
    /* 9.Enable accessing code(write 0x1 to 0x404D)*/
    write_reg(gfx1xm_dev,0x404D, 0x01);

    /* 10.Read 8KB data from 0xC000, check whether burn successfully.*/
    //gettimeofday(&t2, NULL);
    //read_data(0xC000, len, tmpBuf);
    read_4k_bytes(gfx1xm_dev, 0xC000, tmpbuf);
    read_4k_bytes(gfx1xm_dev, 0xC000+4096, tmpbuf+4096);
    // gettimeofday(&t3, NULL);
    //printf("Code use: %ld us\n", 1000000*(t3.tv_sec - t2.tv_sec) + t3.tv_usec - t2.tv_usec);

    if (memcmp(buf, tmpbuf, len)) {
		printk("update_patch_block0 222  Data read back is not the same as the write one.\n");
		return -1;
    }

    /* 11.Disable accessing code(write 0x00 to 0x404D)*/
    write_reg(gfx1xm_dev, 0x404D, 0x00);
    return 0;
}

/**********************update patch 1**********************************
 * 12.Hold SS51 and DSP(write 0x0C to 0x4180)
 * 13.Enbale bank5(write 0x5 to 0x4048)
 * 14.Enable accessing code(write 0x1 to 0x404d)
 * 15.Hold SS51, release DSP(write 0x4 to 0x4180). Waiting the initialization to complete(0x5094==0xAA)
 * 16.Burn the second 8KB of the firmware to the area begin from 0xC000.
 * 17.Send the command(move code[write 0x01 to 0x5094]). Waiting DSP move the code.
 * 18.Select bank5(write 0x5 to 0x4048)
 * 19.Enable accessing code(write 0x1 to 0x404E)
 * 20.Read 8KB data from 0xC000, check whether burn successfully.
 * 21.Disable accessing code(write 0x00 to 0x404E)
 * */
static int update_patch_block1(struct gfx1xm_dev* gfx1xm_dev, unsigned char *buf, unsigned short len)
{
    /* 12.Hold SS51 and DSP(write 0x0C to 0x4180)*/
    write_reg(gfx1xm_dev, 0x4180, 0x0C);
    /* 13.Enbale bank5(write 0x5 to 0x4048)*/
    write_reg(gfx1xm_dev, 0x4048, 0x05);
    /*14.Enable accessing code(write 0x1 to 0x404d)*/
    write_reg(gfx1xm_dev, 0x404E, 0x01);

    /* 15.Hold SS51, release DSP(write 0x4 to 0x4180). Waiting the initialization to complete(0x5094==0xAA)*/
    write_reg(gfx1xm_dev, 0x4180, 0x04);

    if (is_init_complete(gfx1xm_dev)) {
		printk("Init is not complete.\n");
		return -1;
    }

    /* 16.Burn the second 8KB of the firmware to the area begin from 0xC000.*/
    //write_data(0xC000, len, buf);
    write_4k_bytes(gfx1xm_dev, 0xC000, buf);
    write_4k_bytes(gfx1xm_dev, 0xC000+4096, buf+4096);
    mdelay(1);
    //read_data(0xC000, len, tmpBuf);
    read_4k_bytes(gfx1xm_dev, 0xC000, tmpbuf);
    read_4k_bytes(gfx1xm_dev, 0xC000+4096, tmpbuf+4096);
    if (memcmp(buf, tmpbuf, len)) {
		printk("update_patch_block1 111  Data read back is not the same as the write one.\n");
		return -1;
    }

    write_reg(gfx1xm_dev, 0x4180, 0x04);
    mdelay(50);


    /*17.Send the command(move code[write 0x02 to 0x5094]).
     *  Waiting DSP move the code.
     *  */
    write_reg(gfx1xm_dev, 0x5094, 0x02);
    if (is_burn_complete(gfx1xm_dev)) {
		printk("Code is burnning, not completed.\n");
		return -1;
    }

    /* 18.Select bank5(write 0x5 to 0x4048)*/
    write_reg(gfx1xm_dev, 0x4048, 0x05);
    /* 19.Enable accessing code(write 0x1 to 0x404E)*/
    write_reg(gfx1xm_dev, 0x404E, 0x01);

    /* 20.Read 8KB data from 0xC000, check whether burn successfully.*/
    //read_data(0xC000, len, tmpBuf);
    read_4k_bytes(gfx1xm_dev, 0xC000, tmpbuf);
    read_4k_bytes(gfx1xm_dev, 0xC000+4096, tmpbuf+4096);
    if (memcmp(buf, tmpbuf, len)) {
		printk("update_patch_block1 222  Data read back is not the same as the write one.\n");	
		return -1;
    }

    /* 21.Disable accessing code(write 0x00 to 0x404E)*/
    write_reg(gfx1xm_dev, 0x404E, 0x00);

    return 0;
}

/**********************update dsp code**********************************
 * 1.Select bank3(write 0x3 to 0x4048)
 * 2.Hold SS51 and DSP(write 0x0C to 0x4180)
 * 3.Setting scramble(write 0x0 to 0x4218)
 * 4.Hold SS51, release DSP(write 0x4 to 0x4180). Waiting the initialization to complete(0x5094==0xAA)
 * 5.Burn the DSP code(4KB) to the area begin from 0x9000.
 * 6.Send the command(move code[write 0x05 to 0x5094]). Waiting DSP move the code.
 * 7.Read 4KB data from 0x9000 to do verification.
 * */
static int update_dsp_code_9p(struct gfx1xm_dev* gfx1xm_dev, unsigned char *buf, unsigned short len)
{
    /* 1.Select bank3(write 0x3 to 0x4048)*/
    write_reg(gfx1xm_dev, 0x4048, 0x03);
    /* 2.Hold SS51 and DSP(write 0x0C to 0x4180)*/
    write_reg(gfx1xm_dev, 0x4180, 0x0C);
    /* 3.Setting scramble(write 0x0 to 0x4218)*/
    write_reg(gfx1xm_dev, 0x4218, 0x00);

    /* 4.Hold SS51, release DSP(write 0x4 to 0x4180). Waiting the initialization to complete(0x5094==0xAA)*/
    write_reg(gfx1xm_dev, 0x4180, 0x04);

    if (is_init_complete(gfx1xm_dev)) {
		printk("Init is not complete.\n");
		return -1;
    }

    /* 4.Hold SS51, release DSP(write 0x4 to 0x4180). Waiting the initialization to complete(0x5094==0xAA)*/
    //write_data(0x9000, len, buf);
    write_4k_bytes(gfx1xm_dev, 0x9000, buf);
    mdelay(1);
    //read_data(0x9000, len, tmpBuf);
    read_4k_bytes(gfx1xm_dev, 0x9000, tmpbuf);
    if (memcmp(buf, tmpbuf, len)) {
		printk("update_dsp_code_9p 111 Data read back is not the same as the write one.\n");
		printk("tmpBuf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			tmpbuf[0],tmpbuf[1],tmpbuf[2],tmpbuf[3],tmpbuf[4],tmpbuf[5],tmpbuf[6],tmpbuf[7]);
		printk("buf:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
			buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
		return -1;
    }

    /* 6.Send the command(move code[write 0x05 to 0x5094]). Waiting DSP move the code.*/
    write_reg(gfx1xm_dev, 0x5094, 0x05);
    if (is_burn_complete(gfx1xm_dev)) {
		printk("Code is burnning, not completed.\n");
		return -1;
    }

    /* 7.Read 4KB data from 0x9000 to do verification.*/
    //read_data(0x9000, len, tmpBuf);
    read_4k_bytes(gfx1xm_dev, 0x9000, tmpbuf);
    if (memcmp(buf, tmpbuf, len)) {
		printk("update_dsp_code_9p 222 Data read back is not the same as the write one.\n");
		return -1;
    }

    gfx1xm_run_ice(gfx1xm_dev);

    return 0;
}

/*do the real firmware update.*/
int gfx1xm_fw_update(struct gfx1xm_dev* gfx1xm_dev, unsigned char *buf, unsigned short len)
{
    unsigned char *p_fw = NULL;
    int ret = 0;
    if(buf == NULL) {
		printk("Firmware buffer is NULL.\n");
		return -2;
    }

    /*Step1:download dsp_isp. 4KB[38K~42K]*/
    printk("Update DSP ISP.\n");
    p_fw = buf + 38*1024 + 14;
    len = 4096;
    // update_init_9p(gfx1xm_dev);/*first initilize the update interface*/

    if (ret == 0) {
		printk("update_init_9p Succeed.\n");
		ret = download_dsp_isp_9p(gfx1xm_dev, p_fw, len);
		if(ret != 0) {
		    printk("Failed to download_dsp_isp_9p.int = %d\n", ret);
		    return ret;
		} else {
		    printk("Download DSP ISP.Len = %d  Succeed! \n", len);
		}
    } else {
		printk("Failed to update_init_9p. int = %d\n", ret);
    }

    /*Step2:update patch code part0. 8KB[0~8K]*/
    printk("Update patch code part0.\n");
    p_fw = buf + 14;
    len = 8192;
    ret = update_patch_block0(gfx1xm_dev, p_fw, len);
    if(ret == 0) {
		printk("Update patch code part0.Len = %d  Succeed!\n", len);
    } else {
		printk("Failed to update patch code part0. int = %d\n", ret);
		return ret;
    }

    /*Step3:update patch code part1. 8KB[8~16K]*/
    printk("Update patch code part1.\n");
    p_fw = buf + 8192 + 14;
    len = 8192;
    ret = update_patch_block1(gfx1xm_dev, p_fw, len);
    if(ret == 0) {
		printk("Update patch code part1.Len = %d  Succeed!\n", len);
    } else {
		printk("Failed to update patch code part1. int = %d\n", ret);
		return ret;
    }


    /*Step4:update dsp code. 4KB[32K~36K]*/
    printk("Update DSP CODE.\n");
    p_fw = buf + 32*1024 + 14;
    len = 4096;
    ret = update_dsp_code_9p(gfx1xm_dev, p_fw, len);
    if(ret == 0) {
		printk("Update DSP CODE.Len = %d  Succeed!\n", len);
    } else {
		printk("Failed to DSP CODE.int = %d\n", ret);
		return ret;
    }
    printk("Update finished.Succeed!\n");
    return ret;
}
