#include "bootpack.h"

void init_pic(void) {
	// interrupt mask register
	io_out8(PIC0_IMR, 0xff);	// no interrupt
	io_out8(PIC1_IMR, 0xff);

	// initial control word
	io_out8(PIC0_ICW1, 0x11);	// edge trigger mode
	io_out8(PIC0_ICW2, 0x20);	// IRQ0-7 to INT20-27
	io_out8(PIC0_ICW3, 1 << 2);	// PIC1 to PIC2
	io_out8(PIC0_ICW4, 0x01);	// non-buffer mode

	io_out8(PIC1_ICW1, 0x11);	// edge trigger mode
	io_out8(PIC1_ICW2, 0x28);	// IRQ0-7 to INT20-27
	io_out8(PIC1_ICW3, 2);		// PIC1 to PIC2
	io_out8(PIC1_ICW4, 0x01);	// non-buffer mode

	io_out8(PIC0_IMR, 0xfb);	// disable except PIC1
	io_out8(PIC1_IMR, 0xff);
}

void inthandler27(int *esp) {
	io_out8(PIC0_OCW2, 0x67);
	return;
}