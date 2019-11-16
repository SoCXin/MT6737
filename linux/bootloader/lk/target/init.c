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
#include <err.h>
#include <debug.h>
#include <target.h>
#include <compiler.h>

#define EXPAND(NAME) #NAME
#define TARGET(NAME) EXPAND(NAME)
/*
 * default implementations of these routines, if the target code
 * chooses not to implement.
 */
extern unsigned long long g_emmc_size;
extern unsigned long g_nand_size;
__WEAK void target_early_init(void)
{
}

__WEAK void target_init(void)
{
}

__WEAK void *target_get_scratch_address(void)
{
    return (void *)(SCRATCH_ADDR);
}
#if 0
__WEAK unsigned target_get_max_flash_size(void)
{
    return (120 * 1024 * 1024);
}

__WEAK int target_is_emmc_boot(void)
{
#if _EMMC_BOOT
    return 1;
#else
    return 0;
#endif
}
#endif
__WEAK unsigned long long target_get_max_flash_size(void)
{
#ifdef MTK_EMMC_SUPPORT
		return g_emmc_size;
#else
		return (unsigned long long)g_nand_size;
#endif	
}
__WEAK int target_is_emmc_boot(void)
{
#ifdef MTK_EMMC_SUPPORT
	return 1;
#else
	return 0;
#endif
}
__WEAK unsigned check_reboot_mode(void)
{
    return 0;
}

__WEAK void reboot_device(unsigned reboot_reason)
{
}

__WEAK unsigned target_pause_for_battery_charge(void)
{
    return 0;
}

__WEAK unsigned target_baseband()
{
	return 0;
}

__WEAK void target_serialno(unsigned char *buf)
{
	snprintf((char *) buf, 13, "%s",TARGET(BOARD));
}

__WEAK void target_fastboot_init()
{
}

__WEAK int emmc_recovery_init(void)
{
	return 0;
}

__WEAK bool target_use_signed_kernel(void)
{
#if _SIGNED_KERNEL
	return 1;
#else
	return 0;
#endif
}
