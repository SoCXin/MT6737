LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	$(LOCAL_DIR)/video \
	$(LOCAL_DIR)/lcm


OBJS += \
	$(LOCAL_DIR)/dev.o

include $(LOCAL_DIR)/logo/rules.mk
