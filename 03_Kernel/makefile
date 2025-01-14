
CC = gcc
CFLAGS = -m32 -Wall -c -ffreestanding -fno-pic
DIRS = lib/ lib/kernel/ lib/kernel/list/ kernel/ device/ thread/ thread/semaphore/ thread/lock/
INCLUDES = $(addprefix -I, $(DIRS))
BUILD_DIR = ./build
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/timer.o \
	   $(BUILD_DIR)/debug.o $(BUILD_DIR)/print.o $(BUILD_DIR)/kernel.o \
	   $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/memory.o \
	   $(BUILD_DIR)/list.o $(BUILD_DIR)/switch.o $(BUILD_DIR)/thread.o \
	   $(BUILD_DIR)/semaphore.o $(BUILD_DIR)/lock.o $(BUILD_DIR)/console.o 
ENTRY_POINT = 0xc0001500
LD = ld
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
ASM = nasm
ASMFLAGS = -f elf
DISK_IMG = /home/atubo/bochs/hd60M.img

######## C文件编译 ########
$(BUILD_DIR)/main.o: kernel/main.c 
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h 
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h 
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h 
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/timer.o: device/timer.c device/timer.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/string.o: lib/string.c lib/string.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/list.o: lib/kernel/list/list.c lib/kernel/list/list.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/semaphore.o: thread/semaphore/semaphore.c thread/semaphore/semaphore.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/lock.o: thread/lock/lock.c thread/lock/lock.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
$(BUILD_DIR)/console.o: device/console.c device/console.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@


######## 汇编文件编译 ########
$(BUILD_DIR)/kernel.o: kernel/kernel.asm
	$(ASM) $(ASMFLAGS) $< -o $@
$(BUILD_DIR)/print.o: lib/kernel/print.asm
	$(ASM) $(ASMFLAGS) $< -o $@
$(BUILD_DIR)/switch.o: thread/switch.asm
	$(ASM) $(ASMFLAGS) $< -o $@

######## 链接所有目标文件 ########
$(BUILD_DIR)/kernel.bin: $(OBJS)
	echo "$(LD) $(LDFLAGS) $^ -o $@"
	$(LD) $(LDFLAGS) $^ -o $@

.PHONEY: mk_build_dir clean build all dd_disk

build: $(BUILD_DIR)/kernel.bin

mk_build_dir:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

dd_disk:
	dd if=build/kernel.bin of=$(DISK_IMG) bs=512 seek=9 count=200 conv=notrunc
	truncate -s %512 $(DISK_IMG)

all: mk_build_dir build dd_disk