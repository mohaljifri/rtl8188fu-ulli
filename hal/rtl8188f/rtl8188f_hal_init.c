/******************************************************************************
 *
 * Copyright(c) 2007 - 2013 Realtek Corporation. All rights reserved.
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
#define _HAL_INIT_C_

#include <rtl8188f_hal.h>
#include "hal_com_h2c.h"
#include <linux/firmware.h>

#if 0 /* FW tx packet write */
#define FW_DOWNLOAD_SIZE_8188F 8192
#endif

static VOID
_FWDownloadEnable(
	IN	PADAPTER		padapter,
	IN	BOOLEAN			enable
)
{
	u8	tmp, count = 0;

	if (enable) {
		/* 8051 enable */
		tmp = rtw_read8(padapter, REG_SYS_FUNC_EN + 1);
		rtw_write8(padapter, REG_SYS_FUNC_EN + 1, tmp | 0x04);

		tmp = rtw_read8(padapter, REG_MCUFWDL);
		rtw_write8(padapter, REG_MCUFWDL, tmp | 0x01);

		do {
			tmp = rtw_read8(padapter, REG_MCUFWDL);
			if (tmp & 0x01)
				break;
			rtw_write8(padapter, REG_MCUFWDL, tmp | 0x01);
			rtw_msleep_os(1);
		} while (count++ < 100);
		if (count > 0)
			DBG_871X("%s: !!!!!!!!Write 0x80 Fail!: count = %d\n", __func__, count);

		/* 8051 reset */
		tmp = rtw_read8(padapter, REG_MCUFWDL + 2);
		rtw_write8(padapter, REG_MCUFWDL + 2, tmp & 0xf7);
	} else {
		/* MCU firmware download disable. */
		tmp = rtw_read8(padapter, REG_MCUFWDL);
		rtw_write8(padapter, REG_MCUFWDL, tmp & 0xfe);
	}
}

static int
_BlockWrite(
	IN		PADAPTER		padapter,
	IN		PVOID		buffer,
	IN		u32			buffSize
)
{
	int ret = _SUCCESS;

	u32			blockSize_p1 = 4;	/* (Default) Phase #1 : PCI muse use 4-byte write to download FW */
	u32			blockSize_p2 = 8;	/* Phase #2 : Use 8-byte, if Phase#1 use big size to write FW. */
	u32			blockSize_p3 = 1;	/* Phase #3 : Use 1-byte, the remnant of FW image. */
	u32			blockCount_p1 = 0, blockCount_p2 = 0, blockCount_p3 = 0;
	u32			remainSize_p1 = 0, remainSize_p2 = 0;
	u8			*bufferPtr	= (u8 *)buffer;
	u32			i = 0, offset = 0;

#ifdef CONFIG_USB_HCI
	blockSize_p1 = 196; // the same as 8188e
#endif

	/*printk("====>%s %d\n", __func__, __LINE__); */

	/*3 Phase #1 */
	blockCount_p1 = buffSize / blockSize_p1;
	remainSize_p1 = buffSize % blockSize_p1;

	if (blockCount_p1) {
		RT_TRACE(_module_hal_init_c_, _drv_notice_,
				 ("_BlockWrite: [P1] buffSize(%d) blockSize_p1(%d) blockCount_p1(%d) remainSize_p1(%d)\n",
				  buffSize, blockSize_p1, blockCount_p1, remainSize_p1));
	}

	for (i = 0; i < blockCount_p1; i++) {
#ifdef CONFIG_USB_HCI
		ret = rtw_writeN(padapter, (FW_8188F_START_ADDRESS + i * blockSize_p1), blockSize_p1, (bufferPtr + i * blockSize_p1));
#else
		ret = rtw_write32(padapter, (FW_8188F_START_ADDRESS + i * blockSize_p1), le32_to_cpu(*((u32 *)(bufferPtr + i * blockSize_p1))));
#endif
		if (ret == _FAIL) {
			printk("====>%s %d i:%d\n", __func__, __LINE__, i);
			goto exit;
		}
	}

	/*3 Phase #2 */
	if (remainSize_p1) {
		offset = blockCount_p1 * blockSize_p1;

		blockCount_p2 = remainSize_p1 / blockSize_p2;
		remainSize_p2 = remainSize_p1 % blockSize_p2;

		if (blockCount_p2) {
			RT_TRACE(_module_hal_init_c_, _drv_notice_,
					 ("_BlockWrite: [P2] buffSize_p2(%d) blockSize_p2(%d) blockCount_p2(%d) remainSize_p2(%d)\n",
					  (buffSize - offset), blockSize_p2 , blockCount_p2, remainSize_p2));
		}

#ifdef CONFIG_USB_HCI
		for (i = 0; i < blockCount_p2; i++) {
			ret = rtw_writeN(padapter, (FW_8188F_START_ADDRESS + offset + i * blockSize_p2), blockSize_p2, (bufferPtr + offset + i * blockSize_p2));

			if (ret == _FAIL)
				goto exit;
		}
#endif
	}

	/*3 Phase #3 */
	if (remainSize_p2) {
		offset = (blockCount_p1 * blockSize_p1) + (blockCount_p2 * blockSize_p2);

		blockCount_p3 = remainSize_p2 / blockSize_p3;

		RT_TRACE(_module_hal_init_c_, _drv_notice_,
				 ("_BlockWrite: [P3] buffSize_p3(%d) blockSize_p3(%d) blockCount_p3(%d)\n",
				  (buffSize - offset), blockSize_p3, blockCount_p3));

		for (i = 0; i < blockCount_p3; i++) {
			ret = rtw_write8(padapter, (FW_8188F_START_ADDRESS + offset + i), *(bufferPtr + offset + i));

			if (ret == _FAIL) {
				printk("====>%s %d i:%d\n", __func__, __LINE__, i);
				goto exit;
			}
		}
	}
exit:
	return ret;
}

static int
_PageWrite(
	IN		PADAPTER	padapter,
	IN		u32			page,
	IN		PVOID		buffer,
	IN		u32			size
)
{
	u8 value8;
	u8 u8Page = (u8)(page & 0x07);

	value8 = (rtw_read8(padapter, REG_MCUFWDL + 2) & 0xF8) | u8Page;
	rtw_write8(padapter, REG_MCUFWDL + 2, value8);

	return _BlockWrite(padapter, buffer, size);
}

static VOID
_FillDummy(
	u8		*pFwBuf,
	u32	*pFwLen
)
{
	u32	FwLen = *pFwLen;
	u8	remain = (u8)(FwLen % 4);

	remain = (remain == 0) ? 0 : (4 - remain);

	while (remain > 0) {
		pFwBuf[FwLen] = 0;
		FwLen++;
		remain--;
	}

	*pFwLen = FwLen;
}

static int
_WriteFW(
	IN		PADAPTER		padapter,
	IN		PVOID			buffer,
	IN		u32			size
)
{
	/* Since we need dynamic decide method of dwonload fw, so we call this function to get chip version. */
	int ret = _SUCCESS;
	u32		pageNums, remainSize;
	u32		page, offset;
	u8		*bufferPtr = (u8 *)buffer;

	pageNums = size / MAX_DLFW_PAGE_SIZE;
	/*RT_ASSERT((pageNums <= 4), ("Page numbers should not greater then 4\n")); */
	remainSize = size % MAX_DLFW_PAGE_SIZE;

	for (page = 0; page < pageNums; page++) {
		offset = page * MAX_DLFW_PAGE_SIZE;
		ret = _PageWrite(padapter, page, bufferPtr + offset, MAX_DLFW_PAGE_SIZE);

		if (ret == _FAIL) {
			printk("====>%s %d\n", __func__, __LINE__);
			goto exit;
		}
	}
	if (remainSize) {
		offset = pageNums * MAX_DLFW_PAGE_SIZE;
		page = pageNums;
		ret = _PageWrite(padapter, page, bufferPtr + offset, remainSize);

		if (ret == _FAIL) {
			printk("====>%s %d\n", __func__, __LINE__);
			goto exit;
		}
	}
	RT_TRACE(_module_hal_init_c_, _drv_info_, ("_WriteFW Done- for Normal chip.\n"));

exit:
	return ret;
}

void _8051Reset8188(PADAPTER padapter)
{
	u8 cpu_rst;
	u8 io_rst;

#if 0
	io_rst = rtw_read8(padapter, REG_RSV_CTRL);
	rtw_write8(padapter, REG_RSV_CTRL, io_rst & (~BIT1));
#endif

	/* Reset 8051(WLMCU) IO wrapper */
	/* 0x1c[8] = 0 */
	/* Suggested by Isaac@SD1 and Gimmy@SD1, coding by Lucas@20130624 */
	io_rst = rtw_read8(padapter, REG_RSV_CTRL + 1);
	io_rst &= ~BIT(0);
	rtw_write8(padapter, REG_RSV_CTRL + 1, io_rst);

	cpu_rst = rtw_read8(padapter, REG_SYS_FUNC_EN + 1);
	cpu_rst &= ~BIT(2);
	rtw_write8(padapter, REG_SYS_FUNC_EN + 1, cpu_rst);

#if 0
	io_rst = rtw_read8(padapter, REG_RSV_CTRL);
	rtw_write8(padapter, REG_RSV_CTRL, io_rst & (~BIT1));
#endif

	/* Enable 8051 IO wrapper */
	/* 0x1c[8] = 1 */
	io_rst = rtw_read8(padapter, REG_RSV_CTRL + 1);
	io_rst |= BIT(0);
	rtw_write8(padapter, REG_RSV_CTRL + 1, io_rst);

	cpu_rst = rtw_read8(padapter, REG_SYS_FUNC_EN + 1);
	cpu_rst |= BIT(2);
	rtw_write8(padapter, REG_SYS_FUNC_EN + 1, cpu_rst);

	DBG_8192C("%s: Finish\n", __func__);
}

static s32 polling_fwdl_chksum(_adapter *adapter, u32 min_cnt, u32 timeout_ms)
{
	s32 ret = _FAIL;
	u32 value32;
	u32 start = rtw_get_current_time();
	u32 cnt = 0;

	/* polling CheckSum report */
	do {
		cnt++;
		value32 = rtw_read32(adapter, REG_MCUFWDL);
		if (value32 & FWDL_ChkSum_rpt || RTW_CANNOT_IO(adapter))
			break;
		rtw_yield_os();
	} while (rtw_get_passing_time_ms(start) < timeout_ms || cnt < min_cnt);

	if (!(value32 & FWDL_ChkSum_rpt))
		goto exit;

	if (rtw_fwdl_test_trigger_chksum_fail())
		goto exit;

	ret = _SUCCESS;

exit:
	DBG_871X("%s: Checksum report %s! (%u, %dms), REG_MCUFWDL:0x%08x\n", __func__
			 , (ret == _SUCCESS) ? "OK" : "Fail", cnt, rtw_get_passing_time_ms(start), value32);

	return ret;
}

static s32 _FWFreeToGo(_adapter *adapter, u32 min_cnt, u32 timeout_ms)
{
	s32 ret = _FAIL;
	u32	value32;
	u32 start = rtw_get_current_time();
	u32 cnt = 0;
	u32 value_to_check = 0;
	u32 value_expected = (MCUFWDL_RDY | FWDL_ChkSum_rpt | WINTINI_RDY | RAM_DL_SEL);

	value32 = rtw_read32(adapter, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	value32 &= ~WINTINI_RDY;
	rtw_write32(adapter, REG_MCUFWDL, value32);

	_8051Reset8188(adapter);

	/*  polling for FW ready */
	do {
		cnt++;
		value32 = rtw_read32(adapter, REG_MCUFWDL);
		value_to_check = value32 & value_expected;
		if ((value_to_check == value_expected) || RTW_CANNOT_IO(adapter))
			break;
		rtw_yield_os();
	} while (rtw_get_passing_time_ms(start) < timeout_ms || cnt < min_cnt);

	if (value_to_check != value_expected)
		goto exit;

	if (rtw_fwdl_test_trigger_wintint_rdy_fail())
		goto exit;

	ret = _SUCCESS;

exit:
	DBG_871X("%s: Polling FW ready %s! (%u, %dms), REG_MCUFWDL:0x%08x\n", __func__
			 , (ret == _SUCCESS) ? "OK" : "Fail", cnt, rtw_get_passing_time_ms(start), value32);

	return ret;
}

#define IS_FW_81xxC(padapter)	(((GET_HAL_DATA(padapter))->FirmwareSignature & 0xFFF0) == 0x88C0)

void rtl8188f_FirmwareSelfReset(PADAPTER padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8	u1bTmp;
	u8	Delay = 100;

	if (!(IS_FW_81xxC(padapter) &&
		  ((pHalData->FirmwareVersion < 0x21) ||
		   (pHalData->FirmwareVersion == 0x21 &&
			pHalData->FirmwareSubVersion < 0x01)))) { /* after 88C Fw v33.1 */
		/*0x1cf=0x20. Inform 8051 to reset. 2009.12.25. tynli_test */
		rtw_write8(padapter, REG_HMETFR + 3, 0x20);

		u1bTmp = rtw_read8(padapter, REG_SYS_FUNC_EN + 1);
		while (u1bTmp & BIT2) {
			Delay--;
			if (Delay == 0)
				break;
			rtw_udelay_os(50);
			u1bTmp = rtw_read8(padapter, REG_SYS_FUNC_EN + 1);
		}
		RT_TRACE(_module_hal_init_c_, _drv_notice_, ("-%s: 8051 reset success (%d)\n", __func__, Delay));

		if (Delay == 0) {
			RT_TRACE(_module_hal_init_c_, _drv_notice_, ("%s: Force 8051 reset!!!\n", __func__));
			/*force firmware reset */
			u1bTmp = rtw_read8(padapter, REG_SYS_FUNC_EN + 1);
			rtw_write8(padapter, REG_SYS_FUNC_EN + 1, u1bTmp & (~BIT2));
		}
	}
}

#ifdef CONFIG_FILE_FWIMG
extern char *rtw_fw_file_path;
extern char *rtw_fw_wow_file_path;
u8 FwBuffer[FW_8188F_SIZE];
#endif /* CONFIG_FILE_FWIMG */

#if defined(CONFIG_USB_HCI)
void rtl8188f_cal_txdesc_chksum(struct tx_desc *ptxdesc)
{
	u16	*usPtr = (u16 *)ptxdesc;
	u32 count;
	u32 index;
	u16 checksum = 0;


	/* Clear first */
	ptxdesc->txdw7 &= cpu_to_le32(0xffff0000);

	/* checksume is always calculated by first 32 bytes, */
	/* and it doesn't depend on TX DESC length. */
	/* Thomas,Lucas@SD4,20130515 */
	count = 16;

	for (index = 0; index < count; index++)
		checksum ^= le16_to_cpu(*(usPtr + index));

	ptxdesc->txdw7 |= cpu_to_le32(checksum & 0x0000ffff);
}
#endif

/* */
/*	Description: */
/*		Download 8192C firmware code. */
/* */
/* */

MODULE_FIRMWARE("rtlwifi/rtl8188fufw.bin");

s32 rtl8188f_FirmwareDownload(PADAPTER padapter)
{
	s32	rtStatus = _SUCCESS;
	u8 write_fw = 0;
	u32 fwdl_start_time;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);
	u8			*FwImage;
	u32			FwImageLen;
	u8			*pucMappedFile = NULL;
	PRT_8188F_FIRMWARE_HDR		pFwHdr = NULL;
	u8			*pFirmwareBuf;
	u32			FirmwareLen;
	char *fw_name =  "rtlwifi/rtl8188fufw.bin";
	const struct firmware *fw;
	u8			value8;
	struct dvobj_priv *psdpriv = padapter->dvobj;
	struct debug_priv *pdbgpriv = &psdpriv->drv_dbg;
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);

	RT_TRACE(_module_hal_init_c_, _drv_info_, ("+%s\n", __func__));

	{
		u8 tmp_ps = 0, tmp_rf = 0;

		tmp_ps = rtw_read8(padapter, 0xa3);
		tmp_ps &= 0xf8;
		tmp_ps |= 0x02;
		/*1. write 0xA3[:2:0] = 3b'010 */
		rtw_write8(padapter, 0xa3, tmp_ps);
		/*2. read power_state = 0xA0[1:0] */
		tmp_ps = rtw_read8(padapter, 0xa0);
		tmp_ps &= 0x03;
		if (tmp_ps != 0x01) {
			DBG_871X(FUNC_ADPT_FMT" tmp_ps=%x\n", FUNC_ADPT_ARG(padapter), tmp_ps);
			pdbgpriv->dbg_downloadfw_pwr_state_cnt++;
		}
	}

	dev_info(&psdpriv->pusbdev->dev, "request firmware %s\n",fw_name);
	if (request_firmware(&fw, fw_name, &psdpriv->pusbdev->dev)) {
		dev_err(&psdpriv->pusbdev->dev, "Firmware %s not available\n", fw_name);
		goto exit;
	}

	dev_info(&psdpriv->pusbdev->dev, "request firmware %s loaded\n",fw_name);


	if (fw->size > FW_8188F_SIZE) {
		rtStatus = _FAIL;
		DBG_871X_LEVEL(_drv_emerg_, "Firmware size:%u exceed %u\n", (int) fw->size, FW_8188F_SIZE);
		goto exit;
	}

	pFirmwareBuf = (u8 *) fw->data;
	FirmwareLen = fw->size;

	/* To Check Fw header. Added by tynli. 2009.12.04. */
	pFwHdr = (PRT_8188F_FIRMWARE_HDR)pFirmwareBuf;

	pHalData->FirmwareVersion =  le16_to_cpu(pFwHdr->Version);
	pHalData->FirmwareSubVersion = le16_to_cpu(pFwHdr->Subversion);
	pHalData->FirmwareSignature = le16_to_cpu(pFwHdr->Signature);

	DBG_871X("%s: fw_ver=%x fw_subver=%04x sig=0x%x, Month=%02x, Date=%02x, Hour=%02x, Minute=%02x\n",
			 __func__, pHalData->FirmwareVersion, pHalData->FirmwareSubVersion, pHalData->FirmwareSignature
			 , pFwHdr->Month, pFwHdr->Date, pFwHdr->Hour, pFwHdr->Minute);

	if (IS_FW_HEADER_EXIST_8188F(pFwHdr)) {
		DBG_871X("%s(): Shift for fw header!\n", __func__);
		/* Shift 32 bytes for FW header */
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen - 32;
	}

	fwdl_start_time = rtw_get_current_time();

	DBG_871X("%s by IO write!\n", __func__);

	/*
	* Suggested by Filen. If 8051 is running in RAM code, driver should inform Fw to reset by itself,
	* or it will cause download Fw fail. 2010.02.01. by tynli.
	*/
	if (rtw_read8(padapter, REG_MCUFWDL) & RAM_DL_SEL) {
		rtw_write8(padapter, REG_MCUFWDL, 0x00);
		_8051Reset8188(padapter);
	}

	_FWDownloadEnable(padapter, _TRUE);

	while (!RTW_CANNOT_IO(padapter)
		   && (write_fw++ < 3 || rtw_get_passing_time_ms(fwdl_start_time) < 500)) {
		/* reset FWDL chksum */
		rtw_write8(padapter, REG_MCUFWDL, rtw_read8(padapter, REG_MCUFWDL) | FWDL_ChkSum_rpt);

		rtStatus = _WriteFW(padapter, pFirmwareBuf, FirmwareLen);
		if (rtStatus != _SUCCESS)
			continue;

		rtStatus = polling_fwdl_chksum(padapter, 5, 50);
		if (rtStatus == _SUCCESS)
			break;
	}

	_FWDownloadEnable(padapter, _FALSE);

	rtStatus = _FWFreeToGo(padapter, 10, 200);
	if (_SUCCESS != rtStatus)
		goto DLFW_FAIL;

	DBG_871X("%s: DLFW OK !\n", __func__);

