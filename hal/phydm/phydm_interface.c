/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

//============================================================
// include files
//============================================================

#include "mp_precomp.h"
#include "phydm_precomp.h"

//
// ODM IO Relative API.
//

u1Byte
ODM_Read1Byte(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	u4Byte			RegAddr
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	return rtw_read8(Adapter,RegAddr);

}


u2Byte
ODM_Read2Byte(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	u4Byte			RegAddr
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	return rtw_read16(Adapter,RegAddr);

}


u4Byte
ODM_Read4Byte(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	u4Byte			RegAddr
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	return rtw_read32(Adapter,RegAddr);

}


VOID
ODM_Write1Byte(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	u4Byte			RegAddr,
	IN	u1Byte			Data
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	rtw_write8(Adapter,RegAddr, Data);
	
}


VOID
ODM_Write2Byte(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	u4Byte			RegAddr,
	IN	u2Byte			Data
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	rtw_write16(Adapter,RegAddr, Data);

}


VOID
ODM_Write4Byte(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	u4Byte			RegAddr,
	IN	u4Byte			Data
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	rtw_write32(Adapter,RegAddr, Data);

}


VOID
ODM_SetMACReg(	
	IN 	PDM_ODM_T	pDM_Odm,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	PHY_SetBBReg(Adapter, RegAddr, BitMask, Data);
}


u4Byte 
ODM_GetMACReg(	
	IN 	PDM_ODM_T	pDM_Odm,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask
	)
{
	return PHY_QueryBBReg(pDM_Odm->Adapter, RegAddr, BitMask);
}


VOID
ODM_SetBBReg(	
	IN 	PDM_ODM_T	pDM_Odm,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	PHY_SetBBReg(Adapter, RegAddr, BitMask, Data);
}


u4Byte 
ODM_GetBBReg(	
	IN 	PDM_ODM_T	pDM_Odm,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	return PHY_QueryBBReg(Adapter, RegAddr, BitMask);
}


VOID
ODM_SetRFReg(	
	IN 	PDM_ODM_T			pDM_Odm,
	IN	ODM_RF_RADIO_PATH_E	eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask,
	IN	u4Byte				Data
	)
{
	PHY_SetRFReg(pDM_Odm->Adapter, eRFPath, RegAddr, BitMask, Data);
}


u4Byte 
ODM_GetRFReg(	
	IN 	PDM_ODM_T			pDM_Odm,
	IN	ODM_RF_RADIO_PATH_E	eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask
	)
{
	PADAPTER		Adapter = pDM_Odm->Adapter;
	return PHY_QueryRFReg(Adapter, eRFPath, RegAddr, BitMask);
}




//
// ODM Memory relative API.
//
VOID
ODM_AllocateMemory(	
	IN 	PDM_ODM_T	pDM_Odm,
	OUT	PVOID		*pPtr,
	IN	u4Byte		length
	)
{
	*pPtr = rtw_zvmalloc(length);
}

// length could be ignored, used to detect memory leakage.
VOID
ODM_FreeMemory(	
	IN 	PDM_ODM_T	pDM_Odm,
	OUT	PVOID		pPtr,
	IN	u4Byte		length
	)
{
	rtw_vmfree(pPtr, length);
}

VOID
ODM_MoveMemory(	
	IN 	PDM_ODM_T	pDM_Odm,
	OUT PVOID		pDest,
	IN  PVOID		pSrc,
	IN  u4Byte		Length
	)
{
	_rtw_memcpy(pDest, pSrc, Length);
}

void ODM_Memory_Set(
	IN	PDM_ODM_T	pDM_Odm,
	IN	PVOID		pbuf,
	IN	s1Byte		value,
	IN	u4Byte		length
)
{
	_rtw_memset(pbuf,value, length);
}
s4Byte ODM_CompareMemory(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	PVOID           pBuf1,
	IN	PVOID           pBuf2,
	IN	u4Byte          length
       )
{
	return _rtw_memcmp(pBuf1,pBuf2,length);
}



//
// ODM MISC relative API.
//
VOID
ODM_AcquireSpinLock(	
	IN 	PDM_ODM_T			pDM_Odm,
	IN	RT_SPINLOCK_TYPE	type
	)
{
	PADAPTER Adapter = pDM_Odm->Adapter;
	rtw_odm_acquirespinlock(Adapter, type);
}
VOID
ODM_ReleaseSpinLock(	
	IN 	PDM_ODM_T			pDM_Odm,
	IN	RT_SPINLOCK_TYPE	type
	)
{
	PADAPTER Adapter = pDM_Odm->Adapter;
	rtw_odm_releasespinlock(Adapter, type);
}

//
// Work item relative API. FOr MP driver only~!
//
VOID
ODM_InitializeWorkItem(	
	IN 	PDM_ODM_T					pDM_Odm,
	IN	PRT_WORK_ITEM				pRtWorkItem,
	IN	RT_WORKITEM_CALL_BACK		RtWorkItemCallback,
	IN	PVOID						pContext,
	IN	const char*					szID
	)
{
}


VOID
ODM_StartWorkItem(	
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
}


VOID
ODM_StopWorkItem(	
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
}


VOID
ODM_FreeWorkItem(	
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
}


VOID
ODM_ScheduleWorkItem(	
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
}


VOID
ODM_IsWorkItemScheduled(	
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
}



//
// ODM Timer relative API.
//
VOID
ODM_StallExecution(	
	IN	u4Byte	usDelay
	)
{
	rtw_udelay_os(usDelay);
}

