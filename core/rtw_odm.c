/******************************************************************************
 *
 * Copyright(c) 2013 Realtek Corporation. All rights reserved.
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

#include <rtw_odm.h>
#include <hal_data.h>

const char *odm_comp_str[] = {
	/* BIT0 */"ODM_COMP_DIG",
	/* BIT1 */"ODM_COMP_RA_MASK",
	/* BIT2 */"ODM_COMP_DYNAMIC_TXPWR",
	/* BIT3 */"ODM_COMP_FA_CNT",
	/* BIT4 */"ODM_COMP_RSSI_MONITOR",
	/* BIT5 */"ODM_COMP_CCK_PD",
	/* BIT6 */"ODM_COMP_ANT_DIV",
	/* BIT7 */"ODM_COMP_PWR_SAVE",
	/* BIT8 */"ODM_COMP_PWR_TRAIN",
	/* BIT9 */"ODM_COMP_RATE_ADAPTIVE",
	/* BIT10 */"ODM_COMP_PATH_DIV",
	/* BIT11 */"ODM_COMP_PSD",
	/* BIT12 */"ODM_COMP_DYNAMIC_PRICCA",
	/* BIT13 */"ODM_COMP_RXHP",
	/* BIT14 */"ODM_COMP_MP",
	/* BIT15 */"ODM_COMP_CFO_TRACKING",
	/* BIT16 */"ODM_COMP_ACS",
	/* BIT17 */"PHYDM_COMP_ADAPTIVITY",
	/* BIT18 */"PHYDM_COMP_RA_DBG",
	/* BIT19 */"PHYDM_COMP_TXBF",
	/* BIT20 */"ODM_COMP_EDCA_TURBO",
	/* BIT21 */"ODM_COMP_EARLY_MODE",
	/* BIT22 */"ODM_FW_DEBUG_TRACE",
	/* BIT23 */NULL,
	/* BIT24 */"ODM_COMP_TX_PWR_TRACK",
	/* BIT25 */"ODM_COMP_RX_GAIN_TRACK",
	/* BIT26 */"ODM_COMP_CALIBRATION",
	/* BIT27 */NULL,
	/* BIT28 */"ODM_PHY_CONFIG",
	/* BIT29 */"BEAMFORMING_DEBUG",
	/* BIT30 */"ODM_COMP_COMMON",
	/* BIT31 */"ODM_COMP_INIT",
	/* BIT32 */"ODM_COMP_NOISY_DETECT",
};

#define RTW_ODM_COMP_MAX 33

const char *odm_ability_str[] = {
	/* BIT0 */"ODM_BB_DIG",
	/* BIT1 */"ODM_BB_RA_MASK",
	/* BIT2 */"ODM_BB_DYNAMIC_TXPWR",
	/* BIT3 */"ODM_BB_FA_CNT",
	/* BIT4 */"ODM_BB_RSSI_MONITOR",
	/* BIT5 */"ODM_BB_CCK_PD",
	/* BIT6 */"ODM_BB_ANT_DIV",
	/* BIT7 */"ODM_BB_PWR_SAVE",
	/* BIT8 */"ODM_BB_PWR_TRAIN",
	/* BIT9 */"ODM_BB_RATE_ADAPTIVE",
	/* BIT10 */"ODM_BB_PATH_DIV",
	/* BIT11 */"ODM_BB_PSD",
	/* BIT12 */"ODM_BB_RXHP",
	/* BIT13 */"ODM_BB_ADAPTIVITY",
	/* BIT14 */"ODM_BB_CFO_TRACKING",
	/* BIT15 */"ODM_BB_NHM_CNT",
	/* BIT16 */"ODM_BB_PRIMARY_CCA",
	/* BIT17 */"ODM_BB_TXBF",
	/* BIT18 */NULL,
	/* BIT19 */NULL,
	/* BIT20 */"ODM_MAC_EDCA_TURBO",
	/* BIT21 */"ODM_MAC_EARLY_MODE",
	/* BIT22 */NULL,
	/* BIT23 */NULL,
	/* BIT24 */"ODM_RF_TX_PWR_TRACK",
	/* BIT25 */"ODM_RF_RX_GAIN_TRACK",
	/* BIT26 */"ODM_RF_CALIBRATION",
};

