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

#define READ_AND_CONFIG_MP(ic, txt) (ODM_ReadAndConfig_MP_##ic##txt(pDM_Odm))


  #define READ_AND_CONFIG     READ_AND_CONFIG_MP
						
#define GET_VERSION_MP(ic, txt) 		(ODM_GetVersion_MP_##ic##txt())
#define GET_VERSION_TC(ic, txt) 		(ODM_GetVersion_TC_##ic##txt())
#define GET_VERSION(ic, txt) (pDM_Odm->bIsMPChip?GET_VERSION_MP(ic,txt):GET_VERSION_TC(ic,txt))

u1Byte
odm_QueryRxPwrPercentage(
	IN		s1Byte		AntPower
	)
{
	if ((AntPower <= -100) || (AntPower >= 20))
	{
		return	0;
	}
	else if (AntPower >= 0)
	{
		return	100;
	}
	else
	{
		return	(100+AntPower);
	}
	
}



static s4Byte
odm_SignalScaleMapping_92CSeries(	
	IN OUT PDM_ODM_T pDM_Odm,
	IN s4Byte CurrSig 
)
{
	s4Byte RetSig = 0; 

	if((pDM_Odm->SupportInterface  == ODM_ITRF_USB))
	{
		if(CurrSig >= 51 && CurrSig <= 100)
		{
			RetSig = 100;
		}
		else if(CurrSig >= 41 && CurrSig <= 50)
		{
			RetSig = 80 + ((CurrSig - 40)*2);
		}
		else if(CurrSig >= 31 && CurrSig <= 40)
		{
			RetSig = 66 + (CurrSig - 30);
		}
		else if(CurrSig >= 21 && CurrSig <= 30)
		{
			RetSig = 54 + (CurrSig - 20);
		}
		else if(CurrSig >= 10 && CurrSig <= 20)
		{
			RetSig = 42 + (((CurrSig - 10) * 2) / 3);
		}
		else if(CurrSig >= 5 && CurrSig <= 9)
		{
			RetSig = 22 + (((CurrSig - 5) * 3) / 2);
		}
		else if(CurrSig >= 1 && CurrSig <= 4)
		{
			RetSig = 6 + (((CurrSig - 1) * 3) / 2);
		}
		else
		{
			RetSig = CurrSig;
		}
	}

	return RetSig;
}
static s4Byte
odm_SignalScaleMapping(	
	IN OUT PDM_ODM_T pDM_Odm,
	IN	s4Byte CurrSig 
)
{	
        {		
		return odm_SignalScaleMapping_92CSeries(pDM_Odm,CurrSig);
	}
	
}
			
static u1Byte 
odm_EVMdbToPercentage(
    IN		s1Byte Value
    )
{
	//
	// -33dB~0dB to 0%~99%
	//
	s1Byte ret_val;
    
	ret_val = Value;
	ret_val /= 2;

	/*DbgPrint("Value=%d\n", Value);*/
	/*ODM_RT_DISP(FRX, RX_PHY_SQ, ("EVMdbToPercentage92C Value=%d / %x\n", ret_val, ret_val));*/
#ifdef ODM_EVM_ENHANCE_ANTDIV
	if (ret_val >= 0)
		ret_val = 0;

	if (ret_val <= -40)
		ret_val = -40;

	ret_val = 0 - ret_val;
	ret_val *= 3;
#else
	if (ret_val >= 0)
		ret_val = 0;

	if (ret_val <= -33)
		ret_val = -33;

	ret_val = 0 - ret_val;
	ret_val *= 3;

	if (ret_val == 99)
		ret_val = 100;
#endif

	return (u1Byte)ret_val;
}