DLFW_FAIL:
	if (rtStatus == _FAIL) {
		/* Disable FWDL_EN */
		value8 = rtw_read8(padapter, REG_MCUFWDL);
		value8 = (value8 & ~(BIT(0)) & ~(BIT(1)));
		rtw_write8(padapter, REG_MCUFWDL, value8);
	}

	DBG_871X("%s %s. write_fw:%u, %dms\n"
			 , __func__, (rtStatus == _SUCCESS) ? "success" : "fail"
			 , write_fw
			 , rtw_get_passing_time_ms(fwdl_start_time)
			);

exit:
	release_firmware(fw);

	rtl8188f_InitializeFirmwareVars(padapter);

	DBG_871X(" <=== %s()\n", __func__);

	return rtStatus;
}

void rtl8188f_InitializeFirmwareVars(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);

	/* Init Fw LPS related. */
	adapter_to_pwrctl(padapter)->bFwCurrentInPSMode = _FALSE;

	/*Init H2C cmd. */
	rtw_write8(padapter, REG_HMETFR, 0x0f);

	/* Init H2C counter. by tynli. 2009.12.09. */
	pHalData->LastHMEBoxNum = 0;
	/*pHalData->H2CQueueHead = 0; */
	/*pHalData->H2CQueueTail = 0; */
	/*pHalData->H2CStopInsertQueue = _FALSE; */
}

/*=========================================================== */
/*				Efuse related code */
/*=========================================================== */
static u8
hal_EfuseSwitchToBank(
	PADAPTER	padapter,
	u8			bank)
{
	u8 bRet = _FALSE;
	u32 value32 = 0;

	DBG_8192C("%s: Efuse switch bank to %d\n", __func__, bank);
	{
		value32 = rtw_read32(padapter, EFUSE_TEST);
		bRet = _TRUE;
		switch (bank) {
		case 0:
			value32 = (value32 & ~EFUSE_SEL_MASK) | EFUSE_SEL(EFUSE_WIFI_SEL_0);
			break;
		case 1:
			value32 = (value32 & ~EFUSE_SEL_MASK) | EFUSE_SEL(EFUSE_BT_SEL_0);
			break;
		case 2:
			value32 = (value32 & ~EFUSE_SEL_MASK) | EFUSE_SEL(EFUSE_BT_SEL_1);
			break;
		case 3:
			value32 = (value32 & ~EFUSE_SEL_MASK) | EFUSE_SEL(EFUSE_BT_SEL_2);
			break;
		default:
			value32 = (value32 & ~EFUSE_SEL_MASK) | EFUSE_SEL(EFUSE_WIFI_SEL_0);
			bRet = _FALSE;
			break;
		}
		rtw_write32(padapter, EFUSE_TEST, value32);
	}

	return bRet;
}

void
Hal_GetEfuseDefinition(
	PADAPTER	padapter,
	u8			type,
	void		*pOut)
{
	switch (type) {
	case TYPE_EFUSE_MAX_SECTION: {
		u8 *pMax_section;

		pMax_section = (u8 *)pOut;

		*pMax_section = EFUSE_MAX_SECTION_8188F;
	}
	break;

	case TYPE_EFUSE_REAL_CONTENT_LEN: {
		u16 *pu2Tmp;

		pu2Tmp = (u16 *)pOut;

		*pu2Tmp = EFUSE_REAL_CONTENT_LEN_8188F;
	}
	break;

	case TYPE_AVAILABLE_EFUSE_BYTES_BANK: {
		u16	*pu2Tmp;

		pu2Tmp = (u16 *)pOut;

		*pu2Tmp = (EFUSE_REAL_CONTENT_LEN_8188F - EFUSE_OOB_PROTECT_BYTES);
	}
	break;

	case TYPE_AVAILABLE_EFUSE_BYTES_TOTAL: {
		u16 *pu2Tmp;

		pu2Tmp = (u16 *)pOut;

		*pu2Tmp = (EFUSE_REAL_CONTENT_LEN_8188F - EFUSE_OOB_PROTECT_BYTES);
	}
	break;

	case TYPE_EFUSE_MAP_LEN: {
		u16 *pu2Tmp;

		pu2Tmp = (u16 *)pOut;

		*pu2Tmp = EFUSE_MAP_LEN_8188F;
	}
	break;

	case TYPE_EFUSE_PROTECT_BYTES_BANK: {
		u8 *pu1Tmp;

		pu1Tmp = (u8 *)pOut;

		*pu1Tmp = EFUSE_OOB_PROTECT_BYTES;
	}
	break;

	case TYPE_EFUSE_CONTENT_LEN_BANK: {
		u16 *pu2Tmp;

		pu2Tmp = (u16 *)pOut;

		*pu2Tmp = EFUSE_REAL_CONTENT_LEN_8188F;
	}
	break;

	default: {
		u8 *pu1Tmp;

		pu1Tmp = (u8 *)pOut;
		*pu1Tmp = 0;
	}
	break;
	}
}

#define VOLTAGE_V25		0x03
#define LDOE25_SHIFT	28

/*================================================================= */
/*	The following is for compile ok */
/*	That should be merged with the original in the future */
/*================================================================= */
#define EFUSE_ACCESS_ON_8188			0x69	/* For RTL8188 only. */
#define EFUSE_ACCESS_OFF_8188			0x00	/* For RTL8188 only. */
#define REG_EFUSE_ACCESS_8188			0x00CF	/* Efuse access protection for RTL8188 */

void rtl8188fu_EfusePowerSwitch(
	PADAPTER	padapter,
	u8			bWrite,
	u8			PwrState)
{
	u8	tempval;
	u16	tmpV16;


	if (PwrState == _TRUE) {

		rtw_write8(padapter, REG_EFUSE_ACCESS_8188, EFUSE_ACCESS_ON_8188);

		/* Reset: 0x0000h[28], default valid */
		tmpV16 =  rtw_read16(padapter, REG_SYS_FUNC_EN);
		if (!(tmpV16 & FEN_ELDR)) {
			tmpV16 |= FEN_ELDR;
			rtw_write16(padapter, REG_SYS_FUNC_EN, tmpV16);
		}

		/* Clock: Gated(0x0008h[5]) 8M(0x0008h[1]) clock from ANA, default valid */
		tmpV16 = rtw_read16(padapter, REG_SYS_CLKR);
		if ((!(tmpV16 & LOADER_CLK_EN))  || (!(tmpV16 & ANA8M))) {
			tmpV16 |= (LOADER_CLK_EN | ANA8M);
			rtw_write16(padapter, REG_SYS_CLKR, tmpV16);
		}

		if (bWrite == _TRUE) {
			/* Enable LDO 2.5V before read/write action */
			tempval = rtw_read8(padapter, EFUSE_TEST + 3);
			rtw_write8(padapter, EFUSE_TEST + 3, (tempval | 0x80));
		}
	} else {
		rtw_write8(padapter, REG_EFUSE_ACCESS_8188, EFUSE_ACCESS_OFF);

		if (bWrite == _TRUE) {
			/* Disable LDO 2.5V after read/write action */
			tempval = rtw_read8(padapter, EFUSE_TEST + 3);
			rtw_write8(padapter, EFUSE_TEST + 3, (tempval & 0x7F));
		}

	}
}

void
rtl8188fu_efuse_read_efuse(
	PADAPTER	padapter,
	u16			_offset,
	u16			_size_byte,
	u8			*pbuf)
{
	u8	*efuseTbl = NULL;
	u16	eFuse_Addr = 0;
	u8	offset, wden;
	u8	efuseHeader, efuseExtHdr, efuseData;
	u16	i, total, used;
	u8	efuse_usage = 0;

	/*DBG_871X("YJ: ====>%s():_offset=%d _size_byte=%d bPseudoTest=%d\n", __func__, _offset, _size_byte, bPseudoTest); */
	/* */
	/* Do NOT excess total size of EFuse table. Added by Roger, 2008.11.10. */
	/* */
	if ((_offset + _size_byte) > EFUSE_MAX_MAP_LEN) {
		DBG_8192C("%s: Invalid offset(%#x) with read bytes(%#x)!!\n", __func__, _offset, _size_byte);
		return;
	}

	efuseTbl = (u8 *)rtw_malloc(EFUSE_MAX_MAP_LEN);
	if (efuseTbl == NULL) {
		DBG_8192C("%s: alloc efuseTbl fail!\n", __func__);
		return;
	}
	/* 0xff will be efuse default value instead of 0x00. */
	_rtw_memset(efuseTbl, 0xFF, EFUSE_MAX_MAP_LEN);


#ifdef CONFIG_DEBUG
	if (0) {
		for (i = 0; i < 256; i++)
			/*ReadEFuseByte(padapter, i, &efuseTbl[i], _FALSE); */
			efuse_OneByteRead(padapter, i, &efuseTbl[i]);
		DBG_871X("Efuse Content:\n");
		for (i = 0; i < 256; i++)
			printk("%02X%s", efuseTbl[i], (((i + 1) % 16) == 0)?"\n":" ");
	}
#endif


	/* switch bank back to bank 0 for later BT and wifi use. */
	hal_EfuseSwitchToBank(padapter, 0);

	while (AVAILABLE_EFUSE_ADDR(eFuse_Addr)) {
		/*ReadEFuseByte(padapter, eFuse_Addr++, &efuseHeader, bPseudoTest); */
		efuse_OneByteRead(padapter, eFuse_Addr++, &efuseHeader);
		if (efuseHeader == 0xFF) {
			DBG_8192C("%s: data end at address=%#x\n", __func__, eFuse_Addr - 1);
			break;
		}
		/*DBG_8192C("%s: efuse[0x%X]=0x%02X\n", __func__, eFuse_Addr-1, efuseHeader); */

		/* Check PG header for section num. */
		if (EXT_HEADER(efuseHeader)) {	/*extended header */
			offset = GET_HDR_OFFSET_2_0(efuseHeader);
			/*DBG_8192C("%s: extended header offset=0x%X\n", __func__, offset); */

			/*ReadEFuseByte(padapter, eFuse_Addr++, &efuseExtHdr, bPseudoTest); */
			efuse_OneByteRead(padapter, eFuse_Addr++, &efuseExtHdr);
			/*DBG_8192C("%s: efuse[0x%X]=0x%02X\n", __func__, eFuse_Addr-1, efuseExtHdr); */
			if (ALL_WORDS_DISABLED(efuseExtHdr))
				continue;

			offset |= ((efuseExtHdr & 0xF0) >> 1);
			wden = (efuseExtHdr & 0x0F);
		} else {
			offset = ((efuseHeader >> 4) & 0x0f);
			wden = (efuseHeader & 0x0f);
		}
		/*DBG_8192C("%s: Offset=%d Worden=0x%X\n", __func__, offset, wden); */

		if (offset < EFUSE_MAX_SECTION_8188F) {
			u16 addr;
			/* Get word enable value from PG header */
			/*DBG_8192C("%s: Offset=%d Worden=0x%X\n", __func__, offset, wden); */

			addr = offset * PGPKT_DATA_SIZE;
			for (i = 0; i < EFUSE_MAX_WORD_UNIT; i++) {
				/* Check word enable condition in the section */
				if (!(wden & (0x01 << i))) {
					efuseData = 0;
					/*ReadEFuseByte(padapter, eFuse_Addr++, &efuseData, bPseudoTest); */
					efuse_OneByteRead(padapter, eFuse_Addr++, &efuseData);
					/*DBG_8192C("%s: efuse[%#X]=0x%02X\n", __func__, eFuse_Addr-1, efuseData); */
					efuseTbl[addr] = efuseData;

					efuseData = 0;
					/*ReadEFuseByte(padapter, eFuse_Addr++, &efuseData, bPseudoTest); */
					efuse_OneByteRead(padapter, eFuse_Addr++, &efuseData);
					/*DBG_8192C("%s: efuse[%#X]=0x%02X\n", __func__, eFuse_Addr-1, efuseData); */
					efuseTbl[addr + 1] = efuseData;
				}
				addr += 2;
			}
		} else {
			DBG_8192C(KERN_ERR "%s: offset(%d) is illegal!!\n", __func__, offset);
			eFuse_Addr += Efuse_CalculateWordCnts(wden) * 2;
		}
	}

	/* Copy from Efuse map to output pointer memory!!! */
	for (i = 0; i < _size_byte; i++)
		pbuf[i] = efuseTbl[_offset + i];

#if 0
#ifdef CONFIG_DEBUG
	if (1) {
		DBG_871X("Efuse Realmap:\n");
		for (i = 0; i < _size_byte; i++)
			printk("%02X%s", pbuf[i], (((i + 1) % 16) == 0)?"\n":" ");
	}
#endif
#endif
	/* Calculate Efuse utilization */
	total = 0;
	EFUSE_GetEfuseDefinition(padapter, TYPE_AVAILABLE_EFUSE_BYTES_TOTAL, &total);
	used = eFuse_Addr - 1;
	if (total)
		efuse_usage = (u8)((used * 100) / total);
	else
		efuse_usage = 100;

	{
		rtw_hal_set_hwreg(padapter, HW_VAR_EFUSE_BYTES, (u8 *)&used);
		rtw_hal_set_hwreg(padapter, HW_VAR_EFUSE_USAGE, (u8 *)&efuse_usage);
	}

	if (efuseTbl)
		rtw_mfree(efuseTbl, EFUSE_MAX_MAP_LEN);
}

static u16
hal_EfuseGetCurrentSize_WiFi(PADAPTER	padapter)
{
	u16	efuse_addr = 0;
	u16 start_addr = 0; /* for debug */
	u8	hoffset = 0, hworden = 0;
	u8	efuse_data, word_cnts = 0;
	u32 count = 0; /* for debug */


	rtw_hal_get_hwreg(padapter, HW_VAR_EFUSE_BYTES, (u8 *)&efuse_addr);

	start_addr = efuse_addr;
	DBG_8192C("%s: start_efuse_addr=0x%X\n", __func__, efuse_addr);

	/* switch bank back to bank 0 for later BT and wifi use. */
	hal_EfuseSwitchToBank(padapter, 0);

#if 0 /* for debug test */
	efuse_OneByteRead(padapter, 0x1FF, &efuse_data);
	DBG_8192C(FUNC_ADPT_FMT ": efuse raw 0x1FF=0x%02X\n",
			  FUNC_ADPT_ARG(padapter), efuse_data);
	efuse_data = 0xFF;
#endif /* for debug test */

	count = 0;
	while (AVAILABLE_EFUSE_ADDR(efuse_addr)) {
#if 1
		if (efuse_OneByteRead(padapter, efuse_addr, &efuse_data) == _FALSE) {
			DBG_8192C(KERN_ERR "%s: efuse_OneByteRead Fail! addr=0x%X !!\n", __func__, efuse_addr);
			goto error;
		}
#else
		ReadEFuseByte(padapter, efuse_addr, &efuse_data);
#endif

		if (efuse_data == 0xFF) break;

		if ((start_addr != 0) && (efuse_addr == start_addr)) {
			count++;
			DBG_8192C(FUNC_ADPT_FMT ": [WARNING] efuse raw 0x%X=0x%02X not 0xFF!!(%d times)\n",
					  FUNC_ADPT_ARG(padapter), efuse_addr, efuse_data, count);

			efuse_data = 0xFF;
			if (count < 4) {
				/* try again! */

				if (count > 2) {
					/* try again form address 0 */
					efuse_addr = 0;
					start_addr = 0;
				}

				continue;
			}

			goto error;
		}

		if (EXT_HEADER(efuse_data)) {
			hoffset = GET_HDR_OFFSET_2_0(efuse_data);
			efuse_addr++;
			efuse_OneByteRead(padapter, efuse_addr, &efuse_data);
			if (ALL_WORDS_DISABLED(efuse_data))
				continue;

			hoffset |= ((efuse_data & 0xF0) >> 1);
			hworden = efuse_data & 0x0F;
		} else {
			hoffset = (efuse_data >> 4) & 0x0F;
			hworden = efuse_data & 0x0F;
		}

		word_cnts = Efuse_CalculateWordCnts(hworden);
		efuse_addr += (word_cnts * 2) + 1;
	}

	rtw_hal_set_hwreg(padapter, HW_VAR_EFUSE_BYTES, (u8 *)&efuse_addr);

	goto exit;

error:
	/* report max size to prevent write efuse */
	EFUSE_GetEfuseDefinition(padapter, TYPE_AVAILABLE_EFUSE_BYTES_TOTAL, &efuse_addr);

exit:
	DBG_8192C("%s: CurrentSize=%d\n", __func__, efuse_addr);

	return efuse_addr;
}

u16
Hal_EfuseGetCurrentSize(
	PADAPTER	pAdapter)
{
	u16	ret = 0;

	ret = hal_EfuseGetCurrentSize_WiFi(pAdapter);

	return ret;
}

s32
Hal_EfusePgPacketRead(
	PADAPTER	padapter,
	u8			offset,
	u8			*data)
{
	u8	bDataEmpty = _TRUE;
	u8	efuse_data, word_cnts = 0;
	u16	efuse_addr = 0;
	u8	hoffset = 0, hworden = 0;
	u8	i;
	u8	max_section = 0;
	s32	ret;


	if (data == NULL)
		return _FALSE;

	EFUSE_GetEfuseDefinition(padapter, TYPE_EFUSE_MAX_SECTION, &max_section);
	if (offset > max_section) {
		DBG_8192C("%s: Packet offset(%d) is illegal(>%d)!\n", __func__, offset, max_section);
		return _FALSE;
	}

	_rtw_memset(data, 0xFF, PGPKT_DATA_SIZE);
	ret = _TRUE;

	/* */
	/* <Roger_TODO> Efuse has been pre-programmed dummy 5Bytes at the end of Efuse by CP. */
	/* Skip dummy parts to prevent unexpected data read from Efuse. */
	/* By pass right now. 2009.02.19. */
	/* */
	while (AVAILABLE_EFUSE_ADDR(efuse_addr)) {
		if (efuse_OneByteRead(padapter, efuse_addr++, &efuse_data) == _FALSE) {
			ret = _FALSE;
			break;
		}

		if (efuse_data == 0xFF) break;

		if (EXT_HEADER(efuse_data)) {
			hoffset = GET_HDR_OFFSET_2_0(efuse_data);
			efuse_OneByteRead(padapter, efuse_addr++, &efuse_data);
			if (ALL_WORDS_DISABLED(efuse_data)) {
				DBG_8192C("%s: Error!! All words disabled!\n", __func__);
				continue;
			}

			hoffset |= ((efuse_data & 0xF0) >> 1);
			hworden = efuse_data & 0x0F;
		} else {
			hoffset = (efuse_data >> 4) & 0x0F;
			hworden =  efuse_data & 0x0F;
		}

		if (hoffset == offset) {
			for (i = 0; i < EFUSE_MAX_WORD_UNIT; i++) {
				/* Check word enable condition in the section */
				if (!(hworden & (0x01 << i))) {
					/*ReadEFuseByte(padapter, efuse_addr++, &efuse_data, bPseudoTest); */
					efuse_OneByteRead(padapter, efuse_addr++, &efuse_data);
					/*DBG_8192C("%s: efuse[%#X]=0x%02X\n", __func__, efuse_addr+tmpidx, efuse_data); */
					data[i * 2] = efuse_data;

					/*ReadEFuseByte(padapter, efuse_addr++, &efuse_data, bPseudoTest); */
					efuse_OneByteRead(padapter, efuse_addr++, &efuse_data);
					/*DBG_8192C("%s: efuse[%#X]=0x%02X\n", __func__, efuse_addr+tmpidx, efuse_data); */
					data[(i * 2) + 1] = efuse_data;
				}
			}
		} else {
			word_cnts = Efuse_CalculateWordCnts(hworden);
			efuse_addr += word_cnts * 2;
		}
	}

	return ret;
}

