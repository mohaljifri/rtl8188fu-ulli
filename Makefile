# SPDX-License-Identifier: GPL-2.0
EXTRA_CFLAGS += $(USER_EXTRA_CFLAGS)
EXTRA_CFLAGS += -O1
#EXTRA_CFLAGS += -O3
#EXTRA_CFLAGS += -Wall
#EXTRA_CFLAGS += -Wextra
#EXTRA_CFLAGS += -Werror
#EXTRA_CFLAGS += -pedantic
#EXTRA_CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes

EXTRA_CFLAGS += -Wno-unused-variable
EXTRA_CFLAGS += -Wno-unused-value
EXTRA_CFLAGS += -Wno-unused-label
EXTRA_CFLAGS += -Wno-unused-parameter
EXTRA_CFLAGS += -Wno-unused-function
EXTRA_CFLAGS += -Wno-unused
#EXTRA_CFLAGS += -Wno-uninitialized
#EXTRA_CFLAGS += -Wno-error=date-time	# Fix compile error on gcc 4.9 and later

EXTRA_CFLAGS += -I$(src)/include
EXTRA_CFLAGS += -I$(src)/hal/phydm

EXTRA_LDFLAGS += --strip-debug

CONFIG_AUTOCFG_CP = n

########################## WIFI IC ############################
CONFIG_RTL8188F = y
######################### Interface ###########################
CONFIG_USB_HCI = y
########################## Features ###########################
CONFIG_POWER_SAVING = y
CONFIG_USB_AUTOSUSPEND = n
CONFIG_HW_PWRP_DETECTION = n
CONFIG_WIFI_TEST = n
CONFIG_INTEL_WIDI = n
CONFIG_WAPI_SUPPORT = n
CONFIG_EXT_CLK = n
CONFIG_TRAFFIC_PROTECT = y
CONFIG_80211W = n
CONFIG_REDUCE_TX_CPU_LOADING = n
CONFIG_BR_EXT = n
CONFIG_WIFI_MONITOR = n
######### Notify SDIO Host Keep Power During Syspend ##########
CONFIG_RTW_SDIO_PM_KEEP_POWER = y
###################### MP HW TX MODE FOR VHT #######################
CONFIG_MP_VHT_HW_TX_MODE = n
###################### Platform Related #######################
CONFIG_PLATFORM_I386_PC = n
CONFIG_PLATFORM_SYNOLOGY_DSM62_CEDARVIEW = y
###############################################################

CONFIG_DRVEXT_MODULE = n

export TopDIR ?= $(shell pwd)

########### COMMON  #################################

_OS_INTFS_FILES :=	os_dep/osdep_service.o \
			os_dep/linux/os_intfs.o \
			os_dep/linux/usb_intf.o \
			os_dep/linux/usb_ops_linux.o \
			os_dep/linux/ioctl_linux.o \
			os_dep/linux/xmit_linux.o \
			os_dep/linux/mlme_linux.o \
			os_dep/linux/recv_linux.o \
			os_dep/linux/ioctl_cfg80211.o \
			os_dep/linux/wifi_regd.o

_HAL_INTFS_FILES :=	hal/hal_intf.o \
			hal/hal_com.o \
			hal/hal_com_phycfg.o \
			hal/hal_phy.o \
			hal/hal_dm.o \
			hal/hal_hci/hal_usb.o \
			hal/led/hal_usb_led.o

			
_OUTSRC_FILES := hal/phydm/phydm_debug.o	\
		hal/phydm/phydm_interface.o\
		hal/phydm/phydm_hwconfig.o\
		hal/phydm/phydm.o\
		hal/phydm/halphyrf_ce.o\
		hal/phydm/phydm_edcaturbocheck.o\
		hal/phydm/phydm_dig.o\
		hal/phydm/phydm_pathdiv.o\
		hal/phydm/phydm_rainfo.o\
		hal/phydm/phydm_powertracking_ce.o\
		hal/phydm/phydm_dynamictxpower.o\
		hal/phydm/phydm_adaptivity.o\
		hal/phydm/phydm_cfotracking.o\
		hal/phydm/phydm_acs.o\

########### HAL_RTL8188F #################################
ifeq ($(CONFIG_RTL8188F), y)

EXTRA_CFLAGS += -DCONFIG_RTL8188F

