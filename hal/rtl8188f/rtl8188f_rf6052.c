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
/******************************************************************************
 *
 *
 * Module:	rtl8192c_rf6052.c	( Source C File)
 *
 * Note:	Provide RF 6052 series relative API.
 *
 * Function:
 *
 * Export:
 *
 * Abbrev:
 *
 * History:
 * Data			Who		Remark
 *
 * 09/25/2008	MHC		Create initial version.
 * 11/05/2008 	MHC		Add API for tw power setting.
 *
 *
******************************************************************************/

#include <rtl8188f_hal.h>

/*---------------------------Define Local Constant---------------------------*/
/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/
/*------------------------Define global variable-----------------------------*/


/*------------------------Define local variable------------------------------*/
/* 2008/11/20 MH For Debug only, RF */
/*static	RF_SHADOW_T	RF_Shadow[RF6052_MAX_PATH][RF6052_MAX_REG] = {0}; */
static	RF_SHADOW_T	RF_Shadow[RF6052_MAX_PATH][RF6052_MAX_REG];
/*------------------------Define local variable------------------------------*/

/*-----------------------------------------------------------------------------
 * Function:    PHY_RF6052SetBandwidth()
 *
 * Overview:    This function is called by SetBWModeCallback8190Pci() only
 *
 * Input:       PADAPTER				Adapter
 *              WIRELESS_BANDWIDTH_E	Bandwidth	20M or 40M
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		For RF type 0222D
 *---------------------------------------------------------------------------*/
VOID
_rtl8188fu_phy_rf6052_set_bandwidth(
	IN	PADAPTER				Adapter,
	IN	CHANNEL_WIDTH		Bandwidth)	/*20M or 40M */
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	switch (Bandwidth) {
	case CHANNEL_WIDTH_20:
		/*
		RF_A_reg 0x18[11:10]=2'b11
		RF_A_reg 0x87=0x00065
		RF_A_reg 0x1c=0x00000
		RF_A_reg 0xDF=0x00140
		RF_A_reg 0x1b=0x00c6c (for SDIO)
		RF_A_reg 0x1b=0x01c6c (for USB)
		*/
		pHalData->rfreg_chnlval[0] = ((pHalData->rfreg_chnlval[0] & 0xfffff3ff) | BIT10 | BIT11);
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x18, bRFRegOffsetMask, pHalData->rfreg_chnlval[0]); /* RF TRX_BW */

		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x87, bRFRegOffsetMask, 0x00065); /* FILTER BW&RC Corner (ACPR) */
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x1C, bRFRegOffsetMask, 0x00000); /* FILTER BW&RC Corner (ACPR) */

		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0xDF, bRFRegOffsetMask, 0x00140); /* RC Corner */
#ifdef CONFIG_USB_HCI
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x1B, bRFRegOffsetMask, 0x01C6C); /* RC Corner */
#endif
		break;

	case CHANNEL_WIDTH_40:
		/*
		RF_A_reg 0x18[11:10]=2'b01
		RF_A_reg 0x87=0x00025
		RF_A_reg 0x1c=0x00800 (for SDIO)
		RF_A_reg 0x1c=0x01000 (for USB)
		RF_A_reg 0xDF=0x00140
		RF_A_reg 0x1b=0x00c6c (for SDIO)
		RF_A_reg 0x1b=0x01c6c (for USB)
		*/
		pHalData->rfreg_chnlval[0] = ((pHalData->rfreg_chnlval[0] & 0xfffff3ff) | BIT10);
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x18, bRFRegOffsetMask, pHalData->rfreg_chnlval[0]); /* RF TRX_BW */

		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x87, bRFRegOffsetMask, 0x00025); /* FILTER BW&RC Corner (ACPR) */
#ifdef CONFIG_USB_HCI
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x1C, bRFRegOffsetMask, 0x01000); /* FILTER BW&RC Corner (ACPR) */
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0xDF, bRFRegOffsetMask, 0x00140); /* RC Corner */
		PHY_SetRFReg(Adapter, ODM_RF_PATH_A, 0x1B, bRFRegOffsetMask, 0x01C6C); /* RC Corner */
#endif
		break;

	default:
		/*RT_TRACE(COMP_DBG, DBG_LOUD, ("PHY_SetRF8225Bandwidth(): unknown Bandwidth: %#X\n",Bandwidth )); */
		break;
	}

}

