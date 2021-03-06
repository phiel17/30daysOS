#include "bootpack.h"

#define PORT_KEYSTA				(0x0064)
#define KEYSTA_SEND_NOTREADY	(0x02)
#define KEYCMD_WRITE_MODE		(0x60)
#define KBC_MODE				(0x47)

struct FIFO32 *keyfifo;
int keydata;

void wait_KBC_sendready(void) {
	for (;;) {
		if(!(io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY)){
			break;
		}
	}
	return;
}

void init_keyboard(struct FIFO32 *fifo, int data0) {
	keyfifo = fifo;
	keydata = data0;

	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

void inthandler21(int *esp) {	// PS/2 keyboard
	io_out8(PIC0_OCW2, 0x61);	// notify irq-01 is ready to PIC
	unsigned char data = io_in8(PORT_KEYDAT);
	fifo32_put(keyfifo, keydata + data);
	return;
}

