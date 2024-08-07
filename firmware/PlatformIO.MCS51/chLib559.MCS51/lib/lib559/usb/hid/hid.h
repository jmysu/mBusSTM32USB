// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef __hid_h__
#define __hid_h__

#include <stdbool.h>
#include <stdint.h>

#include "../usb_host.h"

enum {
  HID_TYPE_UNKNOWN,
  HID_TYPE_KEYBOARD,
  HID_TYPE_MOUSE,
  HID_TYPE_ZAPPER,
  HID_TYPE_PS3,  // mainly for dualshock3
  HID_TYPE_PS4,
  HID_TYPE_XBOX_360,
  HID_TYPE_XBOX_ONE,
  HID_TYPE_SWITCH,
  HID_TYPE_GENERIC,
};

enum {
  HID_STATE_DISCONNECTED,
  HID_STATE_CONNECTED,
  HID_STATE_NOT_READY,
  HID_STATE_SET_IDLE,
  HID_STATE_GET_REPORT,
  HID_STATE_READY,
};

enum {
  HID_BUTTON_1,
  HID_BUTTON_2,
  HID_BUTTON_3,
  HID_BUTTON_4,
  HID_BUTTON_L1,
  HID_BUTTON_R1,
  HID_BUTTON_L2,
  HID_BUTTON_R2,
  HID_BUTTON_SELECT,
  HID_BUTTON_START,
  HID_BUTTON_L3,
  HID_BUTTON_R3,

  HID_BUTTON_META,
};

struct hid_info {
  uint16_t report_desc_size;
  uint16_t report_size;
  uint16_t axis[6];
  uint16_t hat;
  uint16_t dpad[4];
  uint16_t button[13];
  uint8_t axis_size[6];
  uint8_t axis_shift[6];
  bool axis_sign[6];
  bool axis_polarity[6];
  uint8_t report_id;
  uint8_t type;
  uint8_t state;
};

struct hid {
  void (*report)(uint8_t hub,
                 const struct hid_info* hid_info,
                 const uint8_t* data,
                 uint16_t size)__reentrant;
  void (*detected)(void)__reentrant;

  uint8_t (*get_flags)(void)__reentrant;
};

void hid_init(struct hid* hid);
struct hid_info* hid_get_info(uint8_t hub);
void hid_poll(void);

#endif  // __hid_h__
