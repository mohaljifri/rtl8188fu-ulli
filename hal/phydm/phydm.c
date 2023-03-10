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

const u2Byte dB_Invert_Table[12][8] = {
	{	1,		1,		1,		2,		2,		2,		2,		3},
	{	3,		3,		4,		4,		4,		5,		6,		6},
	{	7,		8,		9,		10,		11,		13,		14,		16},
	{	18,		20,		22,		25,		28,		32,		35,		40},
	{	45,		50,		56,		63,		71,		79,		89,		100},
	{	112,		126,		141,		158,		178,		200,		224,		251},
	{	282,		316,		355,		398,		447,		501,		562,		631},
	{	708,		794,		891,		1000,	1122,	1259,	1413,	1585},
	{	1778,	1995,	2239,	2512,	2818,	3162,	3548,	3981},
	{	4467,	5012,	5623,	6310,	7079,	7943,	8913,	10000},
	{	11220,	12589,	14125,	15849,	17783,	19953,	22387,	25119},
	{	28184,	31623,	35481,	39811,	44668,	50119,	56234,	65535}
};


//============================================================
// Local Function predefine.
//============================================================

/* START------------COMMON INFO RELATED--------------- */

VOID
odm_GlobalAdapterCheck(
	IN		VOID
	);

//move to odm_PowerTacking.h by YuChen


//============================================================
//3 Export Interface
//============================================================

/*Y = 10*log(X)*/
s4Byte
ODM_PWdB_Conversion(
	IN  s4Byte X,
	IN  u4Byte TotalBit,
	IN  u4Byte DecimalBit
	)
{
	s4Byte Y, integer = 0, decimal = 0;
	u4Byte i;

	if(X == 0)
		X = 1; // log2(x), x can't be 0

	for(i = (TotalBit-1); i > 0; i--)
	{
		if(X & BIT(i))
		{
			integer = i;
			if(i > 0)
				decimal = (X & BIT(i-1))?2:0; //decimal is 0.5dB*3=1.5dB~=2dB 
			break;
		}
	}
	
	Y = 3*(integer-DecimalBit)+decimal; //10*log(x)=3*log2(x), 

	return Y;
}

s4Byte
ODM_SignConversion(
    IN  s4Byte value,
    IN  u4Byte TotalBit
    )
{
	if(value&BIT(TotalBit-1))
		value -= BIT(TotalBit);
	return value;
}

VOID
phydm_Init_cck_setting(
	IN		PDM_ODM_T		pDM_Odm
)
{
	u4Byte value_824,value_82c;

	pDM_Odm->bCckHighPower = (BOOLEAN) ODM_GetBBReg(pDM_Odm, ODM_REG(CCK_RPT_FORMAT,pDM_Odm), ODM_BIT(CCK_RPT_FORMAT,pDM_Odm));

}

u1Byte DummyHubUsbMode = 1;/* USB 2.0 */
void	phydm_hook_dummy_member(
	IN	PDM_ODM_T		pDM_Odm
	)
{
	if (pDM_Odm->HubUsbMode == NULL)
		pDM_Odm->HubUsbMode = &DummyHubUsbMode;
}


VOID
odm_CommonInfoSelfInit(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	phydm_Init_cck_setting(pDM_Odm);
	pDM_Odm->RFPathRxEnable = (u1Byte) ODM_GetBBReg(pDM_Odm, ODM_REG(BB_RX_PATH,pDM_Odm), ODM_BIT(BB_RX_PATH,pDM_Odm));

	pDM_Odm->TxRate = 0xFF;

	pDM_Odm->number_linked_client = 0;
	pDM_Odm->pre_number_linked_client = 0;
	pDM_Odm->number_active_client = 0;
	pDM_Odm->pre_number_active_client = 0;
	phydm_hook_dummy_member(pDM_Odm);
	
}

