#include "esp8266_ws154b.h"

#include "stdint.h"
#include "logging.h"
#include "gpio.h"
#include "osapi.h"
#include "driver/spi.h"
#include "driver/spi_interface.h"

// Reference: https://www.waveshare.com/wiki/1.54inch_e-Paper_Module_(B)

// LUT values are from GxEPD Arduino library
static const uint8_t WS154B_LUT_VCOM0[] = {  0x0E  , 0x14 , 0x01 , 0x0A , 0x06 , 0x04 , 0x0A , 0x0A , 0x0F , 0x03 , 0x03 , 0x0C , 0x06 , 0x0A , 0x00 };
static const uint8_t WS154B_LUT_W[] = {  0x0E  , 0x14 , 0x01 , 0x0A , 0x46 , 0x04 , 0x8A , 0x4A , 0x0F , 0x83 , 0x43 , 0x0C , 0x86 , 0x0A , 0x04 };
static const uint8_t WS154B_LUT_B[] = {  0x0E  , 0x14 , 0x01 , 0x8A , 0x06 , 0x04 , 0x8A , 0x4A , 0x0F , 0x83 , 0x43 , 0x0C , 0x06 , 0x4A , 0x04 };
static const uint8_t WS154B_LUT_G1[] = { 0x8E  , 0x94 , 0x01 , 0x8A , 0x06 , 0x04 , 0x8A , 0x4A , 0x0F , 0x83 , 0x43 , 0x0C , 0x06 , 0x0A , 0x04 };
static const uint8_t WS154B_LUT_G2[] = { 0x8E  , 0x94 , 0x01 , 0x8A , 0x06 , 0x04 , 0x8A , 0x4A , 0x0F , 0x83 , 0x43 , 0x0C , 0x06 , 0x0A , 0x04 };
static const uint8_t WS154B_LUT_VCOM1[] = {  0x03  , 0x1D , 0x01 , 0x01 , 0x08 , 0x23 , 0x37 , 0x37 , 0x01 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 };
static const uint8_t WS154B_LUT_RED0[] = { 0x83  , 0x5D , 0x01 , 0x81 , 0x48 , 0x23 , 0x77 , 0x77 , 0x01 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 };
static const uint8_t WS154B_LUT_RED1[] = { 0x03  , 0x1D , 0x01 , 0x01 , 0x08 , 0x23 , 0x37 , 0x37 , 0x01 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 };

static void setup_spi(void);
static void power_settings(ws154b *device);
static void booster_soft_start(ws154b * device);
static void power_on(ws154b * device);
static void panel_setting(ws154b * device);
static void vcom_data_interval(ws154b * device);
static void pll_control(ws154b * device);
static void tcon_resolution(ws154b * device);
static void vcm_dc_settings(ws154b * device);
static void finish_screen_update(ws154b * device);
static void write_byte(ws154b * device, uint32_t byte, uint8_t dc);
static void write_bytes(ws154b *device, uint32_t bytes[], size_t length, uint8_t dc);
static void write_lut(ws154b * device);
static void reset_device(ws154b * device);
static void send_data(ws154b * device, uint8_t data);
static void send_command(ws154b * device, uint8_t command);
static void sleep(ws154b * device);
static void wake(ws154b * device);
static void wait(ws154b * device);

static void ws154b_log(char entry[]) {
#ifdef WS154B_LOGGING
    char buffer[256] = { '\0' };
    os_sprintf(buffer, "<WS154B> %s", entry);
    print_log(buffer);
#endif
}

static void finish_screen_update(ws154b * device) {
    os_delay_us(2000);
    ws154b_refresh(device);
    wait(device);
    sleep(device);
    device->dirty = 0;
}

static void start_screen_update(ws154b * device) {
    wake(device);
    wait(device);
    send_command(device, WS154B_DATA_START_TRANSMISSION_1);
    os_delay_us(2000);
}

void ws154b_blank_screen(ws154b * device) {
    ws154b_log("Blanking screen");
    start_screen_update(device);
    for (uint32_t i = 0; i < WS154B_BUFFER_SIZE * 2; i++) {
        send_data(device, 0xFF);
    }
    os_delay_us(2000);
    send_command(device, WS154B_DATA_START_TRANSMISSION_2);
    os_delay_us(2000);
    for (uint32_t i = 0; i < WS154B_BUFFER_SIZE; i++) {
        send_data(device, 0xFF);
    }
    finish_screen_update(device);
}

