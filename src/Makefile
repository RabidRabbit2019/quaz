.PHONY: clean

release:
	arm-none-eabi-gcc -c -Wall -Wextra -Os -flto -s \
	  main.c \
	  trace.c \
	  stdio.c \
	  gen_dds.c \
	  adc.c \
	  calc.c \
	  display.c \
	  fonts/font_bmp.c \
	  fonts/font_24_26.c \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -lnosys \
	  -nostartfiles -ffreestanding -specs=nosys.specs -pipe
	arm-none-eabi-gcc -c -Wall -Wextra -Os -s \
	  vectors.c \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -lnosys \
	  -nostartfiles -ffreestanding -specs=nosys.specs -pipe
	arm-none-eabi-gcc \
	  vectors.o \
	  main.o \
	  trace.o \
	  stdio.o \
	  gen_dds.o \
	  adc.o \
	  calc.o \
	  display.o \
	  font_bmp.o \
	  font_24_26.o \
	  -Wl,-gc-sections -mcpu=cortex-m3 -mthumb -lnosys -nostartfiles -ffreestanding \
	  -specs=nosys.specs -TSTM32F103C8Tx_FLASH.ld -pipe -flto -o a.elf

debug:
	arm-none-eabi-gcc -c -Wall -Wextra -O0 -g \
	  vectors.c \
	  main.c \
	  trace.c \
	  stdio.c \
	  gen_dds.c \
	  adc.c \
	  calc.c \
	  display.c \
	  fonts/font_bmp.c \
	  fonts/font_24_26.c \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -lnosys -nostartfiles \
	  -ffreestanding -specs=nosys.specs -pipe
	arm-none-eabi-gcc \
	  vectors.o \
	  main.o \
	  trace.o \
	  stdio.o \
	  gen_dds.o \
	  adc.o \
	  calc.o \
	  display.o \
	  font_bmp.o \
	  font_24_26.o \
	  -Wl,-gc-sections -mcpu=cortex-m3 -mthumb -lnosys -nostartfiles -ffreestanding \
	  -specs=nosys.specs -TSTM32F103C8Tx_FLASH.ld -o a.elf

clean:
	rm *.elf
	rm *.o
