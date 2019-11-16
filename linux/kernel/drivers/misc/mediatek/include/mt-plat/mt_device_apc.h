#ifndef _MTK_DEVICE_APC_H
#define _MTK_DEVICE_APC_H

typedef enum {
	E_L0 = 0,
	E_L1,
	E_L2,
	E_L3,
	E_MAX_APC_ATTR
} APC_ATTR;

typedef enum {
	E_DOMAIN_0 = 0,
	E_DOMAIN_1,
	E_DOMAIN_2,
	E_DOMAIN_3,
	E_DOMAIN_4,
	E_DOMAIN_5,
	E_DOMAIN_6,
	E_DOMAIN_7,
	E_MAX_DOMAIN
} E_MASK_DOM;

extern int mt_devapc_emi_initial(void);
extern int mt_devapc_check_emi_violation(void);
extern int mt_devapc_check_emi_mpu_violation(void);
extern int mt_devapc_clear_emi_violation(void);
extern int mt_devapc_clear_emi_mpu_violation(void);
extern int mt_devapc_set_permission(unsigned int, E_MASK_DOM domain, APC_ATTR attr);
#endif