VOID
odm_CommonInfoSelfUpdate(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	u1Byte	EntryCnt = 0, num_active_client = 0;
	u4Byte	i, OneEntry_MACID = 0, ma_rx_tp = 0;
	PSTA_INFO_T   	pEntry;


/* THis variable cannot be used because it is wrong*/
	if (*(pDM_Odm->pBandWidth) == ODM_BW40M) {
		if (*(pDM_Odm->pSecChOffset) == 1)
			pDM_Odm->ControlChannel = *(pDM_Odm->pChannel) - 2;
		else if (*(pDM_Odm->pSecChOffset) == 2)
			pDM_Odm->ControlChannel = *(pDM_Odm->pChannel) + 2;
	} else
		pDM_Odm->ControlChannel = *(pDM_Odm->pChannel);

	pDM_Odm->pre_number_linked_client = pDM_Odm->number_linked_client;
	pDM_Odm->pre_number_active_client = pDM_Odm->number_active_client;
	
	pDM_Odm->number_linked_client = EntryCnt;
	pDM_Odm->number_active_client = num_active_client;	

}

PVOID
PhyDM_Get_Structure(
	IN		PDM_ODM_T		pDM_Odm,
	IN		u1Byte			Structure_Type
)

{
	PVOID	pStruct = NULL;
	switch (Structure_Type){
		case	PHYDM_FALSEALMCNT:
			pStruct = &(pDM_Odm->FalseAlmCnt);
		break;
		
		case	PHYDM_CFOTRACK:
			pStruct = &(pDM_Odm->DM_CfoTrack);
		break;

		case	PHYDM_ADAPTIVITY:
			pStruct = &(pDM_Odm->Adaptivity);
		break;
		
		default:
		break;
	}

	return	pStruct;
}

//
// 2011/09/21 MH Add to describe different team necessary resource allocate??
//
VOID
ODM_DMInit(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	odm_CommonInfoSelfInit(pDM_Odm);
	odm_DIGInit(pDM_Odm);
	Phydm_NHMCounterStatisticsInit(pDM_Odm);
	Phydm_AdaptivityInit(pDM_Odm);
	phydm_ra_info_init(pDM_Odm);
	odm_RateAdaptiveMaskInit(pDM_Odm);
	ODM_CfoTrackingInit(pDM_Odm);
	ODM_EdcaTurboInit(pDM_Odm);
	odm_RSSIMonitorInit(pDM_Odm);
	phydm_rf_init(pDM_Odm);
	odm_TXPowerTrackingInit(pDM_Odm);
	odm_AutoChannelSelectInit(pDM_Odm);
	odm_PathDiversityInit(pDM_Odm);

	if(pDM_Odm->SupportICType & ODM_IC_11N_SERIES)
	{
		odm_DynamicTxPowerInit(pDM_Odm);


	}

}

VOID
ODM_DMReset(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	pDIG_T pDM_DigTable = &pDM_Odm->DM_DigTable;
	
	phydm_setEDCCAThresholdAPI(pDM_Odm, pDM_DigTable->CurIGValue);
}


