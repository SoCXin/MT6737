# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
#
# MediaTek Inc. (C) 2011. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.

# Compatible with Ubuntu 12.04
SHELL := /bin/bash

.DELETE_ON_ERROR:
all:

HOST_OS ?= $(shell uname | tr '[A-Z]' '[a-z]')
##############################################################
# Including Neccesary Files
#

ifndef PRELOADER_OUT
  PRELOADER_OUT := .
endif
ifndef MTK_PROJECT
  $(error MTK_PROJECT is not defined)
else
  export MTK_PROJECT
endif


##############################################################
# Secure Library Building Control
#

export CREATE_SEC_LIB := FALSE

##############################################################
# Variable Initialization
#

export D_ROOT           := $(CURDIR)
export MTK_ROOT_OUT     := $(CURDIR)
export PWD              := $(CURDIR)
ALL                     ?= show_title show_internal_feature show_feature
ALL_PREGEN_FILE         :=
export MTK_ROOT_CUSTOM  := $(D_ROOT)/custom
export MTK_PATH_CUSTOM  := $(MTK_ROOT_CUSTOM)/$(MTK_PROJECT)
PROJECT_CONFIGS         := $(MTK_PATH_CUSTOM)/$(MTK_PROJECT).mk
include $(PROJECT_CONFIGS)
ifndef TARGET
  $(error TARGET is not defined)
else
  export TARGET
endif
ifndef MTK_PLATFORM
  $(error MTK_PLATFORM is not defined)
else
  export MTK_PLATFORM
  PLATFORM := $(shell echo $(MTK_PLATFORM) | tr A-Z a-z)
endif
export MTK_PATH_PLATFORM := $(D_ROOT)/platform/$(PLATFORM)


#
# Image Auth
#
CUSTOM_PATH             := $(MTK_PATH_CUSTOM)/security
MTK_AUTH_CUSTOM         := $(D_ROOT)/../../../../../../vendor/mediatek/proprietary/custom/$(MTK_PROJECT)/security
IMAGE_AUTH_CFG_FILE     := ${PRELOADER_OUT}/inc/KEY_IMAGE_AUTH.h
#IMAGE_AUTH_CONFIG_PATH  := ${CUSTOM_PATH}/image_auth
IMAGE_AUTH_CONFIG_PATH  := ${MTK_AUTH_CUSTOM}/image_auth
IMAGE_AUTH_KEY          := ${IMAGE_AUTH_CONFIG_PATH}/IMG_AUTH_KEY.ini
IMAGE_AUTH_CFG          := ${IMAGE_AUTH_CONFIG_PATH}/IMG_AUTH_CFG.ini
IMAGE_AUTH_KEY_EXIST    := $(if $(wildcard $(IMAGE_AUTH_KEY)),TRUE,FALSE)
ifeq ("$(IMAGE_AUTH_KEY_EXIST)","TRUE")
ALL                     += img_auth_info
ALL_PREGEN_FILE         += $(IMAGE_AUTH_CFG_FILE)
endif

#
# PL Version
#
PL_CFG_FILE             := ${CUSTOM_PATH}/pl_auth/PL_CFG.ini
PL_CFG_HDR              := ${PRELOADER_OUT}/inc/PL_CFG.h
PL_CFG_EXIST            := $(if $(wildcard $(PL_CFG_FILE)),TRUE,FALSE)
ifeq ("$(PL_CFG_EXIST)","TRUE")
ALL                     += pl_cfg_info
ALL_PREGEN_FILE         += $(PL_CFG_HDR)
endif

#
# SML Encode
#
SML_ENCODE_CFG_FILE     := ${PRELOADER_OUT}/inc/KEY_SML_ENCODE.h
#SML_CONFIG_PATH         := ${CUSTOM_PATH}/sml_auth
SML_CONFIG_PATH         := ${MTK_AUTH_CUSTOM}/sml_auth
SML_ENCODE_KEY          := ${SML_CONFIG_PATH}/SML_ENCODE_KEY.ini
SML_ENCODE_CFG          := ${SML_CONFIG_PATH}/SML_ENCODE_CFG.ini
SML_ENCODE_KEY_EXIST    := $(if $(wildcard $(SML_ENCODE_KEY)),TRUE,FALSE)
ifeq ("$(SML_ENCODE_KEY_EXIST)","TRUE")
ALL                     += sml_encode_info
ALL_PREGEN_FILE         += $(SML_ENCODE_CFG_FILE)
endif

#
# SML Auth
#
SML_AUTH_CFG_FILE       := ${PRELOADER_OUT}/inc/KEY_SML_AUTH.h
#SML_AUTH_PATH           := ${CUSTOM_PATH}/sml_auth
SML_AUTH_PATH           := ${MTK_AUTH_CUSTOM}/sml_auth
SML_AUTH_KEY            := ${SML_CONFIG_PATH}/SML_AUTH_KEY.ini
SML_AUTH_CFG            := ${SML_CONFIG_PATH}/SML_AUTH_CFG.ini
SML_AUTH_KEY_EXIST      := $(if $(wildcard $(SML_AUTH_KEY)),TRUE,FALSE)
ifeq ("$(SML_AUTH_KEY_EXIST)","TRUE")
ALL                     += sml_auth_info
ALL_PREGEN_FILE         += $(SML_AUTH_CFG_FILE)
endif

