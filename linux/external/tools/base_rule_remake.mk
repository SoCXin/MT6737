#remake hook
ifeq ($(MTK_INTERNAL),yes)
  MTK_REMAKE_FLAG ?= no
endif
ifeq (,$(strip $(OUT_DIR)))
  ifeq (,$(strip $(OUT_DIR_COMMON_BASE)))
    MTK_REAMKE_MODULE_DEPENDENCIES_OUT := $(TOPDIR)out/target/product/$(MTK_TARGET_PROJECT)/obj/REMAKE_DEP
  else
    MTK_REAMKE_MODULE_DEPENDENCIES_OUT := $(OUT_DIR_COMMON_BASE)/$(notdir $(PWD))/target/product/$(MTK_TARGET_PROJECT)/obj/REMAKE_DEP
  endif
else
    MTK_REAMKE_MODULE_DEPENDENCIES_OUT := $(strip $(OUT_DIR))/target/product/$(MTK_TARGET_PROJECT)/obj/REMAKE_DEP
endif
MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE := $(MTK_REAMKE_MODULE_DEPENDENCIES_OUT)/previous_ProjectConfig.txt
MTK_REMAKE_PREVIOUS_FEATURE_OPTION_FILE := $(MTK_REAMKE_MODULE_DEPENDENCIES_OUT)/previous_FeatureOption.txt


define gather-all-products
$(sort $(foreach p, \
	$(eval _all_products_visited := )
	$(call all-products-inner, $(ALL_PRODUCTS)) \
	, $(if $(strip $(p)),$(strip $(p)),)) \
)
endef

define all-products-inner
	$(foreach p,$(1),\
		$(if $(filter $(p),$(_all_products_visited)),, \
			$(p) \
			$(eval _all_products_visited += $(p)) \
			$(call all-products-inner, $(PRODUCTS.$(strip $(p)).INHERITS_FROM))
		) \
	)
endef
really_all_products := $(call gather-all-products)
MTK_REMAKE_CURRENT_PROJECT_CONFIG_DEPENDENCIES := $(filter-out $(BUILD_SYSTEM)/% out/% %/base_rule_hook.mk %/base_rule_remake.mk,$(MAKEFILE_LIST) $(really_all_products))
MTK_REMAKE_CHANGED_PROJECT_CONFIG_OPTIONS :=
MTK_REMAKE_DELETED_PROJECT_CONFIG_OPTIONS :=
MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS :=
MTK_REMAKE_DELETED_PRODUCT_CONFIG_OPTIONS :=
MTK_REMAKE_ADDED_PRODUCT_CONFIG_OPTIONS :=

