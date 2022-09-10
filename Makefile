CC := gcc
#CFLAGS := -std=gnu99 -Wall -Wextra -Os -nostdlib -m16 -march=i386 \
#  -Wno-unused-function \
#  -ffreestanding -fomit-frame-pointer -fwrapv -fno-strict-aliasing \
#  -fno-leading-underscore -fno-pic -fno-stack-protector \
#  -ggdb3 -m16 -ffreestanding -fno-PIE -nostartfiles

CFLAGS := -ggdb3 -m16 -ffreestanding -fno-PIE -nostartfiles -nostdlib -std=gnu99 \
	-Wall -Wextra -ffreestanding -fomit-frame-pointer -fwrapv -fno-strict-aliasing \
	-fno-stack-protector -fno-pic # -fno-leading-underscore



OPTIMISATIONFLAGS := -Os # -fno-inline-functions-called-once -fno-inline-small-functions
# read the paragraph above the `vga_pixel` function

CFILES := $(wildcard *.c)
ASMFILES := $(wildcard *.asm)

boot.bin boot.elf: $(CFILES) $(ASMFILES) Makefile linker.ld
	nasm -Ovx -g3 -F dwarf -f elf32 $(ASMFILES) -o entry.o
	$(CC) -o main.o $(CFLAGS) $(OPTIMISATIONFLAGS) -c $(CFILES)
	ld -m elf_i386 -o boot.elf -T linker.ld entry.o main.o
	objcopy -O binary boot.elf boot.bin
	./report-size boot.bin
	printf '\x55\xaa' | dd seek=510 bs=1 of=boot.bin

.PHONY: run
run: boot.bin
	qemu-system-i386 -display spice-app boot.bin

.PHONY: disassemble
disassemble: boot.elf
	objdump -Mintel -d -mi386 -Maddr16,data16 boot.elf \
		--visualize-jumps --visualize-jumps=extended-color

.PHONY: disassemble-export
disassemble-export: boot.elf
	sh -c 'objdump -Mintel -d -mi386 -Maddr16,data16 boot.elf > "asm-$$(date +"%M%S").objdump"'

.PHONY: run-debug
run-debug: boot.bin boot.elf
	qemu-system-i386 -display spice-app \
		-hda boot.bin \
		-s -S &

	gdb boot.elf \
		-ex 'target remote localhost:1234' \
		-ex 'set architecture i8086' \
		-ex 'source stepint.py' \
		-ex 'layout src' \
		-ex 'layout regs' \
		-ex 'hbreak *0x7c00' \
		-ex 'continue'

.PHONY: clean
clean: 
	rm *.o *.elf *.bin *.objdump