static u8
hal_EfusePgCheckAvailableAddr(
	PADAPTER	pAdapter)
{
	u16	max_available = 0;
	u16 current_size;


	EFUSE_GetEfuseDefinition(pAdapter, TYPE_AVAILABLE_EFUSE_BYTES_TOTAL, &max_available);
	/*DBG_8192C("%s: max_available=%d\n", __func__, max_available); */

	current_size = Efuse_GetCurrentSize(pAdapter);
	if (current_size >= max_available) {
		DBG_8192C("%s: Error!! current_size(%d)>max_available(%d)\n", __func__, current_size, max_available);
		return _FALSE;
	}
	return _TRUE;
}

static void
hal_EfuseConstructPGPkt(
	u8 				offset,
	u8				word_en,
	u8				*pData,
	PPGPKT_STRUCT	pTargetPkt)
{
	_rtw_memset(pTargetPkt->data, 0xFF, PGPKT_DATA_SIZE);
	pTargetPkt->offset = offset;
	pTargetPkt->word_en = word_en;
	efuse_WordEnableDataRead(word_en, pData, pTargetPkt->data);
	pTargetPkt->word_cnts = Efuse_CalculateWordCnts(pTargetPkt->word_en);
}

#if 0
static u8
wordEnMatched(
	PPGPKT_STRUCT	pTargetPkt,
	PPGPKT_STRUCT	pCurPkt,
	u8				*pWden)
{
	u8	match_word_en = 0x0F;	/* default all words are disabled */
	u8	i;

	/* check if the same words are enabled both target and current PG packet */
	if (((pTargetPkt->word_en & BIT(0)) == 0) &&
		((pCurPkt->word_en & BIT(0)) == 0)) {
		match_word_en &= ~BIT(0);				/* enable word 0 */
	}
	if (((pTargetPkt->word_en & BIT(1)) == 0) &&
		((pCurPkt->word_en & BIT(1)) == 0)) {
		match_word_en &= ~BIT(1);				/* enable word 1 */
	}
	if (((pTargetPkt->word_en & BIT(2)) == 0) &&
		((pCurPkt->word_en & BIT(2)) == 0)) {
		match_word_en &= ~BIT(2);				/* enable word 2 */
	}
	if (((pTargetPkt->word_en & BIT(3)) == 0) &&
		((pCurPkt->word_en & BIT(3)) == 0)) {
		match_word_en &= ~BIT(3);				/* enable word 3 */
	}

	*pWden = match_word_en;

	if (match_word_en != 0xf)
		return _TRUE;
	else
		return _FALSE;
}

static u8
hal_EfuseCheckIfDatafollowed(
	PADAPTER		pAdapter,
	u8				word_cnts,
	u16				startAddr)
{
	u8 bRet = _FALSE;
	u8 i, efuse_data;

	for (i = 0; i < (word_cnts * 2); i++) {
		if (efuse_OneByteRead(pAdapter, (startAddr + i) , &efuse_data) == _FALSE) {
			DBG_8192C("%s: efuse_OneByteRead FAIL!!\n", __func__);
			bRet = _TRUE;
			break;
		}

		if (efuse_data != 0xFF) {
			bRet = _TRUE;
			break;
		}
	}

	return bRet;
}
#endif

void rtl8188fu_read_chip_version(PADAPTER padapter)
{
	u32 value32;
	HAL_DATA_TYPE *pHalData;
	u8	tmpvdr;
	pHalData = GET_HAL_DATA(padapter);

	value32 = rtw_read32(padapter, REG_SYS_CFG);
	pHalData->VersionID.ICType = CHIP_8188F;
	pHalData->VersionID.ChipType = ((value32 & RTL_ID) ? TEST_CHIP : NORMAL_CHIP);
	pHalData->VersionID.RFType = RF_TYPE_1T1R;

	tmpvdr = (value32 & EXT_VENDOR_ID) >> EXT_VENDOR_ID_SHIFT;
	if (tmpvdr == 0x00)
		pHalData->VersionID.VendorType = CHIP_VENDOR_TSMC;
	else if (tmpvdr == 0x01)
		pHalData->VersionID.VendorType = CHIP_VENDOR_SMIC;
	else if (tmpvdr == 0x02)
		pHalData->VersionID.VendorType = CHIP_VENDOR_UMC;

	pHalData->VersionID.CUTVersion = (value32 & CHIP_VER_RTL_MASK) >> CHIP_VER_RTL_SHIFT; /* IC version (CUT) */

	rtw_hal_config_rftype(padapter);

}


void rtl8188f_InitBeaconParameters(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);
	u16 val16;
	u8 val8;


	val8 = DIS_TSF_UDT;
	val16 = val8 | (val8 << 8); /* port0 and port1 */
	rtw_write16(padapter, REG_BCN_CTRL, val16);

	/* TODO: Remove these magic number */
	rtw_write16(padapter, REG_TBTT_PROHIBIT, 0x6404);/* ms */
	/* Firmware will control REG_DRVERLYINT when power saving is enable, */
	/* so don't set this register on STA mode. */
	if (check_fwstate(&padapter->mlmepriv, WIFI_STATION_STATE) == _FALSE)
		rtw_write8(padapter, REG_DRVERLYINT, DRIVER_EARLY_INT_TIME_8188F); /* 5ms */
	rtw_write8(padapter, REG_BCNDMATIM, BCN_DMA_ATIME_INT_TIME_8188F); /* 2ms */

	/* Suggested by designer timchen. Change beacon AIFS to the largest number */
	/* beacause test chip does not contension before sending beacon. by tynli. 2009.11.03 */
	rtw_write16(padapter, REG_BCNTCFG, 0x660F);

	pHalData->RegBcnCtrlVal = rtw_read8(padapter, REG_BCN_CTRL);
	pHalData->RegTxPause = rtw_read8(padapter, REG_TXPAUSE);
	pHalData->RegFwHwTxQCtrl = rtw_read8(padapter, REG_FWHW_TXQ_CTRL + 2);
	pHalData->RegReg542 = rtw_read8(padapter, REG_TBTT_PROHIBIT + 2);
	pHalData->RegCR_1 = rtw_read8(padapter, REG_CR + 1);
}

void rtl8188f_InitBeaconMaxError(PADAPTER padapter, u8 InfraMode)
{
#ifdef CONFIG_ADHOC_WORKAROUND_SETTING
	rtw_write8(padapter, REG_BCN_MAX_ERR, 0xFF);
#else
	/*rtw_write8(Adapter, REG_BCN_MAX_ERR, (InfraMode ? 0xFF : 0x10)); */
#endif
}

void _InitBurstPktLen_8188FS(PADAPTER Adapter)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	rtw_write8(Adapter, 0x4c7, rtw_read8(Adapter, 0x4c7) | BIT(7)); /*enable single pkt ampdu */
	rtw_write8(Adapter, REG_RX_PKT_LIMIT_8188F, 0x18);		/*for VHT packet length 11K */
	rtw_write8(Adapter, REG_MAX_AGGR_NUM_8188F, 0x1F);
	rtw_write8(Adapter, REG_PIFS_8188F, 0x00);
	rtw_write8(Adapter, REG_FWHW_TXQ_CTRL_8188F, rtw_read8(Adapter, REG_FWHW_TXQ_CTRL) & (~BIT(7)));
	if (pHalData->AMPDUBurstMode)
		rtw_write8(Adapter, REG_AMPDU_BURST_MODE_8188F,  0x5F);
	rtw_write8(Adapter, REG_AMPDU_MAX_TIME_8188F, 0x70);

	/* ARFB table 9 for 11ac 5G 2SS */
	rtw_write32(Adapter, REG_ARFR0_8188F, 0x00000010);
	if (IS_NORMAL_CHIP(pHalData->VersionID))
		rtw_write32(Adapter, REG_ARFR0_8188F + 4, 0xfffff000);
	else
		rtw_write32(Adapter, REG_ARFR0_8188F + 4, 0x3e0ff000);

	/* ARFB table 10 for 11ac 5G 1SS */
	rtw_write32(Adapter, REG_ARFR1_8188F, 0x00000010);
	rtw_write32(Adapter, REG_ARFR1_8188F + 4, 0x003ff000);
}

static void ResumeTxBeacon(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);


	/* 2010.03.01. Marked by tynli. No need to call workitem beacause we record the value */
	/* which should be read from register to a global variable. */

	RT_TRACE(_module_hci_hal_init_c_, _drv_info_, ("+ResumeTxBeacon\n"));

	pHalData->RegFwHwTxQCtrl |= BIT(6);
	rtw_write8(padapter, REG_FWHW_TXQ_CTRL + 2, pHalData->RegFwHwTxQCtrl);
	rtw_write8(padapter, REG_TBTT_PROHIBIT + 1, 0xff);
	pHalData->RegReg542 |= BIT(0);
	rtw_write8(padapter, REG_TBTT_PROHIBIT + 2, pHalData->RegReg542);
}

static void StopTxBeacon(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);


	/* 2010.03.01. Marked by tynli. No need to call workitem beacause we record the value */
	/* which should be read from register to a global variable. */

	RT_TRACE(_module_hci_hal_init_c_, _drv_info_, ("+StopTxBeacon\n"));

	pHalData->RegFwHwTxQCtrl &= ~BIT(6);
	rtw_write8(padapter, REG_FWHW_TXQ_CTRL + 2, pHalData->RegFwHwTxQCtrl);
	rtw_write8(padapter, REG_TBTT_PROHIBIT + 1, 0x64);
	pHalData->RegReg542 &= ~BIT(0);
	rtw_write8(padapter, REG_TBTT_PROHIBIT + 2, pHalData->RegReg542);

	CheckFwRsvdPageContent(padapter);  /* 2010.06.23. Added by tynli. */
}

static void _BeaconFunctionEnable(PADAPTER padapter, u8 Enable, u8 Linked)
{
	rtw_write8(padapter, REG_BCN_CTRL, DIS_TSF_UDT | EN_BCN_FUNCTION | DIS_BCNQ_SUB);
	rtw_write8(padapter, REG_RD_CTRL + 1, 0x6F);
}

void rtl8188f_SetBeaconRelatedRegisters(PADAPTER padapter)
{
	u8 val8;
	u32 value32;
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;
	u32 bcn_ctrl_reg;

	/*reset TSF, enable update TSF, correcting TSF On Beacon */

	/*REG_BCN_INTERVAL */
	/*REG_BCNDMATIM */
	/*REG_ATIMWND */
	/*REG_TBTT_PROHIBIT */
	/*REG_DRVERLYINT */
	/*REG_BCN_MAX_ERR */
	/*REG_BCNTCFG //(0x510) */
	/*REG_DUAL_TSF_RST */
	/*REG_BCN_CTRL //(0x550) */


	bcn_ctrl_reg = REG_BCN_CTRL;

	/* */
	/* ATIM window */
	/* */
	rtw_write16(padapter, REG_ATIMWND, 2);

	/* */
	/* Beacon interval (in unit of TU). */
	/* */
	rtw_write16(padapter, REG_BCN_INTERVAL, pmlmeinfo->bcn_interval);

	rtl8188f_InitBeaconParameters(padapter);

	rtw_write8(padapter, REG_SLOT, 0x09);

	/* */
	/* Reset TSF Timer to zero, added by Roger. 2008.06.24 */
	/* */
	value32 = rtw_read32(padapter, REG_TCR);
	value32 &= ~TSFRST;
	rtw_write32(padapter, REG_TCR, value32);

	value32 |= TSFRST;
	rtw_write32(padapter, REG_TCR, value32);

	/* NOTE: Fix test chip's bug (about contention windows's randomness) */
	if (check_fwstate(&padapter->mlmepriv, WIFI_ADHOC_STATE | WIFI_ADHOC_MASTER_STATE | WIFI_AP_STATE) == _TRUE) {
		rtw_write8(padapter, REG_RXTSF_OFFSET_CCK, 0x50);
		rtw_write8(padapter, REG_RXTSF_OFFSET_OFDM, 0x50);
	}

	_BeaconFunctionEnable(padapter, _TRUE, _TRUE);

	ResumeTxBeacon(padapter);
	val8 = rtw_read8(padapter, bcn_ctrl_reg);
	val8 |= DIS_BCNQ_SUB;
	rtw_write8(padapter, bcn_ctrl_reg, val8);
}

u8 rtl8188f_MRateIdxToARFRId(PADAPTER padapter, u8 rate_idx)
{
	u8 ret = 0;
	RT_RF_TYPE_DEF_E rftype = (RT_RF_TYPE_DEF_E)GET_RF_TYPE(padapter);
	switch (rate_idx) {

	case RATR_INX_WIRELESS_NGB:
		if (rftype == RF_1T1R)
			ret = 1;
		else
			ret = 0;
		break;

	case RATR_INX_WIRELESS_N:
	case RATR_INX_WIRELESS_NG:
		if (rftype == RF_1T1R)
			ret = 5;
		else
			ret = 4;
		break;

	case RATR_INX_WIRELESS_NB:
		if (rftype == RF_1T1R)
			ret = 3;
		else
			ret = 2;
		break;

	case RATR_INX_WIRELESS_GB:
		ret = 6;
		break;

	case RATR_INX_WIRELESS_G:
		ret = 7;
		break;

	case RATR_INX_WIRELESS_B:
		ret = 8;
		break;

	case RATR_INX_WIRELESS_MC:
		if (padapter->mlmeextpriv.cur_wireless_mode & WIRELESS_11BG_24N)
			ret = 6;
		else
			ret = 7;
		break;
	case RATR_INX_WIRELESS_AC_N:
		if (rftype == RF_1T1R) /* || padapter->MgntInfo.VHTHighestOperaRate <= MGN_VHT1SS_MCS9) */
			ret = 10;
		else
			ret = 9;
		break;

	default:
		ret = 0;
		break;
	}

	return ret;
}

void UpdateHalRAMask8188F(PADAPTER padapter, u32 mac_id, u8 rssi_level)
{
	u32	mask, rate_bitmap;
	u8	shortGIrate = _FALSE;
	struct sta_info	*psta = NULL;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct macid_ctl_t *macid_ctl = &padapter->dvobj->macid_ctl;

	if (mac_id < macid_ctl->num)
		psta = macid_ctl->sta[mac_id];
	if (psta == NULL) {
		DBG_871X_LEVEL(_drv_always_, FUNC_ADPT_FMT" macid:%u, sta is NULL\n"
			, FUNC_ADPT_ARG(padapter), mac_id);
		return;
	}

	shortGIrate = query_ra_short_GI(psta);

	mask = psta->ra_mask;

	rate_bitmap = 0xffffffff;
	rate_bitmap = ODM_Get_Rate_Bitmap(&pHalData->odmpriv, mac_id, mask, rssi_level);
	DBG_871X("%s => mac_id:%d, networkType:0x%02x, mask:0x%08x\n\t ==> rssi_level:%d, rate_bitmap:0x%08x\n",
			 __func__, mac_id, psta->wireless_mode, mask, rssi_level, rate_bitmap);

	mask &= rate_bitmap;


	if (pHalData->fw_ractrl == _TRUE)
		rtl8188f_set_FwMacIdConfig_cmd(padapter, mac_id, psta->raid, psta->bw_mode, shortGIrate, mask);

	/*set correct initial date rate for each mac_id */
	pHalData->INIDATA_RATE[mac_id] = psta->init_rate;
	DBG_871X("%s(): mac_id=%d raid=0x%x bw=%d mask=0x%x init_rate=0x%x\n", __func__, mac_id, psta->raid, psta->bw_mode, mask, psta->init_rate);
}

/* */
/* Description: In normal chip, we should send some packet to Hw which will be used by Fw */
/*			in FW LPS mode. The function is to fill the Tx descriptor of this packets, then */
/*			Fw can tell Hw to send these packet derectly. */
/* Added by tynli. 2009.10.15. */
/* */
/*type1:pspoll, type2:null */
void rtl8188f_fill_fake_txdesc(
	PADAPTER	padapter,
	u8			*pDesc,
	u32			BufferLen,
	u8			IsPsPoll)
{
 	/* Clear all status */
	_rtw_memset(pDesc, 0, TXDESC_SIZE);

	SET_TX_DESC_FIRST_SEG_8188F(pDesc, 1); /*bFirstSeg; */
	SET_TX_DESC_LAST_SEG_8188F(pDesc, 1); /*bLastSeg; */

	SET_TX_DESC_OFFSET_8188F(pDesc, 0x28); /* Offset = 32 */

	SET_TX_DESC_PKT_SIZE_8188F(pDesc, BufferLen); /* Buffer size + command header */
	SET_TX_DESC_QUEUE_SEL_8188F(pDesc, QSLT_MGNT); /* Fixed queue of Mgnt queue */

	/* Set NAVUSEHDR to prevent Ps-poll AId filed to be changed to error vlaue by Hw. */
	if (_TRUE == IsPsPoll) {
		/* Nothing */
		SET_TX_DESC_NAV_USE_HDR_8188F(pDesc, 1);
	} else {
		SET_TX_DESC_HWSEQ_EN_8188F(pDesc, 1); /* Hw set sequence number */
		SET_TX_DESC_HWSEQ_SEL_8188F(pDesc, 0);
	}

	SET_TX_DESC_USE_RATE_8188F(pDesc, 1); /* use data rate which is set by Sw */
	SET_TX_DESC_OWN_8188F((pu1Byte)pDesc, 1);

	SET_TX_DESC_TX_RATE_8188F(pDesc, DESC8188F_RATE1M);

#if defined(CONFIG_USB_HCI)
	/* USB interface drop packet if the checksum of descriptor isn't correct. */
	/* Using this checksum can let hardware recovery from packet bulk out error (e.g. Cancel URC, Bulk out error.). */
	rtl8188f_cal_txdesc_chksum((struct tx_desc *)pDesc);
#endif
}

void init_hal_spec_8188f(_adapter *adapter)
{
	struct hal_spec_t *hal_spec = GET_HAL_SPEC(adapter);

	hal_spec->ic_name = "rtl8188f";
	hal_spec->macid_num = MACID_NUM_8188F;
	hal_spec->sec_cam_ent_num = SEC_CAM_ENT_NUM_8188F;
	hal_spec->sec_cap = 0;
	hal_spec->nss_num = NSS_NUM_8188F;
	hal_spec->band_cap = BAND_CAP_8188F;
	hal_spec->bw_cap = BW_CAP_8188F;
	hal_spec->proto_cap = PROTO_CAP_8188F;

	hal_spec->wl_func = 0
						| WL_FUNC_P2P
						| WL_FUNC_MIRACAST
						| WL_FUNC_TDLS
						;
}