VOID
odm_RxPhyStatus92CSeries_Parsing(
	IN OUT	PDM_ODM_T					pDM_Odm,
	OUT		PODM_PHY_INFO_T			pPhyInfo,		
	IN 		pu1Byte						pPhyStatus,
	IN		PODM_PACKET_INFO_T			pPktinfo
	)
{							
	u1Byte				i, max_spatial_stream;
	s1Byte				rx_pwr[4], rx_pwr_all=0;
	u1Byte				evm, pwdb_all = 0, PWDB_ALL_BT;
	u1Byte				rssi, total_rssi=0;
	BOOLEAN				is_cck=FALSE;	
	u1Byte				rf_rx_num = 0;
	u1Byte				lna_idx = 0;
	u1Byte				vga_idx = 0;
	struct phy_status_rpt *phystrpt = (struct phy_status_rpt  *) pPhyStatus;

	is_cck = (pPktinfo->DataRate <= ODM_RATE11M) ? TRUE : FALSE;
	pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_A] = -1;
	pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_B] = -1;


	if(is_cck) {
		u1Byte cck_agc_rpt;
		
		// 
		// (1)Hardware does not provide RSSI for CCK
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//

		cck_agc_rpt =  phystrpt->cck_agc_rpt_ofdm_cfosho_a ;
		
		//2011.11.28 LukeLee: 88E use different LNA & VGA gain table
		//The RSSI formula should be modified according to the gain table
		//In 88E, cck_highpwr is always set to 1

		lna_idx = ((cck_agc_rpt & 0xE0) >>5);
		vga_idx = (cck_agc_rpt & 0x1F); 

		rx_pwr_all = odm_CCKRSSI_8188F(lna_idx, vga_idx);
		pwdb_all = odm_QueryRxPwrPercentage(rx_pwr_all);
		if (pwdb_all > 100)
			pwdb_all = 100;

		pDM_Odm->cck_lna_idx = lna_idx;
		pDM_Odm->cck_vga_idx = vga_idx;
		pPhyInfo->RxPWDBAll = pwdb_all;
		pPhyInfo->BTRxRSSIPercentage = pwdb_all;
		pPhyInfo->RecvSignalPower = rx_pwr_all;
		//
		// (3) Get Signal Quality (EVM)
		//
		//if(pPktinfo->bPacketMatchBSSID)
		{
			u1Byte	SQ,SQ_rpt;			
			
			if(pPhyInfo->RxPWDBAll > 40 && !pDM_Odm->bInHctTest){
				SQ = 100;
			}
			else{						
				SQ_rpt = phystrpt->cck_sig_qual_ofdm_pwdb_all;
					
				if(SQ_rpt > 64)
					SQ = 0;
				else if (SQ_rpt < 20)
					SQ = 100;
				else
					SQ = ((64-SQ_rpt) * 100) / 44;
			
			}
			
			//DbgPrint("cck SQ = %d\n", SQ);
			pPhyInfo->SignalQuality = SQ;
			pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_A] = SQ;
			pPhyInfo->RxMIMOSignalQuality[ODM_RF_PATH_B] = -1;
		}

		for (i = ODM_RF_PATH_A; i < ODM_RF_PATH_MAX; i++) {
			if (i == 0)
				pPhyInfo->RxMIMOSignalStrength[0] = pwdb_all;
			else
				pPhyInfo->RxMIMOSignalStrength[1] = 0;
		}
	} else { //2 is OFDM rate
	
		// 
		// (1)Get RSSI for HT rate
		//
		
		for (i = ODM_RF_PATH_A; i < ODM_RF_PATH_MAX; i++) {
			// 2008/01/30 MH we will judge RF RX path now.
			if (pDM_Odm->RFPathRxEnable & BIT(i))
				rf_rx_num++;
			//else
				//continue;

			rx_pwr[i] = ((phystrpt->path_agc[i].gain& 0x3F)*2) - 110;
			pDM_Odm->ofdm_agc_idx[i] = (phystrpt->path_agc[i].gain & 0x3F);

			pPhyInfo->RxPwr[i] = rx_pwr[i];

			/* Translate DBM to percentage. */
			rssi = odm_QueryRxPwrPercentage(rx_pwr[i]);
			total_rssi += rssi;
			//RT_DISP(FRX, RX_PHY_SS, ("RF-%d RXPWR=%x RSSI=%d\n", i, rx_pwr[i], RSSI));
		
			pPhyInfo->RxMIMOSignalStrength[i] =(u1Byte) rssi;

			//Get Rx snr value in DB		
			pPhyInfo->RxSNR[i] = (s4Byte)(phystrpt->path_rxsnr[i]/2);
		
			/* Record Signal Strength for next packet */
			//if(pPktinfo->bPacketMatchBSSID)
		}
		
		
		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		rx_pwr_all = (((phystrpt->cck_sig_qual_ofdm_pwdb_all) >> 1 )& 0x7f) -110;		
		
		PWDB_ALL_BT = pwdb_all = odm_QueryRxPwrPercentage(rx_pwr_all);	
	
	
		pPhyInfo->RxPWDBAll = pwdb_all;
		//ODM_RT_TRACE(pDM_Odm,ODM_COMP_RSSI_MONITOR, ODM_DBG_LOUD, ("ODM OFDM RSSI=%d\n",pPhyInfo->RxPWDBAll));
		pPhyInfo->BTRxRSSIPercentage = PWDB_ALL_BT;
		pPhyInfo->RxPower = rx_pwr_all;
		pPhyInfo->RecvSignalPower = rx_pwr_all;
		
		    {//pMgntInfo->CustomerID != RT_CID_819x_Lenovo
			//
			// (3)EVM of HT rate
			//
			if(pPktinfo->DataRate >=ODM_RATEMCS8 && pPktinfo->DataRate <=ODM_RATEMCS15)
				max_spatial_stream = 2; //both spatial stream make sense
			else
				max_spatial_stream = 1; //only spatial stream 1 makes sense

			for(i=0; i < max_spatial_stream; i++) {
				// Do not use shift operation like "rx_evmX >>= 1" because the compilor of free build environment
				// fill most significant bit to "zero" when doing shifting operation which may change a negative 
				// value to positive one, then the dbm value (which is supposed to be negative)  is not correct anymore.			
				evm = odm_EVMdbToPercentage( (phystrpt->stream_rxevm[i] ));	//dbm

				//GET_RX_STATUS_DESC_RX_MCS(pDesc), pDrvInfo->rxevm[i], "%", EVM));
				
				//if(pPktinfo->bPacketMatchBSSID)
				if(i==ODM_RF_PATH_A) // Fill value in RFD, Get the first spatial stream only
					pPhyInfo->SignalQuality = (u1Byte)(evm & 0xff);

				pPhyInfo->RxMIMOSignalQuality[i] = (u1Byte)(evm & 0xff);
			}
		}
		
	}
	//UI BSS List signal strength(in percentage), make it good looking, from 0~100.
	//It is assigned to the BSS List in GetValueFromBeaconOrProbeRsp().
	if(is_cck) {		
		pPhyInfo->SignalStrength = (u1Byte)(odm_SignalScaleMapping(pDM_Odm, pwdb_all));/*PWDB_ALL;*/
	} else {	
		if (rf_rx_num != 0)
			pPhyInfo->SignalStrength = (u1Byte)(odm_SignalScaleMapping(pDM_Odm, total_rssi /= rf_rx_num));
	}

	//DbgPrint("isCCKrate = %d, pPhyInfo->RxPWDBAll = %d, pPhyStaRpt->cck_agc_rpt_ofdm_cfosho_a = 0x%x\n", 
		//isCCKrate, pPhyInfo->RxPWDBAll, pPhyStaRpt->cck_agc_rpt_ofdm_cfosho_a);

	//For 92C/92D HW (Hybrid) Antenna Diversity
}