#define RTW_ODM_ABILITY_MAX 27

const char *odm_dbg_level_str[] = {
	NULL,
	"ODM_DBG_OFF",
	"ODM_DBG_SERIOUS",
	"ODM_DBG_WARNING",
	"ODM_DBG_LOUD",
	"ODM_DBG_TRACE",
};

#define RTW_ODM_DBG_LEVEL_NUM 6

/* set ODM_CMNINFO_IC_TYPE based on chip_type */
void rtw_odm_init_ic_type(_adapter *adapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	DM_ODM_T *odm = &hal_data->odmpriv;
	u4Byte ic_type = chip_type_to_odm_ic_type(rtw_get_chip_type(adapter));

	rtw_warn_on(!ic_type);

	ODM_CmnInfoInit(odm, ODM_CMNINFO_IC_TYPE, ic_type);
}

void rtw_odm_get_perpkt_rssi(void *sel, _adapter *adapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	DM_ODM_T *odm = &(hal_data->odmpriv);

	DBG_871X_SEL_NL(sel,"RxRate = %s, RSSI_A = %d(%%), RSSI_B = %d(%%)\n",
	HDATA_RATE(odm->RxRate), odm->RSSI_A, odm->RSSI_B);
}


void rtw_odm_acquirespinlock(_adapter *adapter,	RT_SPINLOCK_TYPE type)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(adapter);
	_irqL irqL;

	switch(type)
	{
		case RT_IQK_SPINLOCK:
			_enter_critical_bh(&pHalData->IQKSpinLock, &irqL);
		default:
			break;
	}
}

void rtw_odm_releasespinlock(_adapter *adapter,	RT_SPINLOCK_TYPE type)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(adapter);
	_irqL irqL;

	switch(type)
	{
		case RT_IQK_SPINLOCK:
			_exit_critical_bh(&pHalData->IQKSpinLock, &irqL);
		default:
			break;
	}
}

#ifdef CONFIG_DFS_MASTER
VOID rtw_odm_radar_detect_reset(_adapter *adapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	PDM_ODM_T pDM_Odm = &(hal_data->odmpriv);

	if (pDM_Odm->SupportICType & ODM_RTL8192D) {
		ODM_SetBBReg(pDM_Odm, 0xc84 , BIT25, 0);
		ODM_SetBBReg(pDM_Odm, 0xc84 , BIT25, 1);
	} else if (pDM_Odm->SupportICType & ODM_RTL8821) {
		ODM_SetBBReg(pDM_Odm, 0x924 , BIT15, 0);
		ODM_SetBBReg(pDM_Odm, 0x924 , BIT15, 1);
	} else {
		/* not supported yet */
		rtw_warn_on(1);
	}
}

VOID rtw_odm_radar_detect_disable(_adapter *adapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	PDM_ODM_T pDM_Odm = &(hal_data->odmpriv);

	if (pDM_Odm->SupportICType & ODM_RTL8192D)
		ODM_SetBBReg(pDM_Odm, 0xc84 , BIT25, 0);
	else if (pDM_Odm->SupportICType & ODM_RTL8821)
		ODM_SetBBReg(pDM_Odm, 0x924 , BIT15, 0);
	else
		rtw_warn_on(1);
}

