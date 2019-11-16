////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2012 MStar Semiconductor, Inc.
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
 * @file    mstar_drv_hotknot.c
 *
 * @brief   This file defines the hotknot functions
 *
 *
 */

////////////////////////////////////////////////////////////
/// Included Files
////////////////////////////////////////////////////////////
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>
#include "mstar_drv_hotknot.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_hotknot_queue.h"
#include "mstar_drv_fw_control.h"

#ifdef CONFIG_ENABLE_HOTKNOT
////////////////////////////////////////////////////////////
/// Data Types
////////////////////////////////////////////////////////////


/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/
u8 g_HotKnotState;
struct mutex g_QMutex;    //queue mutex



/*=============================================================*/
// EXTERN VARIABLE DECLARATION
/*=============================================================*/
extern struct mutex g_Mutex;
extern struct i2c_client *g_I2cClient;

extern u32 SLAVE_I2C_ID_DBBUS;
extern u32 SLAVE_I2C_ID_DWI2C;

extern u16 FIRMWARE_MODE_UNKNOWN_MODE;
extern u16 FIRMWARE_MODE_DEMO_MODE;
extern u16 FIRMWARE_MODE_DEBUG_MODE;
//extern u16 FIRMWARE_MODE_RAW_DATA_MODE;

extern u16 g_FirmwareMode;

extern u8 IS_FIRMWARE_DATA_LOG_ENABLED;

extern u8 g_IsHotknotEnabled;
extern u8 g_IsUpdateFirmware;
extern u8 g_IsBypassHotknot;


////////////////////////////////////////////////////////////
/// LOCAL VARIABLE DEFINITION
////////////////////////////////////////////////////////////
DrvCmd_t * _gDrvCmdStack;
unsigned long  _gArgStack;
u8 * _gHKPacket; 
u16  _gHKLength;

static int _gHKFlag = 0;
static DECLARE_WAIT_QUEUE_HEAD(_gHKWaiter);

#ifdef CONFIG_ENABLE_HOTKNOT_RCV_BLOCKING
static int _gHKRcvFlag = 0;
static int _gHKRcvWaitEnable = 0;
static DECLARE_WAIT_QUEUE_HEAD(_gHKRcvWaiter);
#endif

static DrvCmd_t * _gCmdIn = NULL;
static u8 * _gSndData = NULL;
static u8 * _gRcvData = NULL;
static u16 * _gFwMode = NULL;


////////////////////////////////////////////////////////////
/// Macro
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Function Prototypes
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/// Function Implementation
////////////////////////////////////////////////////////////

u8 _GetCheckSum(u8 *pBuf, int nLen)
{
	  s32 nSum = 0;
      int i;
	  for(i=0; i < nLen; i++)
	  {
	  	  nSum += pBuf[i];
	  }
	  return (u8)(-nSum&0xFF);	
}


void _DebugShowArray(u8 *pBuf, u16 nLen)
{
    int i;

    for(i=0; i < nLen; i++)
    {
        DBG(&g_I2cClient->dev, "%02X ", pBuf[i]);       

        if(i%16==15){  
            DBG(&g_I2cClient->dev, "\n");
        }
    }
    DBG(&g_I2cClient->dev, "\n");    
}


int PushHotKnotData(u8 * pBuf, u16 nLength)
{
    int nQRet;
    mutex_lock(&g_QMutex);
    nQRet = PushQueue(pBuf, nLength);
    mutex_unlock(&g_QMutex);
    
    return nQRet;
}


int PopHotKnotData(u8 * pBuf, u16 nLength)
{
    int nQRet;
    mutex_lock(&g_QMutex);
    nQRet = PopQueue(pBuf, nLength);
    mutex_unlock(&g_QMutex); 
    
    return nQRet;
}


int ShowHotKnotData(u8 * pBuf, u16 nLength)
{
    int nQRet;
    mutex_lock(&g_QMutex);
    nQRet = ShowQueue(pBuf, nLength);
    mutex_unlock(&g_QMutex);
    
    return nQRet;
}


