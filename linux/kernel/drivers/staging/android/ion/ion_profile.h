#if !defined(__ION_PROFILE_H__)
#define __ION_PROFILE_H__

typedef enum {
	PROFILE_ALLOC = 0,
	PROFILE_FREE,
	PROFILE_SHARE,
	PROFILE_IMPORT,
	PROFILE_MAP_KERNEL,
	PROFILE_UNMAP_KERNEL,
	PROFILE_MAP_USER,
	PROFILE_UNMAP_USER,
	PROFILE_CUSTOM_IOC,
	PROFILE_GET_PHYS,
	PROFILE_DMA_CLEAN_RANGE,
	PROFILE_DMA_FLUSH_RANGE,
	PROFILE_DMA_INVALID_RANGE,
	PROFILE_DMA_CLEAN_ALL,
	PROFILE_DMA_FLUSH_ALL,
	PROFILE_DMA_INVALID_ALL,
	PROFILE_MAX,
} ION_PROFILE_TYPE;

#define ION_PROFILE

#ifndef ION_PROFILE
#define MMProfileLogEx(...)
#define MMProfileEnable(...)
#define MMProfileStart(...)
#define MMP_Event unsigned int
#define MMP_RootEvent 1
#else
#include <mmprofile.h>

extern void MMProfileEnable(int enable);
extern void MMProfileStart(int start);

extern void ion_profile_init(void);
#endif

extern MMP_Event ION_MMP_Events[PROFILE_MAX];

#endif
