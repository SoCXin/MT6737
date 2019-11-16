#ifndef __CMDQ_MMP_H__
#define __CMDQ_MMP_H__

/* #include <linux/mmprofile.h> */
#include "cmdq_core.h"
#include "mmprofile.h"

typedef struct {
	MMP_Event CMDQ;
	MMP_Event CMDQ_IRQ;
	MMP_Event thread_en;
	MMP_Event warning;
	MMP_Event loopBeat;
	MMP_Event autoRelease_add;
	MMP_Event autoRelease_done;
	MMP_Event consume_add;
	MMP_Event consume_done;
	MMP_Event alloc_task;
	MMP_Event wait_task;
	MMP_Event wait_thread;
	MMP_Event MDP_reset;
} CMDQ_MMP_Events_t;

void cmdq_mmp_init(void);
extern void MMProfileEnable(int enable);
extern void MMProfileStart(int start);
CMDQ_MMP_Events_t *cmdq_mmp_get_event(void);

#endif				/* #ifndef __CMDQ_MMP_H__ */
