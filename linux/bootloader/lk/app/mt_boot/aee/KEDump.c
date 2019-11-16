/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/
#include <stdio.h>
#include <string.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <platform/partition.h>
#else
#include <mt_partition.h>
#endif
#include <platform/boot_mode.h>
#include <platform/ram_console.h>
#include <platform/mt_reg_base.h>
#include "elf.h"
#include "KEHeader.h"

/**************************/
/* ----------------	  */
/* RAM_CONSOLE_DRAM_ADDR  */
/* (1M align)		  */
/* +RAM_CONSOLE_DRAM_SIZE */
/* 			  */
/* ----------------	  */
/* +0xf0000		  */
/* 			  */
/* ----------------	  */
/* KE_RESERVED_MEM_ADDR	  */
/* 			  */
/* ----------------	  */
/* RAMDISK_LOAD_ADDR	  */
/**************************/
#define KE_RESERVED_MEM_ADDR (RAM_CONSOLE_DRAM_ADDR + 0xf0000)
#define EXPDB_RESERVED_OTHER    (3 * 1024 * 1024) //reserved expdb for control block and pl/lk log
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
#define MEM_EXPDB_SIZE 0x300000
static char *mem_expdb;
#endif
struct ke_dev {
	part_dev_t *dev;
	uint  part_id;
	u64 ptn;
	u64 part_size;
};

static struct ke_dev dev;

struct elfhdr {
	void *start;
	unsigned int e_machine;
	unsigned int e_phoff;
	unsigned int e_phnum;
};

extern BOOT_ARGUMENT *g_boot_arg;
#define SZLOG 4096
static char logbuf[SZLOG];

extern void ram_console_addr_size(unsigned long *addr, unsigned long *size);

int sLOG(char *fmt, ...)
{
	va_list args;
	static int pos = 0;
	va_start(args, fmt);
	if (pos < SZLOG - 1) /* vsnprintf bug */
		pos += vsnprintf(logbuf + pos, SZLOG - pos - 1, fmt, args);
	va_end(args);
	return 0;
}

