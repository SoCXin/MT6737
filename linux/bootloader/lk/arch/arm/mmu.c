/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <debug.h>
#include <sys/types.h>
#include <compiler.h>
#include <arch.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>

#if ARM_WITH_MMU

#define MB (1024*1024)
#define GB (1024*1024*1024)

/* the location of the table may be brought in from outside */
#if WITH_EXTERNAL_TRANSLATION_TABLE
#if !defined(MMU_TRANSLATION_TABLE_ADDR)
#error must set MMU_TRANSLATION_TABLE_ADDR in the make configuration
#endif
static uint32_t *tt = (void *)MMU_TRANSLATION_TABLE_ADDR;
#else
/* the main translation table */
static uint32_t tt[4096] __ALIGNED(16384);
#endif
#ifndef MTK_LM_2LEVEL_PAGETABLE_MODE
static uint64_t *lpae_tt = (uint64_t *)tt;
#else
/* L1 must align it's table size */
static uint64_t ld_tt_l1[4] __ALIGNED(32);
static uint64_t *ld_tt_l2 = (uint64_t *)tt;
#endif
void arm_mmu_map_section(addr_t paddr, addr_t vaddr, uint flags)
{
	int index;

	/* Get the index into the translation table */
	index = vaddr / MB;

	/* Set the entry value:
	 * (2<<0): Section entry
	 * (0<<5): Domain = 0
	 *  flags: TEX, CB and AP bit settings provided by the caller.
	 */
	tt[index] = (paddr & ~(MB-1)) | (0<<5) | (2<<0) | flags;

	arm_invalidate_tlb();
}

void arm_mmu_map_block(unsigned long long paddr, addr_t vaddr, unsigned long long flags)
{
	/* Get the index into the translation table */
#ifndef MTK_LM_2LEVEL_PAGETABLE_MODE
	int index = vaddr / GB;

	lpae_tt[index] = (paddr & (0x000000FFC0000000ULL)) | (0x1<<10) | (0x3<<8) | (0x1<<0) | flags;
#else
	int index = vaddr / (2*MB);
	ld_tt_l2[index] = (paddr & (0x000000FFFFE00000ULL)) | (0x1<<10) | (0x3<<8) | (0x1<<0) | flags;
#endif
	arm_invalidate_tlb();
}

unsigned long long arm_mmu_va2pa(unsigned int vaddr)
{
	unsigned long long paddr;
	int index;

	unsigned int ttbcr;
	__asm__ volatile("mrc p15, 0, %0, c2, c0, 2" : "=r" (ttbcr));

	if (!(ttbcr & (0x1 << 31))) {
		index = vaddr / MB;
		paddr = (tt[index] & ~(MB-1)) | (vaddr & (MB-1));
	} else {
#ifndef MTK_LM_2LEVEL_PAGETABLE_MODE
		index = vaddr / GB;
		paddr = (lpae_tt[index] & (0x000000FFC0000000ULL)) | (vaddr & (GB-1));
#else
		index = vaddr / (2*MB);
		paddr = (ld_tt_l2[index] & (0x000000FFFFE00000ULL)) | (vaddr & ((2*MB)-1));
#endif
	}

	return paddr;
}

void arm_mmu_init(void)
{
	unsigned int i;

	/* set some mmu specific control bits:
	 * access flag disabled, TEX remap disabled, mmu disabled
	 */
	arm_write_cr1(arm_read_cr1() & ~((1<<29)|(1<<28)|(1<<0)));

	/* set up an identity-mapped translation table with
	 * strongly ordered memory type and read/write access.
	 */
	for (i=0; i < 4096; i++) {
		arm_mmu_map_section(i * MB,
		                    i * MB,
		                    MMU_MEMORY_TYPE_STRONGLY_ORDERED |
		                    MMU_MEMORY_AP_READ_WRITE);
	}

	/* set up the translation table base */
	arm_write_ttbr((uint32_t)tt);

	/* set up the domain access register */
	arm_write_dacr(0x00000001);
}

void arm_mmu_lpae_init(void)
{
	unsigned int i;
	unsigned int ttbcr = 0xB0003000;
	unsigned int mair0 = 0xeeaa4400;
	unsigned int mair1 = 0xff000004;

	/* set some mmu specific control bits:
	 * access flag disabled, TEX remap disabled, mmu disabled
	 */
	arm_write_cr1(arm_read_cr1() & ~((1<<29)|(1<<28)|(1<<0)));

	__asm__ volatile("mcr p15, 0, %0, c2,  c0, 2" :: "r" (ttbcr));
	__asm__ volatile("mcr p15, 0, %0, c10, c2, 0" :: "r" (mair0));
	__asm__ volatile("mcr p15, 0, %0, c10, c2, 1" :: "r" (mair1));

#ifndef MTK_LM_2LEVEL_PAGETABLE_MODE
	/* set up an identity-mapped translation table with
	 * strongly ordered memory type and read/write access.
	 */
	for (i=0; i < 4; i++) {
		arm_mmu_map_block(i * GB, i * GB, LPAE_MMU_MEMORY_TYPE_STRONGLY_ORDERED);
	}

	/* set up the translation table base */
	arm_write_ttbr((uint32_t)lpae_tt);

#else
	/* setup L1 table */
	for (i=0; i < 4; i++) {
		ld_tt_l1[i] = ((uint64_t)((uint32_t)&ld_tt_l2[512*i])) | (0x3<<0);
	}
	/* l2 table mapping  */
	for (i=0; i < 2048; i++) {
		ld_tt_l2[i] = i*(2*MB) | (0x1<<10) | (0x3<<8)| (0x1<<0) | LPAE_MMU_MEMORY_TYPE_STRONGLY_ORDERED;
	}

	/* set up the translation table base */
	arm_write_ttbr((uint32_t)ld_tt_l1);
#endif
	/* set up the domain access register */
	arm_write_dacr(0x00000001);

	/* turn on the mmu */
	//arm_write_cr1(arm_read_cr1() | 0x1);
}

void arch_enable_mmu(void)
{
	arm_write_cr1(arm_read_cr1() | 0x1);
}

void arch_disable_mmu(void)
{
	arm_write_cr1(arm_read_cr1() & ~(1<<0));
}

#endif // ARM_WITH_MMU