void rtl8188f_init_default_value(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData;
	u8 i;
	pHalData = GET_HAL_DATA(padapter);

	/* init default value */
	pHalData->fw_ractrl = _FALSE;
	if (!adapter_to_pwrctl(padapter)->bkeepfwalive)
		pHalData->LastHMEBoxNum = 0;

	padapter->registrypriv.wireless_mode = WIRELESS_11BG_24N;

	/*init phydm default value */
	pHalData->iqk_initialized = _FALSE;
	pHalData->odmpriv.RFCalibrateInfo.TM_Trigger = 0;/*for IQK */
	pHalData->odmpriv.RFCalibrateInfo.ThermalValue_HP_index = 0;
	for (i = 0; i < HP_THERMAL_NUM; i++)
		pHalData->odmpriv.RFCalibrateInfo.ThermalValue_HP[i] = 0;

#if defined(CONFIG_USB_HCI)
	pHalData->IntrMask[0] = (u32)(
								/* IMR_ROK				| */
								/* IMR_RDU				| */
								/* IMR_VODOK			| */
								/* IMR_VIDOK			| */
								/* IMR_BEDOK			| */
								/* IMR_BKDOK			| */
								/* IMR_MGNTDOK			| */
								/* IMR_HIGHDOK			| */
								/* IMR_CPWM				| */
								/* IMR_CPWM2			| */
								/* IMR_C2HCMD			| */
								/* IMR_HISR1_IND_INT	| */
								/* IMR_ATIMEND			| */
								/* IMR_BCNDMAINT_E		| */
								/* IMR_HSISR_IND_ON_INT	| */
								/* IMR_BCNDOK0			| */
								/* IMR_BCNDMAINT0		| */
								/* IMR_TSF_BIT32_TOGGLE	| */
								/* IMR_TXBCN0OK			| */
								/* IMR_TXBCN0ERR		| */
								/* IMR_GTINT3			| */
								/* IMR_GTINT4			| */
								/* IMR_TXCCK			| */
								0);

	pHalData->IntrMask[1] = (u32)(
								/* IMR_RXFOVW			| */
								/* IMR_TXFOVW			| */
								/* IMR_RXERR			| */
								/* IMR_TXERR			| */
								/* IMR_ATIMEND_E		| */
								/* IMR_BCNDOK1			| */
								/* IMR_BCNDOK2			| */
								/* IMR_BCNDOK3			| */
								/* IMR_BCNDOK4			| */
								/* IMR_BCNDOK5			| */
								/* IMR_BCNDOK6			| */
								/* IMR_BCNDOK7			| */
								/* IMR_BCNDMAINT1		| */
								/* IMR_BCNDMAINT2		| */
								/* IMR_BCNDMAINT3		| */
								/* IMR_BCNDMAINT4		| */
								/* IMR_BCNDMAINT5		| */
								/* IMR_BCNDMAINT6		| */
								/* IMR_BCNDMAINT7		| */
								0);
#endif

	/* init Efuse variables */
	pHalData->EfuseUsedBytes = 0;
	pHalData->EfuseUsedPercentage = 0;
}

u8 GetEEPROMSize8188F(PADAPTER padapter)
{
	u8 size = 0;
	u32	cr;

	cr = rtw_read16(padapter, REG_9346CR);
	/* 6: EEPROM used is 93C46, 4: boot from E-Fuse. */
	size = (cr & BOOT_FROM_EEPROM) ? 6 : 4;

	MSG_8192C("EEPROM type is %s\n", size == 4 ? "E-FUSE" : "93C46");

	return size;
}

/*------------------------------------------------------------------------- */
/* */
/* LLT R/W/Init function */
/* */
/*------------------------------------------------------------------------- */
s32 _rtl8188fu_llt_table_init(PADAPTER padapter)
{
	u32 start, passing_time;
	u32 val32;
	s32 ret;


	ret = _FAIL;

	val32 = rtw_read32(padapter, REG_AUTO_LLT);
	val32 |= BIT_AUTO_INIT_LLT;
	rtw_write32(padapter, REG_AUTO_LLT, val32);

	start = rtw_get_current_time();

	do {
		val32 = rtw_read32(padapter, REG_AUTO_LLT);
		if (!(val32 & BIT_AUTO_INIT_LLT)) {
			ret = _SUCCESS;
			break;
		}

		passing_time = rtw_get_passing_time_ms(start);
		if (passing_time > 1000) {
			DBG_8192C("%s: FAIL!! REG_AUTO_LLT(0x%X)=%08x\n",
					  __func__, REG_AUTO_LLT, val32);
			break;
		}

		rtw_usleep_os(2);
	} while (1);

	return ret;
}

static int _rtl8188fu_get_chnl_group(u8 channel)
{
	int chnlgroup;

	if (1 <= channel && channel <= 2)
		chnlgroup = 0;
	else if (3  <= channel && channel <= 5)
		chnlgroup = 1;
	else if (6  <= channel && channel <= 8)
		chnlgroup = 2;
	else if (9  <= channel && channel <= 11)
		chnlgroup = 3;
	else if (12 <= channel && channel <= 14)
		chnlgroup = 4;
	else
		RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("==>Hal_GetChnlGroup8188F in 2.4 G, but Channel %d in Group not found\n", Channel));

	RT_TRACE(_module_hci_hal_init_c_, _drv_info_, ("<==Hal_GetChnlGroup8188F, Channel = %d, Group =%d,\n",
			 channel, chnlgroup));

	return chnlgroup;
}

void
Hal_InitPGData(
	PADAPTER	padapter,
	u8			*PROMContent)
{

	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u32			i;
	u16			value16;

	if (_FALSE == pHalData->bautoload_fail_flag) {
		/* autoload OK. */
		/*if (IS_BOOT_FROM_EEPROM(padapter)) */
		if (_TRUE == pHalData->EepromOrEfuse) {
			/* Read all Content from EEPROM or EFUSE. */
			for (i = 0; i < HWSET_MAX_SIZE_8188F; i += 2) {
				/*value16 = EF2Byte(ReadEEprom(pAdapter, (u2Byte) (i>>1))); */
				/**((u16*)(&PROMContent[i])) = value16; */
			}
		} else {
			/* Read EFUSE real map to shadow. */
			EFUSE_ShadowMapUpdate(padapter);
			_rtw_memcpy((void *)PROMContent, (void *)pHalData->efuse_eeprom_data, HWSET_MAX_SIZE_8188F);
		}
	} else {
		/*autoload fail */
		RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("AutoLoad Fail reported from CR9346!!\n"));
		/*pHalData->AutoloadFailFlag = _TRUE; */
		/*update to default value 0xFF */
		if (_FALSE == pHalData->EepromOrEfuse)
			EFUSE_ShadowMapUpdate(padapter);
		_rtw_memcpy((void *)PROMContent, (void *)pHalData->efuse_eeprom_data, HWSET_MAX_SIZE_8188F);
	}

}

void
Hal_EfuseParseIDCode(
	IN	PADAPTER	padapter,
	IN	u8			*hwinfo
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u16			EEPROMId;


	/* Checl 0x8129 again for making sure autoload status!! */
	EEPROMId = le16_to_cpu(*((u16 *)hwinfo));
	if (EEPROMId != RTL_EEPROM_ID) {
		DBG_8192C("EEPROM ID(%#x) is invalid!!\n", EEPROMId);
		pHalData->bautoload_fail_flag = _TRUE;
	} else
		pHalData->bautoload_fail_flag = _FALSE;

	RT_TRACE(_module_hal_init_c_, _drv_notice_, ("EEPROM ID=0x%04x\n", EEPROMId));
}

static void
Hal_EEValueCheck(
	IN		u8		EEType,
	IN		PVOID		pInValue,
	OUT		PVOID		pOutValue
)
{
	switch (EEType) {
	case EETYPE_TX_PWR: {
		u8	*pIn, *pOut;

		pIn = (u8 *)pInValue;
		pOut = (u8 *)pOutValue;

		if (*pIn <= 63)
			*pOut = *pIn;
		else {
			RT_TRACE(_module_hci_hal_init_c_, _drv_err_, ("EETYPE_TX_PWR, value=%d is invalid, set to default=0x%x\n",
					 *pIn, EEPROM_Default_TxPowerLevel));
			*pOut = EEPROM_Default_TxPowerLevel;
		}
	}
	break;
	default:
		break;
	}
}

void
_rtl8188fu_read_power_value_fromprom(
	IN	PADAPTER 		Adapter,
	IN	PTxPowerInfo24G	pwrInfo24G,
	IN	u8				 *PROMContent,
	IN	BOOLEAN			AutoLoadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u4Byte rfPath, eeAddr = EEPROM_TX_PWR_INX_8188F, group, TxCount = 0;

	_rtw_memset(pwrInfo24G, 0, sizeof(TxPowerInfo24G));

	if (0xFF == PROMContent[eeAddr + 1])
		AutoLoadFail = TRUE;

	if (AutoLoadFail) {
		DBG_871X("%s(): Use Default value!\n", __func__);
		for (rfPath = 0; rfPath < MAX_RF_PATH; rfPath++) {
			/*2.4G default value */
			for (group = 0; group < MAX_CHNL_GROUP_24G; group++) {
				pwrInfo24G->index_cck_base[rfPath][group] =	EEPROM_DEFAULT_24G_INDEX;
				pwrInfo24G->index_bw40_base[rfPath][group] =	EEPROM_DEFAULT_24G_INDEX;
			}
			for (TxCount = 0; TxCount < MAX_TX_COUNT; TxCount++) {
				if (TxCount == 0) {
					pwrInfo24G->bw20_diff[rfPath][0] =	EEPROM_DEFAULT_24G_HT20_DIFF;
					pwrInfo24G->ofdm_diff[rfPath][0] =	EEPROM_DEFAULT_24G_OFDM_DIFF;
				} else {
					pwrInfo24G->bw20_diff[rfPath][TxCount] =	EEPROM_DEFAULT_DIFF;
					pwrInfo24G->bw40_diff[rfPath][TxCount] =	EEPROM_DEFAULT_DIFF;
					pwrInfo24G->cck_diff[rfPath][TxCount] =	EEPROM_DEFAULT_DIFF;
					pwrInfo24G->ofdm_diff[rfPath][TxCount] =	EEPROM_DEFAULT_DIFF;
				}
			}
		}

		return;
	}

	pHalData->bTXPowerDataReadFromEEPORM = TRUE;		/*YJ,move,120316 */

	for (rfPath = 0; rfPath < MAX_RF_PATH; rfPath++) {
		/*2 2.4G default value */
		for (group = 0; group < MAX_CHNL_GROUP_24G; group++) {
			pwrInfo24G->index_cck_base[rfPath][group] =	PROMContent[eeAddr++];
			if (pwrInfo24G->index_cck_base[rfPath][group] == 0xFF)
				pwrInfo24G->index_cck_base[rfPath][group] = EEPROM_DEFAULT_24G_INDEX;
		}
		for (group = 0; group < MAX_CHNL_GROUP_24G - 1; group++) {
			pwrInfo24G->index_bw40_base[rfPath][group] =	PROMContent[eeAddr++];
			if (pwrInfo24G->index_bw40_base[rfPath][group] == 0xFF)
				pwrInfo24G->index_bw40_base[rfPath][group] =	EEPROM_DEFAULT_24G_INDEX;
		}
		for (TxCount = 0; TxCount < MAX_TX_COUNT; TxCount++) {
			if (TxCount == 0) {
				pwrInfo24G->bw40_diff[rfPath][TxCount] = 0;
				pwrInfo24G->bw20_diff[rfPath][TxCount] = (PROMContent[eeAddr] & 0xf0) >> 4;
				if (pwrInfo24G->bw20_diff[rfPath][TxCount] & BIT3)		/*4bit sign number to 8 bit sign number*/
					pwrInfo24G->bw20_diff[rfPath][TxCount] |= 0xF0;

				pwrInfo24G->ofdm_diff[rfPath][TxCount] = (PROMContent[eeAddr] & 0x0f);
				if (pwrInfo24G->ofdm_diff[rfPath][TxCount] & BIT3)		/*4bit sign number to 8 bit sign number*/
					pwrInfo24G->ofdm_diff[rfPath][TxCount] |= 0xF0;

				pwrInfo24G->cck_diff[rfPath][TxCount] = 0;
				eeAddr++;
			} else {
				pwrInfo24G->bw40_diff[rfPath][TxCount] =	(PROMContent[eeAddr] & 0xf0) >> 4;
				if (pwrInfo24G->bw40_diff[rfPath][TxCount] & BIT3)		/*4bit sign number to 8 bit sign number*/
					pwrInfo24G->bw40_diff[rfPath][TxCount] |= 0xF0;

				pwrInfo24G->bw20_diff[rfPath][TxCount] =	(PROMContent[eeAddr] & 0x0f);
				if (pwrInfo24G->bw20_diff[rfPath][TxCount] & BIT3)		/*4bit sign number to 8 bit sign number*/
					pwrInfo24G->bw20_diff[rfPath][TxCount] |= 0xF0;

				eeAddr++;

				pwrInfo24G->ofdm_diff[rfPath][TxCount] =	(PROMContent[eeAddr] & 0xf0) >> 4;
				if (pwrInfo24G->ofdm_diff[rfPath][TxCount] & BIT3)		/*4bit sign number to 8 bit sign number*/
					pwrInfo24G->ofdm_diff[rfPath][TxCount] |= 0xF0;


				pwrInfo24G->cck_diff[rfPath][TxCount] =	(PROMContent[eeAddr] & 0x0f);
				if (pwrInfo24G->cck_diff[rfPath][TxCount] & BIT3)		/*4bit sign number to 8 bit sign number*/
					pwrInfo24G->cck_diff[rfPath][TxCount] |= 0xF0;

				eeAddr++;
			}
		}

		/* Ignore the unnecessary 5G parameters parsing, but still consider the efuse address offset */
#define	TX_PWR_DIFF_OFFSET_5G	10
		eeAddr += (MAX_CHNL_GROUP_5G + TX_PWR_DIFF_OFFSET_5G);
	}
}


void _rtl8188fu_read_txpower_info_from_hwpg(
	IN	PADAPTER 		padapter,
	IN	u8			*PROMContent,
	IN	BOOLEAN			AutoLoadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	TxPowerInfo24G	pwrInfo24G;
	u8			rfPath, ch, group, TxCount = 1;

	/*RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("%s(): AutoLoadFail = %d\n", __func__, AutoLoadFail)); */
	_rtl8188fu_read_power_value_fromprom(padapter, &pwrInfo24G, PROMContent, AutoLoadFail);
	for (rfPath = 0; rfPath < MAX_RF_PATH; rfPath++) {
		for (ch = 0; ch < CHANNEL_MAX_NUMBER_2G; ch++) {
			group = _rtl8188fu_get_chnl_group(ch + 1);

			if (ch == 14 - 1) {
				pHalData->txpwr_cckdiff[rfPath][ch] = pwrInfo24G.index_cck_base[rfPath][5];
				pHalData->txpwr_ht40diff[rfPath][ch] = pwrInfo24G.index_bw40_base[rfPath][group];
			} else {
				pHalData->txpwr_cckdiff[rfPath][ch] = pwrInfo24G.index_cck_base[rfPath][group];
				pHalData->txpwr_ht40diff[rfPath][ch] = pwrInfo24G.index_bw40_base[rfPath][group];
			}
		}

		for (TxCount = 0; TxCount < MAX_TX_COUNT; TxCount++) {
			pHalData->txpwr_cckdiff[rfPath][TxCount] = pwrInfo24G.cck_diff[rfPath][TxCount];
			pHalData->txpwr_legacyhtdiff[rfPath][TxCount] = pwrInfo24G.ofdm_diff[rfPath][TxCount];
			pHalData->txpwr_ht20diff[rfPath][TxCount] = pwrInfo24G.bw20_diff[rfPath][TxCount];
			pHalData->txpwr_ht40diff[rfPath][TxCount] = pwrInfo24G.bw40_diff[rfPath][TxCount];
		}
	}

	/* 2010/10/19 MH Add Regulator recognize for CU. */
	if (!AutoLoadFail) {
		pHalData->eeprom_regulatory = (PROMContent[EEPROM_RF_BOARD_OPTION_8188F] & 0x7);	/*bit0~2 */
		if (PROMContent[EEPROM_RF_BOARD_OPTION_8188F] == 0xFF)
			pHalData->eeprom_regulatory = (EEPROM_DEFAULT_BOARD_OPTION & 0x7);	/*bit0~2 */
	} else
		pHalData->eeprom_regulatory = 0;
	RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("EEPROMRegulatory = 0x%x\n", pHalData->EEPROMRegulatory));
}

VOID
Hal_EfuseParseEEPROMVer_8188F(
	IN	PADAPTER		padapter,
	IN	u8			*hwinfo,
	IN	BOOLEAN			AutoLoadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	/*RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("%s(): AutoLoadFail = %d\n", __func__, AutoLoadFail)); */
	if (!AutoLoadFail)
		pHalData->EEPROMVersion = hwinfo[EEPROM_VERSION_8188F];
	else
		pHalData->EEPROMVersion = 1;
	RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("Hal_EfuseParseEEPROMVer(), EEVer = %d\n",
			 pHalData->EEPROMVersion));
}

#if 0 /* Do not need for rtl8188f */
VOID
Hal_EfuseParseVoltage_8188F(
	IN	PADAPTER		pAdapter,
	IN	u8			*hwinfo,
	IN	BOOLEAN 	AutoLoadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

	/*_rtw_memcpy(pEEPROM->adjuseVoltageVal, &hwinfo[EEPROM_Voltage_ADDR_8188F], 1); */
	DBG_871X("%s hwinfo[EEPROM_Voltage_ADDR_8188F] =%02x\n", __func__, hwinfo[EEPROM_Voltage_ADDR_8188F]);
	pHalData->adjuseVoltageVal = (hwinfo[EEPROM_Voltage_ADDR_8188F] & 0xf0) >> 4;
	DBG_871X("%s pHalData->adjuseVoltageVal =%x\n", __func__, pHalData->adjuseVoltageVal);
}
#endif

VOID
Hal_EfuseParseChnlPlan_8188F(
	IN	PADAPTER		padapter,
	IN	u8			*hwinfo,
	IN	BOOLEAN			AutoLoadFail
)
{
	padapter->mlmepriv.ChannelPlan = hal_com_config_channel_plan(
		padapter
		, hwinfo ? &hwinfo[EEPROM_COUNTRY_CODE_8188F] : NULL
		, hwinfo ? hwinfo[EEPROM_ChannelPlan_8188F] : 0xFF
		, padapter->registrypriv.alpha2
		, padapter->registrypriv.channel_plan
		, RTW_CHPLAN_WORLD_NULL
		, AutoLoadFail
	);
}

VOID
Hal_EfuseParseCustomerID_8188F(
	IN	PADAPTER		padapter,
	IN	u8			*hwinfo,
	IN	BOOLEAN			AutoLoadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	/*RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("%s(): AutoLoadFail = %d\n", __func__, AutoLoadFail)); */
	if (!AutoLoadFail)
		pHalData->EEPROMCustomerID = hwinfo[EEPROM_CustomID_8188F];
	else
		pHalData->EEPROMCustomerID = 0;
	RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("EEPROM Customer ID: 0x%2x\n", pHalData->EEPROMCustomerID));
}