void _HotKnotRcvInterruptHandler(u8 *pPacket, u16 nLength)    //receive hotknot packet in interrupt 
{
    u16 nPacketDataLen = 0;
    u16 nQPushLen = 0;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if(g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
    { 
        nPacketDataLen = (((DemoHotKnotRcvRet_t*)pPacket)->nActualDataLen_H << 8) | (((DemoHotKnotRcvRet_t*)pPacket)->nActualDataLen_L & 0xFF);
        nQPushLen = ((((DemoHotKnotRcvRet_t*)pPacket)->szData)[0]&0x7F) + 1;
        if(nPacketDataLen >= nQPushLen)
        {
            if(PushHotKnotData(((DemoHotKnotRcvRet_t*)pPacket)->szData, nQPushLen)<0)
            {
                DBG(&g_I2cClient->dev, "Error: HotKnot# Over push data into queue.");
            }
#ifdef CONFIG_ENABLE_HOTKNOT_RCV_BLOCKING
            else
            {
                if(_gHKRcvWaitEnable == 1)
                {
                    _gHKRcvFlag = 1;    
                    DBG(&g_I2cClient->dev, "*** wait up receive_wait. ***\n");                                        
                    wake_up_interruptible(&_gHKRcvWaiter);
                }
            }
#endif //CONFIG_ENABLE_HOTKNOT_RCV_BLOCKING   
        }
        else
        {
            DBG(&g_I2cClient->dev, "Error: HotKnot# Receive data error.");
        }        
    }
    else if(IS_FIRMWARE_DATA_LOG_ENABLED && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE))
    {
        nPacketDataLen = (((DebugHotKnotRcvRet_t*)pPacket)->nActualDataLen_H << 8) | (((DebugHotKnotRcvRet_t*)pPacket)->nActualDataLen_L & 0xFF);
        nQPushLen = ((((DemoHotKnotRcvRet_t*)pPacket)->szData)[0]&0x7F) + 1;

        if(nPacketDataLen >= nQPushLen)
        {        
            if(PushHotKnotData(((DebugHotKnotRcvRet_t*)pPacket)->szData, nQPushLen)<0)
            {
                DBG(&g_I2cClient->dev, "Error: HotKnot# Over push data into queue.");
            }  
#ifdef CONFIG_ENABLE_HOTKNOT_RCV_BLOCKING
            else
            {
                if(_gHKRcvWaitEnable == 1)
                {
                    _gHKRcvFlag = 1;    
                    DBG(&g_I2cClient->dev, "*** wait up receive_wait. ***\n");                                        
                    wake_up_interruptible(&_gHKRcvWaiter);
                }
            }
#endif //CONFIG_ENABLE_HOTKNOT_RCV_BLOCKING                  
        }
        else
        {
            DBG(&g_I2cClient->dev, "Error: HotKnot# Receive data error.");
        }        
    }
}


void ReportHotKnotCmd(u8 *pPacket, u16 nLength)
{
    //DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if(g_HotKnotState == HOTKNOT_TRANS_STATE && pPacket[0] == HOTKNOT_PACKET_ID && pPacket[3] == HOTKNOT_RECEIVE_PACKET_TYPE)
    { 
        _HotKnotRcvInterruptHandler(pPacket, nLength);
    }
    else if(_gDrvCmdStack!=NULL && _gArgStack!=0)
    {
        _gHKPacket = pPacket;
        _gHKLength = nLength; 
    
        _gHKFlag = 1;    
        wake_up_interruptible(&_gHKWaiter);
    }    
}


