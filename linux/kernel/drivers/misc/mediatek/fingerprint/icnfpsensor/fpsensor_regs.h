/*++

 File Name:    fpsensor_regs.h
 Author:       zmtian
 Date :        11,23,2015
 Version:      1.0[.revision]

 History :
     Change logs.
 --*/

#ifndef __FPSENSOR_REGS_H
#define __FPSENSOR_REGS_H

typedef enum
{
    /* --- Common registers --- */
    FPSENSOR_REG_STATUS                      = 0x14,  /* RO, 1 bytes  */
    FPSENSOR_REG_READ_INTERRUPT              = 0x18,  /* RO, 1 byte   */
    FPSENSOR_REG_READ_INTERRUPT_WITH_CLEAR   = 0x1C,  /* RO, 1 byte   */
    FPSENSOR_REG_READ_ERROR_WITH_CLEAR       = 0x0C,  /* RO, 1 byte   */
    FPSENSOR_REG_MISO_EDGE_RIS_EN            = 0x64,  /* WO, 1 byte   */
    FPSENSOR_REG_CONFIG                      = 0x60,  /* RW, 1 byte   */
    FPSENSOR_REG_IMG_SMPL_SETUP              = 0xA4,  /* RW, 3 bytes  */
    FPSENSOR_REG_CLOCK_CONFIG                = 0x6C,  /* RW, 1 byte   */
    FPSENSOR_REG_IMG_CAPT_SIZE               = 0xA8,  /* RW, 4 bytes  */
    FPSENSOR_REG_IMAGE_SETUP                 = 0xAC,  /* RW, 1 byte   */
    FPSENSOR_REG_ADC_TEST_CTRL               = 0x3C,  /* RW, 1 byte   */
    FPSENSOR_REG_IMG_RD                      = 0xA0,  /* RW, 1 byte   */
    FPSENSOR_REG_SAMPLE_PX_DLY               = 0xB8,  /* RW, 8 bytes  */
    FPSENSOR_REG_PXL_RST_DLY                 = 0xB4,  /* RW, 1 byte   */
    FPSENSOR_REG_TST_COL_PATTERN_EN          = 0x50,  /* RW, 2 bytes  */
    FPSENSOR_REG_CLK_BIST_RESULT             = 0x44,  /* RW, 4 bytes  */
    FPSENSOR_REG_ADC_WEIGHT_SETUP            = 0x98,  /* RW, 1 byte   */
    FPSENSOR_REG_ANA_TEST_MUX                = 0x5C,  /* RW, 4 bytes  */
    FPSENSOR_REG_FINGER_DRIVE_CONF           = 0xC4,  /* RW, 1 byte   */
    FPSENSOR_REG_FINGER_DRIVE_DLY            = 0xC0,  /* RW, 1 byte   */
    FPSENSOR_REG_OSC_TRIM                    = 0xCC,  /* RW, 2 bytes  */
    FPSENSOR_REG_ADC_WEIGHT_TABLE            = 0x9C,  /* RW, 10 bytes */
    FPSENSOR_REG_ADC_SETUP                   = 0x90,  /* RW, 5 bytes  */
    FPSENSOR_REG_ADC_SHIFT_GAIN              = 0x94,  /* RW, 2 bytes  */
    FPSENSOR_REG_BIAS_TRIM                   = 0x68,  /* RW, 1 byte   */
    FPSENSOR_REG_PXL_CTRL                    = 0xB0,  /* RW, 2 bytes  */
    FPSENSOR_REG_DEBUG                       = 0xF4,  /* RO, 1 bytes  */
    FPSENSOR_REG_FINGER_PRESENT_STATUS       = 0x80,  /* RO, 2 bytes  */
    FPSENSOR_REG_HWID                        = 0x00,  /* RO, 2 bytes  */
    FPSENSOR_REG_VID                         = 0x04,
    FPSENSOR_REG_ANA_CFG1                    = 0xD0,
    FPSENSOR_REG_ANA_CFG2                    = 0xD4,
    FPSENSOR_REG_RDT                         = 0xF0,
    FPSENSOR_ADDRESS_REMAP                   = 0x08,
    /* --- fpsensor/ specific --- */
    FPSENSOR_REG_FNGR_DET_THRES              = 0x84,  /* RW, 1 byte   */
    FPSENSOR_REG_FNGR_DET_CNTR               = 0x88,  /* RW, 2 bytes  */
    FPSENSOR_REG_FNGR_DET_VAL                = 0x8C,
    /* --- fpsensor1 specific --- */
    FPSENSOR1_REG_OFFSET                      = 1000, /* Not a register ! */
    FPSENSOR1_REG_FNGR_DET_THRES              = 1216, /* RW, 4 byte   */
    FPSENSOR1_REG_FNGR_DET_CNTR               = 1220, /* RW, 4 bytes  */

    FPSENSOR_FGR_DET_SUB_COL                  = 0x70,
    FPSENSOR_FGR_DET_SUB_ROW                  = 0x74,
    FPSENSOR_SLP_DET_SUB                      = 0x78,
    FPSENSOR_FGR_DET_NUM                      = 0x7C,

} fpsensor_reg_t;

