.PHONY: clean

release:
	arm-none-eabi-g++ -c -Wall -Wextra -Os -flto -s \
	  main.cpp \
	  trace.c \
	  stdio.c \
	  gen_dds.cpp \
	  adc.cpp \
	  calc.cpp \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -lnosys \
	  -nostartfiles -ffreestanding -specs=nosys.specs -fno-rtti -fno-exceptions -pipe
	arm-none-eabi-g++ -c -Wall -Wextra -O2 -g \
	  vectors.cpp \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -lnosys \
	  -nostartfiles -ffreestanding -specs=nosys.specs -fno-rtti -fno-exceptions
	arm-none-eabi-g++ \
	  vectors.o \
	  main.o \
	  trace.o \
	  stdio.o \
	  gen_dds.o \
	  adc.o \
	  calc.o \
	  -Wl,-gc-sections -mcpu=cortex-m3 -mthumb -lnosys -nostartfiles -ffreestanding \
	  -specs=nosys.specs -TSTM32F103C8Tx_FLASH.ld -pipe -o a.elf

debug:
	arm-none-eabi-g++ -c -Wall -Wextra -O0 -g \
	  vectors.cpp \
	  main.cpp \
	  trace.c \
	  stdio.c \
	  gen_dds.cpp \
	  adc.cpp \
	  calc.cpp \
	  -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -lnosys -nostartfiles \
	  -ffreestanding -specs=nosys.specs -fno-rtti -fno-exceptions
	arm-none-eabi-g++ \
	  vectors.o \
	  main.o \
	  trace.o \
	  stdio.o \
	  gen_dds.o \
	  adc.o \
	  calc.o \
	  -Wl,-gc-sections -mcpu=cortex-m3 -mthumb -lnosys -nostartfiles -ffreestanding \
	  -specs=nosys.specs -TSTM32F103C8Tx_FLASH.ld -o a.elf

clean:
	rm *.elf
	rm *.o