ifeq ($(strip $(MTK_REMAKE_FLAG)),yes)
MTK_REMAKE_CURRENT_PROJECT_CONFIG_OPTIONS := $(shell cat $(MTK_REMAKE_CURRENT_PROJECT_CONFIG_DEPENDENCIES) | sed -e 's/\s*\#.*//g' | sed -n '/^\S\+\s*[\?\+\:]\?=.*/'p | sed -e 's/\s*[\?\+\:]\?=.*//')
MTK_REMAKE_CURRENT_PROJECT_CONFIG_OPTIONS += TARGET_BUILD_VARIANT MTK_BUILD_ROOT
MTK_REMAKE_IGNORED_PROJECT_CONFIG_OPTIONS := BUILD_NUMBER ADDITIONAL_DEFAULT_PROPERTIES CUSTOM_BUILD_VERNO $(_product_var_list) LOCAL_PATH
# FIXME
MTK_REMAKE_IGNORED_PROJECT_CONFIG_OPTIONS += BUILD_%_CONSYS
MTK_REMAKE_CURRENT_PROJECT_CONFIG_OPTIONS := $(sort $(filter-out $(MTK_REMAKE_IGNORED_PROJECT_CONFIG_OPTIONS),$(MTK_REMAKE_CURRENT_PROJECT_CONFIG_OPTIONS)))
MTK_REAMKE_MODULE_DEPENDENCIES_LIST := $(wildcard $(MTK_REAMKE_MODULE_DEPENDENCIES_OUT)/./*.dep)
include $(MTK_REAMKE_MODULE_DEPENDENCIES_LIST)
  ifneq ($(wildcard $(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE)),)
MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_OPTIONS :=
MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_TEXT := $(shell cat $(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_FILE) | sed -e 's/\s*\#.*//g' | sed -n '/^\S\+\s*=.*/'p | sed -e 's/\s/\?/g')
$(foreach l,$(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_TEXT),\
  $(eval k := $(firstword $(subst =,$(space),$(l))))\
  $(eval v := $(strip $(subst ?,$(space),$(patsubst $(k)=%,%,$(l)))))\
  $(eval f := $(strip $(subst ?,$(space),$(k))))\
  $(eval MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_OPTIONS += $(f))\
  $(if $(filter $(f),$(_product_var_list)),\
    $(if $(strip $(filter-out $(PRODUCTS.$(INTERNAL_PRODUCT).$(f)),$(v))),\
      $(eval MTK_REMAKE_DELETED_PRODUCT_CONFIG_OPTIONS += $(f))\
    )\
    $(if $(strip $(filter-out $(v),$(PRODUCTS.$(INTERNAL_PRODUCT).$(f)))),\
      $(eval MTK_REMAKE_ADDED_PRODUCT_CONFIG_OPTIONS += $(f))\
    )\
  ,\
    $(if $(filter $(f),$(MTK_REMAKE_CURRENT_PROJECT_CONFIG_OPTIONS)),\
      $(if $(strip $(filter-out $($(f)),$(v))$(filter-out $(v),$($(f)))),\
        $(eval MTK_REMAKE_CHANGED_PROJECT_CONFIG_OPTIONS += $(f))\
      )\
    ,\
      $(if $(filter $(f),ALL_MODULES),,\
        $(eval MTK_REMAKE_DELETED_PROJECT_CONFIG_OPTIONS += $(f))\
      )\
    )\
  )\
)
MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS := $(filter-out $(MTK_REMAKE_PREVIOUS_PROJECT_CONFIG_OPTIONS),$(MTK_REMAKE_CURRENT_PROJECT_CONFIG_OPTIONS))
  endif

MTK_REMAKE_CHANGED_COMMON_GLOBAL_CFLAGS := \
  $(MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS) \
  $(filter \
    $(AUTO_ADD_GLOBAL_DEFINE_BY_NAME) \
    $(AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE) \
    $(AUTO_ADD_GLOBAL_DEFINE_BY_VALUE) \
    AUTO_ADD_GLOBAL_DEFINE_BY_NAME \
    AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE \
    AUTO_ADD_GLOBAL_DEFINE_BY_VALUE \
    COMMON_GLOBAL_CFLAGS \
    ,$(MTK_REMAKE_CHANGED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_DELETED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS))

MTK_REMAKE_CHANGED_COMMON_PRODUCT_VARS := \
  $(MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS) \
  $(filter \
    PRODUCT_LOCALES \
    PRODUCT_AAPT_% \
    PRODUCT_CHARACTERISTICS \
    PRODUCT_DEFAULT_DEV_CERTIFICATE \
    PRODUCT_DEX_PREOPT_% \
    ,$(MTK_REMAKE_DELETED_PRODUCT_CONFIG_OPTIONS) $(MTK_REMAKE_ADDED_PRODUCT_CONFIG_OPTIONS))
endif

# $(1): feature option name
ifneq ($(strip $(MTK_REMAKE_CHANGED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_DELETED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS)),)
define mtk-remake-check-option
$(if $(filter $(1),$(MTK_REMAKE_CHANGED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_DELETED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS)),FORCE)
endef
endif

# $(1): LOCAL_MODULE_CLASS
ifneq ($(strip $(MTK_REMAKE_CHANGED_COMMON_GLOBAL_CFLAGS)),)
define mtk-remake-check-native-class
$(if $(filter $(1),EXECUTABLES SHARED_LIBRARIES STATIC_LIBRARIES),FORCE)
endef
endif

ifneq ($(strip $(MTK_REMAKE_CHANGED_COMMON_PRODUCT_VARS)),)
define mtk-remake-check-java-class
$(if $(filter $(1),JAVA_LIBRARIES APPS),FORCE)
endef
endif

# $(1): LOCAL_MODULE_MAKEFILE
ifneq ($(strip $(MTK_REMAKE_CHANGED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_DELETED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS)),)
define mtk-remake-check-makefile
$(if $(filter $(foreach m,$(1),$(MTK_ALL_MODULE_MAKEFILES.$(m).OPTIONS) $(foreach n,$(MTK_ALL_MODULE_MAKEFILES.$(m).INCLUDED),$(MTK_ALL_MODULE_MAKEFILES.$(n).OPTIONS))),$(MTK_REMAKE_CHANGED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_DELETED_PROJECT_CONFIG_OPTIONS) $(MTK_REMAKE_ADDED_PROJECT_CONFIG_OPTIONS)),FORCE)
endef
endif

define base-rules-remake-module-hook
$(eval \
extra_mk := $$(filter-out $$(BUILD_SYSTEM)/% $$(OUT_DIR)/%,$$(MAKEFILE_LIST))
MTK_ALL_MODULE_MAKEFILES.$$(LOCAL_MODULE_MAKEFILE).INCLUDED := $$(sort $$(MTK_ALL_MODULE_MAKEFILES.$$(LOCAL_MODULE_MAKEFILE).INCLUDED) $$(extra_mk))
LOCAL_ADDITIONAL_DEPENDENCIES := $$(LOCAL_ADDITIONAL_DEPENDENCIES) $$(LOCAL_MODULE_MAKEFILE) $$(MTK_ALL_MODULE_MAKEFILES.$$(LOCAL_MODULE_MAKEFILE).INCLUDED) $$(call mtk-remake-check-makefile,$$(LOCAL_MODULE_MAKEFILE)) $$(call mtk-remake-check-native-class,$$(LOCAL_MODULE_CLASS)) $$(call mtk-remake-check-java-class,$$(LOCAL_MODULE_CLASS))
$$(if $$(filter STATIC_LIBRARIES,$$(LOCAL_MODULE_CLASS)),$$(if $$(strip $$(LOCAL_SRC_FILES)),,$$(call intermediates-dir-for,STATIC_LIBRARIES,$$(LOCAL_MODULE),$$(LOCAL_IS_HOST_MODULE),,$$(LOCAL_2ND_ARCH_VAR_PREFIX))/$$(LOCAL_MODULE).a: $$(LOCAL_ADDITIONAL_DEPENDENCIES)))
)
endef


# base-rules-hook
define base-rules-hook
$(call base-rules-release-jack-hook)
$(call base-rules-dump-release-info-hook)
$(call base-rules-remake-module-hook)
endef

