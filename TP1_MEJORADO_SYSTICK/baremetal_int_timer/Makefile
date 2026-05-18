SRC_DIR = src
BUILD_DIR = build
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
BIN = arm-none-eabi-objcopy
CFLAGS = -mthumb -mcpu=cortex-m3 -o0 -g

all: app.bin

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

crt.o: $(BUILD_DIR) $(SRC_DIR)/crt.s
	$(AS) -o $(BUILD_DIR)/crt.o $(SRC_DIR)/crt.s

main.o: $(BUILD_DIR) $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/main.o $(SRC_DIR)/main.c

app.elf: $(BUILD_DIR) $(SRC_DIR)/linker.ld crt.o main.o
	$(LD) -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/app.elf $(BUILD_DIR)/crt.o $(BUILD_DIR)/main.o 

app.bin: app.elf
	$(BIN) -O binary $(BUILD_DIR)/app.elf $(BUILD_DIR)/app.bin

clean:
	rm -rf $(BUILD_DIR)

openocd-flash: app.bin
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg  -c "init" -c "reset halt" -c "flash write_image erase $(BUILD_DIR)/app.bin 0x08000000" -c "reset run" -c "exit"
	
openocd-erase:
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg  -c "init" -c "reset halt" -c "flash erase_sector 0 0 last" -c "reset run" -c "exit"