void _HotKnotCmdInterruptHandler(u8 *pPacket, u16 nLength)
{ 
    //DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    if(_gDrvCmdStack->nCmdId == HOTKNOT_CMD)
    {        
        if(g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            HotKnotCmd_t *pSnd = (HotKnotCmd_t*)_gDrvCmdStack->pSndData;
            DemoHotKnotCmdRet_t *pRcv = (DemoHotKnotCmdRet_t*)pPacket;
            if(pRcv->nHeader == DEMO_PD_PACKET_ID && pSnd->nInstruction == pRcv->nInstruction)
            {                 
                if(pRcv->nResult == RESULT_OK)
                {
                    if(pRcv->nInstruction == READ_PAIR_STATE)
                    {
                        g_HotKnotState = HOTKNOT_BEFORE_TRANS_STATE;
                        ClearQueue();
                    }                       
                    else if(pRcv->nInstruction == ENTER_TRANSFER_MODE)
                    {
                        g_HotKnotState = HOTKNOT_TRANS_STATE;
                    }
                    else if(pRcv->nInstruction == EXIT_TRANSFER_MODE)
                    {
                        g_HotKnotState = HOTKNOT_AFTER_TRANS_STATE;
                    }                      
                    else if(pRcv->nInstruction == ENTER_SLAVE_MODE)
                    {
                        g_HotKnotState = HOTKNOT_NOT_TRANS_STATE;
                    }           	    	

                    if(pRcv->nInstruction == ENABLE_HOTKNOT || pRcv->nInstruction == ENTER_MASTER_MODE || pRcv->nInstruction == ENTER_SLAVE_MODE)
                    {
                        g_IsHotknotEnabled = 1;
                        DBG(&g_I2cClient->dev, "*** g_IsHotknotEnabled = %d ***\n", g_IsHotknotEnabled);
                    }                       
                    else if(pRcv->nInstruction == DISABLE_HOTKNOT)
                    {
                        g_IsHotknotEnabled = 0;
                        DBG(&g_I2cClient->dev, "*** g_IsHotknotEnabled = %d ***\n", g_IsHotknotEnabled);                        
                    }                    
                }
                                        
                memcpy(_gDrvCmdStack->pRcvData, pPacket, nLength);
                //_gDrvCmdStack->nRcvLen = nLength;
                *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEMO_MODE;             
            }
        }
        else if(IS_FIRMWARE_DATA_LOG_ENABLED && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE))
        {
            HotKnotCmd_t *pSnd = (HotKnotCmd_t*)_gDrvCmdStack->pSndData;
            DebugHotKnotCmdRet_t *pRcv = (DebugHotKnotCmdRet_t*)pPacket;
            if(pRcv->nHeader == HOTKNOT_PACKET_ID && pSnd->nInstruction == pRcv->nInstruction)
            {
                if(pRcv->nResult == RESULT_OK)
                {
                    if(pRcv->nInstruction == READ_PAIR_STATE)
                    {
                        g_HotKnotState = HOTKNOT_BEFORE_TRANS_STATE;
                        ClearQueue();
                    }                       
                    else if(pRcv->nInstruction == ENTER_TRANSFER_MODE)
                    {
                        g_HotKnotState = HOTKNOT_TRANS_STATE;
                    }
                    else if(pRcv->nInstruction == EXIT_TRANSFER_MODE)
                    {
                        g_HotKnotState = HOTKNOT_AFTER_TRANS_STATE;
                    }                      
                    else if(pRcv->nInstruction == ENTER_SLAVE_MODE)
                    {
                        g_HotKnotState = HOTKNOT_NOT_TRANS_STATE;
                    }        	    	

                    if(pRcv->nInstruction == ENABLE_HOTKNOT || pRcv->nInstruction == ENTER_MASTER_MODE || pRcv->nInstruction == ENTER_SLAVE_MODE)
                    {
                        g_IsHotknotEnabled = 1;
                        DBG(&g_I2cClient->dev, "*** g_IsHotknotEnabled = %d ***\n", g_IsHotknotEnabled);
                    }                       
                    else if(pRcv->nInstruction == DISABLE_HOTKNOT)
                    {
                        g_IsHotknotEnabled = 0;
                        DBG(&g_I2cClient->dev, "*** g_IsHotknotEnabled = %d ***\n", g_IsHotknotEnabled);                        
                    }                     
                }
                                       
                memcpy(_gDrvCmdStack->pRcvData, pPacket, nLength);
		        //_gDrvCmdStack->nRcvLen = nLength;
                *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEBUG_MODE;   
            }
        }
    }
    else if(_gDrvCmdStack->nCmdId == HOTKNOT_SEND)
    {
        if(g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            HotKnotSnd_t *pSnd = (HotKnotSnd_t*)_gDrvCmdStack->pSndData;
            DemoHotKnotSndRet_t *pRcv = (DemoHotKnotSndRet_t*)pPacket;
            if(pRcv->nHeader == HOTKNOT_PACKET_ID && pRcv->nType == HOTKNOT_PACKET_TYPE && pSnd->nInstruction == pRcv->nInstruction)
            {
                //kthread_stop(_gHotKnotTask);                        
                memcpy(_gDrvCmdStack->pRcvData, pPacket, nLength);
		        //_gDrvCmdStack->nRcvLen = nLength;
                *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEMO_MODE;                         
            } 
        }
        else if(IS_FIRMWARE_DATA_LOG_ENABLED && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE))
        {
            HotKnotSnd_t *pSnd = (HotKnotSnd_t*)_gDrvCmdStack->pSndData;
            DebugHotKnotSndRet_t *pRcv = (DebugHotKnotSndRet_t*)pPacket;
            if(pRcv->nHeader == HOTKNOT_PACKET_ID && pRcv->nType == HOTKNOT_PACKET_TYPE && pSnd->nInstruction == pRcv->nInstruction)
            {
                //kthread_stop(_gHotKnotTask);                        
                memcpy(_gDrvCmdStack->pRcvData, pPacket, nLength);
		        //_gDrvCmdStack->nRcvLen = nLength;
                *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEBUG_MODE;                        
            } 
        }
    }
}


