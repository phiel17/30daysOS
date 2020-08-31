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

struct TASK* open_cons_task(struct SHEET* sheet, unsigned int memtotal) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK* task = task_alloc();
	int *cons_fifobuf = (int *) memman_alloc_4k(memman, 128 * 4);

	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int) &console_task;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	*((int *) (task->tss.esp + 4)) = (int) sheet;
	*((int *) (task->tss.esp + 8)) = memtotal;
	task_run(task, 2, 2);
	fifo32_init(&task->fifo, 128, cons_fifobuf, task);

	return task;
}

struct SHEET* open_console(struct SHEETCTL* sheetctl, unsigned int memtotal) {
	const int xsize = 512, ysize = 512;

	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sheet = sheet_alloc(sheetctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
	
	sheet_setbuf(sheet, buf, xsize, ysize, -1);
	make_window8(buf, sheet->bxsize, sheet->bysize, "console", 0);
	make_textbox8(sheet, 8, 28, sheet->bxsize - 16, sheet->bysize - 28 - 8, COL8_000000);
	sheet->task = open_cons_task(sheet, memtotal);
	sheet->flags |= 0x20;		// cursor on

	return sheet;
}

void close_cons_task(struct TASK* task) {
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int)task->fifo.buf, 128 * 4);
	task->flags = 0;
	return;
}

void close_console(struct SHEET* sheet) {
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	struct TASK *task = sheet->task;
	memman_free_4k(memman, (int)sheet->buf, sheet->bxsize *sheet->bysize);
	sheet_free(sheet);
	close_cons_task(task);
	return;
}

void HariMain(void){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADDR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEETCTL *sheetctl;
	struct SHEET *sheet_back, *sheet_mouse;
	unsigned char *buf_sheet_back, buf_sheet_mouse[256];
	char s[30];
	int fifobuf[128], keycmdbuf[32];
	int keycmd_wait = -1, key_leds = (binfo->leds >> 4) & 7;
	struct FIFO32 fifo, keycmd;

	struct TASK *task_a;

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
	*((int *) 0x0fec) = (int) &fifo;
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
	task_a->langmode = 0;
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

	struct SHEET *key_win = open_console(sheetctl, memtotal);
	
	sheet_slide(sheet_back, 0, 0);
	sheet_slide(sheet_mouse, mx, my);
	sheet_slide(key_win, 32, 4);

	sheet_updown(sheet_back, 0);
	sheet_updown(key_win, 1);
	sheet_updown(sheet_mouse, 2);

	int key_shift = 0;
	int mmx = -1, mmy = -1, new_mx = -1, mmx2 = 0, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	struct SHEET *sheet = 0;
	keywin_on(key_win);

	extern char hankaku[4096];
	unsigned char *japanese;
	int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *)(ADDR_DISKIMG + 0x000200));
	struct FILEINFO *finfo = file_search("japanese.fnt", (struct FILEINFO *)(ADDR_DISKIMG + 0x002600), 224);
	if (finfo) {
		int size = finfo->size;
		japanese = file_loadfile2(finfo->clustno, &size, fat);
	} else {
		japanese = (unsigned char *)memman_alloc_4k(memman, 16 * 256 + 32 * 94 * 47);
		for (int i = 0; i < 16 * 256; i++) {
			japanese[i] = hankaku[i];
		}
		for (int i = 16 * 256; i < 16 * 256 + 32 * 94 * 47; i++) {
			japanese[i] = 0xff;
		}
	}
	*((int *)0x0fe8) = (int)japanese;
	memman_free_4k(memman, (int)fat, 4 * 2880);

	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}

		io_cli();

		if(fifo32_status(&fifo) == 0){
			if (new_mx >= 0) {
				io_sti();
				sheet_slide(sheet_mouse, new_mx, new_my);		// update mouse
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				io_sti();
				sheet_slide(sheet, new_wx, new_wy);				// update window
				new_wx = 0x7fffffff;
			} else {
				task_sleep(task_a);
				io_sti();
			}
		} else {
			int d = fifo32_get(&fifo);
			io_sti();

			if (key_win != 0 && key_win->flags == 0) {		// window closed
				if (sheetctl->top == 1) {	// last window
					key_win = 0;
				} else { 
					key_win = sheetctl->sheets[sheetctl->top - 1];
					keywin_on(key_win);
				}
			}

			if(256 <= d && d < 512) {	// keyboard
				s[0] = (d < 0x80 + 256)? keytable[key_shift][d - 256] : 0;
				if ('A' <= s[0] && s[0] <= 'Z' && ((!(key_leds & 4) && !key_shift) || ((key_leds & 4) && key_shift))) {
					s[0] += 0x20;
				}
				if (s[0] != 0 && key_win) {	// letters, backspace, enter
					fifo32_put(&key_win->task->fifo, s[0] + 256);
				}
				if (d == 256 + 0x0f && key_win) {	// tab
					keywin_off(key_win);
					int j = key_win->height - 1;
					if (j == 0) {
						j = sheetctl->top - 1;
					}
					key_win = sheetctl->sheets[j];
					keywin_on(key_win);
				}
				if (d == 256 + 0x3b && key_shift && key_win) {	// shift + f1 to terminate
					struct TASK* task = key_win->task;
					if (task && task->tss.ss0) {
						cons_putstr0(task->cons, "\nBreak(key): \n");
						io_cli();
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti();
						task_run(task, -1, 0);		// wake up to terminate
					}
				}
				if (d == 256 + 0x3c && key_shift) {	// shift + f2 to open console
					if (key_win) {
						keywin_off(key_win);
					}
					key_win = open_console(sheetctl, memtotal);
					sheet_slide(key_win, 32, 4);
					sheet_updown(key_win, sheetctl->top);
					keywin_on(key_win);
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
					new_mx = mx;
					new_my = my;

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
												task_run(task, -1, 0);		// wake up to terminate
											} else {
												struct TASK *task = sheet->task;
												sheet_updown(sheet, -1);
												keywin_off(key_win);
												key_win = sheetctl->sheets[sheetctl->top - 1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
										} else if (3 <= x && x < sheet->bxsize - 3 && 3 <= y && y < 21) {
											mmx = mx;
											mmy = my;
											mmx2 = sheet->vx0;
											new_wy = sheet->vy0;
										}
										break;
									}
								}
							}
						} else {
							int x = mx - mmx;
							int y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;
						}
					} else {
						mmx = -1;
						if (new_wx != 0x7fffffff) {
							sheet_slide(sheet, new_wx, new_wy);
							new_wx = 0x7fffffff;
						}
					}
				}
			} else if (768 <= d && d < 1024) {
				close_console(sheetctl->sheets0 + (d - 768));
			} else if (1024 <= d && d < 2024) {
				close_cons_task(taskctl->tasks0 + (d - 1024));
			}
		}
	}
}