#define FPSENSOR_REG_MAX_SIZE    16

#define FPSENSOR_REG_SIZE(reg) ( \
                                 ((reg) == FPSENSOR_REG_STATUS) ?                     1 : \
                                 ((reg) == FPSENSOR_REG_READ_INTERRUPT) ?             1 : \
                                 ((reg) == FPSENSOR_REG_READ_INTERRUPT_WITH_CLEAR) ?  1 : \
                                 ((reg) == FPSENSOR_REG_READ_ERROR_WITH_CLEAR) ?      1 : \
                                 ((reg) == FPSENSOR_REG_MISO_EDGE_RIS_EN) ?           1 : \
                                 ((reg) == FPSENSOR_REG_CONFIG) ?                     1 : \
                                 ((reg) == FPSENSOR_REG_IMG_SMPL_SETUP) ?             3 : \
                                 ((reg) == FPSENSOR_REG_CLOCK_CONFIG) ?               1 : \
                                 ((reg) == FPSENSOR_REG_IMG_CAPT_SIZE) ?              4 : \
                                 ((reg) == FPSENSOR_REG_IMAGE_SETUP) ?                1 : \
                                 ((reg) == FPSENSOR_REG_ADC_TEST_CTRL) ?              1 : \
                                 ((reg) == FPSENSOR_REG_IMG_RD) ?                     1 : \
                                 ((reg) == FPSENSOR_REG_SAMPLE_PX_DLY) ?              8 : \
                                 ((reg) == FPSENSOR_REG_PXL_RST_DLY) ?                1 : \
                                 ((reg) == FPSENSOR_REG_TST_COL_PATTERN_EN) ?         2 : \
                                 ((reg) == FPSENSOR_REG_CLK_BIST_RESULT) ?            4 : \
                                 ((reg) == FPSENSOR_REG_ADC_WEIGHT_SETUP) ?           1 : \
                                 ((reg) == FPSENSOR_REG_ANA_TEST_MUX) ?               4 : \
                                 ((reg) == FPSENSOR_REG_FINGER_DRIVE_CONF) ?          1 : \
                                 ((reg) == FPSENSOR_REG_FINGER_DRIVE_DLY) ?           1 : \
                                 ((reg) == FPSENSOR_REG_OSC_TRIM) ?                   2 : \
                                 ((reg) == FPSENSOR_REG_ADC_WEIGHT_TABLE) ?           10 :\
                                 ((reg) == FPSENSOR_REG_ADC_SETUP) ?                  5 : \
                                 ((reg) == FPSENSOR_REG_ADC_SHIFT_GAIN) ?             2 : \
                                 ((reg) == FPSENSOR_REG_BIAS_TRIM) ?                  1 : \
                                 ((reg) == FPSENSOR_REG_PXL_CTRL) ?                   2 : \
                                 ((reg) == FPSENSOR_REG_DEBUG) ?                      2 : \
                                 ((reg) == FPSENSOR_REG_FINGER_PRESENT_STATUS) ?      2 : \
                                 ((reg) == FPSENSOR_REG_HWID) ?                       2 : \
                                 ((reg) == FPSENSOR_REG_VID) ?                        2 : \
                                 ((reg) == FPSENSOR_REG_ANA_CFG1) ?                   8 : \
                                 ((reg) == FPSENSOR_REG_ANA_CFG2) ?                   8 : \
                                 ((reg) == FPSENSOR_REG_RDT) ?                        4 : \
                                 ((reg) == FPSENSOR_ADDRESS_REMAP) ?                  1 : \
                                 \
                                 ((reg) == FPSENSOR_REG_FNGR_DET_THRES) ?             1 : \
                                 ((reg) == FPSENSOR_REG_FNGR_DET_CNTR) ?              2 : \
                                 ((reg) == FPSENSOR_REG_FNGR_DET_VAL) ?               12 : \
                                 \
                                 ((reg) == FPSENSOR1_REG_FNGR_DET_THRES) ?             4 : \
                                 ((reg) == FPSENSOR1_REG_FNGR_DET_CNTR) ?              4 : \
                                 ((reg) == FPSENSOR_FGR_DET_SUB_COL) ?                 12 : \
                                 ((reg) == FPSENSOR_FGR_DET_SUB_ROW) ?                 12 : \
                                 ((reg) == FPSENSOR_FGR_DET_NUM) ?                     2 : \
                                 ((reg) == FPSENSOR_SLP_DET_SUB) ?                     9 : \
                                 0)

#define FPSENSOR_REG_ACCESS_DUMMY_BYTES(reg) (               \
                                                             ((reg) == FPSENSOR_REG_STATUS) ?                     1 : \
                                                             ((reg) == FPSENSOR_REG_DEBUG) ?                      1 : 0)

#define FPSENSOR_REG_TO_ACTUAL(reg) (                        \
                                                             ((reg) >= FPSENSOR1_REG_OFFSET) ? ((reg) - FPSENSOR1_REG_OFFSET) : (reg) )

#endif /* __FPSENSOR_REGS_H */


