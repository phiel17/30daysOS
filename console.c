#include "bootpack.h"

int cons_newline(int cursor_y, struct SHEET *sheet) {
	if (cursor_y < sheet->bysize - 8 - 32) {
		cursor_y += 16;
	} else {
		for (int y = 28; y < sheet->bysize - 8 - 16; y++) {
			for (int x = 8; x < sheet->bxsize - 8; x++) {
				sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
			}
		}
		for (int y = sheet->bysize - 8 - 16; y < sheet->bysize - 8; y++) {
			for (int x = 8; x < sheet->bxsize - 8; x++) {
				sheet->buf[x + y * sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, sheet->bxsize - 8, sheet->bysize - 8);
	}
	return cursor_y;
}

void console_task(struct SHEET* sheet, unsigned int memtotal) {
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR* ) ADDR_GDT;
	struct TASK* task = task_now();
	int fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1;
	char s[64], cmdline[64];
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);

	fifo32_init(&task->fifo, 128, fifobuf, task);
	struct TIMER* timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);

	file_readfat(fat, (unsigned char *) (ADDR_DISKIMG + 0x000200));

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
					struct FILEINFO *finfo = (struct FILEINFO *) (ADDR_DISKIMG + 0x002600);

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
						for (int y = 28; y < sheet->bysize - 8; y++) {
							for (int x = 8; x < sheet->bxsize - 8; x++) {
								sheet->buf[x + y * sheet->bxsize] = COL8_000000;
							}
						}
						sheet_refresh(sheet, 8, 28, sheet->bxsize - 8, sheet->bysize - 8);
						cursor_y = 28;
					} else if (!strcmp(cmdline, "dir")){
						for (int x = 0; x < 224; x++) {
							if (finfo[x].name[0] == 0x00) {
								break;
							}
							if (finfo[x].name[0] != 0xe5) {		// 0xe5 = deleted
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
					} else if (!strncmp(cmdline, "type ", 5)) {
						for(int y = 0; y < 11; y++) {
							s[y] = ' ';
						}
						for(int x = 5, y = 0; y < 11 && cmdline[x] != 0; x++) {
							if (cmdline[x] == '.' && y <= 8) {
								y = 8;
							} else {
								s[y] = cmdline[x];
								if ('a' <= s[y] && s[y] <= 'z') {
									s[y] -= 0x20;
								}
								y++;
							}
						}
						int x;
						for (x = 0; x < 224;) {
							if (finfo[x].name[0] == 0x00) {
								break;
							}
							if ((finfo[x].type & 0x18) == 0) {
								for (int y = 0; y < 11; y++) {
									if (finfo[x].name[y] != s[y]) {
										goto type_next_file;
									}
								}
								break;
							}
						type_next_file:
							x++;
						}
						if (x < 224 && finfo[x].name[0] != 0x00) {	// file exists
							char *p = (char*) memman_alloc_4k(memman, finfo[x].size);
							file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADDR_DISKIMG + 0x003e00));
							cursor_x = 8;
							for (int x = 0; x < finfo[x].size; x++) {
								s[0] = p[x];
								s[1] = 0;

								if (s[0] == 0x09) {		// tab
									for (;;) {
										putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ", 1);
										cursor_x += 8;
										if (cursor_x == sheet->bysize - 8) {
											cursor_x = 8;
											cursor_y = cons_newline(cursor_y, sheet);
										}
										if (((cursor_x - 8) & 0x1f) == 0) {
											break;
										}
									}
								} else if (s[0] == 0x0a) {		// LF
									cursor_x = 8;
									cursor_y = cons_newline(cursor_y, sheet);
								} else if (s[0] == 0x0d) {		// CR
									// nothing
								} else {
									putfonts8_asc_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
									cursor_x += 8;
									if (cursor_x == sheet->bysize - 8) {
										cursor_x = 8;
										cursor_y = cons_newline(cursor_y, sheet);
									}
								}
							}
							memman_free_4k(memman, (int) p, finfo[x].size);
						} else {
							putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found", 15);
							cursor_y = cons_newline(cursor_y, sheet);
						}
						cursor_y = cons_newline(cursor_y, sheet);
					} else if (!strcmp(cmdline, "hlt")) {
						for (int y = 0; y < 11; y++) {
							s[y] = ' ';
						}
						s[0] = 'H';
						s[1] = 'L';
						s[2] = 'T';
						s[8] = 'H';
						s[9] = 'R';
						s[10] = 'B';
						int x;
						for (x = 0; x < 224;) {
							if (finfo[x].name[0] == 0x00) {
								break;
							}
							if ((finfo[x].type & 0x18) == 0) {
								for (int y = 0; y < 11; y++) {
									if (finfo[x].name[y] != s[y]) {
										goto hlt_next_file;
									}
								}
								break;
							}
						hlt_next_file:
							x++;
						}

						if (x < 224 && finfo[x].name[0] != 0x00) {
							char *p = (char*) memman_alloc_4k(memman, finfo[x].size);
							file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADDR_DISKIMG + 0x003e00));
							set_segmdesc(gdt + 1003, finfo[x].size - 1, (int)p, AR_CODE32_ER);
							taskswitch(0, 1003 * 8);
							memman_free_4k(memman, (int)p, finfo[x].size);
						} else {
							putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found", 15);
							cursor_y = cons_newline(cursor_y, sheet);
						}
					} else if (cmdline[0] != 0) {
						putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Bad command.", 30);
						cursor_y = cons_newline(cursor_y, sheet);
						cursor_y = cons_newline(cursor_y, sheet);
					}
					putfonts8_asc_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">", 1);
					cursor_x = 16;
				} else {
					if (cursor_x < sheet->bysize - 16) {
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
