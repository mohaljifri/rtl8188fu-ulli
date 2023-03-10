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
 
 #ifndef __HAL_PHY_RF_H__
 #define __HAL_PHY_RF_H__
 
/*#include "phydm_kfree.h"*/

typedef enum _SPUR_CAL_METHOD {
	PLL_RESET,
	AFE_PHASE_SEL
} SPUR_CAL_METHOD;

typedef enum _PWRTRACK_CONTROL_METHOD {
	BBSWING,
	TXAGC,
	MIX_MODE,
	TSSI_MODE
} PWRTRACK_METHOD;

typedef VOID 	(*FuncSetPwr)(PVOID, PWRTRACK_METHOD, u1Byte, u1Byte);
typedef VOID(*FuncIQK)(PVOID, u1Byte, u1Byte, u1Byte);
typedef VOID 	(*FuncLCK)(PVOID);
typedef VOID  	(*FuncSwing)(PVOID, pu1Byte*, pu1Byte*, pu1Byte*, pu1Byte*);
typedef VOID	(*FuncSwing8814only)(PVOID, pu1Byte*, pu1Byte*, pu1Byte*, pu1Byte*);

typedef struct _TXPWRTRACK_CFG {
	u1Byte		Threshold_DPK;
} TXPWRTRACK_CFG, *PTXPWRTRACK_CFG;

VOID
ODM_ClearTxPowerTrackingState(
	IN		PVOID					pDM_VOID
	);

VOID
ODM_TXPowerTrackingCallback_ThermalMeter(
	IN PADAPTER	Adapter
	);



#define ODM_TARGET_CHNL_NUM_2G_5G	59

u1Byte 
ODM_GetRightChnlPlaceforIQK(
    IN u1Byte chnl
);

void phydm_rf_init(	IN		PVOID					pDM_VOID);
								
#endif	// #ifndef __HAL_PHY_RF_H__