VOID
Hal_EfuseParsePowerSavingMode_8188F(
	IN	PADAPTER		padapter,
	IN	u8				 *hwinfo,
	IN	BOOLEAN			AutoLoadFail
)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct pwrctrl_priv *pwrctl = adapter_to_pwrctl(padapter);
	u8 tmpvalue;

	if (AutoLoadFail) {
		pwrctl->bHWPowerdown = _FALSE;
		pwrctl->bSupportRemoteWakeup = _FALSE;
	} else	{

		/* hw power down mode selection , 0:rf-off ; 1:power down */

		if (padapter->registrypriv.hwpdn_mode == 2)
			pwrctl->bHWPowerdown = (hwinfo[EEPROM_FEATURE_OPTION_8188F] & BIT4);
		else
			pwrctl->bHWPowerdown = padapter->registrypriv.hwpdn_mode;

		/* decide hw if support remote wakeup function */
		/* if hw supported, 8051 (SIE) will generate WeakUP signal( D+/D- toggle) when autoresume */
#ifdef CONFIG_USB_HCI
		pwrctl->bSupportRemoteWakeup = (hwinfo[EEPROM_USB_OPTIONAL_FUNCTION0_8188FU] & BIT1) ? _TRUE : _FALSE;
#endif /* CONFIG_USB_HCI */

		DBG_871X("%s...bHWPwrPindetect(%x)-bHWPowerdown(%x) ,bSupportRemoteWakeup(%x)\n"
				 , __FUNCTION__, pwrctl->bHWPwrPindetect, pwrctl->bHWPowerdown, pwrctl->bSupportRemoteWakeup);

		DBG_871X("### PS params=>  power_mgnt(%x),usbss_enable(%x) ###\n"
				 , padapter->registrypriv.power_mgnt, padapter->registrypriv.usbss_enable);

	}
}

VOID
Hal_EfuseParseAntennaDiversity_8188F(
	IN	PADAPTER		pAdapter,
	IN	u8				 *hwinfo,
	IN	BOOLEAN			AutoLoadFail
)
{
}

VOID
Hal_EfuseParseXtal_8188F(
	IN	PADAPTER		pAdapter,
	IN	u8			 *hwinfo,
	IN	BOOLEAN		AutoLoadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

	/*RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("%s(): AutoLoadFail = %d\n", __func__, AutoLoadFail)); */
	if (!AutoLoadFail) {
		pHalData->crystalcap = hwinfo[EEPROM_XTAL_8188F];
		if (pHalData->crystalcap == 0xFF)
			pHalData->crystalcap = EEPROM_Default_CrystalCap_8188F;	   /*what value should 8812 set? */
	} else
		pHalData->crystalcap = EEPROM_Default_CrystalCap_8188F;
	RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("EEPROM CrystalCap: 0x%2x\n", pHalData->CrystalCap));
}


void
Hal_EfuseParseThermalMeter_8188F(
	PADAPTER	padapter,
	u8			*PROMContent,
	u8			AutoLoadFail
)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);

	/*RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("%s(): AutoLoadFail = %d\n", __func__, AutoLoadFail)); */
	/* */
	/* ThermalMeter from EEPROM */
	/* */
	if (_FALSE == AutoLoadFail)
		pHalData->EEPROMThermalMeter = PROMContent[EEPROM_THERMAL_METER_8188F];
	else
		pHalData->EEPROMThermalMeter = EEPROM_Default_ThermalMeter_8188F;

	if ((pHalData->EEPROMThermalMeter == 0xff) || (_TRUE == AutoLoadFail)) {
		pHalData->odmpriv.RFCalibrateInfo.bAPKThermalMeterIgnore = _TRUE;
		pHalData->EEPROMThermalMeter = EEPROM_Default_ThermalMeter_8188F;
	}

	RT_TRACE(_module_hci_hal_init_c_, _drv_notice_, ("EEPROM ThermalMeter=0x%x\n", pHalData->EEPROMThermalMeter));
}


void Hal_EfuseParseKFreeData_8188F(
	IN		PADAPTER		Adapter,
	IN		u8				*PROMContent,
	IN		BOOLEAN 		AutoloadFail)
{
#ifdef CONFIG_RF_POWER_TRIM
#define THERMAL_K_MEAN_OFFSET_8188F 5 /* 8188F FT thermal K mean value has +5 offset, it's special case */

	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct kfree_data_t *kfree_data = &pHalData->kfree_data;
	u8 pg_pwrtrim = 0xFF, pg_therm = 0xFF;

	pg_pwrtrim = EFUSE_Read1Byte(Adapter, PPG_BB_GAIN_2G_TXA_OFFSET_8188F);
	pg_therm = EFUSE_Read1Byte(Adapter, PPG_THERMAL_OFFSET_8188F);

	if (pg_pwrtrim != 0xFF) {
		kfree_data->bb_gain[BB_GAIN_2G][RF_PATH_A]
			= KFREE_BB_GAIN_2G_TX_OFFSET(pg_pwrtrim & PPG_BB_GAIN_2G_TX_OFFSET_MASK);
		kfree_data->flag |= KFREE_FLAG_ON;
	}

	if (pg_therm != 0xFF) {
		kfree_data->thermal
			= KFREE_THERMAL_OFFSET(pg_therm  & PPG_THERMAL_OFFSET_MASK) - THERMAL_K_MEAN_OFFSET_8188F;
		if (GET_PG_KFREE_THERMAL_K_ON_8188F(PROMContent))
			kfree_data->flag |= KFREE_FLAG_THERMAL_K_ON;
	}

	if (kfree_data->flag & KFREE_FLAG_THERMAL_K_ON)
		pHalData->EEPROMThermalMeter -= kfree_data->thermal;

	DBG_871X("kfree Pwr Trim flag:%u\n", kfree_data->flag);
	if (kfree_data->flag & KFREE_FLAG_ON)
		DBG_871X("bb_gain:%d\n", kfree_data->bb_gain[BB_GAIN_2G][RF_PATH_A]);
	if (kfree_data->flag & KFREE_FLAG_THERMAL_K_ON)
		DBG_871X("thermal:%d\n", kfree_data->thermal);

#endif /*CONFIG_RF_POWER_TRIM */
}

u8
BWMapping_8188F(
	IN	PADAPTER		Adapter,
	IN	struct pkt_attrib	*pattrib
)
{
	u8	BWSettingOfDesc = 0;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

	/*DBG_871X("BWMapping pHalData->current_chan_bw %d, pattrib->bwmode %d\n",pHalData->current_chan_bw,pattrib->bwmode); */

	if (pHalData->current_chan_bw == CHANNEL_WIDTH_80) {
		if (pattrib->bwmode == CHANNEL_WIDTH_80)
			BWSettingOfDesc = 2;
		else if (pattrib->bwmode == CHANNEL_WIDTH_40)
			BWSettingOfDesc = 1;
		else
			BWSettingOfDesc = 0;
	} else if (pHalData->current_chan_bw == CHANNEL_WIDTH_40) {
		if ((pattrib->bwmode == CHANNEL_WIDTH_40) || (pattrib->bwmode == CHANNEL_WIDTH_80))
			BWSettingOfDesc = 1;
		else
			BWSettingOfDesc = 0;
	} else
		BWSettingOfDesc = 0;

	/*if(pTcb->bBTTxPacket) */
	/*	BWSettingOfDesc = 0; */

	return BWSettingOfDesc;
}

u8	SCMapping_8188F(PADAPTER Adapter, struct pkt_attrib *pattrib)
{
	u8	SCSettingOfDesc = 0;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

	/*DBG_871X("SCMapping: pHalData->current_chan_bw %d, pHalData->cur_80_prime_sc %d, pHalData->cur_40_prime_sc %d\n",pHalData->current_chan_bw,pHalData->cur_80_prime_sc,pHalData->cur_40_prime_sc); */

	if (pHalData->current_chan_bw == CHANNEL_WIDTH_80) {
		if (pattrib->bwmode == CHANNEL_WIDTH_80)
			SCSettingOfDesc = VHT_DATA_SC_DONOT_CARE;
		else if (pattrib->bwmode == CHANNEL_WIDTH_40) {
			if (pHalData->cur_80_prime_sc == HAL_PRIME_CHNL_OFFSET_LOWER)
				SCSettingOfDesc = VHT_DATA_SC_40_LOWER_OF_80MHZ;
			else if (pHalData->cur_80_prime_sc == HAL_PRIME_CHNL_OFFSET_UPPER)
				SCSettingOfDesc = VHT_DATA_SC_40_UPPER_OF_80MHZ;
			else
				DBG_871X("SCMapping: DONOT CARE Mode Setting\n");
		} else {
			if ((pHalData->cur_40_prime_sc == HAL_PRIME_CHNL_OFFSET_LOWER) && (pHalData->cur_80_prime_sc == HAL_PRIME_CHNL_OFFSET_LOWER))
				SCSettingOfDesc = VHT_DATA_SC_20_LOWEST_OF_80MHZ;
			else if ((pHalData->cur_40_prime_sc == HAL_PRIME_CHNL_OFFSET_UPPER) && (pHalData->cur_80_prime_sc == HAL_PRIME_CHNL_OFFSET_LOWER))
				SCSettingOfDesc = VHT_DATA_SC_20_LOWER_OF_80MHZ;
			else if ((pHalData->cur_40_prime_sc == HAL_PRIME_CHNL_OFFSET_LOWER) && (pHalData->cur_80_prime_sc == HAL_PRIME_CHNL_OFFSET_UPPER))
				SCSettingOfDesc = VHT_DATA_SC_20_UPPER_OF_80MHZ;
			else if ((pHalData->cur_40_prime_sc == HAL_PRIME_CHNL_OFFSET_UPPER) && (pHalData->cur_80_prime_sc == HAL_PRIME_CHNL_OFFSET_UPPER))
				SCSettingOfDesc = VHT_DATA_SC_20_UPPERST_OF_80MHZ;
			else
				DBG_871X("SCMapping: DONOT CARE Mode Setting\n");
		}
	} else if (pHalData->current_chan_bw == CHANNEL_WIDTH_40) {
		/*DBG_871X("SCMapping: HT Case: pHalData->current_chan_bw %d, pHalData->cur_40_prime_sc %d\n",pHalData->current_chan_bw,pHalData->cur_40_prime_sc); */

		if (pattrib->bwmode == CHANNEL_WIDTH_40)
			SCSettingOfDesc = VHT_DATA_SC_DONOT_CARE;
		else if (pattrib->bwmode == CHANNEL_WIDTH_20) {
			if (pHalData->cur_40_prime_sc == HAL_PRIME_CHNL_OFFSET_UPPER)
				SCSettingOfDesc = VHT_DATA_SC_20_UPPER_OF_80MHZ;
			else if (pHalData->cur_40_prime_sc == HAL_PRIME_CHNL_OFFSET_LOWER)
				SCSettingOfDesc = VHT_DATA_SC_20_LOWER_OF_80MHZ;
			else
				SCSettingOfDesc = VHT_DATA_SC_DONOT_CARE;
		}
	} else
		SCSettingOfDesc = VHT_DATA_SC_DONOT_CARE;

	return SCSettingOfDesc;
}


static u8 fill_txdesc_sectype(struct pkt_attrib *pattrib)
{
	u8 sectype = 0;
	if ((pattrib->encrypt > 0) && !pattrib->bswenc) {
		switch (pattrib->encrypt) {
		/* SEC_TYPE */
		case _WEP40_:
		case _WEP104_:
		case _TKIP_:
		case _TKIP_WTMIC_:
			sectype = 1;
			break;

#ifdef CONFIG_WAPI_SUPPORT
		case _SMS4_:
			sectype = 2;
			break;
#endif
		case _AES_:
			sectype = 3;
			break;

		case _NO_PRIVACY_:
		default:
			break;
		}
	}
	return sectype;
}

static void fill_txdesc_vcs_8188f(PADAPTER padapter, struct pkt_attrib *pattrib, u8 *ptxdesc)
{
	/*DBG_8192C("cvs_mode=%d\n", pattrib->vcs_mode); */

	SET_TX_DESC_HW_RTS_ENABLE_8188F(ptxdesc, 0);

	switch (pattrib->vcs_mode) {
	case RTS_CTS:
		SET_TX_DESC_RTS_ENABLE_8188F(ptxdesc, 1);
		break;
	case CTS_TO_SELF:
		SET_TX_DESC_CTS2SELF_8188F(ptxdesc, 1);
		break;
	case NONE_VCS:
	default:
		break;
	}

#if 1 /* TODO: */
	/* Protection mode related */
	if (pattrib->vcs_mode) {
		SET_TX_DESC_RTS_RATE_8188F(ptxdesc, 8); /*TODO: Hardcode?, RTS Rate=24M (8) */
		SET_TX_DESC_RTS_RATE_FB_LIMIT_8188F(ptxdesc, 0xF); /* TODO: PROTECTION_MODE ?? */

		if (padapter->mlmeextpriv.mlmext_info.preamble_mode == PREAMBLE_SHORT)
			SET_TX_DESC_RTS_SHORT_8188F(ptxdesc, 1);

		/* Set RTS BW */
		if (pattrib->ht_en)
			SET_TX_DESC_RTS_SC_8188F(ptxdesc, SCMapping_8188F(padapter, pattrib));
	}
#endif
}

static void fill_txdesc_phy_8188f(PADAPTER padapter, struct pkt_attrib *pattrib, u8 *ptxdesc)
{
	/*DBG_8192C("bwmode=%d, ch_off=%d\n", pattrib->bwmode, pattrib->ch_offset); */

	if (pattrib->ht_en) {
		SET_TX_DESC_DATA_BW_8188F(ptxdesc, BWMapping_8188F(padapter, pattrib));
		SET_TX_DESC_DATA_SC_8188F(ptxdesc, SCMapping_8188F(padapter, pattrib));
	}
}


void rtl8188f_update_txdesc(struct xmit_frame *pxmitframe, u8 *pbuf)
{
	PADAPTER padapter;
	HAL_DATA_TYPE *pHalData;
	struct mlme_ext_priv *pmlmeext;
	struct mlme_ext_info *pmlmeinfo;
	struct pkt_attrib *pattrib;
	s32 bmcst;

	_rtw_memset(pbuf, 0, TXDESC_SIZE);

	padapter = pxmitframe->padapter;
	pHalData = GET_HAL_DATA(padapter);
	pmlmeext = &padapter->mlmeextpriv;
	pmlmeinfo = &(pmlmeext->mlmext_info);

	pattrib = &pxmitframe->attrib;
	bmcst = IS_MCAST(pattrib->ra);

	if (pxmitframe->frame_tag == DATA_FRAMETAG) {
		u8 drv_userate = 0;

		SET_TX_DESC_MACID_8188F(pbuf, pattrib->mac_id);
		SET_TX_DESC_RATE_ID_8188F(pbuf, pattrib->raid);
		SET_TX_DESC_QUEUE_SEL_8188F(pbuf, pattrib->qsel);
		SET_TX_DESC_SEQ_8188F(pbuf, pattrib->seqnum);

		SET_TX_DESC_SEC_TYPE_8188F(pbuf, fill_txdesc_sectype(pattrib));
		fill_txdesc_vcs_8188f(padapter, pattrib, pbuf);

		if ((pattrib->ether_type != 0x888e) &&
			(pattrib->ether_type != 0x0806) &&
			(pattrib->ether_type != 0x88B4) &&
			(pattrib->dhcp_pkt != 1) &&
			(drv_userate != 1)
		   ) {
			/* Non EAP & ARP & DHCP type data packet */

			if (pattrib->ampdu_en == _TRUE) {
				SET_TX_DESC_AGG_ENABLE_8188F(pbuf, 1);
				SET_TX_DESC_MAX_AGG_NUM_8188F(pbuf, 0x1F);
				SET_TX_DESC_AMPDU_DENSITY_8188F(pbuf, pattrib->ampdu_spacing);
			} else
				SET_TX_DESC_AGG_BREAK_8188F(pbuf, 1);

			fill_txdesc_phy_8188f(padapter, pattrib, pbuf);

			SET_TX_DESC_DATA_RATE_FB_LIMIT_8188F(pbuf, 0x1F);

			if (pHalData->fw_ractrl == _FALSE) {
				SET_TX_DESC_USE_RATE_8188F(pbuf, 1);

				if (pHalData->INIDATA_RATE[pattrib->mac_id] & BIT(7))
					SET_TX_DESC_DATA_SHORT_8188F(pbuf, 1);

				SET_TX_DESC_TX_RATE_8188F(pbuf, pHalData->INIDATA_RATE[pattrib->mac_id] & 0x7F);
			}

			/* modify data rate by iwpriv */
			if (padapter->fix_rate != 0xFF) {
				SET_TX_DESC_USE_RATE_8188F(pbuf, 1);
				if (padapter->fix_rate & BIT(7))
					SET_TX_DESC_DATA_SHORT_8188F(pbuf, 1);
				SET_TX_DESC_TX_RATE_8188F(pbuf, padapter->fix_rate & 0x7F);
				if (!padapter->data_fb)
					SET_TX_DESC_DISABLE_FB_8188F(pbuf, 1);
			}

			if (pattrib->ldpc)
				SET_TX_DESC_DATA_LDPC_8188F(pbuf, 1);

			if (pattrib->stbc)
				SET_TX_DESC_DATA_STBC_8188F(pbuf, 1);

/* ULLI :we leave SET_TX_DESC_DATA_SHORT_8188F here */
#ifdef CONFIG_CMCC_TEST
			SET_TX_DESC_DATA_SHORT_8188F(pbuf, 1); /* use cck short premble */
#endif
		} else {
			/* EAP data packet and ARP packet. */
			/* Use the 1M data rate to send the EAP/ARP packet. */
			/* This will maybe make the handshake smooth. */

			SET_TX_DESC_AGG_BREAK_8188F(pbuf, 1);
			SET_TX_DESC_USE_RATE_8188F(pbuf, 1);
			if (pmlmeinfo->preamble_mode == PREAMBLE_SHORT)
				SET_TX_DESC_DATA_SHORT_8188F(pbuf, 1);
			SET_TX_DESC_TX_RATE_8188F(pbuf, MRateToHwRate(pmlmeext->tx_rate));

			DBG_8192C(FUNC_ADPT_FMT ": SP Packet(0x%04X) rate=0x%x\n",
					  FUNC_ADPT_ARG(padapter), pattrib->ether_type, MRateToHwRate(pmlmeext->tx_rate));
		}

#if defined(CONFIG_USB_TX_AGGREGATION)
		SET_TX_DESC_USB_TXAGG_NUM_8188F(pbuf, pxmitframe->agg_num);
#endif

	} else if (pxmitframe->frame_tag == MGNT_FRAMETAG) {
		/* RT_TRACE(_module_hal_xmit_c_, _drv_notice_, ("%s: MGNT_FRAMETAG\n", __func__)); */

		SET_TX_DESC_MACID_8188F(pbuf, pattrib->mac_id);
		SET_TX_DESC_QUEUE_SEL_8188F(pbuf, pattrib->qsel);
		SET_TX_DESC_RATE_ID_8188F(pbuf, pattrib->raid);
		SET_TX_DESC_SEQ_8188F(pbuf, pattrib->seqnum);
		SET_TX_DESC_USE_RATE_8188F(pbuf, 1);

		SET_TX_DESC_MBSSID_8188F(pbuf, pattrib->mbssid & 0xF);

		SET_TX_DESC_RETRY_LIMIT_ENABLE_8188F(pbuf, 1);
		if (pattrib->retry_ctrl == _TRUE) {
			/* Nothing */
			SET_TX_DESC_DATA_RETRY_LIMIT_8188F(pbuf, 6);
		} else {
			/* Nothing */
			SET_TX_DESC_DATA_RETRY_LIMIT_8188F(pbuf, 12);
		}

#ifdef CONFIG_INTEL_PROXIM
		if ((padapter->proximity.proxim_on == _TRUE) && (pattrib->intel_proxim == _TRUE)) {
			DBG_871X("\n %s pattrib->rate=%d\n", __func__, pattrib->rate);
			SET_TX_DESC_TX_RATE_8188F(pbuf, pattrib->rate);
		} else
#endif
		{
			SET_TX_DESC_TX_RATE_8188F(pbuf, MRateToHwRate(pmlmeext->tx_rate));
		}

	} else if (pxmitframe->frame_tag == TXAGG_FRAMETAG)
		RT_TRACE(_module_hal_xmit_c_, _drv_warning_, ("%s: TXAGG_FRAMETAG\n", __func__));
	else {
		RT_TRACE(_module_hal_xmit_c_, _drv_warning_, ("%s: frame_tag=0x%x\n", __func__, pxmitframe->frame_tag));

		SET_TX_DESC_MACID_8188F(pbuf, pattrib->mac_id);
		SET_TX_DESC_RATE_ID_8188F(pbuf, pattrib->raid);
		SET_TX_DESC_QUEUE_SEL_8188F(pbuf, pattrib->qsel);
		SET_TX_DESC_SEQ_8188F(pbuf, pattrib->seqnum);
		SET_TX_DESC_USE_RATE_8188F(pbuf, 1);
		SET_TX_DESC_TX_RATE_8188F(pbuf, MRateToHwRate(pmlmeext->tx_rate));
	}

	SET_TX_DESC_PKT_SIZE_8188F(pbuf, pattrib->last_txcmdsz);

	{
		u8 pkt_offset, offset;

		pkt_offset = 0;
		offset = TXDESC_SIZE;
#ifdef CONFIG_USB_HCI
		pkt_offset = pxmitframe->pkt_offset;
		offset += (pxmitframe->pkt_offset >> 3);
#endif /* CONFIG_USB_HCI */

		SET_TX_DESC_PKT_OFFSET_8188F(pbuf, pkt_offset);
		SET_TX_DESC_OFFSET_8188F(pbuf, offset);
	}

	if (bmcst)
		SET_TX_DESC_BMC_8188F(pbuf, 1);

	/* 2009.11.05. tynli_test. Suggested by SD4 Filen for FW LPS. */
	/* (1) The sequence number of each non-Qos frame / broadcast / multicast / */
	/* mgnt frame should be controlled by Hw because Fw will also send null data */
	/* which we cannot control when Fw LPS enable. */
	/* --> default enable non-Qos data sequense number. 2010.06.23. by tynli. */
	/* (2) Enable HW SEQ control for beacon packet, because we use Hw beacon. */
	/* (3) Use HW Qos SEQ to control the seq num of Ext port non-Qos packets. */
	/* 2010.06.23. Added by tynli. */
	if (!pattrib->qos_en)
		SET_TX_DESC_HWSEQ_EN_8188F(pbuf, 1);

#if defined(CONFIG_USB_HCI)
	rtl8188f_cal_txdesc_chksum((struct tx_desc *)pbuf);
#endif

}

