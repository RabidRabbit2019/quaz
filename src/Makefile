.PHONY: clean

release:
	arm-none-eabi-gcc -c -Wall -Wextra -Os -flto -s \
	  main.c \
	  trace.c \
	  stdio.c \
	  adc.c \
	  gen_dds.c \
	  settings.c \
	  display.c \
	  buttons.c \
	  fonts/font_bmp.c \
	  fonts/font_25_30.c \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m4 -mthumb -lnosys \
	  -nostartfiles -ffreestanding -specs=nosys.specs -pipe
	arm-none-eabi-gcc -c -Wall -Wextra -Os -s \
	  vectors.c \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m4 -mthumb -lnosys \
	  -nostartfiles -ffreestanding -specs=nosys.specs -pipe
	arm-none-eabi-gcc \
	  vectors.o \
	  main.o \
	  trace.o \
	  stdio.o \
	  adc.o \
	  gen_dds.o \
	  settings.o \
	  display.o \
	  buttons.o \
	  font_bmp.o \
	  font_25_30.o \
	  -Wl,-gc-sections -mcpu=cortex-m4 -mthumb -lnosys -nostartfiles -ffreestanding \
	  -specs=nosys.specs -TSTM32G431CBU6_FLASH.ld -pipe -flto -o a.elf

debug:
	arm-none-eabi-gcc -c -Wall -Wextra -O0 -g \
	  vectors.c \
	  main.c \
	  trace.c \
	  stdio.c \
	  adc.c \
	  gen_dds.c \
	  settings.c \
	  display.c \
	  buttons.c \
	  fonts/font_bmp.c \
	  fonts/font_25_30.c \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m4 -mthumb -lnosys -nostartfiles \
	  -ffreestanding -specs=nosys.specs -pipe
	arm-none-eabi-gcc \
	  vectors.o \
	  main.o \
	  trace.o \
	  stdio.o \
	  adc.o \
	  gen_dds.o \
	  settings.o \
	  display.o \
	  buttons.o \
	  font_bmp.o \
	  font_25_30.o \
	  -Wl,-gc-sections -mcpu=cortex-m4 -mthumb -lnosys -nostartfiles -ffreestanding \
	  -specs=nosys.specs -TSTM32G431CBU6_FLASH.ld -o a.elf

clean:
	rm *.elf
	rm *.o
