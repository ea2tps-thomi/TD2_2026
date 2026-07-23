MAKEFLAGS += -rR
.SUFFIXES:

BUILD_DIR = build

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -g -Wall \
         -ffunction-sections -fdata-sections \
         -MMD -MP \
         -DSTM32F103xB \
         -ICMSIS

LDFLAGS = -T ld/STM32F103XB_FLASH.ld \
          -nostartfiles \
          -Wl,--gc-sections \
          -Wl,-Map=$(BUILD_DIR)/app.map

OBJS = $(BUILD_DIR)/src/main.o \
       $(BUILD_DIR)/CMSIS/syscalls.o \
       $(BUILD_DIR)/CMSIS/system_stm32f1xx.o \
       $(BUILD_DIR)/CMSIS/startup_stm32f103xb.o

DEPS = $(OBJS:.o=.d)

all: $(BUILD_DIR)/app.elf $(BUILD_DIR)/app.bin $(BUILD_DIR)/app.hex

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/src/main.o: src/main.c | $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/src
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/CMSIS/syscalls.o: CMSIS/syscalls.c | $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/CMSIS
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/CMSIS/system_stm32f1xx.o: CMSIS/system_stm32f1xx.c | $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/CMSIS
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/CMSIS/startup_stm32f103xb.o: CMSIS/startup_stm32f103xb.s | $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/CMSIS
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/app.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/app.bin: $(BUILD_DIR)/app.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/app.hex: $(BUILD_DIR)/app.elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD_DIR)/app.elf
	openocd -f interface/stlink.cfg -c "set CPUTAPID 0x2ba01477" -f target/stm32f1x.cfg -c "program $(BUILD_DIR)/app.elf verify reset exit"

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