/* called after ch, bw is set, chance to adjust parameter for different ch conditions */
VOID rtw_odm_radar_detect_enable(_adapter *adapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	PDM_ODM_T pDM_Odm = &(hal_data->odmpriv);

	if (pDM_Odm->SupportICType & ODM_RTL8192D) {
		ODM_SetBBReg(pDM_Odm, 0xc38, BIT23 | BIT22, 2);
		ODM_SetBBReg(pDM_Odm, 0x814, bMaskDWord, 0x04cc4d10);
		ODM_SetBBReg(pDM_Odm, 0xc8c, BIT23 | BIT22, 3);
		ODM_SetBBReg(pDM_Odm, 0xc30, 0xf, 0xa);
		ODM_SetBBReg(pDM_Odm, 0xcdc, 0xf0000, 4);
	} else if (pDM_Odm->SupportICType & ODM_RTL8821) {
		ODM_SetBBReg(pDM_Odm, 0x814, 0x3fffffff, 0x04cc4d10);
		ODM_SetBBReg(pDM_Odm, 0x834, bMaskByte0, 0x06);
		ODM_SetBBReg(pDM_Odm, 0x918, bMaskDWord, 0x1c16ecdf);
		ODM_SetBBReg(pDM_Odm, 0x924, bMaskDWord, 0x0152a400);
		ODM_SetBBReg(pDM_Odm, 0x91c, bMaskDWord, 0x0fa21a20);
		ODM_SetBBReg(pDM_Odm, 0x920, bMaskDWord, 0xe0f57204);
	} else {
		/* not supported yet */
		rtw_warn_on(1);
	}

	rtw_odm_radar_detect_reset(adapter);
}

BOOLEAN rtw_odm_radar_detect(_adapter *adapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	PDM_ODM_T pDM_Odm = &(hal_data->odmpriv);
	BOOLEAN enable_DFS = FALSE;
	BOOLEAN bypass = FALSE;
	BOOLEAN radar_detected = FALSE;

	static u8Byte last_tx_unicast = 0;
	static u8Byte last_rx_unicast = 0;
	static u8Byte throughput = 0;
	int tp_th = ((*pDM_Odm->pBandWidth == ODM_BW40M) ? 45 : 20); /*refer AP team's testing number*/

	throughput = (*(pDM_Odm->pNumTxBytesUnicast) - last_tx_unicast) + (*(pDM_Odm->pNumRxBytesUnicast) - last_rx_unicast);
	last_tx_unicast = *(pDM_Odm->pNumTxBytesUnicast);
	last_rx_unicast = *(pDM_Odm->pNumRxBytesUnicast);

	if (throughput>>18 > tp_th) {
		if (pDM_Odm->SupportICType & ODM_RTL8192D)
			ODM_SetBBReg(pDM_Odm, 0xcdc, BIT8|BIT9, 0);
		bypass = TRUE;
	} else {
		if (pDM_Odm->SupportICType & ODM_RTL8192D)
			ODM_SetBBReg(pDM_Odm, 0xcdc, BIT8|BIT9, 1);
	}

	if (pDM_Odm->SupportICType & ODM_RTL8192D) {
		if (ODM_GetBBReg(pDM_Odm , 0xc84, BIT25))
			enable_DFS = TRUE;
	} else if (pDM_Odm->SupportICType & ODM_RTL8821) {
		if (ODM_GetBBReg(pDM_Odm , 0x924, BIT15))
			enable_DFS = TRUE;
	}

	if (pDM_Odm->SupportICType & ODM_RTL8192D) {
		if (ODM_GetBBReg(pDM_Odm , 0xcf8, BIT23))
			radar_detected = TRUE;
	} else if (pDM_Odm->SupportICType & ODM_RTL8821) {
		if (ODM_GetBBReg(pDM_Odm , 0xf98, BIT17))
			radar_detected = TRUE;
	}

	ODM_RT_TRACE(pDM_Odm, ODM_COMP_COMMON, ODM_DBG_LOUD
		, ("Radar detect: enable_DFS:%d, radar_detected:%d, bypass:%d\n"
			, enable_DFS, radar_detected, bypass));
	if (0)
		DBG_871X("Radar detect: enable_DFS:%d, radar_detected:%d, bypass:%d(throughput:%llu, tp_th:%d)\n"
			, enable_DFS, radar_detected, bypass, throughput, tp_th);

	if (enable_DFS && radar_detected)
		rtw_odm_radar_detect_reset(adapter);

exit:
	return (enable_DFS && radar_detected && !bypass);
}
#endif /* CONFIG_DFS_MASTER */