static int
_rtl8188fu_phy_rf6052_config_parafile(
	IN	PADAPTER		Adapter
)
{
	u32					u4RegValue = 0;
	u8					eRFPath;
	BB_REGISTER_DEFINITION_T	*pPhyReg;

	int					rtStatus = _SUCCESS;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	/*3//----------------------------------------------------------------- */
	/*3// <2> Initialize RF */
	/*3//----------------------------------------------------------------- */
	/*for(eRFPath = RF_PATH_A; eRFPath <pHalData->NumTotalRFPath; eRFPath++) */
	for (eRFPath = 0; eRFPath < pHalData->NumTotalRFPath; eRFPath++) {

		pPhyReg = &pHalData->PHYRegDef[eRFPath];

		/*----Store original RFENV control type----*/
		switch (eRFPath) {
		case RF_PATH_A:
		case RF_PATH_C:
			u4RegValue = PHY_QueryBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF_PATH_B :
		case RF_PATH_D:
			u4RegValue = PHY_QueryBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV << 16);
			break;
		}

		/*----Set RF_ENV enable----*/
		PHY_SetBBReg(Adapter, pPhyReg->rfintfe, bRFSI_RFENV << 16, 0x1);
		rtw_udelay_os(1);/*PlatformStallExecution(1); */

		/*----Set RF_ENV output high----*/
		PHY_SetBBReg(Adapter, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);
		rtw_udelay_os(1);/*PlatformStallExecution(1); */

		/* Set bit number of Address and Data for RF register */
		PHY_SetBBReg(Adapter, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0);	/* Set 1 to 4 bits for 8255 */
		rtw_udelay_os(1);/*PlatformStallExecution(1); */

		PHY_SetBBReg(Adapter, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0);	/* Set 0 to 12  bits for 8255 */
		rtw_udelay_os(1);/*PlatformStallExecution(1); */

		/*----Initialize RF fom connfiguration file----*/
		switch (eRFPath) {
		case RF_PATH_A:
			{
				if (HAL_STATUS_FAILURE == rtl8188fu_phy_config_rf_with_headerfile(&pHalData->odmpriv, CONFIG_RF_RADIO, (ODM_RF_RADIO_PATH_E)eRFPath))
					rtStatus = _FAIL;
			}
			break;
		case RF_PATH_B:
			{
				if (HAL_STATUS_FAILURE == rtl8188fu_phy_config_rf_with_headerfile(&pHalData->odmpriv, CONFIG_RF_RADIO, (ODM_RF_RADIO_PATH_E)eRFPath))
					rtStatus = _FAIL;
			}
			break;
		case RF_PATH_C:
			break;
		case RF_PATH_D:
			break;
		}

		/*----Restore RFENV control type----*/;
		switch (eRFPath) {
		case RF_PATH_A:
		case RF_PATH_C:
			PHY_SetBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV, u4RegValue);
			break;
		case RF_PATH_B :
		case RF_PATH_D:
			PHY_SetBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV << 16, u4RegValue);
			break;
		}

		if (rtStatus != _SUCCESS) {
			/*RT_TRACE(COMP_FPGA, DBG_LOUD, ("phy_RF6052_Config_ParaFile():Radio[%d] Fail!!", eRFPath)); */
			goto phy_RF6052_Config_ParaFile_Fail;
		}

	}

	/*3 ----------------------------------------------------------------- */
	/*3 Configuration of Tx Power Tracking */
	/*3 ----------------------------------------------------------------- */

	/* Ulli : only copy tables !!! */

	ODM_ReadAndConfig_MP_8188F_TxPowerTrack_USB(&pHalData->odmpriv);
	/*RT_TRACE(COMP_INIT, DBG_LOUD, ("<---phy_RF6052_Config_ParaFile()\n")); */
	return rtStatus;

phy_RF6052_Config_ParaFile_Fail:
	return rtStatus;
}


int
rtl8188fu_phy_rf6052_config(
	IN	PADAPTER		Adapter)
{
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	int					rtStatus = _SUCCESS;

	/* */
	/* Initialize general global value */
	/* */
	/* TODO: Extend RF_PATH_C and RF_PATH_D in the future */
	if (pHalData->rf_type == RF_1T1R)
		pHalData->NumTotalRFPath = 1;
	else
		pHalData->NumTotalRFPath = 2;

	/* */
	/* Config BB and RF */
	/* */
	rtStatus = _rtl8188fu_phy_rf6052_config_parafile(Adapter);
	return rtStatus;

}

/* End of HalRf6052.c */

