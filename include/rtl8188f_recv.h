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
#ifndef __RTL8188F_RECV_H__
#define __RTL8188F_RECV_H__

#if defined(CONFIG_USB_HCI)
	#ifndef MAX_RECVBUF_SZ
		#ifdef PLATFORM_OS_CE
			#define MAX_RECVBUF_SZ (8192+1024) // 8K+1k
		#else
			#ifdef CONFIG_MINIMAL_MEMORY_USAGE
				#define MAX_RECVBUF_SZ (4000) // about 4K
			#else
				#ifdef CONFIG_PLATFORM_MSTAR
					#define MAX_RECVBUF_SZ (8192) // 8K
				#elif defined(CONFIG_PLATFORM_HISILICON)
					#define MAX_RECVBUF_SZ (16384) /* 16k */
				#else
					#define MAX_RECVBUF_SZ (32768) /* 32k */
				#endif
				//#define MAX_RECVBUF_SZ (20480) //20K
				//#define MAX_RECVBUF_SZ (10240) //10K 
				//#define MAX_RECVBUF_SZ (16384) //  16k - 92E RX BUF :16K
				//#define MAX_RECVBUF_SZ (8192+1024) // 8K+1k		
			#endif
		#endif
	#endif //!MAX_RECVBUF_SZ
#endif

// Rx smooth factor
#define	Rx_Smooth_Factor (20)

#ifdef CONFIG_USB_HCI
int rtl8188fu_init_recv_priv(_adapter *padapter);
void rtl8188fu_free_recv_priv (_adapter *padapter);
void rtl8188fu_init_recvbuf(_adapter *padapter, struct recv_buf *precvbuf);
#endif

#endif /* __RTL8188F_RECV_H__ */