/* ULLI : note about receive config, registers and values */

static void hw_var_set_monitor(PADAPTER Adapter, u8 variable, u8 *val)
{
	u32	value_rcr, rcr_bits;
	u16	value_rxfltmap2;
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv *pmlmepriv = &(Adapter->mlmepriv);

	if (*((u8 *)val) == _HW_STATE_MONITOR_) {

		/* Leave IPS */
		rtw_pm_set_ips(Adapter, IPS_NONE);
		LeaveAllPowerSaveMode(Adapter);

		/* Receive all type */
		rcr_bits = RCR_AAP | RCR_APM | RCR_AM | RCR_AB | RCR_APWRMGT | RCR_ADF | RCR_ACF | RCR_AMF | RCR_APP_PHYST_RXFF;

		/* Append FCS */
		rcr_bits |= RCR_APPFCS;

#if 0
		/*
		   CRC and ICV packet will drop in recvbuf2recvframe()
		   We no turn on it.
		 */
		rcr_bits |= (RCR_ACRC32 | RCR_AICV);
#endif

		/* Receive all data frames */
		value_rxfltmap2 = 0xFFFF;

		value_rcr = rcr_bits;
		rtw_write32(Adapter, REG_RCR, value_rcr);

		rtw_write16(Adapter, REG_RXFLTMAP2, value_rxfltmap2);

#if 0
		/* tx pause */
		rtw_write8(padapter, REG_TXPAUSE, 0xFF);
#endif
	} else {
		/* do nothing */
	}

}

static void hw_var_set_opmode(PADAPTER padapter, u8 variable, u8 *val)
{
	u8 val8;
	u8 mode = *((u8 *)val);
	static u8 isMonitor = _FALSE;

	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);

	if (isMonitor == _TRUE) {
		/* reset RCR */
		rtw_write32(padapter, REG_RCR, pHalData->ReceiveConfig);
		isMonitor = _FALSE;
	}

	if (mode == _HW_STATE_MONITOR_) {
		isMonitor = _TRUE;
		/* set net_type */
		Set_MSR(padapter, _HW_STATE_NOLINK_);

		hw_var_set_monitor(padapter, variable, val);
		return;
	}

	{
		/* disable Port0 TSF update */
		val8 = rtw_read8(padapter, REG_BCN_CTRL);
		val8 |= DIS_TSF_UDT;
		rtw_write8(padapter, REG_BCN_CTRL, val8);

		/* set net_type */
		Set_MSR(padapter, mode);
		DBG_871X("#### %s() -%d iface_type(0) mode = %d ####\n", __func__, __LINE__, mode);

		if ((mode == _HW_STATE_STATION_) || (mode == _HW_STATE_NOLINK_)) {
			{
				StopTxBeacon(padapter);
#ifdef CONFIG_INTERRUPT_BASED_TXBCN
#ifdef CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT
				rtw_write8(padapter, REG_DRVERLYINT, 0x05); /* restore early int time to 5ms */
				UpdateInterruptMask8812AU(padapter, _TRUE, 0, IMR_BCNDMAINT0_8188F);
#endif /* CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT */

#ifdef CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR
				UpdateInterruptMask8812AU(padapter, _TRUE , 0, (IMR_TXBCN0ERR_8188F | IMR_TXBCN0OK_8188F));
#endif /* CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR */

#endif /* CONFIG_INTERRUPT_BASED_TXBCN */
			}

			/* disable atim wnd */
			rtw_write8(padapter, REG_BCN_CTRL, DIS_TSF_UDT | EN_BCN_FUNCTION | DIS_ATIM);
			/*rtw_write8(padapter,REG_BCN_CTRL, 0x18); */
		} else if ((mode == _HW_STATE_ADHOC_) /*|| (mode == _HW_STATE_AP_)*/) {
			ResumeTxBeacon(padapter);
			rtw_write8(padapter, REG_BCN_CTRL, DIS_TSF_UDT | EN_BCN_FUNCTION | DIS_BCNQ_SUB);
		} else if (mode == _HW_STATE_AP_) {
#ifdef CONFIG_INTERRUPT_BASED_TXBCN
#ifdef CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT
			UpdateInterruptMask8188FU(padapter, _TRUE , IMR_BCNDMAINT0_8188F, 0);
#endif /* CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT */

#ifdef CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR
			UpdateInterruptMask8188FU(padapter, _TRUE , (IMR_TXBCN0ERR_8188F | IMR_TXBCN0OK_8188F), 0);
#endif /* CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR */

#endif /* CONFIG_INTERRUPT_BASED_TXBCN */

			ResumeTxBeacon(padapter);

			rtw_write8(padapter, REG_BCN_CTRL, DIS_TSF_UDT | DIS_BCNQ_SUB);

			/*Set RCR */
			/*rtw_write32(padapter, REG_RCR, 0x70002a8e);//CBSSID_DATA must set to 0 */
			/*rtw_write32(padapter, REG_RCR, 0x7000228e);//CBSSID_DATA must set to 0 */
			rtw_write32(padapter, REG_RCR, 0x7000208e);/*CBSSID_DATA must set to 0,reject ICV_ERR packet */
			/*enable to rx data frame */
			rtw_write16(padapter, REG_RXFLTMAP2, 0xFFFF);
			/*enable to rx ps-poll */
			rtw_write16(padapter, REG_RXFLTMAP1, 0x0400);

			/*Beacon Control related register for first time */
			rtw_write8(padapter, REG_BCNDMATIM, 0x02); /* 2ms */

			/*rtw_write8(padapter, REG_BCN_MAX_ERR, 0xFF); */
			rtw_write8(padapter, REG_ATIMWND, 0x0a); /* 10ms */
			rtw_write16(padapter, REG_BCNTCFG, 0x00);
			rtw_write16(padapter, REG_TBTT_PROHIBIT, 0xff04);
			rtw_write16(padapter, REG_TSFTR_SYN_OFFSET, 0x7fff);/* +32767 (~32ms) */

			/*reset TSF */
			rtw_write8(padapter, REG_DUAL_TSF_RST, BIT(0));

			/*enable BCN0 Function for if1 */
			/*don't enable update TSF0 for if1 (due to TSF update when beacon/probe rsp are received) */
			rtw_write8(padapter, REG_BCN_CTRL, (DIS_TSF_UDT | EN_BCN_FUNCTION | EN_TXBCN_RPT | DIS_BCNQ_SUB));

			/*SW_BCN_SEL - Port0 */
			/*rtw_write8(Adapter, REG_DWBCN1_CTRL_8192E+2, rtw_read8(Adapter, REG_DWBCN1_CTRL_8192E+2) & ~BIT4); */
			rtw_hal_set_hwreg(padapter, HW_VAR_DL_BCN_SEL, NULL);

			/* select BCN on port 0 */
			rtw_write8(padapter, REG_CCK_CHECK_8188F,
					   (rtw_read8(padapter, REG_CCK_CHECK_8188F) & ~BIT_BCN_PORT_SEL));

			/* dis BCN1 ATIM  WND if if2 is station */
			val8 = rtw_read8(padapter, REG_BCN_CTRL_1);
			val8 |= DIS_ATIM;
			rtw_write8(padapter, REG_BCN_CTRL_1, val8);
		}
	}
}

static void hw_var_set_macaddr(PADAPTER padapter, u8 variable, u8 *val)
{
	u8 idx = 0;
	u32 reg_macid;

	{
		reg_macid = REG_MACID;
	}

	for (idx = 0; idx < 6; idx++)
		rtw_write8(GET_PRIMARY_ADAPTER(padapter), (reg_macid + idx), val[idx]);
}

static void hw_var_set_bssid(PADAPTER padapter, u8 variable, u8 *val)
{
	u8	idx = 0;
	u32 reg_bssid;

	{
		reg_bssid = REG_BSSID;
	}

	for (idx = 0; idx < 6; idx++)
		rtw_write8(padapter, (reg_bssid + idx), val[idx]);
}

static void hw_var_set_bcn_func(PADAPTER padapter, u8 variable, u8 *val)
{
	u32 bcn_ctrl_reg;

	{
		bcn_ctrl_reg = REG_BCN_CTRL;
	}

	if (*(u8 *)val)
		rtw_write8(padapter, bcn_ctrl_reg, (EN_BCN_FUNCTION | EN_TXBCN_RPT));
	else {
		u8 val8;
		val8 = rtw_read8(padapter, bcn_ctrl_reg);
		val8 &= ~(EN_BCN_FUNCTION | EN_TXBCN_RPT);
		rtw_write8(padapter, bcn_ctrl_reg, val8);
	}
}

static void hw_var_set_correct_tsf(PADAPTER padapter, u8 variable, u8 *val)
{
	u8 val8;
	u64	tsf;
	struct mlme_ext_priv *pmlmeext;
	struct mlme_ext_info *pmlmeinfo;


	pmlmeext = &padapter->mlmeextpriv;
	pmlmeinfo = &pmlmeext->mlmext_info;

	tsf = pmlmeext->TSFValue - rtw_modular64(pmlmeext->TSFValue, (pmlmeinfo->bcn_interval * 1024)) - 1024; /*us */

	if (((pmlmeinfo->state & 0x03) == WIFI_FW_ADHOC_STATE) ||
		((pmlmeinfo->state & 0x03) == WIFI_FW_AP_STATE))
		StopTxBeacon(padapter);

	{
		/* disable related TSF function */
		val8 = rtw_read8(padapter, REG_BCN_CTRL);
		val8 &= ~EN_BCN_FUNCTION;
		rtw_write8(padapter, REG_BCN_CTRL, val8);

		rtw_write32(padapter, REG_TSFTR, tsf);
		rtw_write32(padapter, REG_TSFTR + 4, tsf >> 32);

		/* enable related TSF function */
		val8 = rtw_read8(padapter, REG_BCN_CTRL);
		val8 |= EN_BCN_FUNCTION;
		rtw_write8(padapter, REG_BCN_CTRL, val8);

	}

	if (((pmlmeinfo->state & 0x03) == WIFI_FW_ADHOC_STATE)
		|| ((pmlmeinfo->state & 0x03) == WIFI_FW_AP_STATE))
		ResumeTxBeacon(padapter);
}

static void hw_var_set_mlme_disconnect(PADAPTER padapter, u8 variable, u8 *val)
{
	u8 val8;

	{
		/* Set RCR to not to receive data frame when NO LINK state */
		/*rtw_write32(padapter, REG_RCR, rtw_read32(padapter, REG_RCR) & ~RCR_ADF); */
		/* reject all data frames */
		rtw_write16(padapter, REG_RXFLTMAP2, 0);
	}

	{
		/* reset TSF */
		rtw_write8(padapter, REG_DUAL_TSF_RST, BIT(0));

		/* disable update TSF */
		val8 = rtw_read8(padapter, REG_BCN_CTRL);
		val8 |= DIS_TSF_UDT;
		rtw_write8(padapter, REG_BCN_CTRL, val8);
	}
}

static void hw_var_set_mlme_sitesurvey(PADAPTER padapter, u8 variable, u8 *val)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	u32	value_rcr, rcr_clear_bit, reg_bcn_ctl;
	u16	value_rxfltmap2;
	u8 val8;
	PHAL_DATA_TYPE pHalData;
	struct mlme_priv *pmlmepriv;
	u8 ap_num;

	pHalData = GET_HAL_DATA(padapter);
	pmlmepriv = &padapter->mlmepriv;

	reg_bcn_ctl = REG_BCN_CTRL;

	rtw_dev_iface_status(padapter, NULL, NULL, NULL, &ap_num, NULL);

#ifdef CONFIG_FIND_BEST_CHANNEL
	rcr_clear_bit = (RCR_CBSSID_BCN | RCR_CBSSID_DATA);

	/* Receive all data frames */
	value_rxfltmap2 = 0xFFFF;
#else /* CONFIG_FIND_BEST_CHANNEL */

	rcr_clear_bit = RCR_CBSSID_BCN;

	/* config RCR to receive different BSSID & not to receive data frame */
	value_rxfltmap2 = 0;

#endif /* CONFIG_FIND_BEST_CHANNEL */

	if ((check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE)
	   )
		rcr_clear_bit = RCR_CBSSID_BCN;

	value_rcr = rtw_read32(padapter, REG_RCR);

	if (*((u8 *)val)) {
		/* under sitesurvey */
	     /*
		* 1. configure REG_RXFLTMAP2
		* 2. disable TSF update &  buddy TSF update to avoid updating wrong TSF due to clear RCR_CBSSID_BCN
		* 3. config RCR to receive different BSSID BCN or probe rsp
		*/

		rtw_write16(padapter, REG_RXFLTMAP2, value_rxfltmap2);

		if (rtw_linked_check(padapter) &&
			check_fwstate(pmlmepriv, WIFI_AP_STATE) != _TRUE) {
			/* disable update TSF */
			rtw_write8(padapter, reg_bcn_ctl, rtw_read8(padapter, reg_bcn_ctl)|DIS_TSF_UDT);
			padapter->mlmeextpriv.en_hw_update_tsf = _FALSE;
		}

		value_rcr &= ~(rcr_clear_bit);
		rtw_write32(padapter, REG_RCR, value_rcr);

		/* Save orignal RRSR setting. */
		pHalData->RegRRSR = rtw_read16(padapter, REG_RRSR);

		if (ap_num)
			StopTxBeacon(padapter);
	} else {
	     /*
		* 1. enable rx data frame
		* 2. config RCR not to receive different BSSID BCN or probe rsp
		* 3. doesn't enable TSF update &  buddy TSF right now to avoid HW conflict
		*	 so, we enable TSF update when rx first BCN after sitesurvey done
		*/

		/* sitesurvey done */
		if (check_fwstate(pmlmepriv, (_FW_LINKED | WIFI_AP_STATE))
		   ) {
			/* enable to rx data frame */
			rtw_write16(padapter, REG_RXFLTMAP2, 0xFFFF);
		}

		value_rcr |= rcr_clear_bit;
		rtw_write32(padapter, REG_RCR, value_rcr);

		if (rtw_linked_check(padapter) &&
			check_fwstate(pmlmepriv, WIFI_AP_STATE) != _TRUE)
			padapter->mlmeextpriv.en_hw_update_tsf = _TRUE;


		/* Restore orignal RRSR setting. */
		rtw_write16(padapter, REG_RRSR, pHalData->RegRRSR);

		if (ap_num) {
			int i;
			_adapter *iface;

			ResumeTxBeacon(padapter);
			for (i = 0; i < dvobj->iface_nums; i++) {
				iface = dvobj->padapters[i];
				if (!iface)
					continue;

				if (check_fwstate(&iface->mlmepriv, WIFI_AP_STATE) == _TRUE
					&& check_fwstate(&iface->mlmepriv, WIFI_ASOC_STATE) == _TRUE
				) {
					iface->mlmepriv.update_bcn = _TRUE;
					#ifndef CONFIG_INTERRUPT_BASED_TXBCN
					#if defined(CONFIG_USB_HCI)
					tx_beacon_hdl(iface, NULL);
					#endif
					#endif

				}
			}
		}
	}
}

static void hw_var_set_mlme_join(PADAPTER padapter, u8 variable, u8 *val)
{
	u8 val8;
	u16 val16;
	u32 val32;
	u8 RetryLimit;
	u8 type;
	PHAL_DATA_TYPE pHalData;
	struct mlme_priv *pmlmepriv;

	RetryLimit = 0x30;
	type = *(u8 *)val;
	pHalData = GET_HAL_DATA(padapter);
	pmlmepriv = &padapter->mlmepriv;

	if (type == 0) { /* prepare to join */
		/*enable to rx data frame.Accept all data frame */
		/*rtw_write32(padapter, REG_RCR, rtw_read32(padapter, REG_RCR)|RCR_ADF); */
		rtw_write16(padapter, REG_RXFLTMAP2, 0xFFFF);

		val32 = rtw_read32(padapter, REG_RCR);
		if (padapter->in_cta_test)
			val32 &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN);/*| RCR_ADF */
		else
			val32 |= RCR_CBSSID_DATA | RCR_CBSSID_BCN;
		rtw_write32(padapter, REG_RCR, val32);

		if (check_fwstate(pmlmepriv, WIFI_STATION_STATE) == _TRUE)
			RetryLimit = (pHalData->CustomerID == RT_CID_CCX) ? 7 : 48;
		else   /* Ad-hoc Mode */
			RetryLimit = 0x7;
	} else if (type == 1)   /*joinbss_event call back when join res < 0 */
		rtw_write16(padapter, REG_RXFLTMAP2, 0x00);
	else if (type == 2) { /*sta add event call back */
		/*enable update TSF */
		val8 = rtw_read8(padapter, REG_BCN_CTRL);
		val8 &= ~DIS_TSF_UDT;
		rtw_write8(padapter, REG_BCN_CTRL, val8);

		if (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE | WIFI_ADHOC_MASTER_STATE))
			RetryLimit = 0x7;
	}

	val16 = (RetryLimit << RETRY_LIMIT_SHORT_SHIFT) | (RetryLimit << RETRY_LIMIT_LONG_SHIFT);
	rtw_write16(padapter, REG_RL, val16);
}

