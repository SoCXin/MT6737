#ifndef _MT_SLEEP_
#define _MT_SLEEP_

#include <linux/kernel.h>
#include "mt_spm.h"
#include "mt_spm_sleep.h"

#define WAKE_SRC_CFG_KEY            (1U << 31)

extern int slp_set_wakesrc(u32 wakesrc, bool enable, bool ck26m_on);

extern wake_reason_t slp_get_wake_reason(void);
extern bool slp_will_infra_pdn(void);
extern void slp_pasr_en(bool en, u32 value);
extern void slp_dpd_en(bool en);

extern void slp_set_auto_suspend_wakelock(bool lock);
extern void slp_start_auto_suspend_resume_timer(u32 sec);
extern void slp_create_auto_suspend_resume_thread(void);

extern void slp_module_init(void);

#endif