VOID
odm_Init_RSSIForDM(
	IN OUT	PDM_ODM_T	pDM_Odm
	)
{

}

VOID
odm_Process_RSSIForDM(	
	IN OUT	PDM_ODM_T					pDM_Odm,
	IN		PODM_PHY_INFO_T				pPhyInfo,
	IN		PODM_PACKET_INFO_T			pPktinfo
	)
{
	
	s4Byte			UndecoratedSmoothedPWDB, UndecoratedSmoothedCCK, UndecoratedSmoothedOFDM, RSSI_Ave;
	u1Byte			i, isCCKrate=0;	
	u1Byte			RSSI_max, RSSI_min;
	u4Byte			OFDM_pkt=0; 
	u4Byte			Weighting=0;
	PSTA_INFO_T           	pEntry;

	if (pPktinfo->StationID >= ODM_ASSOCIATE_ENTRY_NUM)
		return;

	#ifdef CONFIG_S0S1_SW_ANTENNA_DIVERSITY
	odm_S0S1_SwAntDivByCtrlFrame_ProcessRSSI(pDM_Odm, pPhyInfo, pPktinfo);
	#endif

	//
	// 2012/05/30 MH/Luke.Lee Add some description 
	// In windows driver: AP/IBSS mode STA
	//
		pEntry = pDM_Odm->pODM_StaInfo[pPktinfo->StationID];							

	if(!IS_STA_VALID(pEntry) )
	{		
		return;
	}

	{
		if ((!pPktinfo->bPacketMatchBSSID))/*data frame only*/
			return;
	}

	isCCKrate = (pPktinfo->DataRate <= ODM_RATE11M )?TRUE :FALSE;
	pDM_Odm->RxRate = pPktinfo->DataRate;

	UndecoratedSmoothedCCK =  pEntry->rssi_stat.UndecoratedSmoothedCCK;
	UndecoratedSmoothedOFDM = pEntry->rssi_stat.UndecoratedSmoothedOFDM;
	UndecoratedSmoothedPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;	
	
	if(pPktinfo->bPacketToSelf || pPktinfo->bPacketBeacon)
	{

		if(!isCCKrate)//ofdm rate
		{
			{
				if (pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B] == 0) {
					RSSI_Ave = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
					pDM_Odm->RSSI_A = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
					pDM_Odm->RSSI_B = 0;
				} else {
					/*DbgPrint("pRfd->Status.RxMIMOSignalStrength[0] = %d, pRfd->Status.RxMIMOSignalStrength[1] = %d\n",*/ 
						/*pRfd->Status.RxMIMOSignalStrength[0], pRfd->Status.RxMIMOSignalStrength[1]);*/
					pDM_Odm->RSSI_A =  pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
					pDM_Odm->RSSI_B = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B];
				
					if (pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A] > pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B]) {
						RSSI_max = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
						RSSI_min = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B];
					} else {
						RSSI_max = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_B];
						RSSI_min = pPhyInfo->RxMIMOSignalStrength[ODM_RF_PATH_A];
					}
					if ((RSSI_max - RSSI_min) < 3)
						RSSI_Ave = RSSI_max;
					else if ((RSSI_max - RSSI_min) < 6)
						RSSI_Ave = RSSI_max - 1;
					else if ((RSSI_max - RSSI_min) < 10)
						RSSI_Ave = RSSI_max - 2;
					else
						RSSI_Ave = RSSI_max - 3;
				}
			}
					
			//1 Process OFDM RSSI
			if(UndecoratedSmoothedOFDM <= 0)	// initialize
			{
				UndecoratedSmoothedOFDM = pPhyInfo->RxPWDBAll;
			}
			else
			{
				if(pPhyInfo->RxPWDBAll > (u4Byte)UndecoratedSmoothedOFDM)
				{
					UndecoratedSmoothedOFDM = 	
							( ((UndecoratedSmoothedOFDM)*(Rx_Smooth_Factor-1)) + 
							(RSSI_Ave)) /(Rx_Smooth_Factor);
					UndecoratedSmoothedOFDM = UndecoratedSmoothedOFDM + 1;
				}
				else
				{
					UndecoratedSmoothedOFDM = 	
							( ((UndecoratedSmoothedOFDM)*(Rx_Smooth_Factor-1)) + 
							(RSSI_Ave)) /(Rx_Smooth_Factor);
				}
			}				
			if (pEntry->rssi_stat.OFDM_pkt != 64) {
				i = 63;
				pEntry->rssi_stat.OFDM_pkt -= (u4Byte)(((pEntry->rssi_stat.PacketMap>>i)&BIT0)-1);
			}
			pEntry->rssi_stat.PacketMap = (pEntry->rssi_stat.PacketMap<<1) | BIT0;			
										
		}
		else
		{
			RSSI_Ave = pPhyInfo->RxPWDBAll;
			pDM_Odm->RSSI_A = (u1Byte) pPhyInfo->RxPWDBAll;
			pDM_Odm->RSSI_B = 0xFF;
			pDM_Odm->RSSI_C = 0xFF;
			pDM_Odm->RSSI_D = 0xFF;

			//1 Process CCK RSSI
			if(UndecoratedSmoothedCCK <= 0)	// initialize
			{
				UndecoratedSmoothedCCK = pPhyInfo->RxPWDBAll;
			}
			else
			{
				if(pPhyInfo->RxPWDBAll > (u4Byte)UndecoratedSmoothedCCK)
				{
					UndecoratedSmoothedCCK = 	
							( ((UndecoratedSmoothedCCK)*(Rx_Smooth_Factor-1)) + 
							(pPhyInfo->RxPWDBAll)) /(Rx_Smooth_Factor);
					UndecoratedSmoothedCCK = UndecoratedSmoothedCCK + 1;
				}
				else
				{
					UndecoratedSmoothedCCK = 	
							( ((UndecoratedSmoothedCCK)*(Rx_Smooth_Factor-1)) + 
							(pPhyInfo->RxPWDBAll)) /(Rx_Smooth_Factor);
				}
			}
			i = 63;
			pEntry->rssi_stat.OFDM_pkt -= (u4Byte)((pEntry->rssi_stat.PacketMap>>i)&BIT0);			
			pEntry->rssi_stat.PacketMap = pEntry->rssi_stat.PacketMap<<1;			
		}

		//if(pEntry)
		{
			//2011.07.28 LukeLee: modified to prevent unstable CCK RSSI
			if (pEntry->rssi_stat.OFDM_pkt == 64) { /* speed up when all packets are OFDM*/
				UndecoratedSmoothedPWDB = UndecoratedSmoothedOFDM;
			} else {
				if (pEntry->rssi_stat.ValidBit < 64)
					pEntry->rssi_stat.ValidBit++;

				if (pEntry->rssi_stat.ValidBit == 64) {
					Weighting = ((pEntry->rssi_stat.OFDM_pkt<<4) > 64)?64:(pEntry->rssi_stat.OFDM_pkt<<4);
					UndecoratedSmoothedPWDB = (Weighting*UndecoratedSmoothedOFDM+(64-Weighting)*UndecoratedSmoothedCCK)>>6;
				} else {
					if (pEntry->rssi_stat.ValidBit != 0)
						UndecoratedSmoothedPWDB = (pEntry->rssi_stat.OFDM_pkt*UndecoratedSmoothedOFDM+(pEntry->rssi_stat.ValidBit-pEntry->rssi_stat.OFDM_pkt)*UndecoratedSmoothedCCK)/pEntry->rssi_stat.ValidBit;
					else
						UndecoratedSmoothedPWDB = 0;
				}
			}
			if (pEntry->rssi_stat.UndecoratedSmoothedPWDB == -1)
				phydm_ra_rssi_rpt_wk(pDM_Odm);

			pEntry->rssi_stat.UndecoratedSmoothedCCK = UndecoratedSmoothedCCK;
			pEntry->rssi_stat.UndecoratedSmoothedOFDM = UndecoratedSmoothedOFDM;
			pEntry->rssi_stat.UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;

			//DbgPrint("OFDM_pkt=%d, Weighting=%d\n", OFDM_pkt, Weighting);
			//DbgPrint("UndecoratedSmoothedOFDM=%d, UndecoratedSmoothedPWDB=%d, UndecoratedSmoothedCCK=%d\n", 
			//	UndecoratedSmoothedOFDM, UndecoratedSmoothedPWDB, UndecoratedSmoothedCCK);
			
		}
	
	}
}


