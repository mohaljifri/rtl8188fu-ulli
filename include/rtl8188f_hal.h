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
#ifndef __RTL8188F_HAL_H__
#define __RTL8188F_HAL_H__

#include "hal_data.h"

#include "rtl8188f_spec.h"
#include "rtl8188f_rf.h"
#include "rtl8188f_dm.h"
#include "rtl8188f_recv.h"
#include "rtl8188f_xmit.h"
#include "rtl8188f_cmd.h"
#include "rtl8188f_led.h"
#include "Hal8188FPwrSeq.h"
#include "Hal8188FPhyReg.h"
#include "Hal8188FPhyCfg.h"

#define FW_8188F_SIZE			0x8000
#define FW_8188F_START_ADDRESS	0x1000
#define FW_8188F_END_ADDRESS		0x1FFF //0x5FFF

#define IS_FW_HEADER_EXIST_8188F(_pFwHdr)	((le16_to_cpu(_pFwHdr->Signature)&0xFFF0) == 0x88F0)

//
// This structure must be cared byte-ordering
//
// Added by tynli. 2009.12.04.
typedef struct _RT_8188F_FIRMWARE_HDR
{
	// 8-byte alinment required

	//--- LONG WORD 0 ----
	u16		Signature;	// 92C0: test chip; 92C, 88C0: test chip; 88C1: MP A-cut; 92C1: MP A-cut
	u8		Category;	// AP/NIC and USB/PCI
	u8		Function;	// Reserved for different FW function indcation, for further use when driver needs to download different FW in different conditions
	u16		Version;		// FW Version
	u16		Subversion;	// FW Subversion, default 0x00

	//--- LONG WORD 1 ----
	u8		Month;	// Release time Month field
	u8		Date;	// Release time Date field
	u8		Hour;	// Release time Hour field
	u8		Minute;	// Release time Minute field
	u16		RamCodeSize;	// The size of RAM code
	u16		Rsvd2;

	//--- LONG WORD 2 ----
	u32		SvnIdx;	// The SVN entry index
	u32		Rsvd3;

	//--- LONG WORD 3 ----
	u32		Rsvd4;
	u32		Rsvd5;
}RT_8188F_FIRMWARE_HDR, *PRT_8188F_FIRMWARE_HDR;

#define DRIVER_EARLY_INT_TIME_8188F		0x05
#define BCN_DMA_ATIME_INT_TIME_8188F		0x02

// for 8188F
// TX 32K, RX 16K, Page size 128B for TX, 8B for RX
#define PAGE_SIZE_TX_8188F			128
#define PAGE_SIZE_RX_8188F			8

#define RX_DMA_SIZE_8188F			0x4000	// 16K
#define RX_DMA_RESERVED_SIZE_8188F	0x80	// 128B, reserved for tx report

#define RESV_FMWF	0

#define RX_DMA_BOUNDARY_8188F		(RX_DMA_SIZE_8188F - RX_DMA_RESERVED_SIZE_8188F - 1)

// Note: We will divide number of page equally for each queue other than public queue!

//For General Reserved Page Number(Beacon Queue is reserved page)
//Beacon:2, PS-Poll:1, Null Data:1,Qos Null Data:1,BT Qos Null Data:1
#define BCNQ_PAGE_NUM_8188F		0x08
#define BCNQ1_PAGE_NUM_8188F		0x00

#ifdef CONFIG_PNO_SUPPORT
#undef BCNQ1_PAGE_NUM_8188F
#define BCNQ1_PAGE_NUM_8188F		0x00 // 0x04
#endif

//For WoWLan , more reserved page
//ARP Rsp:1, RWC:1, GTK Info:1,GTK RSP:2,GTK EXT MEM:2, PNO: 6
#define WOWLAN_PAGE_NUM_8188F	0x00

#ifdef CONFIG_PNO_SUPPORT
#undef WOWLAN_PAGE_NUM_8188F
#define WOWLAN_PAGE_NUM_8188F	0x15
#endif