void ws154b_update(ws154b *device) {
    ws154b_log("Sending black data...");
    start_screen_update(device);
    uint8_t temp;
    for (size_t i = 0; i < WS154B_BUFFER_SIZE; i++) {
        /* From stm32 example code for the epaper controller */
        temp = 0x00;
        for (int bit = 0; bit < 4; bit++) {
            if ((device->fb_black[i] & (0x80 >> bit)) != 0) {
                temp |= 0xC0 >> (bit * 2);
            }
        }
        send_data(device, temp);
        temp = 0x00;
        for (int bit = 4; bit < 8; bit++) {
            if ((device->fb_black[i] & (0x80 >> bit)) != 0) {
                temp |= 0xC0 >> ((bit - 4) * 2);
            }
        }
        send_data(device, temp);
    }
    ws154b_log("Sent black data");
    os_delay_us(2000);
    ws154b_log("Sending red data...");
    send_command(device, WS154B_DATA_START_TRANSMISSION_2);
    os_delay_us(2000);
    for (int i = 0; i < WS154B_BUFFER_SIZE; i++) {
        send_data(device, device->fb_red[i]);
    }
    ws154b_log("Sent red data");
    finish_screen_update(device);
}

static void setup_pins(ws154b * device) {
    // Enable SPI functions
    WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI_CS0); // CS
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_HSPI_MOSI); // MOSI
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_HSPI_CLK); // CLK

    // Enable GPIO functions
    PIN_FUNC_SELECT(device->pin_dc.periph, device->pin_dc.func);
    PIN_FUNC_SELECT(device->pin_rst.periph, device->pin_rst.func);
    PIN_FUNC_SELECT(device->pin_busy.periph, device->pin_busy.func);

    // Set Busy pin to input
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(device->pin_busy.num));
}

static void wake(ws154b * device) {
    reset_device(device);
    ws154b_log("Waking device...");
    power_settings(device);
    booster_soft_start(device);
    power_on(device);
    panel_setting(device);
    vcom_data_interval(device);
    pll_control(device);
    tcon_resolution(device);
    vcm_dc_settings(device);
    write_lut(device);
    ws154b_log("Finished initialisation");
}

void ws154b_init(ws154b * device) {
    setup_pins(device);
    setup_spi();
    ws154b_update(device);
}

static void sleep(ws154b * device) {
    ws154b_log("Starting sleep mode...");
    send_command(device, WS154B_VCOM_AND_DATA_INTERVAL_SETTING);
    send_data(device, 0x17);
    send_command(device, WS154B_VCM_DC_SETTING_REGISTER);
    send_data(device, 0x00);
    send_command(device, WS154B_POWER_SETTING);
    send_data(device, 0x02);
    send_data(device, 0x00);
    send_data(device, 0x00);
    send_data(device, 0x00);
    ws154b_log("Waiting for the device to settle...");
    system_soft_wdt_feed();
    os_delay_us(1000 * 500);
    system_soft_wdt_feed();
    os_delay_us(1000 * 500);
    system_soft_wdt_feed();
    os_delay_us(1000 * 500);
    send_command((ws154b*)device, WS154B_POWER_OFF);
    ws154b_log("Sent sleep command");
}

static void reset_device(ws154b *device) {
    ws154b_log("Resetting WS154B...");
    GPIO_OUTPUT_SET(GPIO_ID_PIN(device->pin_rst.num), 0);
    os_delay_us(1000 * 200);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(device->pin_rst.num), 1);
    os_delay_us(1000 * 200);
    ws154b_log("Done.");
}


static void wait(ws154b *device) {
    if (!ws154b_busy(device)) {
        return;
    }
    ws154b_log("Waiting for busy signal to clear...");
    volatile uint16_t feeder = 0;
    while ( ws154b_busy(device) ) {
        if (!feeder++) {
            system_soft_wdt_feed();
        }
    }
    ws154b_log("Busy signal cleared");
}