#
# preloader extension
#
PRELOADER_EXT_BIN       := $(MTK_ROOT_CUSTOM)/$(PLATFORM)/preloader/preloader_ext.bin
PRELOADER_EXT_BIN_EXIST := $(if $(wildcard $(PRELOADER_EXT_BIN)),TRUE,FALSE)

#
# Tool
#
EFUSE_PY_FILE			:= tools/efuse/efuse_bingen_v2.py
EFUSE_PY_DEF			:= tools/efuse/$(MTK_PLATFORM)/efuse_definition.xml
EFUSE_INPUT_FILE		:= $(D_ROOT)/../../../../../../vendor/mediatek/proprietary/custom/$(MTK_PROJECT)/security/efuse/input.xml
EFUSE_IMAGE_NAME        := efuse_$(MTK_PROJECT).img

TOOL_PATH               ?= ${D_ROOT}/../../../device/mediatek/build/build/tools

ifeq ($(HOST_OS),darwin)
SIGN_TOOL               := ${TOOL_PATH}/SignTool/SignTool_PL.darwin
CIPHER_TOOL             := ${TOOL_PATH}/CipherTool/CipherTool.darwin
else
SIGN_TOOL               := ${TOOL_PATH}/SignTool/SignTool_PL
CIPHER_TOOL             := ${TOOL_PATH}/CipherTool/CipherTool
endif

ifdef PL_MODE
PL_IMAGE_NAME           := preloader_$(MTK_PROJECT)_$(PL_MODE)
else
PL_IMAGE_NAME           := preloader_$(MTK_PROJECT)
endif

SECURITY_LIB            := $(MTK_PATH_PLATFORM)/src/SecLib.a
DA_VERIFY_LIB           := $(MTK_PATH_PLATFORM)/src/security/auth/DaVerifyLib.a
SEC_PLAT_LIB            := $(MTK_PATH_PLATFORM)/src/SecPlat.a
HW_CRYPTO_LIB           := $(MTK_PATH_PLATFORM)/src/HwCryptoLib.a
ALL                     += build_info project_info
ALL_PREGEN_FILE         += $(PRELOADER_OUT)/inc/preloader.h $(PRELOADER_OUT)/inc/proj_cfg.h
COMMON_DIR_MK           := $(D_ROOT)/build/common-dir.mak
COMMON_FILE_MK          := $(D_ROOT)/build/common.mak
export COMMON_DIR_MK COMMON_FILE_MK

ifeq ($(CREATE_SEC_LIB),TRUE)
    ALL                 += $(SECURITY_LIB) $(DA_VERIFY_LIB) $(SEC_PLAT_LIB)
    SUBDIRS             := $(MTK_PATH_PLATFORM)/src/
else
    ALL                 += preloader_bin
    SUBDIRS             := $(MTK_PATH_PLATFORM)/src/ $(MTK_PATH_CUSTOM)/
endif
ALL                     += dump_build_info

include build/debug.in build/setting.in
include $(MTK_PATH_PLATFORM)/makefile.mak
export EMIGEN_OUT := $(PRELOADER_OUT)
export DRVGEN_OUT := $(PRELOADER_OUT)
export PTGEN_OUT := $(PRELOADER_OUT)
-include tools/emigen/emigen.mk
-include tools/drvgen/drvgen.mk
-include tools/ptgen/$(MTK_PLATFORM)/ptgen.mk
ALL_PREGEN_FILE += $(EMIGEN_FILE_LIST)
ALL_PREGEN_FILE += $(DRVGEN_FILE_LIST)
ALL_PREGEN_FILE += $(PTGEN_FILE_LIST)
export ALL_DEPENDENCY_FILE := $(foreach m,$(MAKEFILE_LIST),$(if $(filter /% ~%,$(m)),$(m),$(D_ROOT)/$(m)))

.PHONY: clean
.PHONY: $(ALL)
all: $(ALL)


#########
# recursive
#
# $(1): subdir
# $(2): target flag
# $(3): dependency
define build-topdirs
$(2): $(3) FORCE
	@mkdir -p $$(dir $$@)
	$$(MAKE) -C $(1) --no-print-directory
endef

.PHONY: FORCE
FORCE:
SUBDIRS_FLAGS :=
$(foreach d,$(SUBDIRS),\
  $(eval r := $(patsubst $(D_ROOT)/%,%,$(patsubst %/,%,$(if $(filter /%,$(d)),,$(CURDIR)/)$(d))))\
  $(eval t := $(PRELOADER_OUT)/obj/$(subst /,_,$(r)).flag)\
  $(eval $(call build-topdirs,$(d),$(t),$(ALL_PREGEN_FILE)))\
  $(eval SUBDIRS_FLAGS += $(t))\
)


