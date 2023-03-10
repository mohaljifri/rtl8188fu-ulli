/* SPDX-License-Identifier: GPL-2.0 */
#pragma once
#ifndef __INC_HW_IMG_H
#define __INC_HW_IMG_H

//
// 2011/03/15 MH Add for different IC HW image file selection. code size consideration.
//
#if RT_PLATFORM == PLATFORM_LINUX

	// For 92C
	#define 	RTL8192CE_HWIMG_SUPPORT 				0
	#define 	RTL8192CE_TEST_HWIMG_SUPPORT			0
	#define 	RTL8192CU_HWIMG_SUPPORT 				1
	#define 	RTL8192CU_TEST_HWIMG_SUPPORT			0

	//For 92D
	#define 	RTL8192DE_HWIMG_SUPPORT 				0
	#define 	RTL8192DE_TEST_HWIMG_SUPPORT			0
	#define 	RTL8192DU_HWIMG_SUPPORT 				1
	#define 	RTL8192DU_TEST_HWIMG_SUPPORT			0

	// For 8723
	#define 	RTL8723E_HWIMG_SUPPORT					0
	#define 	RTL8723U_HWIMG_SUPPORT					1
	#define 	RTL8723S_HWIMG_SUPPORT					0

	//For 88E
	#define		RTL8188EE_HWIMG_SUPPORT					0
	#define		RTL8188EU_HWIMG_SUPPORT					0
	#define		RTL8188ES_HWIMG_SUPPORT					0
	

#endif

#endif //__INC_HW_IMG_H
