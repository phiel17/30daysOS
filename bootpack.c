#include "bootpack.h"

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
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEETCTL *sheetctl;
	struct SHEET *sheet_back, *sheet_mouse, *sheet_win;
	unsigned char *buf_sheet_back, buf_sheet_mouse[256], *buf_sheet_win;
	char s[30];
	int fifobuf[128];
	struct FIFO32 fifo;
	struct TIMER *timer1, *timer2, *timer3;

	static char keytable[0x54] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'
	};


	init_gdtidt();
	init_pic();
	io_sti();

	fifo32_init(&fifo, 32, fifobuf);
	init_pit();
	io_out8(PIC0_IMR, 0xf8);	// enable PIC1 and PIT, keyboard
	io_out8(PIC1_IMR, 0xef);	// enable mouse
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);

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

	int mx = binfo->scrnx / 2 - 8, my = binfo->scrny / 2 - 22;
	make_window8(buf_sheet_win, 160, 52, "window");
	sheet_slide(sheet_back, 0, 0);
	sheet_slide(sheet_mouse, mx, my);
	sheet_slide(sheet_win, 80, 72);
	sheet_updown(sheet_back, 0);
	sheet_updown(sheet_win, 1);
	sheet_updown(sheet_mouse, 2);

	timer1 = timer_alloc();
	timer_init(timer1, &fifo, 10);
	timer_settime(timer1, 1000);

	timer2 = timer_alloc();
	timer_init(timer2, &fifo, 3);
	timer_settime(timer2, 300);

	timer3 = timer_alloc();
	timer_init(timer3, &fifo, 1);
	timer_settime(timer3, 50);

	sprintf(s, "memory %d MB  free: %d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sheet_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

	int temp = 0;
	char t[30] = {0};
	for (;;) {
		io_cli();

		if(fifo32_status(&fifo) == 0){
			io_stihlt();
		} else {
			int d = fifo32_get(&fifo);
			io_sti();

			if(256 <= d && d < 512) {	// keyboard
				sprintf(s, "%x", d - 256);
				putfonts8_asc_sht(sheet_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);

				if (d < 256 + 0x54 && keytable[d - 256]) {
					t[temp] = keytable[d - 256];
					t[temp + 1] = 0;
					putfonts8_asc_sht(sheet_win, 20, 28, COL8_000000, COL8_C6C6C6, t, 13);
					temp++;
				}
				if (d == 256 + 0x0e && temp > 0) {
					temp--;
					t[temp] = 0;
					putfonts8_asc_sht(sheet_win, 20, 28, COL8_000000, COL8_C6C6C6, t, 13);
				}
			} else if (512 <= d && d < 768) {	// mouse
				if (mouse_decode(&mdec, d - 512) == 1) {
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
					putfonts8_asc_sht(sheet_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);

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
					putfonts8_asc_sht(sheet_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
					sheet_slide(sheet_mouse, mx, my);

					if ((mdec.btn & 0x01)) {
						sheet_slide(sheet_win, mx - 80, my - 8);
					}
				}
			} else if (d == 10) {		// timers
				putfonts8_asc_sht(sheet_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]", 7);
			} else if (d == 3) {
				putfonts8_asc_sht(sheet_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]", 6);
			} else if (d == 1 || d == 0) {
				if (d) {
					timer_init(timer3, &fifo, 0);
					boxfill8(buf_sheet_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
				} else {
					timer_init(timer3, &fifo, 1);
					boxfill8(buf_sheet_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
				}
				timer_settime(timer3, 50);
				sheet_refresh(sheet_back, 8, 96, 16, 112);
			}
		}
	}
}