static void hw_var_set_hw_update_tsf(PADAPTER padapter)
{

	u16 reg_bcn_ctl;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;

	reg_bcn_ctl = REG_BCN_CTRL;

	if (!pmlmeext->en_hw_update_tsf)
		return;

	/* check REG_RCR bit is set */
	if (!(rtw_read32(padapter, REG_RCR) & RCR_CBSSID_BCN)) {
		pmlmeext->en_hw_update_tsf = _FALSE;
		return;
	}


	/* enable hw update tsf function for non-AP */
	if (rtw_linked_check(padapter) &&
		check_fwstate(pmlmepriv, WIFI_AP_STATE) != _TRUE)
		/* enable update buddy TSF */
		rtw_write8(padapter, reg_bcn_ctl, rtw_read8(padapter, reg_bcn_ctl)&(~DIS_TSF_UDT));

	pmlmeext->en_hw_update_tsf = _FALSE;
}

s32 c2h_id_filter_ccx_8188f(u8 *buf)
{
	struct c2h_evt_hdr_88xx *c2h_evt = (struct c2h_evt_hdr_88xx *)buf;
	s32 ret = _FALSE;
	if (c2h_evt->id == C2H_CCX_TX_RPT)
		ret = _TRUE;

	return ret;
}


s32 c2h_handler_8188f(PADAPTER padapter, u8 *buf)
{
	struct c2h_evt_hdr_88xx *pC2hEvent = (struct c2h_evt_hdr_88xx *)buf;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	s32 ret = _SUCCESS;
	u8 index = 0;

	if (pC2hEvent == NULL) {
		DBG_8192C("%s(): pC2hEventis NULL\n", __func__);
		ret = _FAIL;
		goto exit;
	}

	switch (pC2hEvent->id) {
	case C2H_DBG: {
		RT_TRACE(_module_hal_init_c_, _drv_info_, ("c2h_handler_8188f: %s\n", pC2hEvent->payload));
	}
	break;

	case C2H_CCX_TX_RPT:
		/*CCX_FwC2HTxRpt(padapter, QueueID, pC2hEvent->payload); */
		break;


	default:
		break;
	}

	/* Clear event to notify FW we have read the command. */
	/* Note: */
	/*	If this field isn't clear, the FW won't update the next command message. */
	/*rtw_write8(padapter, REG_C2HEVT_CLEAR, C2H_EVT_HOST_CLOSE); */
exit:
	return ret;
}

static void process_c2h_event(PADAPTER padapter, PC2H_EVT_HDR pC2hEvent, u8 *c2hBuf)
{
	u8				index = 0;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	PDM_ODM_T			pDM_Odm = &pHalData->odmpriv;

	if (c2hBuf == NULL) {
		DBG_8192C("%s c2hbuff is NULL\n", __func__);
		return;
	}

	switch (pC2hEvent->CmdID) {

	default:
		if (!(phydm_c2H_content_parsing(pDM_Odm, pC2hEvent->CmdID, pC2hEvent->CmdLen, c2hBuf)))
			RT_TRACE(_module_hal_init_c_, _drv_info_, ("%s: [WARNING] unknown C2H(0x%02x)\n", __func__, c2hCmdId));

		break;

	}

#ifndef CONFIG_C2H_PACKET_EN
	/* Clear event to notify FW we have read the command. */
	/* Note: */
	/*	If this field isn't clear, the FW won't update the next command message. */
	rtw_write8(padapter, REG_C2HEVT_CLEAR, C2H_EVT_HOST_CLOSE);
#endif
}

#ifdef CONFIG_C2H_PACKET_EN

static void _rtl8188fu_c2h_packet_handler(PADAPTER padapter, u8 *pbuffer, u16 length)
{
	C2H_EVT_HDR 	C2hEvent;
	u8 *tmpBuf = NULL;
	C2hEvent.CmdID = pbuffer[0];
	C2hEvent.CmdSeq = pbuffer[1];
	C2hEvent.CmdLen = length - 2;
	tmpBuf = pbuffer + 2;

	/*DBG_871X("%s C2hEvent.CmdID:%x C2hEvent.CmdLen:%x C2hEvent.CmdSeq:%x\n", */
	/*		__func__, C2hEvent.CmdID, C2hEvent.CmdLen, C2hEvent.CmdSeq); */
	RT_PRINT_DATA(_module_hal_init_c_, _drv_notice_, "C2HPacketHandler_8188F(): Command Content:\n", tmpBuf, C2hEvent.CmdLen);

	process_c2h_event(padapter, &C2hEvent, tmpBuf);
	/*c2h_handler_8188f(padapter,&C2hEvent); */
	return;
}

void rtl8188fu_c2h_packet_handler(PADAPTER padapter, u8 *pbuf, u16 length)
{
	C2H_EVT_HDR C2hEvent;
	u8 *pdata;


	if (length == 0)
		return;

	C2hEvent.CmdID = pbuf[0];
	C2hEvent.CmdSeq = pbuf[1];
	C2hEvent.CmdLen = length - 2;
	pdata = pbuf + 2;

	DBG_8192C("%s: C2H, ID=%d seq=%d len=%d\n",
			  __func__, C2hEvent.CmdID, C2hEvent.CmdSeq, C2hEvent.CmdLen);

	switch (C2hEvent.CmdID) {
	case C2H_CCX_TX_RPT:
		process_c2h_event(padapter, &C2hEvent, pdata);
		break;

	case C2H_BCN_EARLY_RPT:
		break;

	case C2H_FW_CHNL_SWITCH_COMPLETE:
		break;

	default:
		rtw_c2h_packet_wk_cmd(padapter, pbuf, length);
		break;
	}
}

#else /* !CONFIG_C2H_PACKET_EN */
/* */
/*C2H event format: */
/* Field    TRIGGER     CONTENT    CMD_SEQ    CMD_LEN  CMD_ID */
/* BITS    [127:120]   [119:16]    [15:8]     [7:4]    [3:0] */
/*2009.10.08. by tynli. */
static void C2HCommandHandler(PADAPTER padapter)
{
	C2H_EVT_HDR 	C2hEvent;

#ifdef CONFIG_USB_HCI
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	_rtw_memset(&C2hEvent, 0, sizeof(C2H_EVT_HDR));
	C2hEvent.CmdID = pHalData->C2hArray[USB_C2H_CMDID_OFFSET] & 0xF;
	C2hEvent.CmdLen = (pHalData->C2hArray[USB_C2H_CMDID_OFFSET] & 0xF0) >> 4;
	C2hEvent.CmdSeq = pHalData->C2hArray[USB_C2H_SEQ_OFFSET];
	c2h_handler_8188f(padapter, (u8 *)&C2hEvent);
	/*process_c2h_event(padapter,&C2hEvent,&pHalData->C2hArray[USB_C2H_EVENT_OFFSET]); */
#endif /* CONFIG_USB_HCI */

	/*REG_C2HEVT_CLEAR have done in process_c2h_event */
	return;
exit:
	rtw_write8(padapter, REG_C2HEVT_CLEAR, C2H_EVT_HOST_CLOSE);
	return;
}

#endif /* !CONFIG_C2H_PACKET_EN */

void rtl8188f_set_pll_ref_clk_sel(_adapter *adapter, u8 sel)
{
	u8 value8;

	value8 = rtw_read8(adapter, REG_MAC_PLL_CTRL_EXT_8188F);
	if ((value8 & 0x0F) != (sel & 0x0F)) {
		u16 value16;
		u8 ori_bit_wlock_2c;

		value16 = rtw_read16(adapter, REG_RSV_CTRL_8188F);
		ori_bit_wlock_2c = (value16 & BIT8) ? 1 : 0;
		if (ori_bit_wlock_2c) {
			value16 &= ~BIT8;
			rtw_write16(adapter, REG_RSV_CTRL_8188F, value16);
		}

		DBG_871X_LEVEL(_drv_always_, "switch pll_ref_clk_sel from 0x%x to 0x%x\n"
			, (value8 & 0x0F), (sel & 0x0F));
		value8 = (value8 & 0xF0) | (sel & 0x0F);
		rtw_write8(adapter, REG_MAC_PLL_CTRL_EXT_8188F, value8);

		if (ori_bit_wlock_2c) {
			value16 |= BIT8;
			rtw_write16(adapter, REG_RSV_CTRL_8188F, value16);
		}
	}
}

/*
 * If variable not handled here,
 * some variables will be processed in SetHwReg8188FU()
 */
void rtl8188fu_set_hw_reg(PADAPTER padapter, u8 variable, u8 *val)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);
	u8 val8;
	u16 val16;
	u32 val32;


	switch (variable) {
	case HW_VAR_SET_RPWM:
		rtw_write8(padapter, REG_USB_HRPWM, *val);
		break;

	case HW_VAR_MEDIA_STATUS:
		val8 = rtw_read8(padapter, MSR) & 0x0c;
		val8 |= *val;
		rtw_write8(padapter, MSR, val8);
		break;

	case HW_VAR_MEDIA_STATUS1:
		val8 = rtw_read8(padapter, MSR) & 0x03;
		val8 |= *val << 2;
		rtw_write8(padapter, MSR, val8);
		break;

	case HW_VAR_SET_OPMODE:
		hw_var_set_opmode(padapter, variable, val);
		break;

	case HW_VAR_MAC_ADDR:
		hw_var_set_macaddr(padapter, variable, val);
		break;

	case HW_VAR_BSSID:
		hw_var_set_bssid(padapter, variable, val);
		break;

	case HW_VAR_BASIC_RATE: {
		struct mlme_ext_info *mlmext_info = &padapter->mlmeextpriv.mlmext_info;
		u16 input_b = 0, masked = 0, ioted = 0, BrateCfg = 0;
		u16 rrsr_2g_force_mask = RRSR_CCK_RATES;
		u16 rrsr_2g_allow_mask = (RRSR_24M | RRSR_12M | RRSR_6M | RRSR_CCK_RATES);

		HalSetBrateCfg(padapter, val, &BrateCfg);
		input_b = BrateCfg;

		/* apply force and allow mask */
		BrateCfg |= rrsr_2g_force_mask;
		BrateCfg &= rrsr_2g_allow_mask;
		masked = BrateCfg;

		/* IOT consideration */
		if (mlmext_info->assoc_AP_vendor == HT_IOT_PEER_CISCO) {
			/* if peer is cisco and didn't use ofdm rate, we enable 6M ack */
			if ((BrateCfg & (RRSR_24M | RRSR_12M | RRSR_6M)) == 0)
				BrateCfg |= RRSR_6M;
		}
		ioted = BrateCfg;

		pHalData->BasicRateSet = BrateCfg;

		DBG_8192C("HW_VAR_BASIC_RATE: %#x -> %#x -> %#x\n", input_b, masked, ioted);

		/* Set RRSR rate table. */
		rtw_write16(padapter, REG_RRSR, BrateCfg);
		rtw_write8(padapter, REG_RRSR + 2, rtw_read8(padapter, REG_RRSR + 2) & 0xf0);
	}
	break;

	case HW_VAR_TXPAUSE:
		rtw_write8(padapter, REG_TXPAUSE, *val);
		break;

	case HW_VAR_BCN_FUNC:
		hw_var_set_bcn_func(padapter, variable, val);
		break;

	case HW_VAR_CORRECT_TSF:
		hw_var_set_correct_tsf(padapter, variable, val);
		break;

	case HW_VAR_CHECK_BSSID: {
		u32 val32;

		val32 = rtw_read32(padapter, REG_RCR);
		if (*val)
			val32 |= RCR_CBSSID_DATA | RCR_CBSSID_BCN;
		else
			val32 &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN);
		rtw_write32(padapter, REG_RCR, val32);
	}
	break;

	case HW_VAR_MLME_DISCONNECT:
		hw_var_set_mlme_disconnect(padapter, variable, val);
		break;

	case HW_VAR_MLME_SITESURVEY:
		hw_var_set_mlme_sitesurvey(padapter, variable,  val);

		break;

	case HW_VAR_MLME_JOIN:
		hw_var_set_mlme_join(padapter, variable, val);
		break;

	case HW_VAR_ON_RCR_AM:
		val32 = rtw_read32(padapter, REG_RCR);
		val32 |= RCR_AM;
		rtw_write32(padapter, REG_RCR, val32);
		DBG_8192C("%s, %d, RCR= %x\n", __func__, __LINE__, rtw_read32(padapter, REG_RCR));
		break;

	case HW_VAR_OFF_RCR_AM:
		val32 = rtw_read32(padapter, REG_RCR);
		val32 &= ~RCR_AM;
		rtw_write32(padapter, REG_RCR, val32);
		DBG_8192C("%s, %d, RCR= %x\n", __func__, __LINE__, rtw_read32(padapter, REG_RCR));
		break;

	case HW_VAR_BEACON_INTERVAL:
		rtw_write16(padapter, REG_BCN_INTERVAL, *((u16 *)val));
		break;

	case HW_VAR_SLOT_TIME:
		rtw_write8(padapter, REG_SLOT, *val);
		break;

	case HW_VAR_RESP_SIFS:
#if 0
		/* SIFS for OFDM Data ACK */
		rtw_write8(padapter, REG_SIFS_CTX + 1, val[0]);
		/* SIFS for OFDM consecutive tx like CTS data! */
		rtw_write8(padapter, REG_SIFS_TRX + 1, val[1]);

		rtw_write8(padapter, REG_SPEC_SIFS + 1, val[0]);
		rtw_write8(padapter, REG_MAC_SPEC_SIFS + 1, val[0]);

		/* 20100719 Joseph: Revise SIFS setting due to Hardware register definition change. */
		rtw_write8(padapter, REG_R2T_SIFS + 1, val[0]);
		rtw_write8(padapter, REG_T2T_SIFS + 1, val[0]);

#else
		/*SIFS_Timer = 0x0a0a0808; */
		/*RESP_SIFS for CCK */
		rtw_write8(padapter, REG_RESP_SIFS_CCK, val[0]); /* SIFS_T2T_CCK (0x08) */
		rtw_write8(padapter, REG_RESP_SIFS_CCK + 1, val[1]); /*SIFS_R2T_CCK(0x08) */
		/*RESP_SIFS for OFDM */
		rtw_write8(padapter, REG_RESP_SIFS_OFDM, val[2]); /*SIFS_T2T_OFDM (0x0a) */
		rtw_write8(padapter, REG_RESP_SIFS_OFDM + 1, val[3]); /*SIFS_R2T_OFDM(0x0a) */
#endif
		break;

	case HW_VAR_ACK_PREAMBLE: {
		u8 regTmp;
		u8 bShortPreamble = *val;

		/* Joseph marked out for Netgear 3500 TKIP channel 7 issue.(Temporarily) */
		/*regTmp = (pHalData->cur_40_prime_sc)<<5; */
		regTmp = 0;
		if (bShortPreamble)
			regTmp |= 0x80;
		rtw_write8(padapter, REG_RRSR + 2, regTmp);
	}
	break;

	case HW_VAR_CAM_EMPTY_ENTRY: {
		u8	ucIndex = *val;
		u8	i;
		u32	ulCommand = 0;
		u32	ulContent = 0;
		u32	ulEncAlgo = CAM_AES;

		for (i = 0; i < CAM_CONTENT_COUNT; i++) {
			/* filled id in CAM config 2 byte */
			if (i == 0) {
				ulContent |= (ucIndex & 0x03) | ((u16)(ulEncAlgo) << 2);
				/*ulContent |= CAM_VALID; */
			} else
				ulContent = 0;
			/* polling bit, and No Write enable, and address */
			ulCommand = CAM_CONTENT_COUNT * ucIndex + i;
			ulCommand = ulCommand | CAM_POLLINIG | CAM_WRITE;
			/* write content 0 is equall to mark invalid */
			rtw_write32(padapter, WCAMI, ulContent);  /*delay_ms(40); */
			/*RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_empty_entry(): WRITE A4: %lx\n",ulContent)); */
			rtw_write32(padapter, RWCAM, ulCommand);  /*delay_ms(40); */
			/*RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_empty_entry(): WRITE A0: %lx\n",ulCommand)); */
		}
	}
	break;

	case HW_VAR_CAM_INVALID_ALL:
		rtw_write32(padapter, RWCAM, BIT(31) | BIT(30));
		break;

	case HW_VAR_AC_PARAM_VO:
		rtw_write32(padapter, REG_EDCA_VO_PARAM, *((u32 *)val));
		break;

	case HW_VAR_AC_PARAM_VI:
		rtw_write32(padapter, REG_EDCA_VI_PARAM, *((u32 *)val));
		break;

	case HW_VAR_AC_PARAM_BE:
		pHalData->AcParam_BE = ((u32 *)(val))[0];
		rtw_write32(padapter, REG_EDCA_BE_PARAM, *((u32 *)val));
		break;

	case HW_VAR_AC_PARAM_BK:
		rtw_write32(padapter, REG_EDCA_BK_PARAM, *((u32 *)val));
		break;

	case HW_VAR_ACM_CTRL: {
		u8 ctrl = *((u8 *)val);
		u8 hwctrl = 0;

		if (ctrl != 0) {
			hwctrl |= AcmHw_HwEn;

			if (ctrl & BIT(1)) /* BE */
				hwctrl |= AcmHw_BeqEn;

			if (ctrl & BIT(2)) /* VI */
				hwctrl |= AcmHw_ViqEn;

			if (ctrl & BIT(3)) /* VO */
				hwctrl |= AcmHw_VoqEn;
		}

		DBG_8192C("[HW_VAR_ACM_CTRL] Write 0x%02X\n", hwctrl);
		rtw_write8(padapter, REG_ACMHWCTRL, hwctrl);
	}
	break;

	case HW_VAR_AMPDU_FACTOR: {
		u32	AMPDULen = (*((u8 *)val));

		if (AMPDULen < HT_AGG_SIZE_32K)
			AMPDULen = (0x2000 << (*((u8 *)val))) - 1;
		else
			AMPDULen = 0x7fff;

		rtw_write32(padapter, REG_AMPDU_MAX_LENGTH_8188F, AMPDULen);
	}
	break;

	case HW_VAR_H2C_FW_PWRMODE: {
		u8 psmode = *val;

		/*if (psmode != PS_MODE_ACTIVE)	{ */
		/*	rtl8188f_set_lowpwr_lps_cmd(padapter, _TRUE); */
		/*} else { */
		/*	rtl8188f_set_lowpwr_lps_cmd(padapter, _FALSE); */
		/*} */
		rtl8188f_set_FwPwrMode_cmd(padapter, psmode);
	}
	break;
	case HW_VAR_H2C_PS_TUNE_PARAM:
		rtl8188f_set_FwPsTuneParam_cmd(padapter);
		break;

	case HW_VAR_H2C_FW_JOINBSSRPT:
		rtl8188f_set_FwJoinBssRpt_cmd(padapter, *val);
		break;

	case HW_VAR_EFUSE_USAGE:
		pHalData->EfuseUsedPercentage = *val;
		break;

	case HW_VAR_EFUSE_BYTES:
		pHalData->EfuseUsedBytes = *((u16 *)val);
		break;

		break;

	case HW_VAR_FIFO_CLEARN_UP: {
#define RW_RELEASE_EN		BIT(18)
#define RXDMA_IDLE			BIT(17)

		struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
		u8 trycnt = 100;

		/* pause tx */
		rtw_write8(padapter, REG_TXPAUSE, 0xff);

		/* keep sn */
		padapter->xmitpriv.nqos_ssn = rtw_read16(padapter, REG_NQOS_SEQ);

		if (pwrpriv->bkeepfwalive != _TRUE) {
			/* RX DMA stop */
			val32 = rtw_read32(padapter, REG_RXPKT_NUM);
			val32 |= RW_RELEASE_EN;
			rtw_write32(padapter, REG_RXPKT_NUM, val32);
			do {
				val32 = rtw_read32(padapter, REG_RXPKT_NUM);
				val32 &= RXDMA_IDLE;
				if (val32)
					break;

				DBG_871X("%s: [HW_VAR_FIFO_CLEARN_UP] val=%x times:%d\n", __func__, val32, trycnt);
			} while (--trycnt);
			if (trycnt == 0)
				DBG_8192C("[HW_VAR_FIFO_CLEARN_UP] Stop RX DMA failed......\n");

			/* RQPN Load 0 */
			rtw_write16(padapter, REG_RQPN_NPQ, 0);
			rtw_write32(padapter, REG_RQPN, 0x80000000);
			rtw_mdelay_os(2);
		}
	}
	break;

	case HW_VAR_RESTORE_HW_SEQ:
		/* restore Sequence No. */
		rtw_write8(padapter, 0x4dc, padapter->xmitpriv.nqos_ssn);
		break;

	case HW_VAR_APFM_ON_MAC:
		pHalData->bMacPwrCtrlOn = *val;
		DBG_871X("%s: bMacPwrCtrlOn=%d\n", __func__, pHalData->bMacPwrCtrlOn);
		break;

	case HW_VAR_HCI_SUS_STATE:
		pHalData->hci_sus_state = *val;
		DBG_871X("%s: hci_sus_state=%u\n", __func__, pHalData->hci_sus_state);
		break;

	case HW_VAR_NAV_UPPER: {
		u32 usNavUpper = *((u32 *)val);

		if (usNavUpper > HAL_NAV_UPPER_UNIT_8188F * 0xFF) {
			RT_TRACE(_module_hal_init_c_, _drv_notice_, ("The setting value (0x%08X us) of NAV_UPPER is larger than (%d * 0xFF)!!!\n", usNavUpper, HAL_NAV_UPPER_UNIT_8188F));
			break;
		}

		/* The value of ((usNavUpper + HAL_NAV_UPPER_UNIT_8188F - 1) / HAL_NAV_UPPER_UNIT_8188F) */
		/* is getting the upper integer. */
		usNavUpper = (usNavUpper + HAL_NAV_UPPER_UNIT_8188F - 1) / HAL_NAV_UPPER_UNIT_8188F;
		rtw_write8(padapter, REG_NAV_UPPER, (u8)usNavUpper);
	}
	break;