void _HotKnotTimeOutHandler(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    //handle timeout case              
    if(_gDrvCmdStack->nCmdId == HOTKNOT_CMD)
    {        
        if(g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            HotKnotSnd_t *pSnd = (HotKnotSnd_t*)_gDrvCmdStack->pSndData;
            DemoHotKnotCmdRet_t *pRcv = (DemoHotKnotCmdRet_t*)_gDrvCmdStack->pRcvData;                
            memset(pRcv, 0xFF, DEMO_PD_PACKET_RET_LEN);
            pRcv->nHeader = DEMO_PD_PACKET_ID;
            pRcv->nInstruction = pSnd->nInstruction;
            pRcv->nResult = RESULT_TIMEOUT;                                        
            pRcv->nIdentify = DEMO_PD_PACKET_IDENTIFY;                
            pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, DEMO_PD_PACKET_RET_LEN-1);
            //_gDrvCmdStack->nRcvLen = DEMO_PD_PACKET_RET_LEN;
            *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEMO_MODE;
        }
        else if(IS_FIRMWARE_DATA_LOG_ENABLED && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE))
        {
            HotKnotSnd_t *pSnd = (HotKnotSnd_t*)_gDrvCmdStack->pSndData;
            DebugHotKnotCmdRet_t *pRcv = (DebugHotKnotCmdRet_t*)_gDrvCmdStack->pRcvData;                
            memset(_gDrvCmdStack->pRcvData, 0xFF, MAX_PD_PACKET_RET_LEN);
            pRcv->nHeader = HOTKNOT_PACKET_ID;
            pRcv->nPacketLen_H = MAX_PD_PACKET_RET_LEN>>8;                   
            pRcv->nPacketLen_L = MAX_PD_PACKET_RET_LEN&0xFF;
            pRcv->nType = HOTKNOT_PACKET_TYPE;                
            pRcv->nInstruction = pSnd->nInstruction;
            pRcv->nResult = RESULT_TIMEOUT;
            _gDrvCmdStack->pRcvData[MAX_PD_PACKET_RET_LEN-1] = _GetCheckSum((u8*)pRcv, MAX_PD_PACKET_RET_LEN-1);
            //_gDrvCmdStack->nRcvLen = MAX_PD_PACKET_RET_LEN;
            *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEBUG_MODE;
        }            
    }
    else if(_gDrvCmdStack->nCmdId == HOTKNOT_SEND)
    {
        if(g_FirmwareMode == FIRMWARE_MODE_DEMO_MODE)
        {
            HotKnotSnd_t *pSnd = (HotKnotSnd_t*)_gDrvCmdStack->pSndData;
            DemoHotKnotSndRet_t *pRcv = (DemoHotKnotSndRet_t*)_gDrvCmdStack->pRcvData;                 
            memset(_gDrvCmdStack->pRcvData, 0, DEMO_HOTKNOT_SEND_RET_LEN);
            pRcv->nHeader = HOTKNOT_PACKET_ID;
            pRcv->nPacketLen_H = DEMO_HOTKNOT_SEND_RET_LEN>>8;
            pRcv->nPacketLen_L = DEMO_HOTKNOT_SEND_RET_LEN&0xFF;
            pRcv->nType = HOTKNOT_PACKET_TYPE;                                    
            pRcv->nInstruction = pSnd->nInstruction;
            pRcv->nResult = RESULT_TIMEOUT;                                        
            pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, DEMO_HOTKNOT_SEND_RET_LEN-1);
            //_gDrvCmdStack->nRcvLen = DEMO_HOTKNOT_SEND_RET_LEN;
            *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEMO_MODE;
        }
        else if(IS_FIRMWARE_DATA_LOG_ENABLED && (g_FirmwareMode == FIRMWARE_MODE_DEBUG_MODE))
        {
            HotKnotSnd_t *pSnd = (HotKnotSnd_t*)_gDrvCmdStack->pSndData;
            DebugHotKnotSndRet_t *pRcv = (DebugHotKnotSndRet_t*)_gDrvCmdStack->pRcvData;                
            memset(_gDrvCmdStack->pRcvData, 0, DEBUG_HOTKNOT_SEND_RET_LEN);
            pRcv->nHeader = HOTKNOT_PACKET_ID;
            pRcv->nPacketLen_H = DEBUG_HOTKNOT_SEND_RET_LEN>>8;
            pRcv->nPacketLen_L = DEBUG_HOTKNOT_SEND_RET_LEN&0xFF;
            pRcv->nType = HOTKNOT_PACKET_TYPE;                                    
            pRcv->nInstruction = pSnd->nInstruction;
            pRcv->nResult = RESULT_TIMEOUT;                                        
            pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, DEBUG_HOTKNOT_SEND_RET_LEN-1);
            //_gDrvCmdStack->nRcvLen = DEBUG_HOTKNOT_SEND_RET_LEN;
            *(_gDrvCmdStack->pFwMode) = FIRMWARE_MODE_DEBUG_MODE;
        }               
    }
}


