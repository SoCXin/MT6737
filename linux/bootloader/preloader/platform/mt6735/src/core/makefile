###################################################################
# flag
###################################################################

MOD_SRC  = 
MOD_OBJ  = $(PRELOADER_OUT)/obj

###################################################################
# source files
# - add your file here !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
###################################################################

MOD_SRC  := \
	addr_trans.c \
	blkdev.c \
	div0.c \
	download.c \
	eabi_uldivmod.s \
	eabi_idiv.s \
	eabi.s \
	handshake_uart.c \
	handshake_usb.c \
	main.c \
	partition.c \
	print.c \
	stdlib.c \
	string.c \
	dram_buffer.c
	
ifeq ("$(CFG_FUNCTION_TRACER_SUPPORT)", "1")
  MOD_SRC += function_tracer.c
endif
###################################################################
# target
###################################################################

include $(COMMON_FILE_MK)