VOID
phydm_support_ablity_debug(
	IN		PVOID		pDM_VOID,
	IN		u4Byte		*const dm_value,
	IN		u4Byte			*_used,
	OUT		char			*output,
	IN		u4Byte			*_out_len
	)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
	u4Byte			pre_support_ability;
	u4Byte used = *_used;
	u4Byte out_len = *_out_len;

	pre_support_ability = pDM_Odm->SupportAbility ;	
	PHYDM_SNPRINTF((output+used, out_len-used,"\n%s\n", "================================"));
	if(dm_value[0] == 100)
	{
		PHYDM_SNPRINTF((output+used, out_len-used, "[Supportablity] PhyDM Selection\n"));
		PHYDM_SNPRINTF((output+used, out_len-used,"%s\n", "================================"));
		PHYDM_SNPRINTF((output+used, out_len-used, "00. (( %s ))DIG  \n", ((pDM_Odm->SupportAbility & ODM_BB_DIG)?("V"):(".")) ));
		PHYDM_SNPRINTF((output+used, out_len-used, "01. (( %s ))RA_MASK  \n", ((pDM_Odm->SupportAbility & ODM_BB_RA_MASK)?("V"):(".")) ));
		PHYDM_SNPRINTF((output+used, out_len-used, "02. (( %s ))DYNAMIC_TXPWR  \n", ((pDM_Odm->SupportAbility & ODM_BB_DYNAMIC_TXPWR)?("V"):("."))   ));		
		PHYDM_SNPRINTF((output+used, out_len-used, "03. (( %s ))FA_CNT  \n", ((pDM_Odm->SupportAbility & ODM_BB_FA_CNT)?("V"):("."))  ));
		PHYDM_SNPRINTF((output+used, out_len-used, "04. (( %s ))RSSI_MONITOR  \n", ((pDM_Odm->SupportAbility & ODM_BB_RSSI_MONITOR)?("V"):("."))   ));
		PHYDM_SNPRINTF((output+used, out_len-used, "05. (( %s ))CCK_PD  \n", ((pDM_Odm->SupportAbility & ODM_BB_CCK_PD)?("V"):("."))   ));	
		PHYDM_SNPRINTF((output+used, out_len-used, "09. (( %s ))RATE_ADAPTIVE  \n", ((pDM_Odm->SupportAbility & ODM_BB_RATE_ADAPTIVE)?("V"):("."))   ));
		PHYDM_SNPRINTF((output+used, out_len-used, "14. (( %s ))CFO_TRACKING  \n", ((pDM_Odm->SupportAbility & ODM_BB_CFO_TRACKING)?("V"):(".")) ));
		PHYDM_SNPRINTF((output+used, out_len-used, "15. (( %s ))NHM_CNT  \n", ((pDM_Odm->SupportAbility & ODM_BB_NHM_CNT)?("V"):("."))  ));	
		PHYDM_SNPRINTF((output+used, out_len-used, "20. (( %s ))EDCA_TURBO  \n", ((pDM_Odm->SupportAbility & ODM_MAC_EDCA_TURBO)?("V"):("."))  ));	
		PHYDM_SNPRINTF((output+used, out_len-used, "24. (( %s ))TX_PWR_TRACK  \n", ((pDM_Odm->SupportAbility & ODM_RF_TX_PWR_TRACK)?("V"):("."))  ));	
		PHYDM_SNPRINTF((output+used, out_len-used, "26. (( %s ))RF_CALIBRATION  \n", ((pDM_Odm->SupportAbility & ODM_RF_CALIBRATION)?("V"):("."))   ));
		PHYDM_SNPRINTF((output+used, out_len-used,"%s\n", "================================"));
	}
	/*
	else if(dm_value[0] == 101)
	{
		pDM_Odm->SupportAbility = 0 ;
		DbgPrint("Disable all SupportAbility components \n");
		PHYDM_SNPRINTF((output+used, out_len-used,"%s\n", "Disable all SupportAbility components"));	
	}
	*/
	else
	{

	}
	PHYDM_SNPRINTF((output+used, out_len-used,"pre-SupportAbility  =  0x%x\n",  pre_support_ability ));	
	PHYDM_SNPRINTF((output+used, out_len-used,"Curr-SupportAbility =  0x%x\n", pDM_Odm->SupportAbility ));
	PHYDM_SNPRINTF((output+used, out_len-used,"%s\n", "================================"));
}