#define LOG(fmt, ...)			\
	sLOG(fmt, ##__VA_ARGS__);		\
	printf(fmt, ##__VA_ARGS__)

#define LOGD(fmt, ...)				\
	sLOG(fmt, ##__VA_ARGS__)

#define elf_note	elf32_note
#define PHDR_PTR(ehdr, phdr, mem)		\
	(ehdr->e_machine == EM_ARM ? ((struct elf32_phdr*)phdr)->mem : ((struct elf64_phdr*)phdr)->mem)

#define PHDR_TYPE(ehdr, phdr) PHDR_PTR(ehdr, phdr, p_type)
#define PHDR_ADDR(ehdr, phdr) PHDR_PTR(ehdr, phdr, p_paddr)
#define PHDR_SIZE(ehdr, phdr) PHDR_PTR(ehdr, phdr, p_filesz)
#define PHDR_OFF(ehdr, phdr) PHDR_PTR(ehdr, phdr, p_offset)
#define PHDR_INDEX(ehdr, i)			\
	(ehdr->e_machine == EM_ARM ? ehdr->start + ehdr->e_phoff + sizeof(struct elf32_phdr) * i : ehdr->start + ehdr->e_phoff + sizeof(struct elf64_phdr) *i)

static struct elfhdr* kedump_elf_hdr(void)
{
	char *ei;
	static struct elfhdr kehdr;
	static struct elfhdr *ehdr = (void*)-1;
	if (ehdr != (void*)-1)
		return ehdr;
	ehdr = NULL;
        kehdr.start = (void*)(KE_RESERVED_MEM_ADDR);
	LOGD("kedump: KEHeader %p\n", kehdr.start);
	if (kehdr.start) {
		ei = (char*)kehdr.start; //elf_hdr.e_ident
        /* valid elf header */
		if (ei[0] == 0x7f && ei[1] == 'E' && ei[2] == 'L' && ei[3] == 'F') {
			kehdr.e_machine = ((struct elf32_hdr*)(kehdr.start))->e_machine;
			if (kehdr.e_machine == EM_ARM) {
				kehdr.e_phnum = ((struct elf32_hdr*)(kehdr.start))->e_phnum;
				kehdr.e_phoff = ((struct elf32_hdr*)(kehdr.start))->e_phoff;
				ehdr = &kehdr;
			} else if (kehdr.e_machine == EM_AARCH64) {
				kehdr.e_phnum = ((struct elf64_hdr*)(kehdr.start))->e_phnum;
				kehdr.e_phoff = ((struct elf64_hdr*)(kehdr.start))->e_phoff;
				ehdr = &kehdr;
			}
		}
		if(ehdr == NULL)
			LOG("kedump: invalid header[0x%x%x%x%x]\n", ei[0], ei[1], ei[2], ei[3]);
        }
	LOGD("kedump: mach[%x], phnum[%x], phoff[%x]", kehdr.e_machine, kehdr.e_phnum, kehdr.e_phoff);
	return ehdr;
}

static int kedump_dev_open(void)
{
	int index;
        index = partition_get_index(AEE_IPANIC_PLABLE);
	dev.dev = mt_part_get_device();
        if(index == -1 || dev.dev == NULL) {
		LOG("kedump: no %s partition[%d]\n", AEE_IPANIC_PLABLE, index);
                return -1;
        }
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
        dev.part_id = partition_get_region(index);
#endif
        dev.ptn = partition_get_offset(index);
	dev.part_size = partition_get_size(index);
	LOG("kedump: partiton %d[%llx - %llx]\n", index, dev.ptn, dev.part_size);
        return 0;
}

#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
unsigned long long mem_expdb_write(void *data, unsigned long long offset, unsigned long sz)
{
	unsigned long sz_bak = sz;
	if ((offset + sz) > MEM_EXPDB_SIZE) {
		LOG("overflow!\n");
		return 0;
	}
	memcpy((mem_expdb + offset), data, (unsigned long)sz);

	return sz;
}
#endif
static unsigned long long kedump_dev_write (unsigned long long offset, void *data, unsigned long sz)
{
        unsigned long long size_wrote = 0;

	if (offset >= dev.part_size || sz > dev.part_size - offset) {
		LOG("kedump: write oversize %lx -> %llx > %lx\n", sz, offset, dev.part_size);
		return 0;
	}

#ifdef MTK_EMMC_SUPPORT
#ifdef MTK_NEW_COMBO_EMMC_SUPPORT
        size_wrote = dev.dev->write(dev.dev, data, dev.ptn + offset, sz, dev.part_id);
#else
        size_wrote = dev.dev->write(dev.dev, data, dev.ptn + offset, sz);
#endif
#elif defined(MTK_NAND_SUPPORT)
	size_wrote = dev.dev->write(dev.dev, data, (unsigned long)dev.ptn + offset, sz);
#endif

#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
	size_wrote = mem_expdb_write(data, offset, sz);
#endif
	if ((long long)size_wrote <= 0) {
		LOG("kedump: write failed(%llx), %lx@%p -> %llx\n", size_wrote, sz, data, offset);
		size_wrote = 0;
	}
        return size_wrote;
}

static void kedump_dev_close(void)
{
        return;
}

/* the min offset reserved for the header's size. */
static unsigned long kedump_mrdump_header_size (struct elfhdr *ehdr)
{
	void *phdr = PHDR_INDEX(ehdr, 1);
	return ALIGN(PHDR_OFF(ehdr, phdr) + PHDR_SIZE(ehdr, phdr), PAGE_SIZE);
}

static unsigned int kedump_mini_rdump(struct elfhdr *ehdr, unsigned long long offset)
{
	void *phdr;
	unsigned long addr, size;
	unsigned int i;
	unsigned int total = 0;
	unsigned long elfoff = kedump_mrdump_header_size(ehdr);
	unsigned long sz_header = elfoff;
	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = PHDR_INDEX(ehdr, i);
		LOGD("kedump: PT[%d] %x@%llx -> %x(%x)\n", PHDR_TYPE(ehdr, phdr), PHDR_SIZE(ehdr, phdr), PHDR_ADDR(ehdr, phdr), elfoff, (unsigned int)PHDR_OFF(ehdr, phdr));
		if (PHDR_TYPE(ehdr, phdr) != PT_LOAD)
			continue;
		addr = PHDR_ADDR(ehdr, phdr);
		size = PHDR_SIZE(ehdr, phdr);
		if (ehdr->e_machine == EM_ARM)
			((struct elf32_phdr*)phdr)->p_offset = elfoff;
		else
			((struct elf64_phdr*)phdr)->p_offset = elfoff;
		if (size != 0 && elfoff != 0)
			total += kedump_dev_write(offset + elfoff, (void*)addr, size);
		elfoff += size;
	}
	total += kedump_dev_write(offset, ehdr->start, sz_header);
	return total;
}

static unsigned int kedump_misc(unsigned int addr, unsigned int start, unsigned int size, unsigned long long offset)
{
	unsigned int total;
	LOG("kedump: misc data %x@%x+%x\n", size, addr, start);
	if (start >= size)
		start = start % size;
	total = kedump_dev_write(offset, (void*)addr + start, size - start);
	if (start)
		total += kedump_dev_write(offset + total, (void*)addr, start);
	return total;
}

static unsigned int kedump_misc32(struct mrdump_mini_misc_data32 *data, unsigned long long offset)
{
	unsigned int addr = data->paddr;
	unsigned int start = data->start ? *(unsigned int*)(data->start) : 0;
	unsigned int size = data->size;
	return kedump_misc(addr, start, size, offset);
}

static unsigned int kedump_misc64(struct mrdump_mini_misc_data64 *data, unsigned long long offset)
{
	unsigned int addr = (unsigned int)data->paddr;
	unsigned int start = data->start ? *(unsigned int*)(unsigned long)(data->start) : 0;
	unsigned int size = (unsigned int)data->size;
	return kedump_misc(addr, start, size, offset);
}

struct ipanic_header panic_header;
static void kedump_add2hdr(unsigned int offset, unsigned int size, unsigned datasize, char *name)
{
	struct ipanic_data_header *pdata;
	int i;
	for (i = 0; i < IPANIC_NR_SECTIONS; i++) {
		pdata = &panic_header.data_hdr[i];
		if (pdata->valid == 0)
			break;
	}
	LOG("kedump add: %s[%d] %x/%x@%x\n", name, i, datasize, size, offset);
	if (i < IPANIC_NR_SECTIONS) {
		pdata->offset = offset;
		pdata->total = size;
		pdata->used = datasize;
		strcpy((char*)pdata->name, name);
		pdata->valid = 1;
	}
}

static int kedump_to_expdb(void)
{
	struct elfhdr *ehdr;
	void *phdr_misc;
	struct elf_note *misc, *miscs;
	char *m_name;
	unsigned long sz_misc, addr_misc;
	void *m_data;
	unsigned long long offset;
	unsigned int size, datasize;
	char name[32];
	unsigned int i;

	if (kedump_dev_open() != 0)
                return -1;

        ehdr = kedump_elf_hdr();
        if (0 == ehdr)
		return -1;
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
	mem_expdb = malloc(MEM_EXPDB_SIZE);
	if (mem_expdb == NULL) {
		LOG("mem_expdb malloc fail!\n");
		return -1;
	}
	LOG("mem_expdb malloc success, 0x%x size is 0x%x\n", mem_expdb, MEM_EXPDB_SIZE);
	memset(mem_expdb, 0x0, MEM_EXPDB_SIZE);
#endif
	/* reserve space in expdb for panic header */
	offset = ALIGN(sizeof(panic_header), BLK_SIZE);
	datasize = kedump_mini_rdump(ehdr, offset);
	size = datasize;
	kedump_add2hdr(offset, size, datasize, "SYS_MINI_RDUMP");
	offset += datasize;

	phdr_misc = PHDR_INDEX(ehdr, 1);
	miscs = (struct elf_note*)(ehdr->start + PHDR_OFF(ehdr, phdr_misc));
	LOGD("kedump: misc[%p] %llx@%llx\n", phdr_misc, PHDR_SIZE(ehdr, phdr_misc), PHDR_OFF(ehdr, phdr_misc));
	sz_misc = sizeof(struct elf_note) + miscs->n_namesz + miscs->n_descsz;
	LOGD("kedump: miscs[%p], size %x\n", miscs, sz_misc);
	for (i = 0; i < (PHDR_SIZE(ehdr, phdr_misc)) / sz_misc; i++) {
        char klog_first[16];
        memset(klog_first, 0x0, sizeof(klog_first));
		misc = (struct elf_note*)((void*)miscs + sz_misc * i);
		m_name = (char*)misc + sizeof(struct elf_note);
		if (m_name[0] == 'N' && m_name[1] == 'A' && m_name[2] == '\0')
                        break;
		m_data = (void*)misc + sizeof(struct elf_note) + misc->n_namesz;
		if (misc->n_descsz == sizeof(struct mrdump_mini_misc_data32)) {
            if (strcmp(m_name, "_KERNEL_LOG_") == 0) {
                sprintf(klog_first, "_%u", *(unsigned int *)(((struct mrdump_mini_misc_data32*)m_data)->start));
                ((struct mrdump_mini_misc_data32*)m_data)->start = 0;
            }
			datasize = kedump_misc32((struct mrdump_mini_misc_data32*)m_data, offset);
			size = ((struct mrdump_mini_misc_data32*)m_data)->size;
		} else {
            if (strcmp(m_name, "_KERNEL_LOG_") == 0) {
                sprintf(klog_first, "_%llu", *(unsigned int *)(unsigned long)(((struct mrdump_mini_misc_data64*)m_data)->start));
                ((struct mrdump_mini_misc_data64*)m_data)->start = 0;
            }
			datasize = kedump_misc64((struct mrdump_mini_misc_data64*)m_data, offset);
			size = ((struct mrdump_mini_misc_data64*)m_data)->size;
		}
		/* [SYS_]MISC[_RAW] */
                if (m_name[0] == '_')
                        strcpy (name, "SYS");
		else
			name[0] = 0;
                strcat (name, m_name);
                if (m_name[strlen(m_name)-1] == '_')
                        strcat (name,  "RAW");
        if (klog_first[0] != 0)
            strcat(name, klog_first);
		kedump_add2hdr(offset, size, datasize, name);
		offset += datasize;
	}
	/* ram_console raw log */
	ram_console_addr_size(&addr_misc, &sz_misc);
	if (addr_misc && sz_misc) {
		datasize = kedump_misc((unsigned int)addr_misc, 0, (unsigned int)sz_misc, offset);
		kedump_add2hdr(offset, (unsigned int)sz_misc, datasize, "SYS_RAMCONSOLE_RAW");
		offset += datasize;
	}
	/* save KEdump flow logs */
	datasize = kedump_dev_write(offset, logbuf, SZLOG);
	kedump_add2hdr(offset, SZLOG, datasize, "ZAEE_LOG");
	offset += datasize;

	/* finally write the ipanic header. */
	panic_header.magic = AEE_IPANIC_MAGIC;
	panic_header.version = AEE_IPANIC_PHDR_VERSION;
	panic_header.size = sizeof(panic_header);
	panic_header.blksize = BLK_SIZE;
	panic_header.partsize = dev.part_size;
	kedump_dev_write(0, (void*)(&panic_header), sizeof(panic_header));
#if defined(MTK_MLC_NAND_SUPPORT) || defined(MTK_TLC_NAND_SUPPORT)
	unsigned long long size_wrote = dev.dev->write(dev.dev, mem_expdb, (unsigned long)dev.ptn, MEM_EXPDB_SIZE/*dev.part_size*/, dev.part_id);
	free(mem_expdb);
#endif
	return 0;
}

static int kedump_skip(void)
{
	unsigned int boot_reason = g_boot_arg->boot_reason;
	static int kedump_dumped = 0;
	LOG("kedump: boot_reason(%d)\n", boot_reason);
	ram_console_init();
	/* this flow should be executed once only */
	if (kedump_dumped == 0) {
		kedump_dumped = 1;
		if (ram_console_is_abnormal_boot())
		    return 0;
	}
	return 1;
}

static int kedump_avail(void)
{
        struct elfhdr *ehdr;
        ehdr = kedump_elf_hdr();
        if (0 == ehdr) {
                return -1;
        }
	return 0;
}

static int kedump_done(void)
{
	struct elfhdr* ehdr;
        ehdr = kedump_elf_hdr();
	if (0 == ehdr) {
                return -1;
        }
	((char*)ehdr->start)[0] = 0x81; //->e_ident[0]
	return 0;
}
/* Dump KE infomation to expdb */
/*  0 success , -1 failed*/
int kedump_mini(void)
{
	LOG("kedump mini start\n");
        if (kedump_skip())
                return 0;

        if (kedump_avail())
                return 0;

	kedump_to_expdb();

	kedump_done();
	LOG("kedump mini done\n");
        return 1;
}