void ws154b_refresh(ws154b *device) {
    ws154b_log("Refreshing screen");
    wait(device);
    send_command(device, WS154B_DISPLAY_REFRESH);
}

uint8_t ws154b_busy(ws154b * device) {
    return !GPIO_INPUT_GET(GPIO_ID_PIN(device->pin_busy.num));
}

static void write_lut(ws154b * device) {
    // From GxEPD Arduino library
    unsigned int count;
    send_command(device, 0x20);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_VCOM0[count]);
    }
    send_command(device, 0x21);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_W[count]);
    }
    send_command(device, 0x22);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_B[count]);
    }
    send_command(device, 0x23);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_G1[count]);
    }
    send_command(device, 0x24);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_G2[count]);
    }
    send_command(device, 0x25);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_VCOM1[count]);
    }
    send_command(device, 0x26);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_RED0[count]);
    }
    send_command(device, 0x27);
    for (count = 0; count < 15; count++) {
        send_data(device, WS154B_LUT_RED1[count]);
    }
}

static void power_settings(ws154b *device) {
    ws154b_log("Setting power settings");
    // Values from data sheet
    send_command(device, WS154B_POWER_SETTING);
    send_data(device, 0x7);
    send_data(device, 0x0);
    send_data(device, 0x8);
    send_data(device, 0x0);
}

static void booster_soft_start(ws154b * device) {
    ws154b_log("Setting booster soft start");
    // What is this? Taken from data sheet
    send_command(device, WS154B_BOOSTER_SOFT_START);
    send_data(device, 0x7);
    send_data(device, 0x7);
    send_data(device, 0x7);
}

static void power_on(ws154b * device) {
    ws154b_log("Sending power on command");
    send_command(device, WS154B_POWER_ON);
    wait(device);
}

static void panel_setting(ws154b * device) {
    ws154b_log("Setting panel settings");
    send_command(device, WS154B_PANEL_SETTING);
    send_data(device, 0xCF);
}

static void vcom_data_interval(ws154b * device) {
    ws154b_log("Setting VCOM data interval");
    send_command(device, WS154B_VCOM_AND_DATA_INTERVAL_SETTING);
    send_data(device, 0x37);
}

static void pll_control(ws154b * device) {
    ws154b_log("Setting PLL control");
    send_command(device, WS154B_PLL_CONTROL);
    send_data(device, 0x39);
}

static void tcon_resolution(ws154b * device) {
    ws154b_log("Setting TCON resolution");
    send_command(device, WS154B_TCON_RESOLUTION);
    send_data(device, 0xC8);
    send_data(device, 0x00);
    send_data(device, 0xC8);
}

static void vcm_dc_settings(ws154b * device) {
    ws154b_log("Setting VCM DC settings");
    send_command(device, WS154B_VCM_DC_SETTING_REGISTER);
    send_data(device, 0x0E);
}

static void setup_spi() {
    SpiAttr hSpiAttr;
    hSpiAttr.bitOrder = SpiBitOrder_MSBFirst;
    hSpiAttr.speed = SpiSpeed_10MHz;
    hSpiAttr.mode = SpiMode_Master;
    hSpiAttr.subMode = SpiSubMode_0;
    SPIInit(SpiNum_HSPI, &hSpiAttr);
}

static void write_byte(ws154b * device, uint32_t byte, uint8_t dc) {
    write_bytes(device, &byte, 1, dc);
}

static void write_bytes(ws154b *device, uint32_t bytes[], size_t length, uint8_t dc) {
    // Wait in case SPI data is still being transmitted
    while (READ_PERI_REG(SPI_CMD(HSPI))&SPI_USR);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(device->pin_dc.num), dc);
    SpiData spiData;
    spiData.cmdLen = 0;
    spiData.addrLen = 0;
    spiData.data = bytes;
    spiData.dataLen = length;
    SPIMasterSendData(SpiNum_HSPI, &spiData);
}

static void send_command(ws154b *device, uint8_t command) {
    write_byte(device, command, WS154B_SEND_COMMAND);
}

static void send_data(ws154b *device, uint8_t data) {
    write_byte(device, data, WS154B_SEND_DATA);
}