//
// 2011/09/20 MH This is the entry pointer for all team to execute HW out source DM.
// You can not add any dummy function here, be care, you can only use DM structure
// to perform any new ODM_DM.
//
VOID
_rtl8188fu_dm_watchdog(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	odm_CommonInfoSelfUpdate(pDM_Odm);

	/* ULLI : function name in rtlwifi */
	rtl9188fu_dm_false_alarm_counter_statistics(pDM_Odm);

	/* ULLI : phydm functions for rate decition and RSSI phy*/
	odm_RSSIMonitorCheck(pDM_Odm);

	if(*(pDM_Odm->pbPowerSaving) == TRUE)
	{
		odm_DIGbyRSSI_LPS(pDM_Odm);
		{
			pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
			Phydm_Adaptivity(pDM_Odm, pDM_DigTable->CurIGValue);
		}
		ODM_RT_TRACE(pDM_Odm,ODM_COMP_COMMON, ODM_DBG_LOUD, ("DMWatchdog in power saving mode\n"));
		return;
	}
	
	/* ULLI : obscure Adaptivity */
	Phydm_CheckAdaptivity(pDM_Odm);

	/* ULLI : function name should be in rtlwifi, I'm lazy */
	odm_DIG(pDM_Odm);
	{
		pDIG_T	pDM_DigTable = &pDM_Odm->DM_DigTable;
		Phydm_Adaptivity(pDM_Odm, pDM_DigTable->CurIGValue);
	}

	/* ULLI : function name in rtlwifi */
	odm_CCKPacketDetectionThresh(pDM_Odm);

	/* ULLI : phydm functions for rate decition , in mac80211 */
	odm_RefreshRateAdaptiveMask(pDM_Odm);
	odm_RefreshBasicRateMask(pDM_Odm);

	/* ULLI : function name in rtlwifi */
	odm_EdcaTurboCheck(pDM_Odm);

	/* ULLI : in rtlwifi but we may have one antenna */
	odm_PathDiversity(pDM_Odm);
	ODM_CfoTracking(pDM_Odm);

}


//
// Init /.. Fixed HW value. Only init time.
//
VOID
ODM_CmnInfoInit(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		u4Byte			Value	
	)
{
	//
	// This section is used for init value
	//
	switch	(CmnInfo)
	{
		//
		// Fixed ODM value.
		//
		case	ODM_CMNINFO_ABILITY:
			pDM_Odm->SupportAbility = (u4Byte)Value;
			break;

		case	ODM_CMNINFO_RF_TYPE:
			pDM_Odm->RFType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_INTERFACE:
			pDM_Odm->SupportInterface = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_MP_TEST_CHIP:
			pDM_Odm->bIsMPChip= (u1Byte)Value;
			break;
            
		case	ODM_CMNINFO_IC_TYPE:
			pDM_Odm->SupportICType = Value;
			break;

		case	ODM_CMNINFO_CUT_VER:
			pDM_Odm->CutVersion = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_FAB_VER:
			pDM_Odm->FabVersion = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_RFE_TYPE:
			pDM_Odm->RFEType = (u1Byte)Value;
			break;

		case    ODM_CMNINFO_RF_ANTENNA_TYPE:
			pDM_Odm->AntDivType= (u1Byte)Value;
			break;

		case	ODM_CMNINFO_PACKAGE_TYPE:
			pDM_Odm->PackageType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_GPA:
			pDM_Odm->TypeGPA = (u2Byte)Value;
			break;
		case	ODM_CMNINFO_APA:
			pDM_Odm->TypeAPA = (u2Byte)Value;
			break;
		case	ODM_CMNINFO_GLNA:
			pDM_Odm->TypeGLNA = (u2Byte)Value;
			break;
		case	ODM_CMNINFO_ALNA:
			pDM_Odm->TypeALNA = (u2Byte)Value;
			break;

		case	ODM_CMNINFO_EXT_TRSW:
			pDM_Odm->ExtTRSW = (u1Byte)Value;
			break;
		case 	ODM_CMNINFO_PATCH_ID:
			pDM_Odm->PatchID = (u1Byte)Value;
			break;
		case 	ODM_CMNINFO_BINHCT_TEST:
			pDM_Odm->bInHctTest = (BOOLEAN)Value;
			break;
		case	ODM_CMNINFO_SMART_CONCURRENT:
			pDM_Odm->bDualMacSmartConcurrent = (BOOLEAN )Value;
			break;
		case	ODM_CMNINFO_DOMAIN_CODE_2G:
			pDM_Odm->odm_Regulation2_4G = (u1Byte)Value;
			break;
		case	ODM_CMNINFO_DOMAIN_CODE_5G:
			pDM_Odm->odm_Regulation5G = (u1Byte)Value;
			break;
		case	ODM_CMNINFO_CONFIG_BB_RF:
			pDM_Odm->ConfigBBRF = (BOOLEAN)Value;
			break;
		case	ODM_CMNINFO_IQKFWOFFLOAD:
			pDM_Odm->IQKFWOffload = (u1Byte)Value;
			break;
		//To remove the compiler warning, must add an empty default statement to handle the other values.	
		default:
			//do nothing
			break;	
		
	}

}


