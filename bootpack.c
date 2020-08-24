#include "bootpack.h"

#define KEYCMD_LED		(0xed)

struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};

void make_windowtitle8(unsigned char *buf, int xsize, char *title, char act) {
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

	boxfill8(buf, xsize, tbc		, 3,			3,			xsize - 4,	20);
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
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act) {
	boxfill8(buf, xsize, COL8_C6C6C6, 0,			0,			xsize - 1,	0);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,			1,			xsize - 2,	1);
	boxfill8(buf, xsize, COL8_C6C6C6, 0,			0,			0,			ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,			1,			1,			ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2,	1,			xsize - 2,	ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1,	0,			xsize - 1,	ysize - 2);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,			2,			xsize - 3,	ysize - 1);
	boxfill8(buf, xsize, COL8_848484, 1,			ysize - 2,	xsize - 2,	ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,			ysize - 1,	xsize - 1,	ysize - 1);
	make_windowtitle8(buf, xsize, title, act);
	return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	return;
}

int cons_newline(int cursor_y, struct SHEET *sheet) {
	if (cursor_y < 28 + 112) {
		cursor_y += 16;
	} else {
		for (int y = 28; y < 28 + 112; y++) {
			for (int x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
			}
		}
		for (int y = 28 + 112; y < 28 + 128; y++) {
			for (int x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	}
	return cursor_y;
}

void console_task(struct SHEET* sheet, unsigned int memtotal) {
	struct TASK* task = task_now();
	int fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
	char s[30], cmdline[30];
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

	fifo32_init(&task->fifo, 128, fifobuf, task);
	struct TIMER* timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);

	putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">", 1);

	for (;;) {
		io_cli();

		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			int d = fifo32_get(&task->fifo);
			io_sti();
			if (d == 0 || d == 1) {		// cursor
				if (d) {
					timer_init(timer, &task->fifo, 0);
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
					}
				} else {
					timer_init(timer, &task->fifo, 1);
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
					}
				}
				timer_settime(timer, 50);
			}
			if (d == 2) {
				cursor_c = COL8_FFFFFF;
			}
			if (d == 3) {
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
				cursor_c = -1;
			}
			if (256 <= d && d < 512) {	// keyboard
				if (d == 8 + 256) {	// backspace
					if (cursor_x > 16) {
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
						cursor_x -= 8;
					}
				} else if (d == 10 + 256) {
					putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
					cmdline[cursor_x / 8 - 2] = 0;
					cursor_y = cons_newline(cursor_y, sheet);

					// mem command
					if (!strcmp(cmdline, "mem")) {
						sprintf(s, "total   %d MB", memtotal / (1024 * 1024));
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						sprintf(s, "free    %d MB", memman_total(memman) / 1024);
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					} else if (!strcmp(cmdline, "cls")) {
						for (int y = 28; y < 28 + 128; y++) {
							for (int x = 8; x < 8 + 240; x++) {
								sheet->buf[x + y * sheet->bxsize] = COL8_000000;
							}
						}
						sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
						cursor_y = 28;
					} else if (!strcmp(cmdline, "dir")){
						struct FILEINFO *finfo = (struct FILEINFO *) (ADDR_DISKIMG + 0x002600);

						for (int x = 0; x < 224; x++) {
							if (finfo[x].name[0] == 0x00) {
								break;
							}
							if (finfo[x].name[0] != 0xe5) {
								if ((finfo[x].type & 0x18) == 0) {
									sprintf(s, "filename.ext    %d", finfo[x].size);
									for(int y = 0; y < 8; y++) {
										s[y] = finfo[x].name[y];
									}
									s[9] = finfo[x].ext[0];
									s[10] = finfo[x].ext[1];
									s[11] = finfo[x].ext[2];
									putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s, 30);
									cursor_y = cons_newline(cursor_y, sheet);
								}
							}
						}
						cursor_y = cons_newline(cursor_y, sheet);
					} else if (cmdline[0] != 0) {
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command.", 30);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}
					putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
					cursor_x = 16;
				} else {
					if (cursor_x < 240) {
						s[0] = d - 256;
						s[1] = 0;
						cmdline[cursor_x / 8 - 2] = d - 256;
						cmdline[cursor_x / 8 - 1] = 0;
						putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
						cursor_x += 8;
					}
				}
			}
			if (cursor_c >= 0) {
				boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
			}
			sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
		}
	}
}

