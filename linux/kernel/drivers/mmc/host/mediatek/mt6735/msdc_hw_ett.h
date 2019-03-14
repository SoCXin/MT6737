#ifndef __MSDC_HW_ETT_H
#define __MSDC_HW_ETT_H


typedef enum EMMC_CHIP_TAG {
	SAMSUNG_EMMC_CHIP = 0x15,
	SANDISK_EMMC_CHIP = 0x45,
	HYNIX_EMMC_CHIP = 0x90,
} EMMC_VENDOR_T;

#define MSDC0_ETT_COUNTS 20
#if 0
#define MSDC_SUPPORT_SANDISK_COMBO_ETT
#define MSDC_SUPPORT_SAMSUNG_COMBO_ETT
#endif
extern struct msdc_ett_settings msdc0_ett_settings[MSDC0_ETT_COUNTS];

#ifdef MSDC_SUPPORT_SANDISK_COMBO_ETT
extern struct msdc_ett_settings msdc0_ett_settings_for_sandisk[MSDC0_ETT_COUNTS];
#endif
#ifdef MSDC_SUPPORT_SAMSUNG_COMBO_ETT
extern struct msdc_ett_settings msdc0_ett_settings_for_samsung[MSDC0_ETT_COUNTS];
#endif

#endif
