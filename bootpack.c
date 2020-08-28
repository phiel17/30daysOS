#include "bootpack.h"

#define KEYCMD_LED		(0xed)

void keywin_off(struct SHEET *key_win) {
	change_wtitle8(key_win, 0);
	if (key_win->flags & 0x20) {
		fifo32_put(&key_win->task->fifo, 3);
	}
	return;
}

void keywin_on(struct SHEET *key_win) {
	change_wtitle8(key_win, 1);
	if (key_win->flags & 0x20) {
		fifo32_put(&key_win->task->fifo, 2);
	}
	return;
}

void HariMain(void){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADDR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEETCTL *sheetctl;
	struct SHEET *sheet_back, *sheet_mouse, *sheet_cons[2];
	unsigned char *buf_sheet_back, buf_sheet_mouse[256], *buf_sheet_cons[2];
	char s[30];
	int fifobuf[128], keycmdbuf[32], *cons_fifobuf[2];
	int keycmd_wait = -1, key_leds = (binfo->leds >> 4) & 7;
	struct FIFO32 fifo, keycmd;

	struct TASK *task_a, *task_cons[2];

	static char keytable[2][0x80] = {
		{
			0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08,   0,
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a,   0,   'A', 'S',
			'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
			'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
			'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
		}, {
			0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08,   0,
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a,   0,   'A', 'S',
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

	int sheet_cons_x = 256, sheet_cons_y = 256;
	for (int i = 0; i < 2; i++) {
		sheet_cons[i] = sheet_alloc(sheetctl);
		buf_sheet_cons[i] = (unsigned char *) memman_alloc_4k(memman, sheet_cons_x * sheet_cons_y);
		sheet_setbuf(sheet_cons[i], buf_sheet_cons[i], sheet_cons_x, sheet_cons_y, -1);
		make_window8(buf_sheet_cons[i], sheet_cons[i]->bxsize, sheet_cons[i]->bysize, "console", 0);
		make_textbox8(sheet_cons[i], 8, 28, sheet_cons[i]->bxsize - 16, sheet_cons[i]->bysize - 28 - 8, COL8_000000);
		task_cons[i] = task_alloc();
		task_cons[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
		task_cons[i]->tss.eip = (int) &console_task;
		task_cons[i]->tss.es = 1 * 8;
		task_cons[i]->tss.cs = 2 * 8;
		task_cons[i]->tss.ss = 1 * 8;
		task_cons[i]->tss.ds = 1 * 8;
		task_cons[i]->tss.fs = 1 * 8;
		task_cons[i]->tss.gs = 1 * 8;
		*((int *) (task_cons[i]->tss.esp + 4)) = (int) sheet_cons[i];
		*((int *) (task_cons[i]->tss.esp + 8)) = (int) memtotal;
		sheet_cons[i]->task = task_cons[i];
		sheet_cons[i]->flags |= 0x20;		// cursor on
		cons_fifobuf[i] = (int *) memman_alloc_4k(memman, 128 * 4);
		fifo32_init(&task_cons[i]->fifo, 128, cons_fifobuf[i], task_cons[i]);

		task_run(task_cons[i], 2, 2);
	}

	sheet_slide(sheet_back, 0, 0);
	sheet_slide(sheet_mouse, mx, my);
	sheet_slide(sheet_cons[0], 64, 64);
	sheet_slide(sheet_cons[1], 128, 128);

	sheet_updown(sheet_back, 0);
	sheet_updown(sheet_cons[0], 1);
	sheet_updown(sheet_cons[1], 2);
	sheet_updown(sheet_mouse, 3);

	int key_shift = 0;
	int mmx = -1, mmy = -1;
	struct SHEET *sheet = 0, *key_win = sheet_cons[0];

	keywin_on(key_win);
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
				keywin_on(key_win);
			}

			if(256 <= d && d < 512) {	// keyboard
				s[0] = (d < 0x80 + 256)? keytable[key_shift][d - 256] : 0;
				if ('A' <= s[0] && s[0] <= 'Z' && ((!(key_leds & 4) && !key_shift) || ((key_leds & 4) && key_shift))) {
					s[0] += 0x20;
				}
				if (s[0] != 0) {
					fifo32_put(&key_win->task->fifo, s[0] + 256);
				}
				if (d == 256 + 0x0f) {	// tab
					keywin_off(key_win);
					int j = key_win->height - 1;
					if (j == 0) {
						j = sheetctl->top - 1;
					}
					key_win = sheetctl->sheets[j];
					keywin_on(key_win);
				}
				if (d == 256 + 0x3b && key_shift) {	// shift + f1 to terminate
					struct TASK* task = key_win->task;
					if (task && task->tss.ss0) {
						cons_putstr0(task->cons, "\nBreak(key): \n");
						io_cli();
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti();
					}
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
											keywin_off(key_win);
											key_win = sheet;
											keywin_on(key_win);
										}

										if (sheet->bxsize - 21 <= x && x < sheet->bxsize - 5 && 5 <= y && y < 19) {
											if (sheet->flags & 0x10) {		// is window made by app
												struct TASK* task = sheet->task;
												cons_putstr0(task->cons, "\nBreak(mouse):\n");
												io_cli();
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
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
			}
		}
	}
}