int _DrvHandleHotKnotCmd(DrvCmd_t *pCmd, unsigned long nArg)
{
    long nRet;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    mutex_lock(&g_Mutex);
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    if (g_IsUpdateFirmware == 0 && g_IsBypassHotknot == 0)
    {
        DmaReset();
    }
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    IicWriteData(SLAVE_I2C_ID_DWI2C, pCmd->pSndData, pCmd->nSndLen);    //send HOTKNOT_CMD to fw 

    mutex_unlock(&g_Mutex);


	_gDrvCmdStack = pCmd;
	_gArgStack = nArg;
	
    set_current_state(TASK_INTERRUPTIBLE);  
    nRet = wait_event_interruptible_timeout(_gHKWaiter, _gHKFlag != 0, pCmd->nTimeOut/1000 * HZ);
    _gHKFlag = 0;
    
    if(nRet == 0)
    {
        _HotKnotTimeOutHandler();
    }
    else
    {
        _HotKnotCmdInterruptHandler(_gHKPacket, _gHKLength);    
    }
    
    return 0;
}


void _DrvHandleHotKnotAuth(DrvCmd_t *pCmd)
{    
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    mutex_lock(&g_Mutex);
#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    if (g_IsUpdateFirmware == 0 && g_IsBypassHotknot == 0)
    {
        DmaReset();
    }
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    //for(i=0; i < pCmd->nSndLen; i++)
    //{
    //    DBG(&g_I2cClient->dev, "%02X ", pCmd->pSndData[i]); 
    //}
    
    IicWriteData(SLAVE_I2C_ID_DWI2C, pCmd->pSndData, pCmd->nSndLen);
    mdelay(20);
    IicReadData(SLAVE_I2C_ID_DWI2C, pCmd->pRcvData, pCmd->nRcvLen);

    //for(i=0; i < pCmd->nRcvLen; i++)
    //{
    //    DBG(&g_I2cClient->dev, "%02X ", pCmd->pRcvData[i]); 
    //}    

    mutex_unlock(&g_Mutex);
}


