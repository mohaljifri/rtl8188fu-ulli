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
 
#ifndef	__PHYDMDIG_H__
#define    __PHYDMDIG_H__

#define DIG_VERSION	"1.8"	/*2015.07.01*/

/* Pause DIG & CCKPD */
#define		DM_DIG_MAX_PAUSE_TYPE		0x7

typedef struct _Dynamic_Initial_Gain_Threshold_
{
	BOOLEAN		bIgnoreDIG;
	BOOLEAN		bPSDInProgress;

	u1Byte		Dig_Ext_Port_Stage;
	
	u1Byte		CurSTAConnectState;
	u1Byte		PreSTAConnectState;
	u1Byte		CurMultiSTAConnectState;

	u1Byte		PreIGValue;
	u1Byte		CurIGValue;
	u1Byte		BackupIGValue;		//MP DIG
	u1Byte		BT30_CurIGI;
	u1Byte		IGIBackup;

	u1Byte		rx_gain_range_max;
	u1Byte		rx_gain_range_min;
	u1Byte		Rssi_val_min;

	u1Byte		PreCCK_CCAThres;
	u1Byte		CurCCK_CCAThres;
	u1Byte		PreCCKPDState;
	u1Byte		CurCCKPDState;
	u1Byte		CCKPDBackup;
	u1Byte		pause_cckpd_level;
	u1Byte		pause_cckpd_value[DM_DIG_MAX_PAUSE_TYPE + 1];

	u1Byte		LargeFAHit;
	u1Byte		ForbiddenIGI;
	u4Byte		Recover_cnt;

	u1Byte		DIG_Dynamic_MIN_0;
	u1Byte		DIG_Dynamic_MIN_1;
	BOOLEAN		bMediaConnect_0;
	BOOLEAN		bMediaConnect_1;

	u4Byte		AntDiv_RSSI_max;
	u4Byte		RSSI_max;

	u1Byte		*bP2PInProcess;

	u1Byte		pause_dig_level;
	u1Byte		pause_dig_value[DM_DIG_MAX_PAUSE_TYPE + 1];

}DIG_T,*pDIG_T;

typedef struct _FALSE_ALARM_STATISTICS{
	u32 cnt_parity_fail;
	u32 cnt_rate_illegal;
	u32 cnt_crc8_fail;
	u32 cnt_mcs_fail;
	u32 cnt_fast_fsync_fail;
	u32 cnt_sb_search_fail;
	u32 cnt_ofdm_fail;
	u32 cnt_cck_fail;
	u32 cnt_all;
	u32 cnt_ofdm_cca;
	u32 cnt_cck_cca;
	u32 cnt_cca_all;
	u32 cnt_bw_usc;
	u32 cnt_bw_lsc;
}FALSE_ALARM_STATISTICS, *PFALSE_ALARM_STATISTICS;

typedef enum tag_Dynamic_Init_Gain_Operation_Type_Definition
{
	DIG_TYPE_THRESH_HIGH	= 0,
	DIG_TYPE_THRESH_LOW	= 1,
	DIG_TYPE_BACKOFF		= 2,
	__DIG_TYPE_RX_GAIN_MIN__	= 3,
	__DIG_TYPE_RX_GAIN_MAX__	= 4,
	DIG_TYPE_ENABLE 		= 5,
	DIG_TYPE_DISABLE 		= 6,	
	DIG_OP_TYPE_MAX
}DM_DIG_OP_E;

/*
typedef enum tag_CCK_Packet_Detection_Threshold_Type_Definition
{
	CCK_PD_STAGE_LowRssi = 0,
	CCK_PD_STAGE_HighRssi = 1,
	CCK_PD_STAGE_MAX = 3,
}DM_CCK_PDTH_E;

typedef enum tag_DIG_EXT_PORT_ALGO_Definition
{
	DIG_EXT_PORT_STAGE_0 = 0,
	DIG_EXT_PORT_STAGE_1 = 1,
	DIG_EXT_PORT_STAGE_2 = 2,
	DIG_EXT_PORT_STAGE_3 = 3,
	DIG_EXT_PORT_STAGE_MAX = 4,
}DM_DIG_EXT_PORT_ALG_E;

typedef enum tag_DIG_Connect_Definition
{
	DIG_STA_DISCONNECT = 0,	
	DIG_STA_CONNECT = 1,
	DIG_STA_BEFORE_CONNECT = 2,
	DIG_MultiSTA_DISCONNECT = 3,
	DIG_MultiSTA_CONNECT = 4,
	DIG_CONNECT_MAX
}DM_DIG_CONNECT_E;


#define DM_MultiSTA_InitGainChangeNotify(Event) {DM_DigTable.CurMultiSTAConnectState = Event;}

#define DM_MultiSTA_InitGainChangeNotify_CONNECT(_ADAPTER)	\
	DM_MultiSTA_InitGainChangeNotify(DIG_MultiSTA_CONNECT)

#define DM_MultiSTA_InitGainChangeNotify_DISCONNECT(_ADAPTER)	\
	DM_MultiSTA_InitGainChangeNotify(DIG_MultiSTA_DISCONNECT)
*/

