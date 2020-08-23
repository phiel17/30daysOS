#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act) {
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
	char tc, tbc;
	if (act != 0) {
		tc = COL8_FFFFFF;
		tbc = COL8_000084;
	} else {
		tc = COL8_C6C6C6;
		tbc = COL8_848484;
	}

	boxfill8(buf, xsize, COL8_C6C6C6, 0,			0,			xsize - 1,	0);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,			1,			xsize - 2,	1);
	boxfill8(buf, xsize, COL8_C6C6C6, 0,			0,			0,			ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,			1,			1,			ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2,	1,			xsize - 2,	ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1,	0,			xsize - 1,	ysize - 2);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,			2,			xsize - 3,	ysize - 1);
	boxfill8(buf, xsize, tbc		, 3,			3,			xsize - 4,	20);
	boxfill8(buf, xsize, COL8_848484, 1,			ysize - 2,	xsize - 2,	ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,			ysize - 1,	xsize - 1,	ysize - 1);
	putfonts8_asc(buf, xsize, 24, 4, tc, title);

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

void task_b_main(struct SHEET* sheet_win_b) {
	struct FIFO32 fifo;
	int fifobuf[128];

	fifo32_init(&fifo, 128, fifobuf, 0);
	struct TIMER* timer_1s = timer_alloc();
	timer_init(timer_1s, &fifo, 100);
	timer_settime(timer_1s, 100);

	int count = 0, count0 = 0;
	char s[11];
	for (;;) {
		count++;
		io_cli();

		if (fifo32_status(&fifo) == 0) {
			io_sti();
		} else {
			int data = fifo32_get(&fifo);
			io_sti();
			if (data == 100) {
				sprintf(s, "%d", count - count0);
				putfonts8_asc_sht(sheet_win_b, 24, 28, COL8_000000, COL8_C6C6C6, s, 11);
				count0 = count;
				timer_settime(timer_1s, 100);
			}
		}
	}
}

void HariMain(void){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADDR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEETCTL *sheetctl;
	struct SHEET *sheet_back, *sheet_mouse, *sheet_win, *sheet_win_b[3];
	unsigned char *buf_sheet_back, buf_sheet_mouse[256], *buf_sheet_win, *buf_sheet_win_b;
	char s[30];
	int fifobuf[128];
	struct FIFO32 fifo;
	struct TIMER *timer;

	struct TASK *task_a, *task_b[3];

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

	fifo32_init(&fifo, 32, fifobuf, 0);
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8);	// enable PIC1 and PIT, keyboard
	io_out8(PIC1_IMR, 0xef);	// enable mouse

	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);

	init_palette();
	sheetctl = sheetctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);

	sheet_back = sheet_alloc(sheetctl);
	buf_sheet_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sheet_back, buf_sheet_back, binfo->scrnx, binfo->scrny, -1);	// no transparent color
	init_screen8(buf_sheet_back, binfo->scrnx, binfo->scrny);

	sheet_mouse = sheet_alloc(sheetctl);
	sheet_setbuf(sheet_mouse, buf_sheet_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_sheet_mouse, 99);
	int mx = binfo->scrnx / 2 - 8, my = binfo->scrny / 2 - 22;

	sheet_win = sheet_alloc(sheetctl);
	buf_sheet_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sheet_win, buf_sheet_win, 160, 52, -1);
	make_window8(buf_sheet_win, 160, 52, "task_a", 1);

	for(int i = 0; i < 3; i++) {
		sheet_win_b[i] = sheet_alloc(sheetctl);
		buf_sheet_win_b = (unsigned char *) memman_alloc_4k(memman, 144 * 52);
		sheet_setbuf(sheet_win_b[i], buf_sheet_win_b, 144, 52, -1);
		sprintf(s, "task_b %d", i);
		make_window8(buf_sheet_win_b, 144, 52, s, 0);

		task_b[i] = task_alloc();
		task_b[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
		task_b[i]->tss.eip = (int) &task_b_main;
		task_b[i]->tss.es = 1 * 8;
		task_b[i]->tss.cs = 2 * 8;
		task_b[i]->tss.ss = 1 * 8;
		task_b[i]->tss.ds = 1 * 8;
		task_b[i]->tss.fs = 1 * 8;
		task_b[i]->tss.gs = 1 * 8;
		*((int *) (task_b[i]->tss.esp + 4)) = (int) sheet_win_b[i];
		task_run(task_b[i], 2, i + 1);
	}

	sheet_slide(sheet_back, 0, 0);
	sheet_slide(sheet_mouse, mx, my);
	sheet_slide(sheet_win_b[0], 168, 56);
	sheet_slide(sheet_win_b[1], 8, 116);
	sheet_slide(sheet_win_b[2], 168, 116);
	sheet_slide(sheet_win, 8, 56);

	sheet_updown(sheet_back, 0);
	sheet_updown(sheet_win_b[0], 1);
	sheet_updown(sheet_win_b[1], 2);
	sheet_updown(sheet_win_b[2], 3);
	sheet_updown(sheet_win, 4);
	sheet_updown(sheet_mouse, 5);

	timer = timer_alloc();
	timer_init(timer, &fifo, 10);
	timer_settime(timer, 50);

	sprintf(s, "memory %d MB  free: %d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sheet_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

	int temp = 0;
	char t[30] = {0};
	for (;;) {
		io_cli();

		if(fifo32_status(&fifo) == 0){
			task_sleep(task_a);
			io_sti();
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
			} else if (d == 1 || d == 0) {
				if (d) {
					timer_init(timer, &fifo, 0);
					boxfill8(buf_sheet_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
				} else {
					timer_init(timer, &fifo, 1);
					boxfill8(buf_sheet_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
				}
				timer_settime(timer, 50);
				sheet_refresh(sheet_back, 8, 96, 16, 112);
			}
		}
	}
}
