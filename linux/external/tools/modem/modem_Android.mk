define mtk-install-modem
include $$(CLEAR_VARS)
LOCAL_MODULE := $$(notdir $(1))
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(2)
LOCAL_SRC_FILES := $(1)
include $$(BUILD_PREBUILT)
MTK_MODEM_INSTALLED_MODULES += $$(LOCAL_INSTALLED_MODULE)
MTK_MODEM_BUILT_MODULES += $$(LOCAL_BUILT_MODULE)
endef

MTK_MODEM_INSTALLED_MODULES :=
MTK_MODEM_BUILT_MODULES :=
MTK_MODEM_DATABASE_FILES :=

ifneq ($(strip $(CUSTOM_MODEM)),)
LOCAL_PATH := $(call my-dir)
ifeq ($(strip $(MTK_INTERNAL)),yes)
  MTK_MODEM_USER_SUFFIX ?= _prod
  ifeq ($(strip $(TARGET_BUILD_VARIANT)),eng)
    MTK_MODEM_MODULE_MAKEFILES := $(foreach item,$(CUSTOM_MODEM),$(firstword $(wildcard $(LOCAL_PATH)/$(patsubst %$(MTK_MODEM_USER_SUFFIX),%,$(item))/Android.mk $(LOCAL_PATH)/$(item)/Android.mk)))
  else
    MTK_MODEM_MODULE_MAKEFILES := $(foreach item,$(CUSTOM_MODEM),$(firstword $(wildcard $(LOCAL_PATH)/$(patsubst %$(MTK_MODEM_USER_SUFFIX),%,$(item))$(MTK_MODEM_USER_SUFFIX)/Android.mk $(LOCAL_PATH)/$(item)/Android.mk)))
  endif
else
  MTK_MODEM_MODULE_MAKEFILES := $(foreach item,$(CUSTOM_MODEM),$(wildcard $(LOCAL_PATH)/$(item)/Android.mk))
endif
$(info CUSTOM_MODEM = $(CUSTOM_MODEM))
$(info including $(MTK_MODEM_MODULE_MAKEFILES) ...)
include $(wildcard $(MTK_MODEM_MODULE_MAKEFILES))
ALL_DEFAULT_INSTALLED_MODULES += $(MTK_MODEM_DATABASE_FILES)

LOCAL_PATH := device/mediatek/build/build/tools/modem
MTK_MODEM_PARTITION_FILES := $(foreach item,md1rom.img md1dsp.img md1arm7.img md3rom.img,$(if $(filter %/$(item),$(MTK_MODEM_DATABASE_FILES)),,$(if $(wildcard $(LOCAL_PATH)/$(item)),$(item))))
$(info Use default MTK_MODEM_PARTITION_FILES for $(strip $(MTK_MODEM_PARTITION_FILES)))
$(foreach item,$(MTK_MODEM_PARTITION_FILES),$(eval $(call mtk-install-modem,$(item),$(PRODUCT_OUT))))
ALL_DEFAULT_INSTALLED_MODULES += $(MTK_MODEM_INSTALLED_MODULES)

  ifneq ($(filter update-modem,$(MAKECMDGOALS)),)
MTK_MODEM_REMOVED_MODULES := $(filter-out $(MTK_MODEM_INSTALLED_MODULES),$(wildcard $(TARGET_OUT_ETC)/firmware/modem*.img $(TARGET_OUT_ETC)/mddb/*))
.PHONY: update-modem
.PHONY: $(MTK_MODEM_BUILT_MODULES)
.PHONY: $(MTK_MODEM_INSTALLED_MODULES)
    ifeq ($(filter-out $(INTERNAL_MODIFIER_TARGETS) update-modem,$(MAKECMDGOALS)),)
MAKECMDGOALS := $(filter-out update-modem,$(MAKECMDGOALS))
update-modem: snod
snod: $(MTK_MODEM_INSTALLED_MODULES)
    endif
  endif
  ifneq ($(filter clean-modem,$(MAKECMDGOALS)),)
MTK_MODEM_REMOVED_MODULES := $(wildcard $(MTK_MODEM_INSTALLED_MODULES) $(TARGET_OUT_ETC)/firmware/modem*.img $(TARGET_OUT_ETC)/mddb/*)
.PHONY: clean-modem
clean-modem:

  endif
  ifneq ($(strip $(MTK_MODEM_REMOVED_MODULES)),)
$(info clean-modem: $(MTK_MODEM_REMOVED_MODULES))
MTK_TEMP := $(shell rm -f $(MTK_MODEM_REMOVED_MODULES))
  endif

endif

