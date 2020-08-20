#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;

struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};

#define PORT_KEYDAT				(0x0060)
#define PORT_KEYSTA				(0x0064)
#define PORT_KEYCMD				(0x0064)
#define KEYSTA_SEND_NOTREADY	(0x02)
#define KEYCMD_WRITE_MODE		(0x60)
#define KBC_MODE				(0x47)

#define KEYCMD_SENDTO_MOUSE		(0xd4)
#define MOUSECMD_ENABLE			(0xf4)

void wait_KBC_sendready(void) {
	for (;;) {
		if(!(io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY)){
			break;
		}
	}
	return;
}

void init_keyboard(void) {
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

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

void HariMain(void){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADDR_BOOTINFO;
	char mcursor[256], keybuf[32], mousebuf[128];
	int mx = binfo->scrnx / 2 - 8, my = binfo->scrny / 2 - 22;
	struct MOUSE_DEC mdec;

	init_gdtidt();
	init_pic();
	io_sti();

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	char s[30];
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9);	// enable PIC1 and keyboard
	io_out8(PIC1_IMR, 0xef);	// enable mouse

	init_keyboard();
	enable_mouse(&mdec);

	for (;;) {
		io_cli();

		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
			io_stihlt();
		} else {
			if(fifo8_status(&keyfifo)) {
				int d = fifo8_get(&keyfifo);
				io_sti();

				sprintf(s, "%x", d);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo)) {
				int d = fifo8_get(&mousefifo);
				io_sti();

				if (mouse_decode(&mdec, d) == 1) {
					sprintf(s, "[lcr %x %x]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01)){
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02)){
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04)){
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					sprintf(s, "(%d, %d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
				}
			}
		}
	}
}
