.PHONY: all clean

CC		= gcc
CFLAGS	= -march=i486 -m32 -nostdlib -fno-builtin -fno-pie -no-pie -g

TARGET	= haribote.img
SRCS	= $(wildcard *.c)
OBJS	= $(patsubst %c, %o, $(SRCS))
APPSRCS	= $(filter-out app/a_nasm.asm, $(wildcard app/*.asm) $(wildcard app/*.c))
APP		= $(patsubst %c, %hrb, $(patsubst %asm, %hrb, $(APPSRCS)))

all: $(TARGET) $(APP)

$(APP):
	make -C app

$(TARGET): ipl10.bin haribote.sys $(APP)
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::
	mcopy ipl10.asm -i haribote.img ::
	mcopy Makefile -i haribote.img ::
	mcopy $(APP) -i haribote.img ::

haribote.sys: asmhead.bin bootpack.hrb
	cat $^ > $@

%.bin: %.asm
	nasm $< -o $@

nasmfunc.o: nasmfunc.asm
	nasm -g -f elf $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bootpack.hrb: $(OBJS) nasmfunc.o har.ld
	$(CC) $(CFLAGS) -T har.ld $(OBJS) nasmfunc.o -Wl,-Map=bootpack.map -o $@

run: $(TARGET)
	qemu-system-i386 -fda $< -m 32 -enable-kvm

clean:
	rm -f *.img *.bin *.sys *.hrb *.o *.map app/*.hrb app/*.o
