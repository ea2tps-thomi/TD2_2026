PREFIX   = arm-none-eabi-
CC       = $(PREFIX)gcc
OBJCOPY  = $(PREFIX)objcopy
CFLAGS = -mcpu=cortex-m3 -mthumb -g -Wall
CFLAGS += -Isrc -I. -ICMSIS
CFLAGS += -IFreeRTOS/include -IFreeRTOS/Source/include
CFLAGS += -IFreeRTOS/portable/GCC/ARM_CM3
CFLAGS += -DSTM32F103xB
CFLAGS += -ffunction-sections -fdata-sections

CFLAGS += -MMD -MP
LDFLAGS = -T ld/STM32F103XB_FLASH.ld -nostartfiles 
LDFLAGS+=  -Wl,--gc-sections -Wl,-Map=app.map

SRCS = 	CMSIS/startup_stm32f103xb.s \
	CMSIS/system_stm32f1xx.c \
	CMSIS/syscalls.c \
	src/main.c \
	FreeRTOS/list.c \
	FreeRTOS/queue.c \
	FreeRTOS/tasks.c \
	FreeRTOS/timers.c \
       	FreeRTOS/portable/MemMang/heap_4.c \
       	FreeRTOS/portable/GCC/ARM_CM3/port.c

# Convertimos cada .c/.s a build/<nombre>.o

OBJS = $(SRCS:.c=.o)
OBJS := $(OBJS:.s=.o)

# Archivos de dependencias
DEPS = $(OBJS:.o=.d)

# Regla para C
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Regla para ensamblador
%.o: %.s
	$(CC) -c -mcpu=cortex-m3 -mthumb -o $@ $<

# Incluir dependencias de headers
-include $(DEPS)

all: app.elf app.bin app.hex

app.elf: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
app.bin: app.elf
	$(OBJCOPY) -O binary $< $@
	
app.hex: app.elf
	$(OBJCOPY) -O ihex $< $@	
	

flash: app.elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program app.elf verify reset exit"

clean:
	rm -f $(OBJS) $(DEPS) app.elf app.map

