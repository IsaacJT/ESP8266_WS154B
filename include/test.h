#ifndef __TEST_H__
#define __TEST_H__

#include "eagle_soc.h"

// pins
#define PIN_D0  16
#define PIN_D1  5
#define PIN_D2  4
#define PIN_D3  0
#define PIN_D4  2
#define PIN_D5  14
#define PIN_D6  12
#define PIN_D7  13
#define PIN_D8  15
#define PIN_D9  3
#define PIN_D10 1

#define WS154B_PIN_DC PERIPHS_IO_MUX_GPIO0_U
#define WS154B_PIN_DC_NUM PIN_D3
#define WS154B_PIN_DC_FUNC FUNC_GPIO0
#define WS154B_PIN_RST PERIPHS_IO_MUX_GPIO2_U
#define WS154B_PIN_RST_NUM PIN_D4
#define WS154B_PIN_RST_FUNC FUNC_GPIO2
#define WS154B_PIN_BUSY PERIPHS_IO_MUX_GPIO4_U
#define WS154B_PIN_BUSY_NUM PIN_D2
#define WS154B_PIN_BUSY_FUNC FUNC_GPIO4


#endif
