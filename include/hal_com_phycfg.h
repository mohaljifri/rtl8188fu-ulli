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
#ifndef __HAL_COM_PHYCFG_H__
#define __HAL_COM_PHYCFG_H__

#define		PathA                     			0x0	// Useless
#define		PathB                     			0x1
#define		PathC                     			0x2
#define		PathD                     			0x3

typedef enum _RF_TX_NUM {
	RF_1TX = 0,
	RF_2TX,
	RF_3TX,
	RF_4TX,
	RF_MAX_TX_NUM,
	RF_TX_NUM_NONIMPLEMENT,
} RF_TX_NUM;

#define MAX_POWER_INDEX 		0x3F

typedef enum _REGULATION_TXPWR_LMT {
	TXPWR_LMT_FCC = 0,
	TXPWR_LMT_MKK = 1,
	TXPWR_LMT_ETSI = 2,
	TXPWR_LMT_WW = 3,	

	TXPWR_LMT_MAX_REGULATION_NUM = 4
} REGULATION_TXPWR_LMT;

#define TX_PWR_LMT_REF_VHT_FROM_HT	BIT0
#define TX_PWR_LMT_REF_HT_FROM_VHT	BIT1

/*------------------------------Define structure----------------------------*/ 
typedef struct _BB_REGISTER_DEFINITION{
	u32 rfintfs;			// set software control: 
						//		0x870~0x877[8 bytes]
							
	u32 rfintfo; 			// output data: 
						//		0x860~0x86f [16 bytes]
							
	u32 rfintfe; 			// output enable: 
						//		0x860~0x86f [16 bytes]
							
	u32 rf3wireOffset;	// LSSI data:
						//		0x840~0x84f [16 bytes]

	u32 rfHSSIPara2; 	// wire parameter control2 : 
						//		0x824~0x827,0x82c~0x82f, 0x834~0x837, 0x83c~0x83f [16 bytes]
								
	u32 rfLSSIReadBack; 	//LSSI RF readback data SI mode
						//		0x8a0~0x8af [16 bytes]

	u32 rfLSSIReadBackPi; 	//LSSI RF readback data PI mode 0x8b8-8bc for Path A and B

}BB_REGISTER_DEFINITION_T, *PBB_REGISTER_DEFINITION_T;


//----------------------------------------------------------------------
u8
PHY_GetTxPowerByRateBase(
	IN	PADAPTER		Adapter,
	IN	u8				Band,
	IN	u8				RfPath,
	IN	u8				TxNum,
	IN	RATE_SECTION	RateSection
	);

#ifdef TX_POWER_BY_RATE_OLD
u8
PHY_GetRateSectionIndexOfTxPowerByRate(
	IN	PADAPTER	pAdapter,
	IN	u32			RegAddr,
	IN	u32			BitMask
	);
#endif /* TX_POWER_BY_RATE_OLD */

VOID 
rtl8188fu_phy_set_txpower_index_by_rate_section(
	IN	PADAPTER		pAdapter,
	IN	u8				RFPath,	
	IN	u8				Channel,
	IN	u8				RateSection
	);

s8
_rtl8188fu_get_tx_power_by_rate(
	IN	PADAPTER	pAdapter,
	IN	u8			Band,
	IN	u8			RFPath,
	IN	u8			TxNum,
	IN	u8			RateIndex
	);

s8
rtl8188fu_get_tx_power_by_rate( 
	IN	PADAPTER	pAdapter, 
	IN	u8			Band, 
	IN	u8			RFPath, 
	IN	u8			TxNum, 
	IN	u8			RateIndex
	);

VOID
PHY_SetTxPowerLevelByPath(
	IN	PADAPTER	Adapter,
	IN	u8			channel,
	IN	u8			path
	);

VOID 
rtl8188fu_phy_set_txpower_index_by_rate_array(
	IN	PADAPTER		pAdapter,
	IN	u8				RFPath,
	IN	CHANNEL_WIDTH	BandWidth,	
	IN	u8				Channel,
	IN	u8*				Rates,
	IN	u8				RateArraySize
	);

u8
rtl8188fu_phy_get_txpower_index_base(
	IN	PADAPTER		pAdapter,
	IN	u8				RFPath,
	IN	u8				Rate,	
	IN	CHANNEL_WIDTH	BandWidth,	
	IN	u8				Channel,
	OUT PBOOLEAN		bIn24G
	);

s8
rtl8188fu_phy_get_txpower_limit(
	IN	PADAPTER		Adapter,
	IN	u32				RegPwrTblSel,
	IN	BAND_TYPE		Band,
	IN	CHANNEL_WIDTH	Bandwidth,
	IN	u8				RfPath,
	IN	u8				DataRate,
	IN	u8				Channel
	);

s8
rtl8188fu_phy_get_tx_power_tracking_offset( 
	PADAPTER	pAdapter,
	u8			Rate,
	u8			RFPath
	);

u8
PHY_GetTxPowerIndex(
	IN	PADAPTER			pAdapter,
	IN	u8					RFPath,
	IN	u8					Rate,	
	IN	CHANNEL_WIDTH		BandWidth,	
	IN	u8					Channel
	);

VOID
rtl8188fu_phy_set_txpower_index(
	IN	PADAPTER		pAdapter,
	IN	u32				PowerIndex,
	IN	u8				RFPath,	
	IN	u8				Rate
	);

bool phy_is_tx_power_by_rate_needed(_adapter *adapter);
int phy_load_tx_power_by_rate(_adapter *adapter);
int phy_load_tx_power_limit(_adapter *adapter);
void phy_load_tx_power_ext_info(_adapter *adapter);

int rtw_get_phy_file_path(_adapter *adapter, const char *file_name);

u8 rtl8188fu_phy_get_txpower_index(
	IN	PADAPTER			pAdapter,
	IN	u8					RFPath,
	IN	u8					Rate,
	IN	CHANNEL_WIDTH		BandWidth,
	IN	u8					Channel
);

#endif /* __HAL_COMMON_H__ */