typedef enum tag_PHYDM_Pause_Type {
	PHYDM_PAUSE = BIT0,
	PHYDM_RESUME = BIT1
} PHYDM_PAUSE_TYPE;

typedef enum tag_PHYDM_Pause_Level {
/* number of pause level can't exceed DM_DIG_MAX_PAUSE_TYPE */
	PHYDM_PAUSE_LEVEL_0 = 0,
	PHYDM_PAUSE_LEVEL_1 = 1,
	PHYDM_PAUSE_LEVEL_2 = 2,
	PHYDM_PAUSE_LEVEL_3 = 3,
	PHYDM_PAUSE_LEVEL_4 = 4,
	PHYDM_PAUSE_LEVEL_5 = 5,
	PHYDM_PAUSE_LEVEL_6 = 6,
	PHYDM_PAUSE_LEVEL_7 = DM_DIG_MAX_PAUSE_TYPE		/* maximum level */
} PHYDM_PAUSE_LEVEL;


#define		DM_DIG_THRESH_HIGH			40
#define		DM_DIG_THRESH_LOW			35

#define		DM_FALSEALARM_THRESH_LOW	400
#define		DM_FALSEALARM_THRESH_HIGH	1000

#define		DM_DIG_MAX_NIC				0x3e
#define		DM_DIG_MIN_NIC				0x1e //0x22//0x1c
#define		DM_DIG_MAX_OF_MIN_NIC		0x3e

#define		DM_DIG_MAX_AP					0x3e
#define		DM_DIG_MIN_AP					0x1c
#define		DM_DIG_MAX_OF_MIN			0x2A	//0x32
#define		DM_DIG_MIN_AP_DFS				0x20

#define		DM_DIG_MAX_NIC_HP			0x46
#define		DM_DIG_MIN_NIC_HP				0x2e

#define		DM_DIG_MAX_AP_HP				0x42
#define		DM_DIG_MIN_AP_HP				0x30

//vivi 92c&92d has different definition, 20110504
//this is for 92c
	#ifdef CONFIG_SPECIAL_SETTING_FOR_FUNAI_TV
	#define		DM_DIG_FA_TH0				0x80//0x20
	#else
	#define		DM_DIG_FA_TH0				0x200//0x20
	#endif

#define		DM_DIG_FA_TH1					0x300
#define		DM_DIG_FA_TH2					0x400
//this is for 92d
#define		DM_DIG_FA_TH0_92D				0x100
#define		DM_DIG_FA_TH1_92D				0x400
#define		DM_DIG_FA_TH2_92D				0x600

#define		DM_DIG_BACKOFF_MAX			12
#define		DM_DIG_BACKOFF_MIN			-4
#define		DM_DIG_BACKOFF_DEFAULT		10

#define 		DM_DIG_FA_TH0_LPS				4 //-> 4 in lps
#define 		DM_DIG_FA_TH1_LPS				15 //-> 15 lps
#define 		DM_DIG_FA_TH2_LPS				30 //-> 30 lps
#define 		RSSI_OFFSET_DIG				0x05

VOID
rtl8188fu_dm_write_dig(
	IN		PVOID					pDM_VOID, 	
	IN		u1Byte					CurrentIGI
	);

VOID
odm_PauseDIG(
	IN		PVOID					pDM_VOID,
	IN		PHYDM_PAUSE_TYPE		PauseType,
	IN		PHYDM_PAUSE_LEVEL		pause_level,
	IN		u1Byte					IGIValue
	);

VOID
odm_DIGInit(
	IN		PVOID					pDM_VOID
	);

VOID	
odm_DIG(
	IN		PVOID					pDM_VOID
	);

VOID
odm_DIGbyRSSI_LPS(
	IN		PVOID					pDM_VOID
	);

VOID 
rtl9188fu_dm_false_alarm_counter_statistics(
	IN		PVOID					pDM_VOID
	);

VOID
odm_PauseCCKPacketDetection(
	IN		PVOID					pDM_VOID,
	IN		PHYDM_PAUSE_TYPE		PauseType,
	IN		PHYDM_PAUSE_LEVEL		pause_level,
	IN		u1Byte					CCKPDThreshold
	);

VOID 
odm_CCKPacketDetectionThresh(
	IN		PVOID					pDM_VOID
	);

VOID 
ODM_Write_CCK_CCA_Thres(
	IN		PVOID					pDM_VOID, 
	IN		u1Byte					CurCCK_CCAThres
	);


#endif