#define TX_TOTAL_PAGE_NUMBER_8188F	(0xFF - BCNQ_PAGE_NUM_8188F - BCNQ1_PAGE_NUM_8188F - WOWLAN_PAGE_NUM_8188F)
#define TX_PAGE_BOUNDARY_8188F		(TX_TOTAL_PAGE_NUMBER_8188F + 1)

#define WMM_NORMAL_TX_TOTAL_PAGE_NUMBER_8188F	TX_TOTAL_PAGE_NUMBER_8188F
#define WMM_NORMAL_TX_PAGE_BOUNDARY_8188F		(WMM_NORMAL_TX_TOTAL_PAGE_NUMBER_8188F + 1)

// For Normal Chip Setting
// (HPQ + LPQ + NPQ + PUBQ) shall be TX_TOTAL_PAGE_NUMBER_8188F
#define NORMAL_PAGE_NUM_HPQ_8188F		0x0C
#define NORMAL_PAGE_NUM_LPQ_8188F		0x02
#define NORMAL_PAGE_NUM_NPQ_8188F		0x02

// Note: For Normal Chip Setting, modify later
#define WMM_NORMAL_PAGE_NUM_HPQ_8188F		0x30
#define WMM_NORMAL_PAGE_NUM_LPQ_8188F		0x20
#define WMM_NORMAL_PAGE_NUM_NPQ_8188F		0x20


#include "HalVerDef.h"
#include "hal_com.h"

#define EFUSE_OOB_PROTECT_BYTES (34 + 1)

#define HAL_EFUSE_MEMORY

#define HWSET_MAX_SIZE_8188F			512
#define EFUSE_REAL_CONTENT_LEN_8188F	256
#define EFUSE_MAP_LEN_8188F				512
#define EFUSE_MAX_SECTION_8188F			(EFUSE_MAP_LEN_8188F / 8)

#define EFUSE_IC_ID_OFFSET			506	//For some inferiority IC purpose. added by Roger, 2009.09.02.
#define AVAILABLE_EFUSE_ADDR(addr) 	(addr < EFUSE_REAL_CONTENT_LEN_8188F)

#define EFUSE_ACCESS_ON			0x69	// For RTL8188 only.
#define EFUSE_ACCESS_OFF			0x00	// For RTL8188 only.

//========================================================
//			EFUSE for BT definition
//========================================================
#define EFUSE_BT_REAL_BANK_CONTENT_LEN	512
#define EFUSE_BT_REAL_CONTENT_LEN		1536	// 512*3
#define EFUSE_BT_MAP_LEN				1024	// 1k bytes
#define EFUSE_BT_MAX_SECTION			128		// 1024/8

#define EFUSE_PROTECT_BYTES_BANK		16

typedef struct _C2H_EVT_HDR
{
	u8	CmdID;
	u8	CmdLen;
	u8	CmdSeq;
} __attribute__((__packed__)) C2H_EVT_HDR, *PC2H_EVT_HDR;

// rtl8188a_hal_init.c
s32 rtl8188f_FirmwareDownload(PADAPTER padapter);
void rtl8188f_FirmwareSelfReset(PADAPTER padapter);
void rtl8188f_InitializeFirmwareVars(PADAPTER padapter);

void rtl8188f_init_default_value(PADAPTER padapter);

s32 _rtl8188fu_llt_table_init(PADAPTER padapter);

