#include "bootpack.h"

#define KEYCMD_LED		(0xed)

int keywin_off(struct SHEET *key_win, struct SHEET* sheet_win, int cur_c, int cur_x) {
	change_wtitle8(key_win, 0);
	if (key_win == sheet_win) {
		cur_c = -1;
		boxfill8(sheet_win->buf, sheet_win->bxsize, COL8_FFFFFF, cur_x, 28, cur_x + 7, 43);
		sheet_refresh(sheet_win, cur_x, 28, cur_x + 8, 44);
	} else {
		if (key_win->flags & 0x20) {
			fifo32_put(&key_win->task->fifo, 3);
		}
	}
	return cur_c;
}

int keywin_on(struct SHEET *key_win, struct SHEET* sheet_win, int cur_c) {
	change_wtitle8(key_win, 1);
	if (key_win == sheet_win) {
		cur_c = COL8_000000;
	} else {
		if (key_win->flags & 0x20) {
			fifo32_put(&key_win->task->fifo, 2);
		}
	}
	return cur_c;
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

	struct TASK *task_a;

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

	fifo32_init(&fifo, 128, fifobuf, 0);
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

	init_palette();
	sheetctl = sheetctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	*((int*) 0x0fe4) = (int) sheetctl;

	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);

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

	int sheet_cons_x = 768, sheet_cons_y = 645;
	sheet_cons = sheet_alloc(sheetctl);
	buf_sheet_cons = (unsigned char *) memman_alloc_4k(memman, sheet_cons_x * sheet_cons_y);
	sheet_setbuf(sheet_cons, buf_sheet_cons, sheet_cons_x, sheet_cons_y, -1);
	make_window8(buf_sheet_cons, sheet_cons->bxsize, sheet_cons->bysize, "console", 0);
	make_textbox8(sheet_cons, 8, 28, sheet_cons->bxsize - 16, sheet_cons->bysize - 28 - 8, COL8_000000);
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
	sheet_cons->task = task_cons;
	sheet_cons->flags |= 0x20;		// cursor on
	task_run(task_cons, 2, 2);

	sheet_slide(sheet_back, 0, 0);
	sheet_slide(sheet_mouse, mx, my);
	sheet_slide(sheet_cons, 64, 64);
	sheet_slide(sheet_win, 8, 16);

	sheet_updown(sheet_back, 0);
	sheet_updown(sheet_cons, 1);
	sheet_updown(sheet_win, 2);
	sheet_updown(sheet_mouse, 3);

	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);

	int key_shift = 0;
	int mmx = -1, mmy = -1;
	int cursor_x = 8, cursor_c = COL8_FFFFFF;
	struct SHEET *sheet = 0, *key_win = sheet_win;
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

			if (key_win->flags == 0) {	// window closed
				key_win = sheetctl->sheets[sheetctl->top - 1];
				cursor_c = keywin_on(key_win, sheet_win, cursor_c);
			}

			if(256 <= d && d < 512) {	// keyboard
				s[0] = (d < 0x80 + 256)? keytable[key_shift][d - 256] : 0;
				if ('A' <= s[0] && s[0] <= 'Z' && ((!(key_leds & 4) && !key_shift) || ((key_leds & 4) && key_shift))) {
					s[0] += 0x20;
				}
				if (s[0] != 0) {
					if (key_win == sheet_win){
						if (cursor_x < sheet_win->bxsize - 16) {
							s[1] = 0;
							putfonts8_asc_sht(sheet_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
							cursor_x += 8;
						}
					} else {
						fifo32_put(&task_cons->fifo, s[0] + 256);
					}
				}
				if (d == 256 + 0x0e) {	// backspace
					if (key_win == sheet_win){
						if (cursor_x > 8) {
							putfonts8_asc_sht(sheet_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
							cursor_x -= 8;
						}
					} else {
						fifo32_put(&task_cons->fifo, 8 + 256);
					}
				}
				if (d == 256 + 0x0f) {	// tab
					cursor_c = keywin_off(key_win, sheet_win, cursor_c, cursor_x);
					int j = key_win->height - 1;
					if (j == 0) {
						j = sheetctl->top - 1;
					}
					key_win = sheetctl->sheets[j];
					cursor_c = keywin_on(key_win, sheet_win, cursor_c);
				}
				if (d == 256 + 0x1c) {	// enter
					if (key_win != sheet_win) {
						fifo32_put(&task_cons->fifo, 10 + 256);
					}
				}
				if (d == 256 + 0x3b && key_shift && task_cons->tss.ss0) {	// shift + f1 to terminate
					struct CONSOLE* cons = (struct CONSOLE*) *((int*) 0x0fec);
					cons_putstr0(cons, "\nBreak(key): \n");
					io_cli();
					task_cons->tss.eax = (int) &(task_cons->tss.esp0);
					task_cons->tss.eip = (int) asm_end_app;
					io_sti();
				}
				if (d == 256 + 0x57 && sheetctl->top > 2) {
					sheet_updown(sheetctl->sheets[1], sheetctl->top - 1);
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
					sheet_slide(sheet_mouse, mx, my);

					if ((mdec.btn & 0x01)) {
						if (mmx < 0) {
							for (int i = sheetctl->top - 1; i > 0; i--) {
								sheet = sheetctl->sheets[i];
								int x = mx - sheet->vx0;
								int y = my - sheet->vy0;
								if (0 <= x && x < sheet->bxsize && 0 <= y && y < sheet->bysize) {
									if (sheet->buf[y * sheet->bxsize + x] != sheet->col_transparent) {
										sheet_updown(sheet, sheetctl->top - 1);

										if (sheet != key_win) {
											cursor_c = keywin_off(key_win, sheet_win, cursor_c, cursor_x);
											key_win = sheet;
											cursor_c = keywin_on(key_win, sheet_win, cursor_c);
										}

										if (sheet->bxsize - 21 <= x && x < sheet->bxsize - 5 && 5 <= y && y < 19) {
											if (sheet->flags & 0x10) {		// is window made by app
												struct CONSOLE* cons = (struct CONSOLE *) *((int *) 0x0fec);
												cons_putstr0(cons, "\nBreak(mouse):\n");
												io_cli();
												task_cons->tss.eax = (int) &(task_cons->tss.esp0);
												task_cons->tss.eip = (int) asm_end_app;
												io_sti();
											}
										} else if (3 <= x && x < sheet->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;
											mmy = my;
										}
										break;
									}
								}
							}
						} else {
							int x = mx - mmx;
							int y = my - mmy;
							sheet_slide(sheet, sheet->vx0 + x, sheet->vy0 + y);
							mmx = mx;
							mmy = my;
						}
					} else {
						mmx = -1;
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