##############################################################
# ELF Generation
#
ifeq ($(CFG_PRELOADER_DRAM_USE), 1)
  PL_IMG_SECOND_PARTION_SECTION :=.pl_dram.text .pl_dram.data .pl_dram.rodata .pl_dram.start
  OBJONCHIP_FLAG := $(addprefix -R , $(PL_IMG_SECOND_PARTION_SECTION))
  OBJSECOND_FLAG := $(addprefix -j , $(PL_IMG_SECOND_PARTION_SECTION))
  PL_DRAM_IMAGE_NAME := $(PL_IMAGE_NAME)_dram
  PL_DRAM_HDRBIN_PATH := $(D_BIN)/hd_$(PL_DRAM_IMAGE_NAME).bin
  LDSCRIPT := $(D_BIN)/$(MTK_PROJECT)_gen.ld
else
  OBJONCHIP_FLAG :=
  OBJSECOND_FLAG :=
  PL_DRAM_IMAGE_NAME :=
  PL_DRAM_HDRBIN_PATH :=
endif

ifdef CFG_TEE_DRAM_SIZE
PL_DRAM_RESERVE_SIZE := $(CFG_TEE_DRAM_SIZE)
else
PL_DRAM_RESERVE_SIZE := 0
endif

ifdef CFG_PRELOADER_DRAM_USE
ifeq ($(CFG_PRELOADER_DRAM_USE), 1)
$(LDSCRIPT): $(LDSCRIPT_DRAM)
	@echo generating $@
	$(MKDIR)
	sed "s/%CFG_DRAM_SIZE%/$(CFG_DRAM_SIZE)/;s/%EXT_PRELOAER_DRAMSIZE%/$(EXT_PRELOAER_DRAMSIZE)/;s/%PL_DRAM_RESERVE_SIZE%/$(PL_DRAM_RESERVE_SIZE)/" < $< > $@
endif
endif

preloader_bin: $(D_BIN)/$(PL_IMAGE_NAME).bin $(PRELOADER_OUT)/report-codesize.csv $(PRELOADER_OUT)/codesize_profiling.csv

ifeq ($(MTK_EFUSE_WRITER_SUPPORT),yes)
	rm -rf $(PRELOADER_OUT)/../../efuse.img
	rm -rf $(PRELOADER_OUT)/efuse_bingen.log
	$(PBP_TOOL) -func keyhash -i $(CHIP_KEY) -o $(D_BIN)/keyhash.txt
	python $(EFUSE_PY_FILE) -f $(EFUSE_INPUT_FILE) -d $(EFUSE_PY_DEF) -k $(D_BIN)/keyhash.txt -o $(D_BIN)/$(EFUSE_IMAGE_NAME) -l $(PRELOADER_OUT)/efuse_bingen.log
	rm -rf $(D_BIN)/keyhash.txt
	cp -f $(D_BIN)/$(EFUSE_IMAGE_NAME) $(PRELOADER_OUT)/../../efuse.img
endif
$(D_BIN)/$(PL_IMAGE_NAME).elf: $(SUBDIRS_FLAGS) $(wildcard $(DA_VERIFY_LIB) $(SEC_PLAT_LIB) $(HW_CRYPTO_LIB)) $(LDSCRIPT)
$(D_BIN)/$(PL_IMAGE_NAME).elf: $(ALL_DEPENDENCY_FILE)
	@mkdir -p $(dir $@)
	$(LD) --gc-sections -Bstatic -T$(LDSCRIPT) \
	`find $(PRELOADER_OUT)/obj/ -type f -name "*.o" | LC_COLLATE=C sort` $(SECURITY_LIB) \
	--start-group \
	$(wildcard $(DA_VERIFY_LIB)) \
	$(SEC_PLAT_LIB) \
	$(wildcard $(HW_CRYPTO_LIB)) \
	--end-group \
	-Map $(PRELOADER_OUT)/system.map -o $(D_BIN)/$(PL_IMAGE_NAME).elf

$(D_BIN)/preloader.elf: $(D_BIN)/$(PL_IMAGE_NAME).elf
	$(OBJCOPY) -R .dram $(D_BIN)/$(PL_IMAGE_NAME).elf -O elf32-littlearm $(D_BIN)/preloader.elf

ifeq ($(CFG_PRELOADER_DRAM_USE), 1)
preloader_bin: $(D_BIN)/$(PL_DRAM_IMAGE_NAME).bin
$(D_BIN)/$(PL_DRAM_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME).elf
	$(hide) $(OBJCOPY) ${OBJCFLAGS} $(OBJSECOND_FLAG) $(D_BIN)/$(PL_IMAGE_NAME).elf -O binary $(D_BIN)/$(PL_DRAM_IMAGE_NAME).bin

endif

