// ===================================================================================
// USB VENDOR-Specific Control and Data Transfer Functions for CH551, CH552 and CH554
// ===================================================================================

#include "usb_vendor.h"

// ===================================================================================
// Variables and Defines
// ===================================================================================

volatile __xdata uint8_t VEN_EP1_readByteCount = 0; // number of data bytes in IN buffer
volatile __xdata uint8_t VEN_EP1_readPointer   = 0; // data pointer for fetching
volatile __bit VEN_EP1_writeBusyFlag = 0;           // flag of whether upload pointer is busy
__xdata uint8_t VEN_EP1_writePointer = 0;           // data pointer for writing

volatile __bit VEN_BOOT_flag    = 0;                // bootloader flag
volatile __bit VEN_I2C_flag     = 0;                // I2C active flag
volatile __bit VEN_LED17_flag     = 0;                // led17 state flag
volatile __bit VEN_LED16_flag     = 0;                // LED16 state flag
// ===================================================================================
// Bulk Data Transfer Functions
// ===================================================================================

// Flush the OUT buffer
void VEN_flush(void) {
  if(!VEN_EP1_writeBusyFlag && VEN_EP1_writePointer > 0) {  // not busy and buffer not empty?
    UEP1_T_LEN = VEN_EP1_writePointer;                      // number of bytes in OUT buffer
    UEP1_CTRL  = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // respond ACK
    VEN_EP1_writeBusyFlag = 1;                              // busy for now
    VEN_EP1_writePointer  = 0;                              // reset write pointer
  }
}

// Write single byte to OUT buffer
void VEN_write(uint8_t b) {
  while(!VEN_ready());                                      // wait for ready to write
  //EP1_bufHID[64 + VEN_EP1_writePointer++] = b; // write data byte
  EP1_bufHIDTX[VEN_EP1_writePointer++] = b; // write data byte
 
  if(VEN_EP1_writePointer == EP1_SIZE) VEN_flush();         // flush if buffer full
}

// Read single byte from IN buffer
uint8_t VEN_read(void) {
  uint8_t b;
  while(!VEN_available());                                  // wait for data
  b = EP1_bufHID[VEN_EP1_readPointer++];                    // get data byte
  if(--VEN_EP1_readByteCount == 0)                          // dec number of bytes in buffer
    UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;// request new data if empty
  return b;
}

// ===================================================================================
// Vendor-Specific Setup and Reset Functions
// ===================================================================================

// Setup endpoints
void VEN_setup(void) {
  UEP1_DMA    = EP1_ADDR;                   // EP1 data transfer address
  UEP1_CTRL   = bUEP_AUTO_TOG               // EP1 Auto flip sync flag
              | UEP_T_RES_NAK               // EP1 IN transaction returns NAK
              | UEP_R_RES_ACK;              // EP1 OUT transaction returns ACK
  UEP4_1_MOD  = bUEP1_RX_EN | bUEP1_TX_EN;  // EP1 double buffer
  VEN_EP1_readByteCount = 0;                // reset received bytes counter
  VEN_EP1_writeBusyFlag = 0;                // reset write busy flag
}

// ===================================================================================
// Vendor-Specific USB Control Transfers
// ===================================================================================

// Vendor-Specific USB SETUP Requests
uint8_t VEN_control(void) {
  uint8_t len;

  switch(USB_SetupReq) {

    case VEN_REQ_BOOTLOADER:                // enter bootloader
      VEN_BOOT_flag = 1;
      return 0;

    case VEN_REQ_LED17_ON:                 // turn on LED17
      VEN_LED17_flag = 1;
      return 0;
    case VEN_REQ_LED17_OFF:                // turn off 
      VEN_LED17_flag = 0;
      return 0;
    case VEN_REQ_LED16_ON:                 // turn on LED16
      VEN_LED16_flag = 1;
      return 0;
    case VEN_REQ_LED16_OFF:                // turn off 
      VEN_LED16_flag = 0;
      return 0;

    case VEN_REQ_I2C_START:                 // set start condition on I2C bus
      VEN_I2C_flag = 1;
      VEN_LED16_flag = 1;
      return 0;

    case VEN_REQ_I2C_STOP:                  // set stop condition on I2C bus
      VEN_I2C_flag = 0;
      VEN_LED16_flag = 0;
      return 0;

    #ifdef WCID_VENDOR_CODE
    case WCID_VENDOR_CODE:
      if(USB_SetupBuf->wIndexL == 0x04) {
        USB_pDescr = WCID_FEATURE_DESCR;
        len = WCID_FEATURE_DESCR[0];
        if(USB_SetupLen > len) USB_SetupLen = len;
        len = USB_SetupLen >= EP0_SIZE ? EP0_SIZE : USB_SetupLen;
        USB_EP0_copyDescr(len);
        return len;
      }
      return 0;
    #endif

    default:
      return 0xff;
  }
}

// Endpoint 0 Vendor-Specific USB IN Transfers
void VEN_EP0_IN(void) {
  uint8_t len;
  len = USB_SetupLen >= EP0_SIZE ? EP0_SIZE : USB_SetupLen;

  switch(USB_SetupReq) {

    #ifdef WCID_VENDOR_CODE
    case WCID_VENDOR_CODE:
      USB_EP0_copyDescr(len);                             
      break;
    #endif

    default:
      UEP0_CTRL = bUEP_R_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
      return;
  }

  USB_SetupLen -= len;
  UEP0_T_LEN    = len;
  UEP0_CTRL    ^= bUEP_T_TOG;
}

/*
// Endpoint 0 Vendor-Specific USB OUT Transfers
void VEN_EP0_OUT(void) {
  uint8_t len;

  switch(USB_SetupReq) {

    default:
      UEP0_CTRL = bUEP_T_TOG | UEP_T_RES_ACK | UEP_R_RES_ACK;
      return;
  }
}
*/

// ===================================================================================
// Vendor-Specific BULK Transfers
// ===================================================================================

// Endpoint 1 IN handler (bulk data transfer to host)
void VEN_EP1_IN(void) {
  UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;  // default NAK
  VEN_EP1_writeBusyFlag = 0;                                  // clear busy flag
}

// Endpoint 1 OUT handler (bulk data transfer from host)
void VEN_EP1_OUT(void) {
  if(U_TOG_OK) {                            // discard unsynchronized packets
    VEN_EP1_readByteCount = USB_RX_LEN;     // set number of received data bytes
    VEN_EP1_readPointer = 0;                // reset read pointer for fetching
    if(VEN_EP1_readByteCount) 
      UEP1_CTRL = (UEP1_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_NAK; // respond NAK after a packet. Let main code change response after handling.
  }
}
