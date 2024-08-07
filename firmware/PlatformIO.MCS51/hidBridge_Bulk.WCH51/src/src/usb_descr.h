// ===================================================================================
// USB Descriptors and Definitions
// ===================================================================================
//
// Definition of USB descriptors and endpoint sizes and addresses.
//
// The following must be defined in config.h:
// USB_VENDOR_ID            - Vendor ID (16-bit word)
// USB_PRODUCT_ID           - Product ID (16-bit word)
// USB_DEVICE_VERSION       - Device version (16-bit BCD)
// USB_MAX_POWER_mA         - Device max power in mA
// All string descriptors.
//
// In the makefile the following microcontroller settings must be made:
// XRAM_LOC   = 0x0100
// XRAM_SIZE  = 0x0300

#pragma once
#include <stdint.h>
#include "usb.h"
#include "config.h"

// ===================================================================================
// USB Endpoint Definitions
// ===================================================================================
#define EP0_SIZE        8
#define EP1_SIZE        64

#define EP0_ADDR        0
#define EP1_ADDR        (EP0_ADDR + EP0_BUF_SIZE)
#define EP1_ADDRTX      (EP1_ADDR + 64)
#define EP_END          (EP1_ADDR + EP1_BUF_SIZE + 1)

#define EP0_BUF_SIZE    EP_BUF_SIZE(EP0_SIZE)
#define EP1_BUF_SIZE    EP_BUF_SIZE(EP1_SIZE) *2

#define EP_BUF_SIZE(x)  (x+2<64 ? x+2 : 64)

__xdata __at (EP0_ADDR)   uint8_t EP0_bufCNTRL[EP0_BUF_SIZE];     
__xdata __at (EP1_ADDR)   uint8_t EP1_bufHID[EP1_BUF_SIZE];
__xdata __at (EP1_ADDRTX) uint8_t EP1_bufHIDTX[EP1_SIZE];
__xdata __at (EP_END)     uint8_t __EP_END__[1];
// ===================================================================================
// Device and Configuration Descriptors
// ===================================================================================
typedef struct _USB_CFG_DESCR_HID {
  USB_CFG_DESCR config;
  USB_ITF_DESCR interface0;
  USB_ENDP_DESCR ep1OUT;
  USB_ENDP_DESCR ep1IN;
} USB_CFG_DESCR_HID, *PUSB_CFG_DESCR_HID;
typedef USB_CFG_DESCR_HID __xdata *PXUSB_CFG_DESCR_HID;

extern __code USB_DEV_DESCR DevDescr;
extern __code USB_CFG_DESCR_HID CfgDescr;

// ===================================================================================
// String Descriptors
// ===================================================================================
extern __code uint16_t LangDescr[];
extern __code uint16_t ManufDescr[];
extern __code uint16_t ProdDescr[];
extern __code uint16_t SerDescr[];
extern __code uint16_t InterfDescr[];

#define USB_STR_DESCR_i0    (uint8_t*)LangDescr
#define USB_STR_DESCR_i1    (uint8_t*)ManufDescr
#define USB_STR_DESCR_i2    (uint8_t*)ProdDescr
#define USB_STR_DESCR_i3    (uint8_t*)SerDescr
#define USB_STR_DESCR_i4    (uint8_t*)InterfDescr
#define USB_STR_DESCR_ix    (uint8_t*)SerDescr

// ===================================================================================
// Windows Compatible ID (WCID) descriptors for automated driver installation
// ===================================================================================

#ifdef WCID_VENDOR_CODE
extern __code uint8_t WCID_FEATURE_DESCR[];
extern __code uint16_t MicrosoftDescr[];
#define USB_STR_DESCR_ixee  (uint8_t*)MicrosoftDescr
#endif