#if(ODM_IC_11N_SERIES_SUPPORT ==1)
//
// Endianness before calling this API
//
VOID
ODM_PhyStatusQuery_92CSeries(
	IN OUT	PDM_ODM_T					pDM_Odm,
	OUT		PODM_PHY_INFO_T				pPhyInfo,
	IN 		pu1Byte						pPhyStatus,	
	IN		PODM_PACKET_INFO_T			pPktinfo
	)
{
	odm_RxPhyStatus92CSeries_Parsing(pDM_Odm, pPhyInfo, pPhyStatus, pPktinfo);
	odm_Process_RSSIForDM(pDM_Odm, pPhyInfo, pPktinfo);
}
#endif


//
// Endianness before calling this API
//

VOID
ODM_PhyStatusQuery(
	IN OUT	PDM_ODM_T					pDM_Odm,
	OUT		PODM_PHY_INFO_T			pPhyInfo,
	IN 		pu1Byte						pPhyStatus,	
	IN		PODM_PACKET_INFO_T			pPktinfo
	)
{
#if	ODM_IC_11N_SERIES_SUPPORT
	if(pDM_Odm->SupportICType & ODM_IC_11N_SERIES )
		ODM_PhyStatusQuery_92CSeries(pDM_Odm,pPhyInfo,pPhyStatus,pPktinfo);
#endif
}
	
