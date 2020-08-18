.PHONY: all
all: haribote.img

haribote.img: ipl10.bin haribote.sys
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::

haribote.sys: asmhead.bin bootpack.hrb
	cat $^ > $@

ipl10.bin: ipl10.asm
	nasm $< -o $@

asmhead.bin: asmhead.asm
	nasm $< -o $@

nasmfunc.o: nasmfunc.asm
	nasm -g -f elf $< -o $@

bootpack.hrb: bootpack.c har.ld nasmfunc.o
	gcc -march=i486 -m32 -nostdlib -fno-pie -no-pie -T har.ld -g $< nasmfunc.o -o $@

run: haribote.img
	qemu-system-i386 -fda $<

.PHONY: clean
clean:
	rm -f haribote.img ipl10.bin asmhead.sys asmhead.bin haribote.sys nasmfunc.o bootpack.hrb