void HariMain(void){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADDR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEETCTL *sheetctl;
	struct SHEET *sheet_back, *sheet_mouse, *sheet_win, *sheet_cons;
	unsigned char *buf_sheet_back, buf_sheet_mouse[256], *buf_sheet_win, *buf_sheet_cons;
	char s[30];
	int fifobuf[128], keycmdbuf[32];
	int keycmd_wait = -1, key_leds = (binfo->leds >> 4) & 7;
	struct FIFO32 fifo, keycmd;
	struct TIMER *timer;

	struct TASK *task_a, *cons;

	static char keytable[2][0x80] = {
		{
			0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
			'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
			'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
			'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
		}, {
			0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0,   0,
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
			'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
			'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
			'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
		}
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

	fifo32_init(&keycmd, 32, keycmdbuf, 0);
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);

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
	make_textbox8(sheet_win, 8, 28, 128, 16, COL8_FFFFFF);

	sheet_cons = sheet_alloc(sheetctl);
	buf_sheet_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sheet_cons, buf_sheet_cons, 256, 165, -1);
	make_window8(buf_sheet_cons, 256, 165, "console", 0);
	make_textbox8(sheet_cons, 8, 28, 240, 128, COL8_000000);
	struct TASK* task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_cons->tss.eip = (int) &console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	*((int *) (task_cons->tss.esp + 4)) = (int) sheet_cons;
	*((int *) (task_cons->tss.esp + 8)) = (int) memtotal;
	task_run(task_cons, 2, 2);

	sheet_slide(sheet_back, 0, 0);
	sheet_slide(sheet_mouse, mx, my);
	sheet_slide(sheet_cons, 64, 128);
	sheet_slide(sheet_win, 8, 56);

	sheet_updown(sheet_back, 0);
	sheet_updown(sheet_cons, 1);
	sheet_updown(sheet_win, 2);
	sheet_updown(sheet_mouse, 3);

	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);

	// sprintf(s, "memory %d MB  free: %d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	// putfonts8_asc_sht(sheet_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);

	int key_to = 0, key_shift = 0;
	int cursor_x = 8, cursor_c = COL8_FFFFFF;
	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}

		io_cli();

		if(fifo32_status(&fifo) == 0){
			task_sleep(task_a);
			io_sti();
		} else {
			int d = fifo32_get(&fifo);
			io_sti();

			if(256 <= d && d < 512) {	// keyboard
				// sprintf(s, "%x", d - 256);
				// putfonts8_asc_sht(sheet_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);

				s[0] = (d < 0x80 + 256)? keytable[key_shift][d - 256] : 0;
				if ('A' <= s[0] && s[0] <= 'Z' && ((!(key_leds & 4) && !key_shift) || ((key_leds & 4) && key_shift))) {
					s[0] += 0x20;
				}
				if (s[0] != 0) {
					if (key_to == 0){
						s[1] = 0;
						putfonts8_asc_sht(sheet_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
						cursor_x += 8;
					} else {
						fifo32_put(&task_cons->fifo, s[0] + 256);
					}
				}
				if (d == 256 + 0x0e) {	// backspace
					if (key_to == 0){
						if (cursor_x > 8) {
							putfonts8_asc_sht(sheet_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					} else {
						fifo32_put(&task_cons->fifo, 8 + 256);
					}
				}
				if (d == 256 + 0x0f) {	// tab
					if (key_to == 0) {
						key_to = 1;
						make_windowtitle8(buf_sheet_win, sheet_win->bxsize, "task_a", 0);
						make_windowtitle8(buf_sheet_cons, sheet_cons->bxsize, "console", 1);
						cursor_c = -1;
						boxfill8(sheet_win->buf, sheet_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
						fifo32_put(&task_cons->fifo, 2);
					} else {
						key_to = 0;
						make_windowtitle8(buf_sheet_win, sheet_win->bxsize, "task_a", 1);
						make_windowtitle8(buf_sheet_cons, sheet_cons->bxsize, "console", 0);
						cursor_c = COL8_000000;
						fifo32_put(&task_cons->fifo, 3);
					}
					sheet_refresh(sheet_win, 0, 0, sheet_win->bxsize, 21);
					sheet_refresh(sheet_cons, 0, 0, sheet_cons->bxsize, 21);
				}
				if (d == 256 + 0x1c) {
					if (key_to != 0) {
						fifo32_put(&task_cons->fifo, 10 + 256);
					}
				}

				if (d == 256+ 0x2a) {	// left shift on
					key_shift |= 1;
				}
				if (d == 256+ 0x36) {	// right shift on
					key_shift |= 2;
				}
				if (d == 256+ 0xaa) {	// left shift off
					key_shift &= ~1;
				}
				if (d == 256+ 0xb6) {	// right shift off
					key_shift &= ~2;
				}
				if (d == 256 + 0x3a) {	/* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (d == 256 + 0x45) {	/* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (d == 256 + 0x46) {	/* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (d == 256 + 0xfa) {	// keyboard successfully get data
					keycmd_wait = -1;
				}
				if (d == 256 + 0xfe) {	// keyboard failed to get data
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);
				}
				if (cursor_c >= 0) {
					boxfill8(sheet_win->buf, sheet_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				}
				sheet_refresh(sheet_win, cursor_x, 28, cursor_x + 8, 44);
			} else if (512 <= d && d < 768) {	// mouse
				if (mouse_decode(&mdec, d - 512) == 1) {
					// sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
					// if ((mdec.btn & 0x01)){
					// 	s[1] = 'L';
					// }
					// if ((mdec.btn & 0x02)){
					// 	s[3] = 'R';
					// }
					// if ((mdec.btn & 0x04)){
					// 	s[2] = 'C';
					// }
					// putfonts8_asc_sht(sheet_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);

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
					// sprintf(s, "(%d, %d)", mx, my);
					// putfonts8_asc_sht(sheet_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
					sheet_slide(sheet_mouse, mx, my);

					if ((mdec.btn & 0x01)) {
						sheet_slide(sheet_win, mx - 80, my - 8);
					}
				}
			} else if (d == 1 || d == 0) {
				if (d) {
					timer_init(timer, &fifo, 0);
					if (cursor_c >= 0){
						cursor_c = COL8_000000;
					}
				} else {
					timer_init(timer, &fifo, 1);
					if (cursor_c >= 0){
						cursor_c = COL8_FFFFFF;
					}
				}
				timer_settime(timer, 50);
				if (cursor_c >= 0){
					boxfill8(buf_sheet_win, sheet_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sheet_win, cursor_x, 28, cursor_x + 8, 44);
				}
			}
		}
	}
}