// For future use.
VOID
ODM_MacStatusQuery(
	IN OUT	PDM_ODM_T					pDM_Odm,
	IN 		pu1Byte						pMacStatus,
	IN		u1Byte						MacID,	
	IN		BOOLEAN						bPacketMatchBSSID,
	IN		BOOLEAN						bPacketToSelf,
	IN		BOOLEAN						bPacketBeacon
	)
{
	// 2011/10/19 Driver team will handle in the future.
	
}


//
// If you want to add a new IC, Please follow below template and generate a new one.
// 
//

HAL_STATUS
rtl8188fu_phy_config_rf_with_headerfile(
	IN 	PDM_ODM_T	        	pDM_Odm,
	IN 	ODM_RF_Config_Type 		ConfigType,
	IN 	ODM_RF_RADIO_PATH_E 	eRFPath
    )
{

   ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, 
		 		("===>ODM_ConfigRFWithHeaderFile (%s)\n", (pDM_Odm->bIsMPChip) ? "MPChip" : "TestChip"));
    ODM_RT_TRACE(pDM_Odm, ODM_COMP_INIT, ODM_DBG_LOUD, 
				("pDM_Odm->SupportInterface: 0x%X, pDM_Odm->board_type: 0x%X\n",
				pDM_Odm->SupportInterface, pDM_Odm->board_type));