VOID
ODM_delay_ms(IN u4Byte	ms)
{
	rtw_mdelay_os(ms);
}

VOID
ODM_delay_us(IN u4Byte	us)
{
	rtw_udelay_os(us);
}

VOID
ODM_sleep_ms(IN u4Byte	ms)
{
	rtw_msleep_os(ms);
}

VOID
ODM_sleep_us(IN u4Byte	us)
{
	rtw_usleep_os(us);
}

VOID
ODM_SetTimer(	
	IN 	PDM_ODM_T		pDM_Odm,
	IN	PRT_TIMER 		pTimer, 
	IN	u4Byte 			msDelay
	)
{
	_set_timer(pTimer,msDelay ); //ms

}

VOID
ODM_InitializeTimer(
	IN 	PDM_ODM_T			pDM_Odm,
	IN	PRT_TIMER 			pTimer, 
	IN	RT_TIMER_CALL_BACK	CallBackFunc, 
	IN	PVOID				pContext,
	IN	const char*			szID
	)
{
	PADAPTER Adapter = pDM_Odm->Adapter;
	_init_timer(pTimer,Adapter->pnetdev,CallBackFunc,pDM_Odm);
}


VOID
ODM_CancelTimer(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	PRT_TIMER		pTimer
	)
{
	_cancel_timer_ex(pTimer);
}


VOID
ODM_ReleaseTimer(
	IN 	PDM_ODM_T		pDM_Odm,
	IN	PRT_TIMER		pTimer
	)
{
}

BOOLEAN
phydm_actingDetermine(
	IN PDM_ODM_T		pDM_Odm,
	IN PHYDM_ACTING_TYPE	type
	)
{
	BOOLEAN		ret = FALSE;
	PADAPTER	Adapter = pDM_Odm->Adapter;

	struct mlme_priv			*pmlmepriv = &(Adapter->mlmepriv);

	if (type == PhyDM_ACTING_AS_AP)
		ret = check_fwstate(pmlmepriv, WIFI_AP_STATE);
	else if (type == PhyDM_ACTING_AS_IBSS)
		ret = check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) || check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE);

	return ret;

}


u1Byte
phydm_trans_h2c_id(
	IN	PDM_ODM_T	pDM_Odm,
	IN	u1Byte		phydm_h2c_id
)
{
	u1Byte platform_h2c_id=0xff;

	
	switch(phydm_h2c_id)
	{
		//1 [0]
		case ODM_H2C_RSSI_REPORT:

				platform_h2c_id = H2C_RSSI_SETTING;

			
				break;

		//1 [3]	
		case ODM_H2C_WIFI_CALIBRATION:
			
				break;		
	
			
		//1 [4]
		case ODM_H2C_IQ_CALIBRATION:
			
				break;
		//1 [5]
		case ODM_H2C_RA_PARA_ADJUST:

			
				break;


		//1 [6]
		case PHYDM_H2C_DYNAMIC_TX_PATH:

			
				break;

		/* [7]*/
		case PHYDM_H2C_FW_TRACE_EN:

			
				break;

		case PHYDM_H2C_TXBF:
		break;

		default:
			platform_h2c_id=0xff;
			break;	
	}	
	
	return platform_h2c_id;
	
}

//
// ODM FW relative API.
//

VOID
ODM_FillH2CCmd(
	IN	PDM_ODM_T		pDM_Odm,
	IN	u1Byte 			phydm_h2c_id,
	IN	u4Byte 			CmdLen,
	IN	pu1Byte			pCmdBuffer
)
{
	PADAPTER 	Adapter = pDM_Odm->Adapter;
	u1Byte		platform_h2c_id;

	platform_h2c_id=phydm_trans_h2c_id(pDM_Odm, phydm_h2c_id);

	if(platform_h2c_id==0xff)
	{
		ODM_RT_TRACE(pDM_Odm,PHYDM_COMP_RA_DBG, ODM_DBG_LOUD, ("[H2C] Wrong H2C CMD-ID !! platform_h2c_id==0xff ,  PHYDM_ElementID=((%d )) \n",phydm_h2c_id));
		return;
	}

		rtw_hal_fill_h2c_cmd(Adapter, platform_h2c_id, CmdLen, pCmdBuffer);

}

u1Byte
phydm_c2H_content_parsing(
	IN	PVOID			pDM_VOID,
	IN	u1Byte			c2hCmdId,
	IN	u1Byte			c2hCmdLen,
	IN	pu1Byte			tmpBuf
)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
	u1Byte	Extend_c2hSubID = 0;
	u1Byte	find_c2h_cmd = TRUE;

	switch (c2hCmdId) {
	case PHYDM_C2H_RA_RPT:
		phydm_c2h_ra_report_handler(pDM_Odm, tmpBuf, c2hCmdLen);
		break;

	case PHYDM_C2H_IQK_FINISH:
		break;

	default:
		find_c2h_cmd = FALSE;
		break;
	}

	return find_c2h_cmd;

}

u8Byte
ODM_GetCurrentTime(	
	IN 	PDM_ODM_T		pDM_Odm
	)
{
	return (u8Byte)rtw_get_current_time();
}

u8Byte
ODM_GetProgressingTime(	
	IN 	PDM_ODM_T		pDM_Odm,
	IN	u8Byte			Start_Time
	)
{
	return rtw_get_passing_time_ms((u4Byte)Start_Time);
}


