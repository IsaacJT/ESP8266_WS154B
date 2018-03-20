#ifndef __ESP8266_WS154B_H__
#define __ESP8266_WS154B_H__

#include "eagle_soc.h"
#include "stdint.h"

#define FUNC_HSPI_CS0 2
#define FUNC_HSPI_MOSI 2
#define FUNC_HSPI_CLK 2

#define WS154B_HEIGHT 200
#define WS154B_WIDTH 200
#define WS154B_BUFFER_SIZE (WS154B_HEIGHT * WS154B_WIDTH / 8)

#define WS154B_SEND_COMMAND 0
#define WS154B_SEND_DATA 1

// WS154B command bytes
#define WS154B_PANEL_SETTING 0x00
#define WS154B_POWER_SETTING 0x01
#define WS154B_POWER_OFF 0x02
#define WS154B_POWER_OFF_SEQUENCE_SETTING 0x03
#define WS154B_POWER_ON 0x04
#define WS154B_POWER_ON_MEASURE 0x05
#define WS154B_BOOSTER_SOFT_START 0x06
#define WS154B_DEEP_SLEEP 0x07
#define WS154B_DATA_START_TRANSMISSION_1 0x10
#define WS154B_DATA_STOP 0x11
#define WS154B_DISPLAY_REFRESH 0x12
#define WS154B_DATA_START_TRANSMISSION_2 0x13
#define WS154B_PLL_CONTROL 0x30
#define WS154B_TEMPERATURE_SENSOR_COMMAND 0x40
#define WS154B_TEMPERATURE_SENSOR_CALIBRATION 0x41
#define WS154B_TEMPERATURE_SENSOR_WRITE 0x42
#define WS154B_TEMPERATURE_SENSOR_READ 0x43
#define WS154B_VCOM_AND_DATA_INTERVAL_SETTING 0x50
#define WS154B_LOW_POWER_DETECTION 0x51
#define WS154B_TCON_SETTING 0x60
#define WS154B_TCON_RESOLUTION 0x61
#define WS154B_SOURCE_AND_GATE_START_SETTING 0x62
#define WS154B_GET_STATUS 0x71
#define WS154B_AUTO_MEASURE_VCOM 0x80
#define WS154B_VCOM_VALUE 0x81
#define WS154B_VCM_DC_SETTING_REGISTER 0x82
#define WS154B_PROGRAM_MODE 0xA0
#define WS154B_ACTIVE_PROGRAM 0xA1
#define WS154B_READ_OTP_DATA 0xA2

typedef struct {
    int32_t periph; // e.g. PERIPHS_IO_MUX_GPIO0_U
    int32_t num; // e.g. PIN_D3 or 3
    int32_t func; // e.g. FUNC_GPIO0
} ws154b_gpio;

typedef struct {
    // GPIO output pin for the "DC" wire to the screen
    ws154b_gpio pin_dc;
    // GPIO output pin for the reset wire
    ws154b_gpio pin_rst;
    // GPIO input pin for the busy signal from the screen
    ws154b_gpio pin_busy;
    // Framebuffer for red
    uint8_t fb_red[WS154B_BUFFER_SIZE];
    // Framebuffer for black
    uint8_t fb_black[WS154B_BUFFER_SIZE];
    // Dirty bit to show if the framebuffer contents have changed
    // and the screen should be updated
    uint8_t dirty;
} ws154b;

/**
 * Performs initialisation of the device, GPIO, and SPI.
 */
extern void ws154b_init(ws154b * device);

/**
 * Sends a refresh command to the screen to instruct it to clear the screen
 * and display the data in its SRAM.
 */
extern void ws154b_refresh(ws154b * device);

/**
 * Checks if the screen is currently showing that it's busy.
 * Returns 1 if busy, 0 if not.
 */
extern uint8_t ws154b_busy(ws154b * device);

/**
 * Clears the screen by sending 0xFF (white) values for both red and black
 * and sends a refresh command.
 */
extern void ws154b_blank_screen(ws154b * device);

/**
 * Sends the contents of the framebuffers to the device and sends a refresh
 * command.
 */
extern void ws154b_update(ws154b * device);

#endif
