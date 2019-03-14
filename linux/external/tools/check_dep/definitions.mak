# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
#
# MediaTek Inc. (C) 2010. All rights reserved.
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


##
## Common build system used make function definitions.
##

# dependency error counter
DEP_ERR_CNT:=

###########################################################
## A common function to show feature dependency error
## $(1): dependency error message
###########################################################
define dep-err-common
$(eval DEP_ERR_CNT += error) \
$(warning *** $(strip $(1)))
endef


###########################################################
## A specified function to show following well-formed 
## dependency error
##
## ex. PLEASE set OptA as ValA or set OptB as ValB
##     PLEASE set OptA as ValA or set OptB as ValB1/ValB2/.../ValBn
##     PLEASE set OptA as ValA or set OptB as non ValB
##
## usage:  $(call dep-err-seta-or-setb,OptA,ValA,OptB,ValB)
##         $(call dep-err-seta-or-setb,OptA,ValA,OptB,ValB1/ValB2/.../ValBn)
##         $(call dep-err-seta-or-setb,OptA,ValA,OptB,non ValB)
##
## $(1): 1st feature option name
## $(2): 1st feature option value
## $(3): 2nd feature option name
## $(4): 2nd feature option value/value list
###########################################################
define dep-err-seta-or-setb
$(call dep-err-common,PLEASE set $(1) as "$(2)" or set $(3) as "$(4)")
endef


###########################################################
## A specified function to show following well-formed
## dependency error
##
## ex. PLEASE turn on OptA or turn off OptB
##     PLEASE turn on OptA or turn off OptB & OptC & ... & OptN
##
## usage:  $(call dep-err-ona-or-offb,OptA,OptB)
##         $(call dep-err-ona-or-offb,OptA,OptB & OptC & ... & OptN)
##
## $(1): 1st feature option name
## $(2): 1st feature option name/name list
###########################################################
define dep-err-ona-or-offb
$(call dep-err-common,PLEASE turn on $(1) or turn off $(2))
endef


###########################################################
## A specified function to show following well-formed
## dependency error
##
## ex. PLEASE set OptA as ValA or turn off OptB
##     PLEASE set OptA as ValA1/ValA2/.../ValAn or turn off OptB
##     PLEASE set OptA as non ValA or turn off OptB 
##
## usage:  $(call dep-err-seta-or-offb,OptA,ValA,OptB)
##         $(call dep-err-seta-or-offb,OptA,ValA1/ValA2/.../ValAn,OptB)
##         $(call dep-err-seta-or-offb,OptA,non ValA,OptB)
##
## $(1): 1st feature option name
## $(2): 1st feature option value/value list
## $(3): 2nd feature option name
###########################################################
define dep-err-seta-or-offb
$(call dep-err-common,PLEASE set $1 as "$(2)" or turn off $(3))
endef


###########################################################
## A specified function to show following well-form dependency error
##
## ex. PLEASE set OptA as ValA or turn on OptB
##     PLEASE set OptA as ValA1/ValA2/.../ValAn or turn on OptB
##     PLEASE set OptA as non ValA or turn on OptB

## usage: $(call DEP_ERR_SETA_OR_ONB,OptA,ValA,OptB)
##        $(call DEP_ERR_SETA_OR_ONB,OptA,ValA1/ValA2/.../ValAn,OptB)
##        $(call DEP_ERR_SETA_OR_ONB,OptA,non ValA,OptB)
##
## $(1): 1st feature option name
## $(2): 1st feature option value/value list
## $(3): 2nd feature option name
###########################################################
define dep-err-seta-or-onb
$(call dep-err-common,PLEASE set $(1) as "$(2)" or turn on $(3))
endef


###########################################################
## A specified function to show following well-form dependency error
## 
## ex. PLEASE turn off OptA or turn off OptB
##
## usage: $(call dep-err-offa-or-offb,OptA,OptB)
##
## $(1): 1st feature option name
## #(2): 2nd feature option name
###########################################################
define dep-err-offa-or-offb
$(call dep-err-common,PLEASE turn off $(1) or turn off $(2))
endef


###########################################################
## A specified function to show following well-form dependency error
## 
## ex. PLEASE turn on OptA or turn on OptB
##
## usage: $(call dep-err-ona-or-onb,OptA,OptB)
##
## $(1): 1st feature option name
## #(2): 2nd feature option name
###########################################################
define dep-err-ona-or-onb
$(call dep-err-common,PLEASE turn on $(1) or turn on $(2))
endef


###########################################################
## A function to remove duplicate value from a list
## $(1): value list
###########################################################
define remove-duplicate
$(strip \
  $(eval var :=) \
  $(foreach i,$(1), \
     $(if $(filter $(i),$(var)),,$(eval var += $(i))) \
   ) \
  $(var) \
 )
endef


###########################################################
## Output the command lines, or not
###########################################################

ifeq ($(strip $(SHOW_COMMANDS)),)
define pretty
@echo $1
endef
hide := @
else
define pretty
endef
hide :=
endif


###########################################################
## transfer string from upper to lower
###########################################################
define upper-to-lower
$(shell echo $1 | awk '{print tolower($$0);}')
endef

###########################################################
## transfer string from lower to upper
###########################################################
define lower-to-upper
$(shell echo $1 | awk '{print toupper($$0);}')
endef


