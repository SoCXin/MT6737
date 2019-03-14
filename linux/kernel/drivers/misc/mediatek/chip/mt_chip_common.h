#ifndef __MT_CHIP_COMMON_H__
#define __MT_CHIP_COMMON_H__
#include <mt-plat/mt_chip.h>

#define CHIP_INFO_BIT(ID) (2 << ID)
#define CHIP_INFO_SUP(MASK, ID) ((MASK & CHIP_INFO_BIT(ID)) ? (1) : (0))

#define C_UNKNOWN_CHIP_ID (0x0000FFFF)

struct mt_chip_drv {
    /* raw information */
	unsigned int info_bit_mask;
	unsigned int (*get_chip_info)(unsigned int id);
};

typedef unsigned int (*chip_info_cb)(void);
struct mt_chip_drv *get_mt_chip_drv(void);


#endif