#ifdef CONFIG_ENABLE_HOTKNOT_RCV_BLOCKING
void _DrvHandleHotKnotRcv(DrvCmd_t *pCmd)
{
    u8 nQueueData;
    int nDataLen = -1;
    long nRet;    
    DemoHotKnotLibRcvRet_t *pRcv = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    pCmd->nRcvLen = DEMO_HOTKNOT_RECEIVE_RET_LEN;
    pRcv = (DemoHotKnotLibRcvRet_t*)pCmd->pRcvData;  
    memset(pRcv, 0, DEMO_HOTKNOT_RECEIVE_RET_LEN);
    pRcv->nHeader = RECEIVE_DATA; 

    if(ShowHotKnotData(&nQueueData, 1) <= 0)    //check how many bytes to fetch
    {
        DBG(&g_I2cClient->dev, "ShowHotKnotData: No hotknot data in first check.\n");
    }
    else
    {
        nDataLen = PopHotKnotData(pRcv->szData, (nQueueData&0x7F) + 1);
    }
    
    if(nDataLen > 0)
    {
        pRcv->nActualHotKnotLen_H = nDataLen>>8;  
        pRcv->nActualHotKnotLen_L = nDataLen&0xFF;         
        pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, DEMO_HOTKNOT_RECEIVE_RET_LEN-1);        
        return;
    }

    _gHKRcvWaitEnable = 1;    //in wait status
    DBG(&g_I2cClient->dev, "*** _gHKRcvWaitEnable = 1, receive_timeout = %d***\n", pCmd->nTimeOut * HZ/1000);    
    set_current_state(TASK_INTERRUPTIBLE);  
    nRet = wait_event_interruptible_timeout(_gHKRcvWaiter, _gHKRcvFlag != 0, pCmd->nTimeOut * HZ/1000);
    _gHKRcvWaitEnable = 0;    //no wait
    DBG(&g_I2cClient->dev, "*** _gHKRcvWaitEnable = 0 ***\n");        
    _gHKRcvFlag = 0;


    if(nRet == 0)    //timeout
    {
        DBG(&g_I2cClient->dev, "*** receive_timeout ***\n");     
        pRcv->nActualHotKnotLen_H = 0;  
        pRcv->nActualHotKnotLen_L = 0;          
        pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, DEMO_HOTKNOT_RECEIVE_RET_LEN-1);
    }
    else
    {
        DBG(&g_I2cClient->dev, "*** receive_get_data ***\n");     
        int nDataLen = -1;    
        if(ShowHotKnotData(&nQueueData, 1) <= 0)    //check how many bytes to fetch
        {
            DBG(&g_I2cClient->dev, "ShowHotKnotData: No hotknot data\n");
        }
        else
        {
            nDataLen = PopHotKnotData(pRcv->szData, (nQueueData&0x7F) + 1);
        }
    
        if(nDataLen < 0)
        {
            pRcv->nActualHotKnotLen_H = 0;  
            pRcv->nActualHotKnotLen_L = 0;    		
        }
        else
        {
            pRcv->nActualHotKnotLen_H = nDataLen>>8;  
            pRcv->nActualHotKnotLen_L = nDataLen&0xFF;         
        }
        pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, DEMO_HOTKNOT_RECEIVE_RET_LEN-1); 
    }    
}
#else
void _DrvHandleHotKnotRcv(DrvCmd_t *pCmd)
{
    u8 nQueueData;
    int nDataLen = -1;
    DemoHotKnotLibRcvRet_t *pRcv = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
    
    pCmd->nRcvLen = DEMO_HOTKNOT_RECEIVE_RET_LEN;
    pRcv = (DemoHotKnotLibRcvRet_t*)pCmd->pRcvData;  
    memset(pRcv, 0, DEMO_HOTKNOT_RECEIVE_RET_LEN);
    pRcv->nHeader = RECEIVE_DATA; 

    if(ShowHotKnotData(&nQueueData, 1) <= 0)    //check how many bytes to fetch
    {
        DBG(&g_I2cClient->dev, "ShowHotKnotData: No hotknot data\n");
    }
    else
    {
        nDataLen = PopHotKnotData(pRcv->szData, (nQueueData&0x7F) + 1);
    }
    
    if(nDataLen < 0)
    {
        pRcv->nActualHotKnotLen_H = 0;  
        pRcv->nActualHotKnotLen_L = 0;    		
    }
    else
    {
        pRcv->nActualHotKnotLen_H = nDataLen>>8;  
        pRcv->nActualHotKnotLen_L = nDataLen&0xFF;         
    }
    pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, DEMO_HOTKNOT_RECEIVE_RET_LEN-1);
}
#endif


void _DrvHandleHotKnotGetQueue(DrvCmd_t *pCmd)
{
    DemoHotKnotGetQRet_t *pRcv = NULL;  

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    pCmd->nRcvLen = HOTKNOT_QUEUE_SIZE+6;
    pRcv = (DemoHotKnotGetQRet_t*)pCmd->pRcvData;  
    memset(pRcv, 0, HOTKNOT_QUEUE_SIZE+6);
    pRcv->nHeader = GET_QUEUE; 
    ShowAllQueue(pRcv->szData, &(pRcv->nFront), &(pRcv->nRear));
    pRcv->nCheckSum = _GetCheckSum((u8*)pRcv, HOTKNOT_QUEUE_SIZE+5);
}