_HAL_INTFS_FILES += hal/HalPwrSeqCmd.o \
			hal/rtl8188f/Hal8188FPwrSeq.o\

_HAL_INTFS_FILES +=	hal/rtl8188f/rtl8188f_hal_init.o \
			hal/rtl8188f/rtl8188f_phycfg.o \
			hal/rtl8188f/rtl8188f_rf6052.o \
			hal/rtl8188f/rtl8188f_dm.o \
			hal/rtl8188f/rtl8188f_cmd.o \


_HAL_INTFS_FILES +=	\
			hal/rtl8188f/usb/usb_halinit.o \
			hal/rtl8188f/usb/rtl8188fu_led.o \
			hal/rtl8188f/usb/rtl8188fu_xmit.o \
			hal/rtl8188f/usb/rtl8188fu_recv.o

_HAL_INTFS_FILES += hal/rtl8188f/usb/usb_ops.o
_HAL_INTFS_FILES +=hal/efuse/rtl8188f/HalEfuseMask8188F_USB.o

_OUTSRC_FILES += hal/phydm/rtl8188f/halhwimg8188f_bb.o\
			hal/phydm/rtl8188f/halhwimg8188f_mac.o\
			hal/phydm/rtl8188f/halhwimg8188f_rf.o\
			hal/phydm/rtl8188f/phydm_regconfig8188f.o\
			hal/phydm/rtl8188f/halphyrf_8188f.o \
			hal/phydm/rtl8188f/phydm_rtl8188f.o

endif

########### END OF PATH  #################################


ifeq ($(CONFIG_USB_HCI), y)
ifeq ($(CONFIG_USB_AUTOSUSPEND), y)
EXTRA_CFLAGS += -DCONFIG_USB_AUTOSUSPEND
endif
endif

ifeq ($(CONFIG_POWER_SAVING), y)
EXTRA_CFLAGS += -DCONFIG_POWER_SAVING
endif

ifeq ($(CONFIG_HW_PWRP_DETECTION), y)
EXTRA_CFLAGS += -DCONFIG_HW_PWRP_DETECTION
endif

ifeq ($(CONFIG_WIFI_TEST), y)
EXTRA_CFLAGS += -DCONFIG_WIFI_TEST
endif

ifeq ($(CONFIG_INTEL_WIDI), y)
EXTRA_CFLAGS += -DCONFIG_INTEL_WIDI
endif

ifeq ($(CONFIG_WAPI_SUPPORT), y)
EXTRA_CFLAGS += -DCONFIG_WAPI_SUPPORT
endif

ifeq ($(CONFIG_EXT_CLK), y)
EXTRA_CFLAGS += -DCONFIG_EXT_CLK
endif

ifeq ($(CONFIG_TRAFFIC_PROTECT), y)
EXTRA_CFLAGS += -DCONFIG_TRAFFIC_PROTECT
endif

ifeq ($(CONFIG_80211W), y)
EXTRA_CFLAGS += -DCONFIG_IEEE80211W
endif

ifeq ($(CONFIG_PNO_SUPPORT), y)
EXTRA_CFLAGS += -DCONFIG_PNO_SUPPORT
ifeq ($(CONFIG_PNO_SET_DEBUG), y)
EXTRA_CFLAGS += -DCONFIG_PNO_SET_DEBUG
endif
endif

ifneq ($(CONFIG_WAKEUP_GPIO_IDX), default)
EXTRA_CFLAGS += -DWAKEUP_GPIO_IDX=$(CONFIG_WAKEUP_GPIO_IDX)
endif

ifeq ($(CONFIG_REDUCE_TX_CPU_LOADING), y)
EXTRA_CFLAGS += -DCONFIG_REDUCE_TX_CPU_LOADING
endif

ifeq ($(CONFIG_BR_EXT), y)
BR_NAME = br0
EXTRA_CFLAGS += -DCONFIG_BR_EXT
EXTRA_CFLAGS += '-DCONFIG_BR_EXT_BRNAME="'$(BR_NAME)'"'
endif

ifeq ($(CONFIG_WIFI_MONITOR), y)
EXTRA_CFLAGS += -DCONFIG_WIFI_MONITOR
endif

