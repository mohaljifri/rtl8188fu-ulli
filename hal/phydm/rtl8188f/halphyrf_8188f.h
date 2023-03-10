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

#ifndef __HAL_PHY_RF_8188F_H__
#define __HAL_PHY_RF_8188F_H__


/*--------------------------Define Parameters-------------------------------*/
#define	IQK_DELAY_TIME_8188F		25		/* ms */
#define	IQK_DEFERRED_TIME_8188F		4
#define	index_mapping_NUM_8188F		15
#define	AVG_THERMAL_NUM_8188F		4
#define	RF_T_METER_8188F			0x42


VOID
rtl8188fu_dm_tx_power_track_set_power(
	IN	PVOID		pDM_VOID,
	PWRTRACK_METHOD 	Method,
	u1Byte 				RFPath,
	u1Byte 				ChannelMappedIndex
	);

//1 7.	IQK

void	
rtl8188fu_phy_iq_calibrate(	
	IN PADAPTER	Adapter,
	IN	BOOLEAN 	bReCovery);


//
// LC calibrate
//
void	
rtl8188fu_phy_lc_calibrate(
	IN	PVOID		pDM_VOID
);

void	
PHY_DigitalPredistortion_8188F(		IN	PADAPTER	pAdapter);


VOID
_PHY_SaveADDARegisters_8188F(
	IN	PADAPTER	pAdapter,
	IN	pu4Byte		ADDAReg,
	IN	pu4Byte		ADDABackup,
	IN	u4Byte		RegisterNum
	);

VOID
_PHY_PathADDAOn_8188F(
	IN	PADAPTER	pAdapter,
	IN	pu4Byte		ADDAReg,
	IN	BOOLEAN		isPathAOn,
	IN	BOOLEAN		is2T
	);

VOID
_PHY_MACSettingCalibration_8188F(
	IN	PADAPTER	pAdapter,
	IN	pu4Byte		MACReg,
	IN	pu4Byte		MACBackup	
	);


VOID
_PHY_PathAStandBy(
	IN	PADAPTER	pAdapter
	);

								
#endif	// #ifndef __HAL_PHY_RF_8188E_H__								