$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin: $(D_BIN)/$(PL_IMAGE_NAME).elf
ifeq ($(PRELOADER_EXT_BIN_EXIST),TRUE)
$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin: $(PRELOADER_EXT_BIN)
endif
$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin: $(PRELOADER_OUT)/MTK_Loader_Info.tag
$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin: zero_padding.sh
$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin: $(PRELOADER_OUT)/report-objsize.txt
$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin: $(PRELOADER_OUT)/report-codesize.txt
$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin: $(PRELOADER_OUT)/function.map
ifeq ($(CFG_PRELOADER_DRAM_USE), 1)
	$(hide) $(OBJCOPY) $(OBJCFLAGS) $(OBJONCHIP_FLAG) $(D_BIN)/$(PL_IMAGE_NAME).elf -O binary $@
else
	$(hide) $(OBJCOPY) $(OBJCFLAGS) $(D_BIN)/$(PL_IMAGE_NAME).elf -O binary $@
endif
ifeq ("$(PRELOADER_EXT_BIN_EXIST)","TRUE")
	cat $(PRELOADER_EXT_BIN) >> $@
endif
	./zero_padding.sh $@ 4
	./alignment_check.sh $(PRELOADER_OUT)/MTK_Loader_Info.tag 4
	cat $(PRELOADER_OUT)/MTK_Loader_Info.tag >> $@
	PL_IMAGE=$@ PL_FUN_MAP=$(PRELOADER_OUT)/function.map PRELOADER_OUT=$(PRELOADER_OUT) $(SHELL) $(MTK_PATH_PLATFORM)/check_size.sh

$(PRELOADER_OUT)/report-codesize.txt: $(D_BIN)/$(PL_IMAGE_NAME).elf
	$(CROSS_COMPILE)readelf -s $(D_BIN)/$(PL_IMAGE_NAME).elf | awk -F':' '/Num/ {print $$2}' > $@
	$(CROSS_COMPILE)readelf -s $(D_BIN)/$(PL_IMAGE_NAME).elf | awk -F':' '{if ($$1>0) print $$2}' | awk -F' ' '{if ($$2>0) print $$0}' | sort -k1 >> $@

$(PRELOADER_OUT)/report-codesize.csv: $(PRELOADER_OUT)/report-codesize.txt
	cat $(PRELOADER_OUT)/report-codesize.txt | sed 's/[ ]+/,/g' > $@

$(PRELOADER_OUT)/report-objsize.txt: $(D_BIN)/$(PL_IMAGE_NAME).elf
	$(CROSS_COMPILE)size `ls $(PRELOADER_OUT)/obj/*.o` > $@

$(PRELOADER_OUT)/codesize_profiling.csv: $(D_BIN)/$(PL_IMAGE_NAME).elf
$(PRELOADER_OUT)/codesize_profiling.csv: codesize_profiling.sh
	./codesize_profiling.sh $(PRELOADER_OUT)/system.map $@

##############################################################
# Security Library Generation
#

