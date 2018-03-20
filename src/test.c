#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "esp8266_ws154b.h"
#include "driver/uart.h"
#include "test.h"

static struct station_config station_conf;
static os_timer_t refresh_timer;
static ws154b epaper  = {
    .pin_dc = {
        .periph = WS154B_PIN_DC,
        .num = WS154B_PIN_DC_NUM,
        .func = WS154B_PIN_DC_FUNC
    },
    .pin_rst = {
        .periph = WS154B_PIN_RST,
        .num = WS154B_PIN_RST_NUM,
        .func = WS154B_PIN_RST_FUNC
    },
    .pin_busy = {
        .periph = WS154B_PIN_BUSY,
        .num = WS154B_PIN_BUSY_NUM,
        .func = WS154B_PIN_BUSY_FUNC
    },
    .fb_black = { 0 },
    .fb_red = { 0 },
    .dirty = 1
};

void refresh_timer_cb(void) {
    if (epaper.dirty) {
        ws154b_update(&epaper);
    }
    os_timer_arm(&refresh_timer, 500, 0);
}

void ICACHE_FLASH_ATTR sdk_init_done_cb(void) {
    wifi_set_opmode(NULL_MODE);
    // Initialise the buffers with a black and white pattern
    for (size_t i = 0; i < WS154B_BUFFER_SIZE; i++) {
        epaper.fb_black[i] = i % 2 ? 0xFF : 0x00;
        epaper.fb_red[i] = 0xFF;
    }
    ws154b_init(&epaper);
    os_timer_disarm(&refresh_timer);
    os_timer_setfn(&refresh_timer, refresh_timer_cb, NULL);
    os_timer_arm(&refresh_timer, 500, 0);
}

void ICACHE_FLASH_ATTR user_init(void) {
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    os_printf("SDK version:%s\n", system_get_sdk_version());
    gpio_init();
    system_init_done_cb(sdk_init_done_cb);
}

void ICACHE_FLASH_ATTR user_rf_pre_init(void) {
}

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void) {
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map)
    {
    case FLASH_SIZE_4M_MAP_256_256:
        rf_cal_sec = 128 - 8;
        break;

    case FLASH_SIZE_8M_MAP_512_512:
        rf_cal_sec = 256 - 5;
        break;

    case FLASH_SIZE_16M_MAP_512_512:
    case FLASH_SIZE_16M_MAP_1024_1024:
        rf_cal_sec = 512 - 5;
        break;

    case FLASH_SIZE_32M_MAP_512_512:
    case FLASH_SIZE_32M_MAP_1024_1024:
        rf_cal_sec = 1024 - 5;
        break;

    default:
        rf_cal_sec = 0;
        break;
    }

    return rf_cal_sec;
}