ifeq ($(CONFIG_MP_VHT_HW_TX_MODE), y)
EXTRA_CFLAGS += -DCONFIG_MP_VHT_HW_TX_MODE
ifeq ($(CONFIG_PLATFORM_I386_PC), y)
## For I386 X86 ToolChain use Hardware FLOATING
EXTRA_CFLAGS += -mhard-float
else
## For ARM ToolChain use Hardware FLOATING
EXTRA_CFLAGS += -mfloat-abi=hard
endif
endif
## ULLI set DM_ODM_SUPPORT_TYPE to ODM_CE
EXTRA_CFLAGS += -DDM_ODM_SUPPORT_TYPE=0x04

ifeq ($(CONFIG_PLATFORM_I386_PC), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211 -DRTW_USE_CFG80211_STA_EVENT
SUBARCH := $(shell uname -m | sed -e s/i.86/i386/)
ARCH ?= $(SUBARCH)
CROSS_COMPILE ?=
KVER  := $(shell uname -r)
KSRC := /lib/modules/$(KVER)/build
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/
INSTALL_PREFIX :=
endif

ifeq ($(CONFIG_PLATFORM_SYNOLOGY_DSM62_CEDARVIEW), y)
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211 -DRTW_USE_CFG80211_STA_EVENT
ARCH := x86_64
CROSS_COMPILE := /usr/local/x86_64-pc-linux-gnu/bin/x86_64-pc-linux-gnu-
KVER := 3.10.105
KSRC := /usr/local/x86_64-pc-linux-gnu/x86_64-pc-linux-gnu/sys-root/usr/lib/modules/DSM-6.2/build
endif

ifneq ($(KERNELRELEASE),)

rtk_core :=	core/rtw_cmd.o \
		core/rtw_security.o \
		core/rtw_debug.o \
		core/rtw_io.o \
		core/rtw_ioctl_query.o \
		core/rtw_ioctl_set.o \
		core/rtw_ieee80211.o \
		core/rtw_mlme.o \
		core/rtw_mlme_ext.o \
		core/rtw_wlan_util.o \
		core/rtw_pwrctrl.o \
		core/rtw_rf.o \
		core/rtw_recv.o \
		core/rtw_sta_mgt.o \
		core/rtw_ap.o \
		core/rtw_xmit.o	\
		core/rtw_p2p.o \
		core/rtw_iol.o \
		core/rtw_odm.o \
		core/efuse/rtw_efuse.o 

rtl8188fu-y += $(rtk_core)

rtl8188fu-$(CONFIG_INTEL_WIDI) += core/rtw_intel_widi.o

rtl8188fu-$(CONFIG_WAPI_SUPPORT) += core/rtw_wapi.o	\
					core/rtw_wapi_sms4.o

rtl8188fu-y += $(_OS_INTFS_FILES)
rtl8188fu-y += $(_HAL_INTFS_FILES)
rtl8188fu-y += $(_OUTSRC_FILES)
rtl8188fu-y += $(_PLATFORM_FILES)

obj-$(CONFIG_RTL8188FU) := rtl8188fu.o

else

export CONFIG_RTL8188FU = m

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd)  modules

strip:
	$(CROSS_COMPILE)strip $(MODULE_NAME).ko --strip-unneeded

installfw:
	mkdir -p /lib/firmware/rtlwifi
	cp -n firmware/* /lib/firmware/rtlwifi/.

install:
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	ifneq ($(CONFIG_PLATFORM_SYNOLOGY_DSM62_CEDARVIEW), y)
	  /sbin/depmod -a ${KVER}
	endif

uninstall:
	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a ${KVER}

config_r:
	@echo "make config"
	/bin/bash script/Configure script/config.in


.PHONY: modules clean

clean:
	cd hal/phydm/ ; rm -fr */*.mod.c */*.mod */*.o */.*.cmd */*.ko
	cd hal/phydm/ ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd hal/led ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd hal ; rm -fr */*/*.mod.c */*/*.mod */*/*.o */*/.*.cmd */*/*.ko
	cd hal ; rm -fr */*.mod.c */*.mod */*.o */.*.cmd */*.ko
	cd hal ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd core/efuse ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd core ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd os_dep/linux ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd os_dep ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	rm -fr Module.symvers ; rm -fr Module.markers ; rm -fr modules.order
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
endif

