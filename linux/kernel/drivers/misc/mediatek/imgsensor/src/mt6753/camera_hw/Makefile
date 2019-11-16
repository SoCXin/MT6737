#
# Makefile for misc devices that really don't fit anywhere else.
#
#include $(srctree)/drivers/misc/mediatek/Makefile.custom

MTK_CUSTOM_PATH  := $(srctree)/drivers/misc/mediatek

ccflags-y += -I$(MTK_CUSTOM_PATH)/imgsensor/inc \
                    
$(warning "CONFIG_TABLET_PROJECT:$(CONFIG_TABLET_PROJECT)")
ifeq ($(CONFIG_TABLET_PROJECT),y)
# is tablet project
obj-y				+= tb_kd_camera_hw.o
else
# not tablet project
obj-y				+= kd_camera_hw.o
endif