#ifndef CONFIG_C2H_PACKET_EN
	case HW_VAR_C2H_HANDLE:
		C2HCommandHandler(padapter);
		break;
#endif

	case HW_VAR_BCN_VALID:
		{
			/* BCN_VALID, BIT16 of REG_TDECTRL = BIT0 of REG_TDECTRL+2, write 1 to clear, Clear by sw */
			val8 = rtw_read8(padapter, REG_TDECTRL + 2);
			val8 |= BIT(0);
			rtw_write8(padapter, REG_TDECTRL + 2, val8);
		}
		break;

	case HW_VAR_DL_BCN_SEL:
		{
			/* SW_BCN_SEL - Port0 */
			val8 = rtw_read8(padapter, REG_DWBCN1_CTRL_8188F + 2);
			val8 &= ~BIT(4);
			rtw_write8(padapter, REG_DWBCN1_CTRL_8188F + 2, val8);
		}
		break;

	case HW_VAR_DO_IQK:
		if (*val)
			pHalData->bNeedIQK = _TRUE;
		else
			pHalData->bNeedIQK = _FALSE;
		break;

	case HW_VAR_DL_RSVD_PAGE:
		{
			rtl8188f_download_rsvd_page(padapter, RT_MEDIA_CONNECT);
		}
		break;

	case HW_VAR_MACID_SLEEP: {
		/* TODO: 16 MACID? */
		u32 reg_macid_sleep;
		u8 bit_shift;
		u8 id = *(u8 *)val;

		if (id < 32) {
			reg_macid_sleep = REG_MACID_SLEEP;
			bit_shift = id;
		} else if (id < 64) {
			reg_macid_sleep = REG_MACID_SLEEP_1;
			bit_shift = id - 32;
		} else if (id < 96) {
			reg_macid_sleep = REG_MACID_SLEEP_2;
			bit_shift = id - 64;
		} else if (id < 128) {
			reg_macid_sleep = REG_MACID_SLEEP_3;
			bit_shift = id - 96;
		} else {
			rtw_warn_on(1);
			break;
		}

		val32 = rtw_read32(padapter, reg_macid_sleep);
		DBG_8192C(FUNC_ADPT_FMT ": [HW_VAR_MACID_SLEEP] macid=%d, org reg_0x%03x=0x%08X\n",
				  FUNC_ADPT_ARG(padapter), id, reg_macid_sleep, val32);

		if (val32 & BIT(bit_shift))
			break;

		val32 |= BIT(bit_shift);
		rtw_write32(padapter, reg_macid_sleep, val32);
	}
	break;

	case HW_VAR_MACID_WAKEUP: {
		/* TODO: 16 MACID? */
		u32 reg_macid_sleep;
		u8 bit_shift;
		u8 id = *(u8 *)val;

		if (id < 32) {
			reg_macid_sleep = REG_MACID_SLEEP;
			bit_shift = id;
		} else if (id < 64) {
			reg_macid_sleep = REG_MACID_SLEEP_1;
			bit_shift = id - 32;
		} else if (id < 96) {
			reg_macid_sleep = REG_MACID_SLEEP_2;
			bit_shift = id - 64;
		} else if (id < 128) {
			reg_macid_sleep = REG_MACID_SLEEP_3;
			bit_shift = id - 96;
		} else {
			rtw_warn_on(1);
			break;
		}

		val32 = rtw_read32(padapter, reg_macid_sleep);
		DBG_8192C(FUNC_ADPT_FMT ": [HW_VAR_MACID_WAKEUP] macid=%d, org reg_0x%03x=0x%08X\n",
				  FUNC_ADPT_ARG(padapter), id, reg_macid_sleep, val32);

		if (!(val32 & BIT(bit_shift)))
			break;

		val32 &= ~BIT(bit_shift);
		rtw_write32(padapter, reg_macid_sleep, val32);
	}
	break;
	case HW_VAR_EN_HW_UPDATE_TSF:
		hw_var_set_hw_update_tsf(padapter);
		break;
	default:
		_rtl8188fu_set_hw_reg(padapter, variable, val);
		break;
	}

}

struct qinfo_8188f {
	u32 head: 8;
	u32 pkt_num: 7;
	u32 tail: 8;
	u32 ac: 2;
	u32 macid: 7;
};

struct bcn_qinfo_8188f {
	u16 head: 8;
	u16 pkt_num: 8;
};

void dump_qinfo_8188f(void *sel, struct qinfo_8188f *info, const char *tag)
{
	/*if (info->pkt_num) */
	DBG_871X_SEL_NL(sel, "%shead:0x%02x, tail:0x%02x, pkt_num:%u, macid:%u, ac:%u\n"
					, tag ? tag : "", info->head, info->tail, info->pkt_num, info->macid, info->ac
				   );
}

void dump_bcn_qinfo_8188f(void *sel, struct bcn_qinfo_8188f *info, const char *tag)
{
	/*if (info->pkt_num) */
	DBG_871X_SEL_NL(sel, "%shead:0x%02x, pkt_num:%u\n"
					, tag ? tag : "", info->head, info->pkt_num
				   );
}

void dump_mac_qinfo_8188f(void *sel, _adapter *adapter)
{
	u32 q0_info;
	u32 q1_info;
	u32 q2_info;
	u32 q3_info;
	u32 q4_info;
	u32 q5_info;
	u32 q6_info;
	u32 q7_info;
	u32 mg_q_info;
	u32 hi_q_info;
	u16 bcn_q_info;

	q0_info = rtw_read32(adapter, REG_Q0_INFO);
	q1_info = rtw_read32(adapter, REG_Q1_INFO);
	q2_info = rtw_read32(adapter, REG_Q2_INFO);
	q3_info = rtw_read32(adapter, REG_Q3_INFO);
	q4_info = rtw_read32(adapter, REG_Q4_INFO);
	q5_info = rtw_read32(adapter, REG_Q5_INFO);
	q6_info = rtw_read32(adapter, REG_Q6_INFO);
	q7_info = rtw_read32(adapter, REG_Q7_INFO);
	mg_q_info = rtw_read32(adapter, REG_MGQ_INFO);
	hi_q_info = rtw_read32(adapter, REG_HGQ_INFO);
	bcn_q_info = rtw_read16(adapter, REG_BCNQ_INFO);

	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q0_info, "Q0 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q1_info, "Q1 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q2_info, "Q2 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q3_info, "Q3 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q4_info, "Q4 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q5_info, "Q5 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q6_info, "Q6 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&q7_info, "Q7 ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&mg_q_info, "MG ");
	dump_qinfo_8188f(sel, (struct qinfo_8188f *)&hi_q_info, "HI ");
	dump_bcn_qinfo_8188f(sel, (struct bcn_qinfo_8188f *)&bcn_q_info, "BCN ");
}

void rtl8188fu_get_hw_reg(PADAPTER padapter, u8 variable, u8 *val)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);
	u8 val8;
	u16 val16;
	u32 val32;


	switch (variable) {
	case HW_VAR_TXPAUSE:
		*val = rtw_read8(padapter, REG_TXPAUSE);
		break;

	case HW_VAR_BCN_VALID:
		{
			/* BCN_VALID, BIT16 of REG_TDECTRL = BIT0 of REG_TDECTRL+2 */
			val8 = rtw_read8(padapter, REG_TDECTRL + 2);
			*val = (BIT(0) & val8) ? _TRUE : _FALSE;
		}
		break;

	case HW_VAR_FWLPS_RF_ON: {
		/* When we halt NIC, we should check if FW LPS is leave. */
		u32 valRCR;

		if ((rtw_is_surprise_removed(padapter)) ||
			(adapter_to_pwrctl(padapter)->rf_pwrstate == rf_off)) {
			/* If it is in HW/SW Radio OFF or IPS state, we do not check Fw LPS Leave, */
			/* because Fw is unload. */
			*val = _TRUE;
		} else {
			valRCR = rtw_read32(padapter, REG_RCR);
			valRCR &= 0x00070000;
			if (valRCR)
				*val = _FALSE;
			else
				*val = _TRUE;
		}
	}
	break;

	case HW_VAR_EFUSE_USAGE:
		*val = pHalData->EfuseUsedPercentage;
		break;

	case HW_VAR_EFUSE_BYTES:
		*((u16 *)val) = pHalData->EfuseUsedBytes;
		break;

	case HW_VAR_APFM_ON_MAC:
		*val = pHalData->bMacPwrCtrlOn;
		break;
	case HW_VAR_HCI_SUS_STATE:
		*val = pHalData->hci_sus_state;
		break;
	case HW_VAR_CHK_HI_QUEUE_EMPTY:
		val16 = rtw_read16(padapter, REG_TXPKT_EMPTY);
		*val = (val16 & BIT(10)) ? _TRUE : _FALSE;
		break;
	case HW_VAR_DUMP_MAC_QUEUE_INFO:
		dump_mac_qinfo_8188f(val, padapter);
		break;
	default:
		_rtl8188fu_get_hw_reg(padapter, variable, val);
		break;
	}
}

/*
 *	Description:
 *		Change default setting of specified variable.
 */
u8 SetHalDefVar8188F(PADAPTER padapter, HAL_DEF_VARIABLE variable, void *pval)
{
	PHAL_DATA_TYPE pHalData;
	u8 bResult;


	pHalData = GET_HAL_DATA(padapter);
	bResult = _SUCCESS;

	switch (variable) {
	default:
		bResult = SetHalDefVar(padapter, variable, pval);
		break;
	}

	return bResult;
}

#ifdef CONFIG_C2H_PACKET_EN
void SetHwRegWithBuf8188F(PADAPTER padapter, u8 variable, u8 *pbuf, int len)
{
	PHAL_DATA_TYPE pHalData;


	pHalData = GET_HAL_DATA(padapter);

	switch (variable) {
	case HW_VAR_C2H_HANDLE:
		_rtl8188fu_c2h_packet_handler(padapter, pbuf, len);
		break;

	default:
		break;
	}
}
#endif /* CONFIG_C2H_PACKET_EN */

void hal_ra_info_dump(_adapter *padapter , void *sel)
{
	int i;
	u8 mac_id;
	u32 cmd;
	u32 ra_info1, ra_info2, bw_set;
	u32 rate_mask1, rate_mask2;
	u8 curr_tx_rate, curr_tx_sgi, hight_rate, lowest_rate;
	HAL_DATA_TYPE *HalData = GET_HAL_DATA(padapter);
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct macid_ctl_t *macid_ctl = dvobj_to_macidctl(dvobj);

	for (i = 0; i < macid_ctl->num; i++) {

		if (rtw_macid_is_used(macid_ctl, i) && !rtw_macid_is_bmc(macid_ctl, i)) {

			mac_id = (u8) i;
			DBG_871X_SEL(sel , "============ RA status check  Mac_id:%d ===================\n", mac_id);

			cmd = 0x40000100 | mac_id;
			rtw_write32(padapter, REG_HMEBOX_DBG_2_8188F, cmd);
			rtw_msleep_os(10);
			ra_info1 = rtw_read32(padapter, 0x2F0);
			curr_tx_rate = ra_info1 & 0x7F;
			curr_tx_sgi = (ra_info1 >> 7) & 0x01;

			DBG_871X_SEL(sel , "[ ra_info1:0x%08x ] =>cur_tx_rate= %s,cur_sgi:%d\n" , ra_info1 , HDATA_RATE(curr_tx_rate), curr_tx_sgi);
			DBG_871X_SEL(sel , "[ ra_info1:0x%08x ] => PWRSTS = 0x%02x\n", ra_info1, (ra_info1 >> 8)  & 0x07);

			cmd = 0x40000400 | mac_id;
			rtw_write32(padapter, REG_HMEBOX_DBG_2_8188F, cmd);
			rtw_msleep_os(10);
			ra_info1 = rtw_read32(padapter, 0x2F0);
			ra_info2 = rtw_read32(padapter, 0x2F4);
			rate_mask1 = rtw_read32(padapter, 0x2F8);
			rate_mask2 = rtw_read32(padapter, 0x2FC);
			hight_rate = ra_info2 & 0xFF;
			lowest_rate = (ra_info2 >> 8)  & 0xFF;
			bw_set = (ra_info1 >> 8)  & 0xFF;

			DBG_871X_SEL(sel , "[ ra_info1:0x%08x ] => VHT_EN=0x%02x, ", ra_info1, (ra_info1>>24) & 0xFF);

			switch (bw_set) {

			case CHANNEL_WIDTH_20:
				DBG_871X_SEL(sel , "BW_setting=20M\n");
				break;

			case CHANNEL_WIDTH_40:
				DBG_871X_SEL(sel , "BW_setting=40M\n");
				break;

			case CHANNEL_WIDTH_80:
				DBG_871X_SEL(sel , "BW_setting=80M\n");
				break;

			case CHANNEL_WIDTH_160:
				DBG_871X_SEL(sel , "BW_setting=160M\n");
				break;

			default:
				DBG_871X_SEL(sel , "BW_setting=0x%02x\n", bw_set);
				break;

			}

			DBG_871X_SEL(sel , "[ ra_info1:0x%08x ] =>RSSI=%d,  DISRA=0x%02x\n",
					ra_info1,
					ra_info1 & 0xFF,
					(ra_info1 >> 16) & 0xFF);

			DBG_871X_SEL(sel , "[ ra_info2:0x%08x ] =>hight_rate=%s, lowest_rate=%s, SGI=0x%02x, RateID=%d\n",
					ra_info2,
					HDATA_RATE(hight_rate),
					HDATA_RATE(lowest_rate),
					(ra_info2 >> 16) & 0xFF,
					(ra_info2 >> 24) & 0xFF);

			DBG_871X_SEL(sel , "rate_mask2=0x%08x, rate_mask1=0x%08x\n", rate_mask2, rate_mask1);


		}
	}
}

/*
 *	Description:
 *		Query setting of specified variable.
 */
u8 GetHalDefVar8188F(PADAPTER padapter, HAL_DEF_VARIABLE variable, void *pval)
{
	PHAL_DATA_TYPE pHalData;
	u8 bResult;


	pHalData = GET_HAL_DATA(padapter);
	bResult = _SUCCESS;

	switch (variable) {
	case HAL_DEF_MAX_RECVBUF_SZ:
		*((u32 *)pval) = MAX_RECVBUF_SZ;
		break;

	case HAL_DEF_RX_PACKET_OFFSET:
		*((u32 *)pval) = RXDESC_SIZE + DRVINFO_SZ * 8;
		break;

	case HW_VAR_MAX_RX_AMPDU_FACTOR:
		/* Stanley@BB.SD3 suggests 16K can get stable performance */
		/* The experiment was done on SDIO interface */
		/* coding by Lucas@20130730 */
		*(HT_CAP_AMPDU_FACTOR *)pval = MAX_AMPDU_FACTOR_16K;
		break;
	case HW_VAR_BEST_AMPDU_DENSITY:
		*((u32 *)pval) = AMPDU_DENSITY_VALUE_7;
		break;
	case HAL_DEF_TX_LDPC:
	case HAL_DEF_RX_LDPC:
		*((u8 *)pval) = _FALSE;
		break;
	case HAL_DEF_TX_STBC:
		*((u8 *)pval) = 0;
		break;
	case HAL_DEF_RX_STBC:
		*((u8 *)pval) = 1;
		break;
	case HAL_DEF_EXPLICIT_BEAMFORMER:
	case HAL_DEF_EXPLICIT_BEAMFORMEE:
		*((u8 *)pval) = _FALSE;
		break;

	case HW_DEF_RA_INFO_DUMP:
		hal_ra_info_dump(padapter, pval);
	break;

	case HAL_DEF_TX_PAGE_BOUNDARY:
		*(u8 *)pval = TX_PAGE_BOUNDARY_8188F;
		break;

	case HAL_DEF_MACID_SLEEP:
		*(u8 *)pval = _TRUE; /* support macid sleep */
		break;
	case HAL_DEF_TX_PAGE_SIZE:
		*((u32 *)pval) = PAGE_SIZE_128;
		break;
	case HAL_DEF_RX_DMA_SZ_WOW:
		*(u32 *)pval = RX_DMA_SIZE_8188F - RESV_FMWF;
		break;
	case HAL_DEF_RX_DMA_SZ:
		*(u32 *)pval = RX_DMA_BOUNDARY_8188F + 1;
		break;
	case HAL_DEF_RX_PAGE_SIZE:
		*((u32 *)pval) = 8;
		break;
	default:
		bResult = GetHalDefVar(padapter, variable, pval);
		break;
	}

	return bResult;
}


void rtl8188f_start_thread(_adapter *padapter)
{
}

void rtl8188f_stop_thread(_adapter *padapter)
{
}



