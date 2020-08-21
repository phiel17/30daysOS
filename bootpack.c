#include "bootpack.h"

extern struct FIFO8 keyfifo, mousefifo;

void make_window8(unsigned char *buf, int xsize, int ysize, char *title) {
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};

	boxfill8(buf, xsize, COL8_C6C6C6, 0,			0,			xsize - 1,	0);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,			1,			xsize - 2,	1);
	boxfill8(buf, xsize, COL8_C6C6C6, 0,			0,			0,			ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,			1,			1,			ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2,	1,			xsize - 2,	ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1,	0,			xsize - 1,	ysize - 2);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,			2,			xsize - 3,	ysize - 1);
	boxfill8(buf, xsize, COL8_000084, 3,			3,			xsize - 4,	20);
	boxfill8(buf, xsize, COL8_848484, 1,			ysize - 2,	xsize - 2,	ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,			ysize - 1,	xsize - 1,	ysize - 1);
	putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);

	for (int y = 0; y < 14; y++) {
		for (int x = 0; x < 16; x++) {
			char c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}

void HariMain(void){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADDR_BOOTINFO;
	char keybuf[32], mousebuf[128], s[32];
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEETCTL *sheetctl;
	struct SHEET *sheet_back, *sheet_mouse, *sheet_win;
	unsigned char *buf_sheet_back, buf_sheet_mouse[256], *buf_sheet_win;

	init_gdtidt();
	init_pic();
	io_sti();

	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9);	// enable PIC1 and keyboard
	io_out8(PIC1_IMR, 0xef);	// enable mouse
	init_keyboard();
	enable_mouse(&mdec);

	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	sheetctl = sheetctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sheet_back = sheet_alloc(sheetctl);
	sheet_mouse = sheet_alloc(sheetctl);
	sheet_win = sheet_alloc(sheetctl);
	buf_sheet_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_sheet_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sheet_back, buf_sheet_back, binfo->scrnx, binfo->scrny, -1);	// no transparent color
	sheet_setbuf(sheet_mouse, buf_sheet_mouse, 16, 16, 99);
	sheet_setbuf(sheet_win, buf_sheet_win, 160, 52, -1);
	init_screen8(buf_sheet_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_sheet_mouse, 99);

	make_window8(buf_sheet_win, 160, 52, "window");

	sheet_slide(sheet_back, 0, 0);
	int mx = binfo->scrnx / 2 - 8, my = binfo->scrny / 2 - 22;
	sheet_slide(sheet_mouse, mx, my);
	sheet_slide(sheet_win, 80, 72);
	sheet_updown(sheet_back, 0);
	sheet_updown(sheet_win, 1);
	sheet_updown(sheet_mouse, 2);

	sprintf(s, "memory %d MB  free: %d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc(buf_sheet_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	sheet_refresh(sheet_back, 0, 0, binfo->scrnx, 48);

	unsigned int counter = 0;
	for (;;) {
		counter++;
		sprintf(s, "%d", counter);
		boxfill8(buf_sheet_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
		putfonts8_asc(buf_sheet_win, 160, 40, 28, COL8_000000, s);
		sheet_refresh(sheet_win, 40, 28, 120, 44);

		io_cli();

		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
			io_sti();
		} else {
			if(fifo8_status(&keyfifo)) {
				int d = fifo8_get(&keyfifo);
				io_sti();

				sprintf(s, "%x", d);
				boxfill8(buf_sheet_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(buf_sheet_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
				sheet_refresh(sheet_back, 0, 16, 16, 32);
			} else if (fifo8_status(&mousefifo)) {
				int d = fifo8_get(&mousefifo);
				io_sti();

				if (mouse_decode(&mdec, d) == 1) {
					sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01)){
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02)){
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04)){
						s[2] = 'C';
					}
					boxfill8(buf_sheet_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(buf_sheet_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					sheet_refresh(sheet_back, 32, 16, 32 + 15 * 8, 32);

					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sprintf(s, "(%d, %d)", mx, my);
					boxfill8(buf_sheet_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfonts8_asc(buf_sheet_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
					sheet_refresh(sheet_back, 0, 0, 80, 16);
					sheet_slide(sheet_mouse, mx, my);
				}
			}
		}
	}
}