// EFuse
u8 GetEEPROMSize8188F(PADAPTER padapter);
void Hal_InitPGData(PADAPTER padapter, u8 *PROMContent);
void Hal_EfuseParseIDCode(PADAPTER padapter, u8 *hwinfo);
void _rtl8188fu_read_txpower_info_from_hwpg(PADAPTER padapter, u8 *PROMContent, BOOLEAN AutoLoadFail);
/* void Hal_EfuseParseBTCoexistInfo_8188F(PADAPTER padapter, u8 *hwinfo, BOOLEAN AutoLoadFail); */
void Hal_EfuseParseEEPROMVer_8188F(PADAPTER padapter, u8 *hwinfo, BOOLEAN AutoLoadFail);
void Hal_EfuseParseChnlPlan_8188F(PADAPTER padapter, u8 *hwinfo, BOOLEAN AutoLoadFail);
void Hal_EfuseParseCustomerID_8188F(PADAPTER padapter, u8 *hwinfo, BOOLEAN AutoLoadFail);
void Hal_EfuseParsePowerSavingMode_8188F(PADAPTER pAdapter, u8 *hwinfo, BOOLEAN AutoLoadFail);
void Hal_EfuseParseAntennaDiversity_8188F(PADAPTER padapter, u8 *hwinfo, BOOLEAN AutoLoadFail);
void Hal_EfuseParseXtal_8188F(PADAPTER pAdapter, u8 *hwinfo, u8 AutoLoadFail);
void Hal_EfuseParseThermalMeter_8188F(PADAPTER padapter, u8 *hwinfo, u8 AutoLoadFail);
void Hal_EfuseParseKFreeData_8188F(PADAPTER pAdapter, u8 *hwinfo, BOOLEAN AutoLoadFail);
VOID
rtl8188fu_get_delta_swing_table(
	IN PVOID pDM_VOID,
	OUT pu1Byte *TemperatureUP_A,
	OUT pu1Byte *TemperatureDOWN_A,
	OUT pu1Byte *TemperatureUP_B,
	OUT pu1Byte *TemperatureDOWN_B
);

#if 0 /* Do not need for rtl8188f */
VOID Hal_EfuseParseVoltage_8188F(PADAPTER pAdapter,u8* hwinfo,BOOLEAN 	AutoLoadFail); 
#endif

#ifdef CONFIG_C2H_PACKET_EN
void rtl8188fu_c2h_packet_handler(PADAPTER padapter, u8 *pbuf, u16 length);
#endif

void rtl8188f_set_pll_ref_clk_sel(_adapter *adapter, u8 sel);

void init_hal_spec_8188f(_adapter *adapter);
void rtl8188fu_set_hw_reg(PADAPTER padapter, u8 variable, u8 *val);
void rtl8188fu_get_hw_reg(PADAPTER padapter, u8 variable, u8 *val);
#ifdef CONFIG_C2H_PACKET_EN
void SetHwRegWithBuf8188F(PADAPTER padapter, u8 variable, u8 *pbuf, int len);
#endif // CONFIG_C2H_PACKET_EN
u8 SetHalDefVar8188F(PADAPTER padapter, HAL_DEF_VARIABLE variable, void *pval);
u8 GetHalDefVar8188F(PADAPTER padapter, HAL_DEF_VARIABLE variable, void *pval);

// register
void rtl8188f_InitBeaconParameters(PADAPTER padapter);
void rtl8188f_InitBeaconMaxError(PADAPTER padapter, u8 InfraMode);
void	_InitBurstPktLen_8188FS(PADAPTER Adapter);
void _8051Reset8188(PADAPTER padapter);

void rtl8188f_start_thread(_adapter *padapter);
void rtl8188f_stop_thread(_adapter *padapter);

s32 c2h_id_filter_ccx_8188f(u8 *buf);
s32 c2h_handler_8188f(PADAPTER padapter, u8 *pC2hEvent);
u8 MRateToHwRate8188F(u8  rate);
u8 HwRateToMRate8188F(u8	 rate);

u16 Hal_EfuseGetCurrentSize(PADAPTER pAdapter);
void rtl8188fu_read_chip_version(PADAPTER padapter);
void UpdateHalRAMask8188F(PADAPTER padapter, u32 mac_id, u8 rssi_level);
void rtl8188f_SetBeaconRelatedRegisters(PADAPTER padapter);
void rtl8188fu_EfusePowerSwitch(PADAPTER padapter, u8 bWrite, u8 PwrState);
void rtl8188fu_efuse_read_efuse(PADAPTER padapter, u16 _offset, u16 _size_byte, 
		   u8 *pbuf);
void Hal_GetEfuseDefinition(PADAPTER padapter, u8 type,
			    void *pOut);
s32 Hal_EfusePgPacketRead(PADAPTER padapter,u8 offset,
			  u8 *data);


/* ULLI : hal/rtl8188f/usb/usb_halinit.c */

u32 rtl8188fu_init_power_on(PADAPTER padapter);

#endif
