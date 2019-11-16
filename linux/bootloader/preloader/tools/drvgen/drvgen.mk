ifndef DRVGEN_OUT
DRVGEN_OUT := $(PRELOADER_OUT)
endif
export DRVGEN_OUT

DRVGEN_OUT_PATH := $(DRVGEN_OUT)/inc

ALL_DRVGEN_FILE :=

ifeq ($(filter MT2601,$(MTK_PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_kpd.h
  ALL_DRVGEN_FILE += inc/cust_eint.h
  ALL_DRVGEN_FILE += inc/cust_gpio_boot.h
  ALL_DRVGEN_FILE += inc/cust_gpio_usage.h
  ALL_DRVGEN_FILE += inc/cust_adc.h
  ALL_DRVGEN_FILE += inc/pmic_drv.h
  ALL_DRVGEN_FILE += inc/pmic_drv.c
endif

ifeq ($(filter MT2601 MT8127 MT8163,$(MTK_PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_eint_md1.h
endif

ifeq ($(filter MT2601 MT6572 MT6582 MT6592 MT8127,$(MTK_PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_eint.dtsi
endif

ifeq ($(filter MT2601 MT6580,$(MTK_PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_power.h
endif

ifeq ($(filter MT2601 MT6572 MT6582 MT6592 MT8127 MT8163,$(MTK_PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_clk_buf.h
endif

ifeq ($(filter MT2601 MT6572 MT6582 MT6592 MT8127 MT8163,$(MTK_PLATFORM)),)
  ALL_DRVGEN_FILE += inc/cust_i2c.h
endif

ifeq ($(MTK_PLATFORM),MT2601)
  ALL_DRVGEN_FILE += inc/cust_kpd.h
  ALL_DRVGEN_FILE += inc/cust_eint.h
  ALL_DRVGEN_FILE += inc/cust_gpio_boot.h
  ALL_DRVGEN_FILE += inc/cust_gpio_usage.h
endif

ifeq ($(MTK_PLATFORM),MT6752)
  ALL_DRVGEN_FILE += inc/cust_eint_md2.h
endif

ifeq ($(MTK_PLATFORM),MT6595)
  ALL_DRVGEN_FILE += inc/cust_gpio_suspend.h
endif

ifeq ($(MTK_PLATFORM),MT6580)
  ALL_DRVGEN_FILE += inc/cust_i2c.dtsi
endif

ifeq ($(MTK_PLATFORM),MT8127)
  ALL_DRVGEN_FILE += inc/cust_eint_ext.h
endif

ifeq ($(MTK_PLATFORM),MT6735)
  ALL_DRVGEN_FILE += inc/cust_adc.dtsi
  ALL_DRVGEN_FILE += inc/cust_i2c.dtsi
  ALL_DRVGEN_FILE += inc/cust_md1_eint.dtsi
  ALL_DRVGEN_FILE += inc/cust_kpd.dtsi
  ALL_DRVGEN_FILE += inc/cust_clk_buf.dtsi
  ALL_DRVGEN_FILE += inc/cust_gpio.dtsi
  ALL_DRVGEN_FILE += inc/cust_adc.dtsi
  ALL_DRVGEN_FILE += inc/cust_pmic.dtsi
  ALL_DRVGEN_FILE += inc/mt6735-pinfunc.h
  ALL_DRVGEN_FILE += inc/pinctrl-mtk-mt6735.h
endif

ifeq ($(MTK_PLATFORM),MT6797)
  PMIC_DRV_C_TARGET = pmic_drv_c
  PMIC_DRV_H_TARGET = pmic_drv_h
else
  PMIC_DRV_C_TARGET = pmic_c
  PMIC_DRV_H_TARGET = pmic_h
endif

DRVGEN_FILE_LIST := $(addprefix $(DRVGEN_OUT)/,$(ALL_DRVGEN_FILE))
ifeq ($(HOST_OS),darwin)
DRVGEN_TOOL := $(D_ROOT)/tools/dct/DrvGen.darwin
else
DRVGEN_TOOL := $(D_ROOT)/tools/dct/DrvGen
endif
DWS_FILE := $(MTK_PATH_CUSTOM)/dct/$(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)/codegen.dws
DRVGEN_PREBUILT_PATH := $(MTK_PATH_CUSTOM)
DRVGEN_PREBUILT_CHECK := $(filter-out $(wildcard $(addprefix $(DRVGEN_PREBUILT_PATH)/,$(ALL_DRVGEN_FILE))),$(addprefix $(DRVGEN_PREBUILT_PATH)/,$(ALL_DRVGEN_FILE)))

.PHONY: drvgen
drvgen: $(DRVGEN_FILE_LIST)
ifneq ($(DRVGEN_PREBUILT_CHECK),)

$(DRVGEN_OUT)/inc/cust_kpd.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) kpd_h

$(DRVGEN_OUT)/inc/cust_eint.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) eint_h

$(DRVGEN_OUT)/inc/cust_gpio_boot.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) gpio_boot_h

$(DRVGEN_OUT)/inc/cust_gpio_usage.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) gpio_usage_h

$(DRVGEN_OUT)/inc/cust_adc.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) adc_h

$(DRVGEN_OUT)/inc/cust_eint_md1.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) md1_eint_h

$(DRVGEN_OUT)/inc/cust_power.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) power_h

$(DRVGEN_OUT)/inc/pmic_drv.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) $(PMIC_DRV_H_TARGET)

$(DRVGEN_OUT)/inc/cust_i2c.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) i2c_h

$(DRVGEN_OUT)/inc/cust_clk_buf.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) clk_buf_h

$(DRVGEN_OUT)/inc/cust_eint_md2.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) md2_eint_h

$(DRVGEN_OUT)/inc/cust_gpio_suspend.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) suspend_h

$(DRVGEN_OUT)/inc/cust_eint_ext.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) eint_ext_h

$(DRVGEN_OUT)/inc/cust_eint.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) eint_dtsi

$(DRVGEN_OUT)/inc/pmic_drv.c: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) $(PMIC_DRV_C_TARGET)

$(DRVGEN_OUT)/inc/cust_i2c.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) i2c_dtsi

$(DRVGEN_OUT)/inc/cust_adc.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) adc_dtsi

$(DRVGEN_OUT)/inc/cust_md1_eint.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) md1_eint_dtsi

$(DRVGEN_OUT)/inc/cust_kpd.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) kpd_dtsi

$(DRVGEN_OUT)/inc/cust_clk_buf.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) clk_buf_dtsi

$(DRVGEN_OUT)/inc/cust_gpio.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) gpio_dtsi

$(DRVGEN_OUT)/inc/cust_adc.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) adc_dtsi

$(DRVGEN_OUT)/inc/cust_pmic.dtsi: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) pmic_dtsi

$(DRVGEN_OUT)/inc/mt6735-pinfunc.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) mt6735_pinfunc_h

$(DRVGEN_OUT)/inc/pinctrl-mtk-mt6735.h: $(DRVGEN_TOOL) $(DWS_FILE)
	@mkdir -p $(dir $@)
	@$(DRVGEN_TOOL) $(DWS_FILE) $(DRVGEN_OUT_PATH) $(DRVGEN_OUT_PATH) pinctrl_mtk_mt6735_h

else
$(DRVGEN_FILE_LIST): $(DRVGEN_OUT)/% : $(DRVGEN_PREBUILT_PATH)/%
	@mkdir -p $(dir $@)
	cp -f $< $@
endif