void _DrvHandleHotKnotSndTest(DrvCmd_t *pCmd)
{
    u16 nQPushLen = 0;
    HotKnotSnd_t *pSnd = NULL;

    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
  
    pSnd = (HotKnotSnd_t*)pCmd->pSndData;
    nQPushLen = (pSnd->nDataLen_H << 8) | (pSnd->nDataLen_L & 0xFF);

    DBG(&g_I2cClient->dev, "nQPushLen = %d\n", nQPushLen);
    if(PushHotKnotData(pSnd->szData, nQPushLen)<0)
    {
        DBG(&g_I2cClient->dev, "Error: HotKnot# Over push data into queue.");
    }  

    //u8 * pShowArray = (u8*)kmalloc(sizeof(u8)* 256, GFP_KERNEL );
    //ShowHotKnotData(pShowArray, 256);    //check queue data
    //_DebugShowArray(pShowArray, 256);
    //kfree(pShowArray);
}


/*
static DrvCmd_t* _TransCmdFromUser( unsigned long nArg )
{
    long nRet; 
    DrvCmd_t tCmdIn;
    DrvCmd_t *pTransCmd = (DrvCmd_t*)kmalloc( sizeof( DrvCmd_t ), GFP_KERNEL );

    nRet = copy_from_user( &tCmdIn, (void*)nArg, sizeof( DrvCmd_t ) );
    pTransCmd->nCmdId = tCmdIn.nCmdId;
	
    pTransCmd->nSndLen = tCmdIn.nSndLen;
	pTransCmd->pSndData = (u8*)kmalloc(sizeof(u8)*pTransCmd->nSndLen, GFP_KERNEL );	
    nRet = copy_from_user( pTransCmd->pSndData, tCmdIn.pSndData, pTransCmd->nSndLen );    	

	pTransCmd->nRcvLen = tCmdIn.nRcvLen;
	pTransCmd->pRcvData = (u8*)kmalloc(sizeof(u8)*pTransCmd->nRcvLen, GFP_KERNEL );           

	pTransCmd->pFwMode= (u16*)kmalloc(sizeof(u16), GFP_KERNEL );
	pTransCmd->nTimeOut = tCmdIn.nTimeOut;
  
    return pTransCmd;
}


static void _TransCmdToUser( DrvCmd_t *pTransCmd, unsigned long nArg )
{
    DrvCmd_t tCmdOut;
    long nRet;
    
    nRet = copy_from_user( &tCmdOut, (void*)nArg, sizeof( DrvCmd_t ) );   	
    nRet = copy_to_user( tCmdOut.pRcvData, pTransCmd->pRcvData, tCmdOut.nRcvLen);
    nRet = copy_to_user( tCmdOut.pFwMode, pTransCmd->pFwMode, sizeof(u16));
    
    if ( pTransCmd )
    {
        kfree( pTransCmd->pSndData );		
        kfree( pTransCmd->pRcvData );
        kfree( pTransCmd->pFwMode );		
        kfree( pTransCmd );
    }
}
*/


void _ClearHotKnotMem(void)
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
  
    memset(_gCmdIn, 0, sizeof( DrvCmd_t ));
    memset(_gSndData, 0, DEBUG_HOTKNOT_SEND_RET_LEN);
    memset(_gRcvData, 0, DEBUG_HOTKNOT_RECEIVE_RET_LEN);
    memset(_gFwMode, 0, sizeof(u16));    
}


static DrvCmd_t* _TransCmdFromUser( unsigned long nArg )
{
    long nRet; 
    DrvCmd_t tCmdIn;    
    DrvCmd_t *pTransCmd;

    _ClearHotKnotMem();
    pTransCmd = (DrvCmd_t *)_gCmdIn;    
    nRet = copy_from_user( &tCmdIn, (void*)nArg, sizeof( DrvCmd_t ) );
    pTransCmd->nCmdId = tCmdIn.nCmdId;
	
    pTransCmd->nSndLen = tCmdIn.nSndLen;
	pTransCmd->pSndData = _gSndData;
    nRet = copy_from_user( pTransCmd->pSndData, tCmdIn.pSndData, pTransCmd->nSndLen );    	

	pTransCmd->nRcvLen = tCmdIn.nRcvLen;
	pTransCmd->pRcvData = _gRcvData;	

	pTransCmd->pFwMode= _gFwMode;	
	pTransCmd->nTimeOut = tCmdIn.nTimeOut;
  
    return pTransCmd;
}


