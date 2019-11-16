////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_common.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_common.h"

/*=============================================================*/
// MACRO DEFINITION
/*=============================================================*/

/*=============================================================*/
// CONSTANT VALUE DEFINITION
/*=============================================================*/


/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

static u32 _gCrc32Table[256]; 


/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/

extern struct i2c_client *g_I2cClient;

/*=============================================================*/
// DATA TYPE DEFINITION
/*=============================================================*/

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

/// CRC
u32 DrvCommonCrcDoReflect(u32 nRef, s8 nCh)
{
    u32 nValue = 0;
    u32 i = 0;

    for (i = 1; i < (nCh + 1); i ++)
    {
        if (nRef & 1)
        {
            nValue |= 1 << (nCh - i);
        }
        nRef >>= 1;
    }

    return nValue;
}

u32 DrvCommonCrcGetValue(u32 nText, u32 nPrevCRC)
{
    u32 nCRC = nPrevCRC;

    nCRC = (nCRC >> 8) ^ _gCrc32Table[(nCRC & 0xFF) ^ nText];

    return nCRC;
}

void DrvCommonCrcInitTable(void)
{
    u32 nMagicNumber = 0x04c11db7;
    u32 i, j;

    for (i = 0; i <= 0xFF; i ++)
    {
        _gCrc32Table[i] = DrvCommonCrcDoReflect(i, 8) << 24;
        for (j = 0; j < 8; j ++)
        {
            _gCrc32Table[i] = (_gCrc32Table[i] << 1) ^ (_gCrc32Table[i] & (0x80000000L) ? nMagicNumber : 0);
        }
        _gCrc32Table[i] = DrvCommonCrcDoReflect(_gCrc32Table[i], 32);
    }
}

u8 DrvCommonCalculateCheckSum(u8 *pMsg, u32 nLength)
{
    s32 nCheckSum = 0;
    u32 i;

    for (i = 0; i < nLength; i ++)
    {
        nCheckSum += pMsg[i];
    }

    return (u8)((-nCheckSum) & 0xFF);
}

u32 DrvCommonConvertCharToHexDigit(char *pCh, u32 nLength)
{
    u32 nRetVal = 0;
    u32 i;
    
    DBG(&g_I2cClient->dev, "nLength = %d\n", nLength);

    for (i = 0; i < nLength; i ++)
    {
        char ch = *pCh++;
        u32 n = 0;
        u8  nIsValidDigit = 0;
        
        if ((i == 0 && ch == '0') || (i == 1 && ch == 'x'))
        {
            continue;		
        }
        
        if ('0' <= ch && ch <= '9')
        {
            n = ch-'0';
            nIsValidDigit = 1;
        }
        else if ('a' <= ch && ch <= 'f')
        {
            n = 10 + ch-'a';
            nIsValidDigit = 1;
        }
        else if ('A' <= ch && ch <= 'F')
        {
            n = 10 + ch-'A';
            nIsValidDigit = 1;
        }
        
        if (1 == nIsValidDigit)
        {
            nRetVal = n + nRetVal*16;
        }
    }
    
    return nRetVal;
}

void DrvCommonReadFile(char *pFilePath, u8 *pBuf, u16 nLength)
{
    struct file *pFile = NULL;
    mm_segment_t old_fs;
    ssize_t nReadBytes = 0;    

    old_fs = get_fs();
    set_fs(get_ds());

    pFile = filp_open(pFilePath, O_RDONLY, 0);
    if (IS_ERR(pFile)) {
        DBG(&g_I2cClient->dev, "Open file failed: %s\n", pFilePath);
        return;
    }

    pFile->f_op->llseek(pFile, 0, SEEK_SET);
    nReadBytes = pFile->f_op->read(pFile, pBuf, nLength, &pFile->f_pos);
    DBG(&g_I2cClient->dev, "Read %d bytes!\n", (int)nReadBytes);

    set_fs(old_fs);        
    filp_close(pFile, NULL);    
}

//------------------------------------------------------------------------------//