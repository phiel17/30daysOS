.PHONY: all

CC		= gcc
CFLAGS	= -march=i486 -m32 -nostdlib -fno-builtin -fno-pie -no-pie -g

TARGET	= haribote.img
SRCS	= $(wildcard *.c)
OBJS	= $(patsubst %c, %o, $(SRCS))

all: $(TARGET)

$(TARGET): ipl10.bin haribote.sys
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::

haribote.sys: asmhead.bin bootpack.hrb
	cat $^ > $@

%.bin: %.asm
	nasm $< -o $@

nasmfunc.o: nasmfunc.asm
	nasm -g -f elf $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bootpack.hrb: $(OBJS) nasmfunc.o har.ld
	$(CC) $(CFLAGS) -T har.ld $(OBJS) nasmfunc.o -o $@

run: $(TARGET)
	qemu-system-i386 -fda $< -m 32

.PHONY: clean
clean:
	rm -f *.img *.bin *.sys *.hrb *.o