static void _TransCmdToUser( DrvCmd_t *pTransCmd, unsigned long nArg )
{
    DrvCmd_t tCmdOut;
    long nRet;
    
    nRet = copy_from_user( &tCmdOut, (void*)nArg, sizeof( DrvCmd_t ) );   	
    nRet = copy_to_user( tCmdOut.pRcvData, pTransCmd->pRcvData, tCmdOut.nRcvLen);
    nRet = copy_to_user( tCmdOut.pFwMode, pTransCmd->pFwMode, sizeof(u16));
}


long HotKnotIoctl( struct file *pFile, unsigned int nCmd, unsigned long nArg )
{
    long nRet = 0;	
	
    switch ( nCmd )
    {
        case HOTKNOT_IOCTL_RUN_CMD:
            {      
                DrvCmd_t *pTransCmd;			

                DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
                
                pTransCmd = _TransCmdFromUser( nArg );

                if(pTransCmd->nCmdId == HOTKNOT_CMD ||
                  (pTransCmd->nCmdId == HOTKNOT_SEND && pTransCmd->pSndData[1] == SEND_DATA))
                {
				    _DrvHandleHotKnotCmd(pTransCmd, nArg);
					_TransCmdToUser(_gDrvCmdStack, _gArgStack);
      	            _gDrvCmdStack = NULL;
                    _gArgStack = 0;						
                } 					            
                else if(pTransCmd->nCmdId == HOTKNOT_AUTH || 
                       (pTransCmd->nCmdId == HOTKNOT_SEND && pTransCmd->pSndData[1] == AUTH_WRITECIPHER))
                {
				    _DrvHandleHotKnotAuth(pTransCmd);
				    _TransCmdToUser(pTransCmd, nArg);                   
                }
                else if(pTransCmd->nCmdId == HOTKNOT_RECEIVE && pTransCmd->pSndData[1] == RECEIVE_DATA)
                {
				    _DrvHandleHotKnotRcv(pTransCmd);
				    _TransCmdToUser(pTransCmd, nArg); 
                }
                else if(pTransCmd->nCmdId == HOTKNOT_RECEIVE && pTransCmd->pSndData[1] == GET_QUEUE)
                {
				    _DrvHandleHotKnotGetQueue(pTransCmd);
				    _TransCmdToUser(pTransCmd, nArg); 
                }                 
                else if(pTransCmd->nCmdId == HOTKNOT_SEND && pTransCmd->pSndData[1] == SEND_DATA_TEST)
                {
				    _DrvHandleHotKnotSndTest(pTransCmd);						
                }                
            }   
		    break;
		
        case HOTKNOT_IOCTL_QUERY_VENDOR:
            {
                 char szVendorName[30] = "msg28xx";

                 DBG(&g_I2cClient->dev, "*** Query vendor name! ***\n"); 
                 //_DebugShowArray(szVendorName,30);
                 if(copy_to_user((void*)nArg, szVendorName, 30))
                 {
                     DBG(&g_I2cClient->dev, "*** Query vendor name failed! ***\n");                      
                 }
            }
            break;
        
        default:
            nRet = -EINVAL;
            break;
    }

    return nRet;
}


void CreateHotKnotMem()
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);

    _gCmdIn = (DrvCmd_t*)kmalloc( sizeof( DrvCmd_t ), GFP_KERNEL );
	_gSndData = (u8*)kmalloc(DEBUG_HOTKNOT_SEND_RET_LEN, GFP_KERNEL );	
	_gRcvData = (u8*)kmalloc(DEBUG_HOTKNOT_RECEIVE_RET_LEN, GFP_KERNEL );           
	_gFwMode= (u16*)kmalloc(sizeof(u16), GFP_KERNEL );
}


void DeleteHotKnotMem()
{
    DBG(&g_I2cClient->dev, "*** %s() ***\n", __func__);
  
    if (_gCmdIn)
    {
        kfree(_gCmdIn);
        _gCmdIn = NULL;
    }
    
    if (_gSndData)
    {
        kfree(_gSndData);
        _gSndData = NULL;
    }
    
    if (_gRcvData)
    {
        kfree(_gRcvData);
        _gRcvData = NULL;
    }
    
    if (_gFwMode)
    {
        kfree(_gFwMode);
        _gFwMode = NULL;    
    }
}

#endif //CONFIG_ENABLE_HOTKNOT)