VOID
ODM_CmnInfoHook(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		PVOID			pValue	
	)
{
	//
	// Hook call by reference pointer.
	//
	switch	(CmnInfo)
	{
		//
		// Dynamic call by reference pointer.
		//
		case	ODM_CMNINFO_MAC_PHY_MODE:
			pDM_Odm->pMacPhyMode = (u1Byte *)pValue;
			break;
		
		case	ODM_CMNINFO_TX_UNI:
			pDM_Odm->pNumTxBytesUnicast = (u8Byte *)pValue;
			break;

		case	ODM_CMNINFO_RX_UNI:
			pDM_Odm->pNumRxBytesUnicast = (u8Byte *)pValue;
			break;

		case	ODM_CMNINFO_WM_MODE:
			pDM_Odm->pWirelessMode = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_BAND:
			pDM_Odm->pBandType = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_SEC_CHNL_OFFSET:
			pDM_Odm->pSecChOffset = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_SEC_MODE:
			pDM_Odm->pSecurity = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_BW:
			pDM_Odm->pBandWidth = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_CHNL:
			pDM_Odm->pChannel = (u1Byte *)pValue;
			break;
		
		case	ODM_CMNINFO_DMSP_GET_VALUE:
			pDM_Odm->pbGetValueFromOtherMac = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_BUDDY_ADAPTOR:
			pDM_Odm->pBuddyAdapter = (PADAPTER *)pValue;
			break;

		case	ODM_CMNINFO_DMSP_IS_MASTER:
			pDM_Odm->pbMasterOfDMSP = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_SCAN:
			pDM_Odm->pbScanInProcess = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_POWER_SAVING:
			pDM_Odm->pbPowerSaving = (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_ONE_PATH_CCA:
			pDM_Odm->pOnePathCCA = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_DRV_STOP:
			pDM_Odm->pbDriverStopped =  (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_PNP_IN:
			pDM_Odm->pbDriverIsGoingToPnpSetPowerSleep =  (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_INIT_ON:
			pDM_Odm->pinit_adpt_in_progress =  (BOOLEAN *)pValue;
			break;

		case	ODM_CMNINFO_ANT_TEST:
			pDM_Odm->pAntennaTest =  (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_NET_CLOSED:
			pDM_Odm->pbNet_closed = (BOOLEAN *)pValue;
			break;

		case 	ODM_CMNINFO_FORCED_RATE:
			pDM_Odm->pForcedDataRate = (pu2Byte)pValue;
			break;

		case  ODM_CMNINFO_FORCED_IGI_LB:
			pDM_Odm->pu1ForcedIgiLb = (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_P2P_LINK:
			pDM_Odm->DM_DigTable.bP2PInProcess = (u1Byte *)pValue;
			break;

		case 	ODM_CMNINFO_IS1ANTENNA:
			pDM_Odm->pIs1Antenna = (BOOLEAN *)pValue;
			break;
			
		case 	ODM_CMNINFO_RFDEFAULTPATH:
			pDM_Odm->pRFDefaultPath= (u1Byte *)pValue;
			break;

		case	ODM_CMNINFO_FCS_MODE:
			pDM_Odm->pIsFcsModeEnable = (BOOLEAN *)pValue;
			break;
		/*add by YuChen for beamforming PhyDM*/
		case	ODM_CMNINFO_HUBUSBMODE:
			pDM_Odm->HubUsbMode = (u1Byte *)pValue;
			break;
		case	ODM_CMNINFO_FWDWRSVDPAGEINPROGRESS:
			pDM_Odm->pbFwDwRsvdPageInProgress = (BOOLEAN *)pValue;
			break;
		case	ODM_CMNINFO_TX_TP:
			pDM_Odm->pCurrentTxTP = (u4Byte *)pValue;
			break;
		case	ODM_CMNINFO_RX_TP:
			pDM_Odm->pCurrentRxTP = (u4Byte *)pValue;
			break;
		case	ODM_CMNINFO_SOUNDING_SEQ:
			pDM_Odm->pSoundingSeq = (u1Byte *)pValue;
			break;
		//case	ODM_CMNINFO_RTSTA_AID:
		//	pDM_Odm->pAidMap =  (u1Byte *)pValue;
		//	break;

		//case	ODM_CMNINFO_BT_COEXIST:
		//	pDM_Odm->BTCoexist = (BOOLEAN *)pValue;		

		//case	ODM_CMNINFO_STA_STATUS:
			//pDM_Odm->pODM_StaInfo[] = (PSTA_INFO_T)pValue;
			//break;

		//case	ODM_CMNINFO_PHY_STATUS:
		//	pDM_Odm->pPhyInfo = (ODM_PHY_INFO *)pValue;
		//	break;

		//case	ODM_CMNINFO_MAC_STATUS:
		//	pDM_Odm->pMacInfo = (ODM_MAC_INFO *)pValue;
		//	break;
		//To remove the compiler warning, must add an empty default statement to handle the other values.				
		default:
			//do nothing
			break;

	}

}


VOID
ODM_CmnInfoPtrArrayHook(
	IN		PDM_ODM_T		pDM_Odm,
	IN		ODM_CMNINFO_E	CmnInfo,
	IN		u2Byte			Index,
	IN		PVOID			pValue	
	)
{
	//
	// Hook call by reference pointer.
	//
	switch	(CmnInfo)
	{
		//
		// Dynamic call by reference pointer.
		//		
		case	ODM_CMNINFO_STA_STATUS:
			pDM_Odm->pODM_StaInfo[Index] = (PSTA_INFO_T)pValue;
			
			if (IS_STA_VALID(pDM_Odm->pODM_StaInfo[Index]))
				pDM_Odm->platform2phydm_macid_table[((PSTA_INFO_T)pValue)->mac_id] = Index;
			
			break;		
		//To remove the compiler warning, must add an empty default statement to handle the other values.				
		default:
			//do nothing
			break;
	}
	
}


//
// Update Band/CHannel/.. The values are dynamic but non-per-packet.
//
VOID
ODM_CmnInfoUpdate(
	IN		PDM_ODM_T		pDM_Odm,
	IN		u4Byte			CmnInfo,
	IN		u8Byte			Value	
	)
{
	//
	// This init variable may be changed in run time.
	//
	switch	(CmnInfo)
	{
		case ODM_CMNINFO_LINK_IN_PROGRESS:
			pDM_Odm->bLinkInProcess = (BOOLEAN)Value;
			break;
		
		case	ODM_CMNINFO_ABILITY:
			pDM_Odm->SupportAbility = (u4Byte)Value;
			break;

		case	ODM_CMNINFO_RF_TYPE:
			pDM_Odm->RFType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_WIFI_DIRECT:
			pDM_Odm->bWIFI_Direct = (BOOLEAN)Value;
			break;

		case	ODM_CMNINFO_WIFI_DISPLAY:
			pDM_Odm->bWIFI_Display = (BOOLEAN)Value;
			break;

		case	ODM_CMNINFO_LINK:
			pDM_Odm->bLinked = (BOOLEAN)Value;
			break;

		case	ODM_CMNINFO_STATION_STATE:
			pDM_Odm->bsta_state = (BOOLEAN)Value;
			break;
			
		case	ODM_CMNINFO_RSSI_MIN:
			pDM_Odm->RSSI_Min= (u1Byte)Value;
			break;

		case	ODM_CMNINFO_RA_THRESHOLD_HIGH:
			pDM_Odm->RateAdaptive.HighRSSIThresh = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_RA_THRESHOLD_LOW:
			pDM_Odm->RateAdaptive.LowRSSIThresh = (u1Byte)Value;
			break;
#if defined(BT_30_SUPPORT) && (BT_30_SUPPORT == 1)
		// The following is for BT HS mode and BT coexist mechanism.
		case ODM_CMNINFO_BT_ENABLED:
			pDM_Odm->bBtEnabled = (BOOLEAN)Value;
			break;
			
		case ODM_CMNINFO_BT_HS_CONNECT_PROCESS:
			pDM_Odm->bBtConnectProcess = (BOOLEAN)Value;
			break;
		
		case ODM_CMNINFO_BT_HS_RSSI:
			pDM_Odm->btHsRssi = (u1Byte)Value;
			break;
			
		case	ODM_CMNINFO_BT_OPERATION:
			pDM_Odm->bBtHsOperation = (BOOLEAN)Value;
			break;

		case	ODM_CMNINFO_BT_LIMITED_DIG:
			pDM_Odm->bBtLimitedDig = (BOOLEAN)Value;
			break;	

		case ODM_CMNINFO_BT_DIG:
			pDM_Odm->btHsDigVal = (u1Byte)Value;
			break;
			
		case	ODM_CMNINFO_BT_BUSY:
			pDM_Odm->bBtBusy = (BOOLEAN)Value;
			break;	

		case	ODM_CMNINFO_BT_DISABLE_EDCA:
			pDM_Odm->bBtDisableEdcaTurbo = (BOOLEAN)Value;
			break;
#endif

		case	ODM_CMNINFO_AP_TOTAL_NUM:
			pDM_Odm->APTotalNum = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_POWER_TRAINING:
			pDM_Odm->bDisablePowerTraining = (BOOLEAN)Value;
			break;

/*
		case	ODM_CMNINFO_OP_MODE:
			pDM_Odm->OPMode = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_WM_MODE:
			pDM_Odm->WirelessMode = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_BAND:
			pDM_Odm->BandType = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_SEC_CHNL_OFFSET:
			pDM_Odm->SecChOffset = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_SEC_MODE:
			pDM_Odm->Security = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_BW:
			pDM_Odm->BandWidth = (u1Byte)Value;
			break;

		case	ODM_CMNINFO_CHNL:
			pDM_Odm->Channel = (u1Byte)Value;
			break;			
*/	
                default:
			//do nothing
			break;
	}

	
}


/*
VOID
odm_FindMinimumRSSI(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	u4Byte	i;
	u1Byte	RSSI_Min = 0xFF;

	for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++)
	{
//		if(pDM_Odm->pODM_StaInfo[i] != NULL)
		if(IS_STA_VALID(pDM_Odm->pODM_StaInfo[i]) )
		{
			if(pDM_Odm->pODM_StaInfo[i]->RSSI_Ave < RSSI_Min)
			{
				RSSI_Min = pDM_Odm->pODM_StaInfo[i]->RSSI_Ave;
			}
		}
	}

	pDM_Odm->RSSI_Min = RSSI_Min;

}

VOID
odm_IsLinked(
	IN		PDM_ODM_T		pDM_Odm
	)
{
	u4Byte i;
	BOOLEAN Linked = FALSE;
	
	for(i=0; i<ODM_ASSOCIATE_ENTRY_NUM; i++)
	{
			if(IS_STA_VALID(pDM_Odm->pODM_StaInfo[i]) )
			{			
				Linked = TRUE;
				break;
			}
		
	}

	pDM_Odm->bLinked = Linked;
}
*/

VOID
ODM_InitAllTimers(
	IN PDM_ODM_T	pDM_Odm 
	)
{

}

VOID
ODM_CancelAllTimers(
	IN PDM_ODM_T	pDM_Odm 
	)
{


}


VOID
ODM_ReleaseAllTimers(
	IN PDM_ODM_T	pDM_Odm 
	)
{


}


//3============================================================
//3 Tx Power Tracking
//3============================================================







// need to ODM CE Platform
//move to here for ANT detection mechanism using

u4Byte
GetPSDData(
	IN PDM_ODM_T	pDM_Odm,
	unsigned int 	point,
	u1Byte initial_gain_psd)
{
	//unsigned int	val, rfval;
	//int	psd_report;
	u4Byte	psd_report;
	
	//HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	//Debug Message
	//val = PHY_QueryBBReg(Adapter,0x908, bMaskDWord);
	//DbgPrint("Reg908 = 0x%x\n",val);
	//val = PHY_QueryBBReg(Adapter,0xDF4, bMaskDWord);
	//rfval = PHY_QueryRFReg(Adapter, ODM_RF_PATH_A, 0x00, bRFRegOffsetMask);
	//DbgPrint("RegDF4 = 0x%x, RFReg00 = 0x%x\n",val, rfval);
	//DbgPrint("PHYTXON = %x, OFDMCCA_PP = %x, CCKCCA_PP = %x, RFReg00 = %x\n",
		//(val&BIT25)>>25, (val&BIT14)>>14, (val&BIT15)>>15, rfval);

	//Set DCO frequency index, offset=(40MHz/SamplePts)*point
	ODM_SetBBReg(pDM_Odm, 0x808, 0x3FF, point);

	//Start PSD calculation, Reg808[22]=0->1
	ODM_SetBBReg(pDM_Odm, 0x808, BIT22, 1);
	//Need to wait for HW PSD report
	ODM_StallExecution(1000);
	ODM_SetBBReg(pDM_Odm, 0x808, BIT22, 0);
	//Read PSD report, Reg8B4[15:0]
	psd_report = ODM_GetBBReg(pDM_Odm,0x8B4, bMaskDWord) & 0x0000FFFF;
	
	psd_report = (u4Byte) (odm_ConvertTo_dB(psd_report))+(u4Byte)(initial_gain_psd-0x1c);

	return psd_report;
	
}

u4Byte 
odm_ConvertTo_dB(
	u4Byte 	Value)
{
	u1Byte i;
	u1Byte j;
	u4Byte dB;

	Value = Value & 0xFFFF;

	for (i = 0; i < 12; i++)
	{
		if (Value <= dB_Invert_Table[i][7])
		{
			break;
		}
	}

	if (i >= 12)
	{
		return (96);	// maximum 96 dB
	}

	for (j = 0; j < 8; j++)
	{
		if (Value <= dB_Invert_Table[i][j])
		{
			break;
		}
	}

	dB = (i << 3) + j + 1;

	return (dB);
}

u4Byte 
odm_ConvertTo_linear(
	u4Byte 	Value)
{
	u1Byte i;
	u1Byte j;
	u4Byte linear;
	
	/* 1dB~96dB */
	
	Value = Value & 0xFF;

	i = (u1Byte)((Value - 1) >> 3);
	j = (u1Byte)(Value - 1) - (i << 3);

	linear = dB_Invert_Table[i][j];

	return (linear);
}

//
// ODM multi-port consideration, added by Roger, 2013.10.01.
//
VOID
ODM_AsocEntry_Init(
	IN	PDM_ODM_T	pDM_Odm
	)
{
}

/*===========================================================*/
/* The following is for compile only*/
/*===========================================================*/
/*#define TARGET_CHNL_NUM_2G_5G	59*/
/*===========================================================*/

