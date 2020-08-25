.PHONY: all clean

CC		= gcc
CFLAGS	= -march=i486 -m32 -nostdlib -fno-builtin -fno-pie -no-pie -g

SRCS	= $(filter-out a_nasm.asm, $(wildcard *.asm) $(wildcard *.c))
TARGETS	= $(patsubst %c, %hrb, $(patsubst %asm, %hrb, $(SRCS)))

all: $(TARGETS)

%.hrb: %.o a_nasm.o
	$(CC) $(CFLAGS) -T apphar.ld $^ -o $@

%.o: %.asm
	nasm -felf -g $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

a_nasm.o: a_nasm.asm
	nasm -g -f elf $< -o $@

