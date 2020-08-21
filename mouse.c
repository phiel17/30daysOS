#include "bootpack.h"

#define KEYCMD_SENDTO_MOUSE		(0xd4)
#define MOUSECMD_ENABLE			(0xf4)

struct FIFO8 mousefifo;

void enable_mouse(struct MOUSE_DEC *mdec) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 3;
	return;		// ACK(0xfa) will be returned when success
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
	if (mdec->phase == 3) {
		if (dat == 0xfa) {	// wait for ACK
			mdec->phase = 0;
		}
		return 0;
	} else if (mdec->phase == 0){
		if((dat & 0xc8) == 0x08){
			mdec->buf[mdec->phase] = dat;
			mdec->phase++;
		}
		return 0;
	} else if (mdec->phase == 1){
		mdec->buf[mdec->phase] = dat;
		mdec->phase++;
		return 0;
	} else if (mdec->phase == 2){
		mdec->buf[mdec->phase] = dat;
		mdec->phase = 0;

		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if (mdec->buf[0] & 0x10) {
			mdec->x |= 0xffffff00;
		}
		if (mdec->buf[0] & 0x20) {
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y;
		return 1;
	}
	return -1;
}

void inthandler2c(int *esp) {
	io_out8(PIC1_OCW2, 0x64);	// notify irq-12 is ready to PIC
	io_out8(PIC0_OCW2, 0x62);	// notify irq-02 is ready to PIC
	unsigned char data = io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo, data);
	return;
}