ifeq ($(CREATE_SEC_LIB),TRUE)
$(SECURITY_LIB): $(SUBDIRS_FLAGS)
	rm -rf $@
	@echo AR $(PRELOADER_OUT)/out/*
	$(AR) -r $@ $(PRELOADER_OUT)/out/*
	@echo =================================================================
	@echo Security Library
	@echo '$@' built at
	@echo time : $(shell date)
	@echo =================================================================

$(DA_VERIFY_LIB): $(SUBDIRS_FLAGS)
	rm -rf $@
	@echo AR $(PRELOADER_OUT)/out_da_verify/*
	$(AR) -r $@ $(PRELOADER_OUT)/out_da_verify/*
	@echo =================================================================
	@echo DA Verify Library
	@echo '$@' built at
	@echo time : $(shell date)
	@echo =================================================================

$(SEC_PLAT_LIB): $(SUBDIRS_FLAGS)
	rm -rf $@
	@echo AR $(PRELOADER_OUT)/out_plat/*
	$(AR) -r $@ $(PRELOADER_OUT)/out_plat/*
	@echo =================================================================
	@echo Security Library
	@echo '$@' built at
	@echo time : $(shell date)
	@echo =================================================================
endif

##############################################################
# File for Debugging
#

$(PRELOADER_OUT)/function.map: $(D_BIN)/$(PL_IMAGE_NAME).elf
	$(NM) $< | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | sort > $@

##############################################################
# Dump Configurations
#

show_title:
	@echo =================================================================
	@echo Building Configuration:
	@echo Project              = $(MTK_PROJECT)
	@echo Platform             = $(MTK_PLATFORM)
	@echo Buildspec            = buildspec.mak
	@echo Create SecLib        = $(CREATE_SEC_LIB)
	@echo Image Auth key exist = $(IMAGE_AUTH_KEY_EXIST)
	@echo SML Encode key exist = $(SML_ENCODE_KEY_EXIST)
	@echo SML Auth key exist   = $(SML_AUTH_KEY_EXIST)
	@echo Preloader EXT exist  = $(PRELOADER_EXT_BIN_EXIST)
	@echo SECRO AC support     = $(MTK_SEC_SECRO_AC_SUPPORT)
	@echo =================================================================

show_internal_feature:
	@echo =================================================================
	@echo Internal Feature:
	@echo HW_INIT_ONLY         = $(HW_INIT_ONLY)
	@echo CFG_MDJTAG_SWITCH= $(CFG_MDJTAG_SWITCH)
	@echo CFG_MDMETA_DETECT= $(CFG_MDMETA_DETECT)
	@echo CFG_HW_WATCHDOG= $(CFG_HW_WATCHDOG)
	@echo CFG_APWDT_DISABLE= $(CFG_APWDT_DISABLE)
	@echo CFG_MDWDT_DISABLE= $(CFG_MDWDT_DISABLE)
	@echo CFG_SYS_STACK_SZ= $(CFG_SYS_STACK_SZ)
	@echo CFG_MMC_ADDR_TRANS= $(CFG_MMC_ADDR_TRANS)
	@echo CFG_BOOT_ARGUMENT= $(CFG_BOOT_ARGUMENT)
	@echo CFG_RAM_CONSOLE= $(CFG_RAM_CONSOLE)
	@echo =================================================================
	@echo MTK_PATH_PLATFORM= $(MTK_PATH_PLATFORM)
	@echo MTK_PATH_CUSTOM= $(MTK_PATH_CUSTOM)
	@echo MTK_ROOT_CUSTOM= $(MTK_ROOT_CUSTOM)
	@echo CUSTOM_PATH= $(CUSTOM_PATH)
	@echo =================================================================

show_feature:
	@if [ -e "$(MTK_PATH_CUSTOM)/cust_bldr.mak" ]; then \
	echo  =========================================== ; \
	echo Platform Feature: ; \
	echo 'CFG_FPGA_PLATFORM'= $(CFG_FPGA_PLATFORM) ; \
	echo 'CFG_EVB_PLATFORM'= $(CFG_EVB_PLATFORM) ; \
	echo 'CFG_BATTERY_DETECT'= $(CFG_BATTERY_DETECT) ; \
	echo 'CFG_PMT_SUPPORT'= $(CFG_PMT_SUPPORT) ; \
	echo  =========================================== ; \
	echo Communication Feature: ; \
	echo 'CFG_UART_TOOL_HANDSHAKE'= $(CFG_UART_TOOL_HANDSHAKE) ; \
	echo 'CFG_USB_TOOL_HANDSHAKE'= $(CFG_USB_TOOL_HANDSHAKE) ; \
	echo 'CFG_USB_DOWNLOAD'= $(CFG_USB_DOWNLOAD) ; \
	echo 'CFG_LOG_BAUDRATE'= $(CFG_LOG_BAUDRATE) ; \
	echo 'CFG_META_BAUDRATE'= $(CFG_META_BAUDRATE) ; \
	echo 'CFG_UART_LOG'= $(CFG_UART_LOG) ; \
	echo 'CFG_UART_META'= $(CFG_UART_META) ; \
	echo 'CFG_EMERGENCY_DL_SUPPORT'= $(CFG_EMERGENCY_DL_SUPPORT) ; \
	echo 'CFG_EMERGENCY_DL_TIMEOUT_MS'= $(CFG_EMERGENCY_DL_TIMEOUT_MS) ; \
	echo 'CFG_USB_UART_SWITCH'= $(CFG_USB_UART_SWITCH) ; \
	echo  =========================================== ; \
	echo Image Loading: ; \
	echo 'CFG_LOAD_UBOOT'= $(CFG_LOAD_UBOOT) ; \
	echo 'CFG_LOAD_AP_ROM'= $(CFG_LOAD_AP_ROM) ; \
	echo 'CFG_LOAD_MD_ROM'= $(CFG_LOAD_MD_ROM) ; \
	echo 'CFG_LOAD_MD_RAMDISK'= $(CFG_LOAD_MD_RAMDISK) ; \
	echo 'CFG_LOAD_CONN_SYS'= $(CFG_LOAD_CONN_SYS) ; \
	echo 'CFG_UBOOT_MEMADDR'= $(CFG_UBOOT_MEMADDR) ; \
	echo 'CFG_AP_ROM_MEMADDR'= $(CFG_AP_ROM_MEMADDR) ; \
	echo 'CFG_MD1_ROM_MEMADDR'= $(CFG_MD1_ROM_MEMADDR) ; \
	echo 'CFG_MD1_RAMDISK_MEMADDR'= $(CFG_MD1_RAMDISK_MEMADDR) ; \
	echo 'CFG_MD2_ROM_MEMADDR'= $(CFG_MD2_ROM_MEMADDR) ; \
	echo 'CFG_MD2_RAMDISK_MEMADDR'= $(CFG_MD2_RAMDISK_MEMADDR) ; \
	echo 'CFG_CONN_SYS_MEMADDR'= $(CFG_CONN_SYS_MEMADDR) ; \
	echo 'ONEKEY_REBOOT_NORMAL_MODE_PL'= $(ONEKEY_REBOOT_NORMAL_MODE_PL) ; \
	echo 'KPD_USE_EXTEND_TYPE'= $(KPD_USE_EXTEND_TYPE) ; \
	echo 'KPD_PMIC_LPRST_TD'= $(KPD_PMIC_LPRST_TD) ; \
	echo 'MTK_PMIC_RST_KEY'= $(MTK_PMIC_RST_KEY) ; \
	echo 'CFG_ATF_SUPPORT'= $(CFG_ATF_SUPPORT) ; \
	echo 'CFG_ATF_LOG_SUPPORT'= $(CFG_ATF_LOG_SUPPORT) ; \
	echo 'CFG_TEE_SUPPORT'= $(CFG_TEE_SUPPORT) ; \
	echo 'CFG_TRUSTONIC_TEE_SUPPORT'= $(CFG_TRUSTONIC_TEE_SUPPORT) ; \
	echo 'CFG_MICROTRUST_TEE_SUPPORT'= $(CFG_MICROTRUST_TEE_SUPPORT) ; \
	echo 'CFG_TEE_SECURE_MEM_PROTECTED'= $(CFG_TEE_SECURE_MEM_PROTECTED) ; \
	echo 'CFG_TEE_TRUSTED_APP_HEAP_SIZE'= $(CFG_TEE_TRUSTED_APP_HEAP_SIZE) ; \
	echo 'MTK_EFUSE_WRITER_SUPPORT'= $(MTK_EFUSE_WRITER_SUPPORT) ; \
	echo 'CFG_GOOGLE_TRUSTY_SUPPORT'= $(CFG_GOOGLE_TRUSTY_SUPPORT) ; \
	fi
##############################################################
# Adding Build Time
#

build_info: $(PRELOADER_OUT)/inc/preloader.h
$(PRELOADER_OUT)/inc/preloader.h: $(ALL_DEPENDENCY_FILE)
	@mkdir -p $(dir $@)
	@echo // Auto generated. Build Time Information > $@
	@echo '#'define BUILD_TIME '"'$(shell date +%Y%m%d-%H%M%S)'"' >> $@


##############################################################
# Adding Project Configuration
#

project_info: $(PRELOADER_OUT)/inc/proj_cfg.h
$(PRELOADER_OUT)/inc/proj_cfg.h: $(MTK_PATH_PLATFORM)/default.mak $(MTK_PATH_CUSTOM)/cust_bldr.mak $(PROJECT_CONFIGS)
$(PRELOADER_OUT)/inc/proj_cfg.h: $(ALL_DEPENDENCY_FILE)
	@mkdir -p $(dir $@)
	@echo // Auto generated. Import ProjectConfig.mk > $@
	@echo '#'define CUSTOM_SUSBDL_CFG $(MTK_SEC_USBDL) >> $@
	@echo '#'define CUSTOM_SBOOT_CFG $(MTK_SEC_BOOT) >> $@
	@echo '#'define MTK_SEC_MODEM_AUTH $(MTK_SEC_MODEM_AUTH) >> $@
ifdef MTK_SEC_SECRO_AC_SUPPORT
	@echo '#'define MTK_SEC_SECRO_AC_SUPPORT $(MTK_SEC_SECRO_AC_SUPPORT) >> $@
endif

##############################################################
# Generate Key Info File
#

img_auth_info: $(IMAGE_AUTH_CFG_FILE)
$(IMAGE_AUTH_CFG_FILE): $(SIGN_TOOL) $(IMAGE_AUTH_KEY) $(IMAGE_AUTH_CFG)
$(IMAGE_AUTH_CFG_FILE): $(ALL_DEPENDENCY_FILE)
	@mkdir -p $(dir $@)
	@touch $@
	@chmod 777 $@
	$(SIGN_TOOL) $(IMAGE_AUTH_KEY) $(IMAGE_AUTH_CFG) $@ IMG

sml_encode_info: $(SML_ENCODE_CFG_FILE)
$(SML_ENCODE_CFG_FILE): $(CIPHER_TOOL) $(SML_ENCODE_KEY) $(SML_ENCODE_CFG)
$(SML_ENCODE_CFG_FILE): $(ALL_DEPENDENCY_FILE)
	@mkdir -p $(dir $@)
	@touch $@
	@chmod 777 $@
	$(CIPHER_TOOL) GEN_HEADER $(SML_ENCODE_KEY) $(SML_ENCODE_CFG) $@ SML

sml_auth_info: $(SML_AUTH_CFG_FILE)
$(SML_AUTH_CFG_FILE): $(SIGN_TOOL) $(SML_AUTH_KEY) $(SML_AUTH_CFG)
$(SML_AUTH_CFG_FILE): $(ALL_DEPENDENCY_FILE)
	@mkdir -p $(dir $@)
	@touch $@
	@chmod 777 $@
	$(SIGN_TOOL) $(SML_AUTH_KEY) $(SML_AUTH_CFG) $@ SML

pl_cfg_info: $(PL_CFG_HDR)
$(PL_CFG_HDR): $(PL_CFG_FILE)
	@mkdir -p $(dir $@)
	@touch $@
	@chmod 777 $@
	echo '#'define $(shell cat $(PL_CFG_FILE) | sed -s 's/=//g') > $@

##############################################################
# Clean
#

clean:
	@rm -rf $(PRELOADER_OUT)/obj
	@rm -rf $(PRELOADER_OUT)/inc
	@rm -rf $(PRELOADER_OUT)/bin
	@rm -rf $(PRELOADER_OUT)/out_da_verify
	@rm -rf $(PRELOADER_OUT)/out_plat
	@rm -rf $(PRELOADER_OUT)/out_hw_crypto_lib


#
# Binary Secure Postprocessing
#
ifeq ($(HOST_OS),darwin)
PBP_TOOL := tools/pbp/PBP.darwin
else
PBP_TOOL := tools/pbp/PBP
endif
CHIP_CONFIG_PATH := $(CUSTOM_PATH)/chip_config

#
# Encode Key
#
ifeq ($(HOST_OS),darwin)
KEY_ENCODE_TOOL := tools/ke/KeyEncode.darwin
else
KEY_ENCODE_TOOL := tools/ke/KeyEncode
endif

ifneq ($(wildcard $(PBP_TOOL)),)
  #key_encode
  ifneq ($(wildcard $(KEY_ENCODE_TOOL)),)

$(D_BIN)/KEY_ENCODED_PL: $(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin | $(KEY_ENCODE_TOOL)
	$(KEY_ENCODE_TOOL) $< $@
	@echo "$(KEY_ENCODE_TOOL) pass !!!!"

  else

$(D_BIN)/KEY_ENCODED_PL: $(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin
	cp -f $< $@

  endif

$(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin: $(D_BIN)/KEY_ENCODED_PL
	@echo "[ Load Chip Config. ]"
	@echo "============================================"
	@echo " : MTK_SEC_CHIP_SUPPORT - $(MTK_SEC_CHIP_SUPPORT)"
	cp -f $< $@

  ifeq ($(strip $(MTK_SEC_CHIP_SUPPORT)),no)
    CHIP_CONFIG_PATH := $(MTK_PATH_PLATFORM)/gfh/default
    #ns_chip
    GFH_PATH := $(CHIP_CONFIG_PATH)/ns
    ifneq ($(wildcard $(GFH_PATH)/GFH_CONFIG.ini),)
      #ns_chip_ini

$(D_BIN)/$(PL_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin $(PBP_TOOL) $(GFH_PATH)/GFH_CONFIG.ini
	@echo "[ Only for Non-Secure Chip ]"
	@echo "============================================"
	@echo "INI_GFH_GEN=YES"
	cp -f $< $@
	$(PBP_TOOL) -g $(GFH_PATH)/GFH_CONFIG.ini $@
	@echo "$(PBP_TOOL) pass !!!!"

    else
      #ns_chip_legacy
      ifeq ($(MTK_EMMC_SUPPORT),yes)
        GFH_INFO := $(GFH_PATH)/GFH_INFO_EMMC.txt
      else
        GFH_INFO := $(GFH_PATH)/GFH_INFO.txt
      endif
      GFH_HASH := $(GFH_PATH)/GFH_HASH.txt

$(D_BIN)/$(PL_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin $(GFH_INFO) $(GFH_HASH) $(PBP_TOOL)
	@echo "[ Only for Non-Secure Chip ]"
	@echo "============================================"
	@echo "INI_GFH_GEN=NO"
	@echo "[ Attach $(MTK_PLATFORM) GFH ]"
	@echo "============================================"
	@echo " : GFH_INFO             - $(GFH_INFO)"
	@echo " : GFH_HASH             - $(GFH_HASH)"
	cp -f $(GFH_INFO) $@
	@chmod 777 $@
	cat $< >> $@
	cat $(GFH_HASH) >> $@
	$(PBP_TOOL) $@
	@echo "$(PBP_TOOL) pass !!!!"

    endif
  else ifeq ($(strip $(MTK_SEC_CHIP_SUPPORT)),yes)
    CHIP_CONFIG_PATH := $(CUSTOM_PATH)/chip_config
    #s_chip_support
    GFH_PATH := $(CHIP_CONFIG_PATH)/s/gfh
    CONFIG_PATH := $(CHIP_CONFIG_PATH)/s/cfg
    KEY_PATH := $(CHIP_CONFIG_PATH)/s/key
    CHIP_CONFIG := $(CONFIG_PATH)/CHIP_CONFIG.ini
    CHIP_KEY := $(KEY_PATH)/CHIP_TEST_KEY.ini
    ifneq ($(wildcard $(GFH_PATH)/GFH_CONFIG.ini),)
      #s_chip_ini

$(D_BIN)/$(PL_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin $(PBP_TOOL) $(CHIP_CONFIG) $(CHIP_KEY) $(GFH_PATH)/GFH_CONFIG.ini
	@echo "[ Enable Secure Chip Support ]"
	@echo "============================================"
	@echo "INI_GFH_GEN=YES"
	cp -f $< $@
	$(PBP_TOOL) -m $(CHIP_CONFIG) -i $(CHIP_KEY) -g $(GFH_PATH)/GFH_CONFIG.ini $@
	@echo "$(PBP_TOOL) pass !!!!"

    else
      #s_chip_legacy
      ifeq ($(MTK_EMMC_SUPPORT),yes)
        GFH_INFO := $(GFH_PATH)/GFH_INFO_EMMC.txt
      else
        GFH_INFO := $(GFH_PATH)/GFH_INFO.txt
      endif
      GFH_SEC_KEY := $(GFH_PATH)/GFH_SEC_KEY.txt
      GFH_ANTI_CLONE := $(GFH_PATH)/GFH_ANTI_CLONE.txt
      GFH_HASH_SIGNATURE := $(GFH_PATH)/GFH_HASH_AND_SIG.txt
      GFH_PADDING := $(GFH_PATH)/GFH_PADDING.txt
      include $(CONFIG_PATH)/SECURE_JTAG_CONFIG.ini
      ifeq ($(strip $(SECURE_JTAG_ENABLE)),TRUE)
        SECURE_JTAG_GFH := $(GFH_PATH)/GFH_SEC_CFG_JTAG_ON.txt
      else ifeq ($(strip $(SECURE_JTAG_ENABLE)),FALSE)
        SECURE_JTAG_GFH := $(GFH_PATH)/GFH_SEC_CFG_JTAG_OFF.txt
      else
        $(error BUILD FAIL. Unexpected SECURE_JTAG_ENABLE=$(SECURE_JTAG_ENABLE) in $(CONFIG_PATH)/SECURE_JTAG_CONFIG.ini)
      endif

$(D_BIN)/$(PL_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME)_NO_GFH.bin $(GFH_INFO) $(GFH_SEC_KEY) $(GFH_ANTI_CLONE) $(SECURE_JTAG_GFH) $(GFH_PADDING) $(GFH_HASH_SIGNATURE) $(PBP_TOOL) $(CHIP_CONFIG) $(CHIP_KEY) $(CONFIG_PATH)/SECURE_JTAG_CONFIG.ini
	@echo "[ Enable Secure Chip Support ]"
	@echo "============================================"
	@echo "INI_GFH_GEN=NO"
	@echo " : SECURE_JTAG_ENABLE - $(SECURE_JTAG_ENABLE)"
	@echo "[ Attach $(MTK_PLATFORM) GFH ]"
	@echo "============================================"
	@echo " : GFH_INFO             - $(GFH_INFO)"
	@echo " : GFH_SEC_KEY          - $(GFH_SEC_KEY)"
	@echo " : GFH_ANTI_CLONE       - $(GFH_ANTI_CLONE)"
	@echo " : GFH_JTAG_CFG         - $(SECURE_JTAG_GFH)"
	@echo " : GFH_PADDING          - $(GFH_PADDING)"
	@echo " : GFH_HASH_SIGNATURE   - $(GFH_HASH_SIGNATURE)"
	cp -f $(GFH_INFO) $@
	@chmod 777 $@
	cat $(GFH_SEC_KEY) >> $@
	cat $(GFH_ANTI_CLONE) >> $@
	cat $(SECURE_JTAG_GFH) >> $@
	cat $(GFH_PADDING) >> $@
	cat $< >> $@
	cat $(GFH_HASH_SIGNATURE) >> $@
	@echo "[ Load Configuration ]"
	@echo "============================================"
	@echo " : CONFIG               - $(CHIP_CONFIG)"
	@echo " : RSA KEY              - $(CHIP_KEY)"
	@echo " : AC_K                 - $(CHIP_KEY)"
	$(PBP_TOOL) -m $(CHIP_CONFIG) -i $(CHIP_KEY) $@
	@echo "$(PBP_TOOL) pass !!!!"

    endif
  else
    $(error BUILD FAIL. Unexpected MTK_SEC_CHIP_SUPPORT=$(MTK_SEC_CHIP_SUPPORT) in $(PROJECT_CONFIGS))
  endif
else
$(D_BIN)/$(PL_IMAGE_NAME).bin: $(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin
	cp -f $< $@

endif

dump_build_info: $(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin $(D_BIN)/$(PL_IMAGE_NAME).elf
	@echo "============================================"
	@echo "${MTK_PROJECT} preloader load"
	@echo "$(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin built at"
	@echo "time : $(shell date)"
	@echo "img  size : `wc -c < $(D_BIN)/$(PL_IMAGE_NAME)_LINKED.bin` byte"
	@echo "bss  size : 0x`$(CROSS_COMPILE)readelf -SW $(D_BIN)/$(PL_IMAGE_NAME).elf | grep bss | awk '{if (NF==11) print $$6; else print $$7;}'` byte"
	@echo "dram size : 0x`$(CROSS_COMPILE)readelf -SW $(D_BIN)/$(PL_IMAGE_NAME).elf | grep dram | awk '{if (NF==11) print $$6; else print $$7;}'` byte"
	@echo ============================================

