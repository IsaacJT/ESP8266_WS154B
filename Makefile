LIB_SRC = src/esp8266_ws154b.c
LIB_OBJ = $(LIB_SRC:.c=.o)

TEST_SRC = src/test.c src/spi.c src/uart.c src/logging.c src/spi_interface.c
TEST_OBJ = $(TEST_SRC:%.c=%.o)

.DEFAULT_GOAL := libesp8266_ws154b.a

BUILD_DIR = build
SDK_DIR = /opt/xtensa/sdk

CC = xtensa-lx106-elf-gcc
AR = xtensa-lx106-elf-ar

INC = -Iinclude \
      -Iinclude/driver \
      -I$(SDK_DIR)/include

CFLAGS = -std=gnu99 \
         -nostdlib \
         -Os \
         -mlongcalls \
         -mtext-section-literals \
         -ffunction-sections \
         -fdata-sections \
         -fno-builtin-printf \
         -DICACHE_FLASH \
         -L$(SDK_DIR)/ld \
         -L$(SDK_DIR)/lib \
         -L$(BUILD_DIR) \
		 -Wall \
         $(INC)

LDLIBS = -Wl,--start-group \
         -lesp8266_ws154b \
         -lmain \
         -lnet80211 \
         -lupgrade \
         -lwpa \
         -llwip \
         -lpp \
         -lphy \
         -lc \
         -lhal \
         -lpwm \
         -lssl \
         -lcrypto \
         -lwpa2 \
         -lgcc \
         -Wl,--end-group

LDFLAGS = -Teagle.app.v6.ld

builddir:
	mkdir -p $(BUILD_DIR)

libesp8266_ws154b.a: builddir $(LIB_OBJ)
	$(CC) -g -c $(CFLAGS) $(INC) -o $(LIB_OBJ) $(LIB_SRC)
	$(AR) cvq $(BUILD_DIR)/libesp8266_ws154b.a $(LIB_OBJ)

ws154b_test: $(TEST_OBJ) builddir libesp8266_ws154b.a
	$(CC) $(TEST_OBJ) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $(BUILD_DIR)/ws154b_test

ws154b_test-0x00000.bin: ws154b_test
	esptool.py elf2image $(BUILD_DIR)/ws154b_test

test: ws154b_test
all: libesp8266_ws154b.a test
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LIB_OBJ)
	rm -f $(TEST_OBJ)

.PHONY: flash
flash: ws154b_test-0x00000.bin
	esptool.py --baud 2000000 write_flash --flash_size 4MB --flash_mode dio --flash_freq 80m 0 build/ws154b_test-0x00000.bin 0x10000 build/ws154b_test-0x10000.bin

erase:
	esptool.py erase_flash
	esptool.py write_flash 0x3fc000 $(SDK_DIR)/bin/esp_init_data_default_v08.bin
