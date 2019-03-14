LOCAL_DIR := $(GET_LOCAL_DIR)

DEFINES += BUILD_LK
DEFINES += SKIP_MTK_PARTITION_HEADER_CHECK \
           MTK_GPT_SCHEME_SUPPORT

PLATFORM := mt6735

MODULES += \
	dev/keys \
	lib/ptable \
	dev/lcm \

DUMMY_AP := no

ifeq ($(DUMMY_AP), yes)
MEMBASE := 0x50000000 # SDRAM
else
MEMBASE := 0x41E00000 # SDRAM
endif
MEMSIZE := 0x00900000 # 9MB

SCRATCH_ADDR     := 0x45000000

HAVE_CACHE_PL310 := no
MTK_LM_MODE := no
MTK_FASTBOOT_SUPPORT := yes
LK_PROFILING := yes
DEVICE_TREE_SUPPORT := yes
MTK_DLPT_SUPPORT := yes

MACH_TYPE := mt6735

DEFINES += \
	MEMBASE=$(MEMBASE)\
	SCRATCH_ADDR=$(SCRATCH_ADDR)\

#DEFINES += ENABLE_L2_SHARING

ifeq ($(DUMMY_AP), yes)
DEFINES += DUMMY_AP
endif

ifeq ($(HAVE_CACHE_PL310), yes)
DEFINES += HAVE_CACHE_PL310
endif

ifeq ($(MTK_LM_MODE), yes)
DEFINES += MTK_LM_MODE
endif

ifeq ($(DEVICE_TREE_SUPPORT), yes)
DEFINES += DEVICE_TREE_SUPPORT
endif

ifeq ($(MTK_FASTBOOT_SUPPORT), yes)
DEFINES += MTK_FASTBOOT_SUPPORT
endif

ifeq ($(LK_PROFILING), yes)
DEFINES += LK_PROFILING
endif

ifneq ($(filter user userdebug, $(TARGET_BUILD_VARIANT)),)
DEFINES += USER_BUILD
endif

ifeq ($(MTK_DLPT_SUPPORT), yes)
DEFINES += MTK_DLPT_SUPPORT
endif

INCLUDES += -I$(LOCAL_DIR)/include
INCLUDES += -I$(LOCAL_DIR)/include/target
INCLUDES += -I$(LOCAL_DIR)/inc

OBJS += \
        $(LOCAL_DIR)/init.o \
        $(LOCAL_DIR)/cust_msdc.o\
        $(LOCAL_DIR)/cust_display.o\
        $(LOCAL_DIR)/cust_leds.o\
        $(LOCAL_DIR)/power_off.o\
        $(LOCAL_DIR)/fastboot_oem_commands.o\