//1 AP doesn't use PHYDM power tracking table in these ICs

//1 All platforms support
	if (ConfigType == CONFIG_RF_RADIO) {
		if (eRFPath == ODM_RF_PATH_A)
			ODM_ReadAndConfig_MP_8188F_RadioA(pDM_Odm);
	} else if (ConfigType == CONFIG_RF_TXPWR_LMT)
		ODM_ReadAndConfig_MP_8188F_TXPWR_LMT(pDM_Odm);

//1 New ICs (WIN only)

	return HAL_STATUS_SUCCESS;
}

HAL_STATUS
_rtl8188fu_phy_config_bb_with_headerfile(
	IN 	PDM_ODM_T	             	pDM_Odm,
	IN 	ODM_BB_Config_Type 		ConfigType
	)
{

//1 AP doesn't use PHYDM initialization in these ICs

	if (ConfigType == CONFIG_BB_PHY_REG) 
		ODM_ReadAndConfig_MP_8188F_PHY_REG(pDM_Odm);
	else if (ConfigType == CONFIG_BB_AGC_TAB) 
		ODM_ReadAndConfig_MP_8188F_AGC_TAB(pDM_Odm);
	else if (ConfigType == CONFIG_BB_PHY_REG_PG) 
		ODM_ReadAndConfig_MP_8188F_PHY_REG_PG(pDM_Odm);

//1 New ICs (WIN only)

	return HAL_STATUS_SUCCESS; 
}                 

u4Byte 
ODM_GetHWImgVersion(
	IN	PDM_ODM_T	pDM_Odm
	)
{
    u4Byte  Version=0;

//1 AP doesn't use PHYDM initialization in these ICs
/*1 All platforms support*/
	Version = GET_VERSION_MP(8188F, _MAC_REG);

//1 New ICs (WIN only)

	return Version;
}


