// ===================================================================================
// USB VENDOR-Specific Control and Data Transfer Functions for CH551, CH552 and CH554
// ===================================================================================

#pragma once
#include <stdint.h>
#include "ch554.h"
#include "usb.h"
#include "usb_descr.h"
#include "usb_handler.h"

// Setup function
#define VEN_init    USB_init                        // setup USB vendor-specific device

// Vendor class control requests
#define VEN_REQ_BOOTLOADER  1                       // enter bootloader
#define VEN_REQ_LED17_ON    0x71                       // turn on LED
#define VEN_REQ_LED17_OFF   0x70                       // turn off LED
#define VEN_REQ_LED16_ON    0x61                     // turn on LED
#define VEN_REQ_LED16_OFF   0x60                      // turn off LED
#define VEN_REQ_I2C_START   4                       // set start condition on I2C bus
#define VEN_REQ_I2C_STOP    5                       // set stop condition on I2C bus

// Bulk data transfer functions
#define VEN_available()   (VEN_EP1_readByteCount)   // number of received bytes
#define VEN_ready()       (!VEN_EP1_writeBusyFlag)  // check if ready to write
uint8_t VEN_read(void);                             // read byte from BULK IN buffer
void VEN_write(uint8_t b);                          // write byte to BULK OUT buffer
void VEN_flush(void);                               // flush BULK OUT buffer

// Variables
extern volatile __bit VEN_BOOT_flag;                // bootloader flag
extern volatile __bit VEN_I2C_flag;                 // I2C active flag
extern volatile __bit VEN_LED17_flag;              // LED17 state flag
extern volatile __bit VEN_LED16_flag;              // LED16  state flag

extern volatile __xdata uint8_t VEN_EP1_readByteCount;
extern volatile __bit VEN_EP1_writeBusyFlag;
