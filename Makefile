.PHONY: all clean app

CC		= gcc
CFLAGS	= -march=i486 -m32 -nostdlib -fno-builtin -fno-pie -no-pie
MAKE	= make --no-print-directory

TARGET		= haribote.img
SRCS		= $(wildcard haribote/*.c)
OBJS		= $(patsubst %c, %o, $(SRCS))
APPSRCS		= $(wildcard app/*.asm) $(wildcard app/*.c)
APP			= $(patsubst %c, %hrb, $(patsubst %asm, %hrb, $(APPSRCS)))
APPAPISRCS	= $(wildcard app/api/*.asm) $(wildcard app/api/*.c)
APPAPIOBJS	= $(patsubst %c, %o, $(patsubst %asm, %o, $(APPAPISRCS)))

all: $(TARGET)

$(TARGET): haribote/ipl20.bin haribote/haribote.sys japanese/japanese.fnt $(APP)
	mformat -f 1440 -C -B haribote/ipl20.bin -i $@ ::
	mcopy haribote/haribote.sys -i $@ ::
	mcopy $(APP) -i $@ ::
	mcopy japanese/japanese.fnt -i $@ ::

haribote/haribote.sys: haribote/asmhead.bin haribote/bootpack.hrb
	cat $^ > $@

haribote/%.bin: haribote/%.asm
	nasm $< -o $@

haribote/nasmfunc.o: haribote/nasmfunc.asm
	nasm -f elf $< -o $@

haribote/%.o: haribote/%.c
	$(CC) $(CFLAGS) -c $< -o $@

haribote/bootpack.hrb: $(OBJS) haribote/nasmfunc.o haribote/har.ld
	$(CC) $(CFLAGS) -T haribote/har.ld $(OBJS) haribote/nasmfunc.o -o $@

app/%.hrb: app/%.o app/apilib.a
	$(CC) $(CFLAGS) -T app/apphar.ld $^ -o $@

app/%.o: app/%.asm
	nasm -felf $^ -o $@

app/%.o: app/%.c
	$(CC) $(CFLAGS) -c $^ -o $@

app/apilib.a: $(APPAPIOBJS)
	ar r $@ $^ 

app/api/%.o: app/api/%.asm
	nasm -felf $< -o $@

app/api/%.o: app/api/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	qemu-system-i386 -fda $< -m 32 -enable-kvm

clean:
	rm -f *.img haribote/*.bin haribote/*.sys haribote/*.hrb haribote/*.o haribote/*.map app/*.hrb app/*.o app/*.a app/api/*.o
