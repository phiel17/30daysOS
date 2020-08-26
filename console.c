#include "bootpack.h"

void cons_newline(struct CONSOLE *cons) {
	if (cons->cur_y < cons->sheet->bysize - 8 - 32) {
		cons->cur_y += 16;
	} else {
		for (int y = 28; y < cons->sheet->bysize - 8 - 16; y++) {
			for (int x = 8; x < cons->sheet->bxsize - 8; x++) {
				cons->sheet->buf[x + y * cons->sheet->bxsize] = cons->sheet->buf[x + (y + 16) * cons->sheet->bxsize];
			}
		}
		for (int y = cons->sheet->bysize - 8 - 16; y < cons->sheet->bysize - 8; y++) {
			for (int x = 8; x < cons->sheet->bxsize - 8; x++) {
				cons->sheet->buf[x + y * cons->sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(cons->sheet, 8, 28, cons->sheet->bxsize - 8, cons->sheet->bysize - 8);
	}
	cons->cur_x = 8;
	return;
}

void cons_putchar(struct CONSOLE *cons, int chr, char move) {
	char s[2];
	s[0] = chr;
	s[1] = 0;

	if (s[0] == 0x09) {		// tab
		for (;;) {
			putfonts8_asc_sht(cons->sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			cons->cur_x += 8;
			if (cons->cur_x >= cons->sheet->bxsize - 8) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 8) & 0x1f) == 0) {
				break;
			}
		}
	} else if (s[0] == 0x0a) {		// LF
		cons_newline(cons);
	} else if (s[0] == 0x0d) {		// CR
		// nothing
	} else {
		putfonts8_asc_sht(cons->sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		if (move){
			cons->cur_x += 8;
			if (cons->cur_x == cons->sheet->bxsize - 8) {
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s) {
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
}

void cons_putstr1(struct CONSOLE *cons, char* s, int l) {
	for (int i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
}

void cons_cmd_mem(struct CONSOLE *cons, int memtotal) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[60];

	sprintf(s, "total   %d MB\nfree    %d KB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
}

void cons_cmd_clear(struct CONSOLE *cons) {
	for (int y = 28; y < cons->sheet->bysize - 8; y++) {
		for (int x = 8; x < cons->sheet->bxsize - 8; x++) {
			cons->sheet->buf[x + y * cons->sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(cons->sheet, 8, 28, cons->sheet->bxsize - 8, cons->sheet->bysize - 8);
	cons->cur_y = 28;
}

void cons_cmd_ls(struct CONSOLE *cons) {
	struct FILEINFO *finfo = (struct FILEINFO *) (ADDR_DISKIMG + 0x002600);
	char s[30];

	for (int i = 0; i < 224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {		// 0xe5 = deleted
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext    %d\n", finfo[i].size);
				for(int j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
}

void cons_cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline) {
	struct MEMMAN* memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search(cmdline + 4, (struct FILEINFO *) (ADDR_DISKIMG + 0x002600), 224);

	if (finfo){		// file exists
		char *p = (char*) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADDR_DISKIMG + 0x003e00));
		cons_putstr1(cons, p, finfo->size);
		memman_free_4k(memman, (int) p, finfo->size);
	} else {
		cons_putstr0(cons, "File not found.\n");
	}
	cons_newline(cons);
}

int cons_cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADDR_GDT;
	struct TASK* task = task_now();
	char name[18];

	int index;
	for (index = 0; index < 13; index++) {
		if (cmdline[index] <= ' ') {
			break;
		}
		name[index] = cmdline[index];
	}
	name[index] = 0;
	struct FILEINFO *finfo = file_search(name, (struct FILEINFO *) (ADDR_DISKIMG + 0x002600), 224);

	if (!finfo && name[index - 1] != '.') {
		name[index] = '.';
		name[index + 1] = 'H';
		name[index + 2] = 'R';
		name[index + 3] = 'B';
		name[index + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADDR_DISKIMG + 0x002600), 224);
	}

	if (finfo) {
		char *p = (char*) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADDR_DISKIMG + 0x003e00));
		if (finfo->size >= 8 && !strncmp(p + 4, "Hari", 4)) {
			int segsize = *((int *) (p + 0x0000));
			int esp = *((int *) (p + 0x000c));
			int datsize = *((int *) (p + 0x0010));
			int dathrb = *((int *) (p + 0x0014));
			char *q = (char*) memman_alloc_4k(memman, 64 * segsize);
			*((int *) 0xfe8) = (int) q;

			set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
			set_segmdesc(gdt + 1004, segsize - 1, (int) q, AR_DATA32_RW + 0x60);

			for (int i = 0; i < datsize; i++) {
				q[esp + i] = p[dathrb + i];
			}

			start_app(0x1b, 1003 * 8, 64 * 1024, 1004 * 8, &(task->tss.esp0));

			struct SHEETCTL* sheetctl = (struct SHEETCTL *) *((int *) 0x0fe4);
			for (int i = 0; i < MAX_SHEETS; i++) {
				struct SHEET* sheet = &(sheetctl->sheets0[i]);
				if (sheet->flags && sheet->task == task) {
					sheet_free(sheet);
				}
			}
			memman_free_4k(memman, (int)q, segsize);
		} else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}

		memman_free_4k(memman, (int)p, finfo->size);
		cons_newline(cons);
		return 1;
	}
	return 0;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
	if (!strcmp(cmdline, "mem")) {
		cons_cmd_mem(cons, memtotal);
	} else if(!strcmp(cmdline, "clear")) {
		cons_cmd_clear(cons);
	} else if(!strcmp(cmdline, "ls")) {
		cons_cmd_ls(cons);
	} else if(!strncmp(cmdline, "cat ", 5)) {
		cons_cmd_cat(cons, fat, cmdline);
	} else if (cmdline[0]) {
		if (!cons_cmd_app(cons, fat, cmdline)) {
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}

void console_task(struct SHEET* sheet, unsigned int memtotal) {
	struct TASK* task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct CONSOLE cons;

	cons.sheet = sheet;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_c = -1;

	*((int *) 0x0fec) = (int) &cons;

	int fifobuf[128];
	fifo32_init(&task->fifo, 128, fifobuf, task);
	cons.timer = timer_alloc();
	timer_init(cons.timer, &task->fifo, 1);
	timer_settime(cons.timer, 50);

	int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADDR_DISKIMG + 0x000200));

	cons_putchar(&cons, '>', 1);

	char cmdline[64];
	for (;;) {
		io_cli();

		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			int d = fifo32_get(&task->fifo);
			io_sti();
			if (d == 0 || d == 1) {		// cursor blink
				if (d) {
					timer_init(cons.timer, &task->fifo, 0);
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_FFFFFF;
					}
				} else {
					timer_init(cons.timer, &task->fifo, 1);
					if (cons.cur_c >= 0) {
						cons.cur_c = COL8_000000;
					}
				}
				timer_settime(cons.timer, 50);
			}
			if (d == 2) {	// cursor on
				cons.cur_c = COL8_FFFFFF;
			}
			if (d == 3) {	// cursor off
				boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				cons.cur_c = -1;
			}
			if (256 <= d && d < 512) {	// keyboard
				if (d == 8 + 256) {	// backspace
					if (cons.cur_x > 16) {
						putfonts8_asc_sht(sheet, cons.cur_x, cons.cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
						cons.cur_x -= 8;
					}
				} else if (d == 10 + 256) {		// enter
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);
					cons_putchar(&cons, '>', 1);
				} else {
					if (cons.cur_x < sheet->bxsize - 16) {
						cmdline[cons.cur_x / 8 - 2] = d - 256;
						cons_putchar(&cons, d - 256, 1);
					}
				}
			}
			if (cons.cur_c >= 0) {
				boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
			}
			sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
		}
	}
}

void hrb_api_linewin(struct SHEET *sheet, int x0, int y0, int x1, int y1, int col) {
	int dx = x1 - x0;
	int dy = y1 - y0;
	int x = x0 << 10;
	int y = y0 << 10;
	if (dx < 0) {
		dx = -dx;
	}
	if (dy < 0) {
		dy = -dy;
	}

	int len;
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) {
			dx = -1024;
		} else {
			dx = 1024;
		}
		if (y0 <= y1) {
			dy = ((y1 - y0 + 1) << 10) / len;
		} else {
			dy = ((y1 - y0 - 1) << 10) / len;
		}
	} else {
		len = dy + 1;
		if (y0 > y1) {
			dy = -1024;
		} else {
			dy = 1024;
		}
		if (x0 <= x1) {
			dx = ((x1 - x0 + 1) << 10) / len;
		} else {
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	for (int i = 0; i < len; i++) {
		sheet->buf[(y >> 10) * sheet->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}
	return;
}

int* hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
	int ds_base = *((int *) 0xfe8);
	struct TASK* task = task_now();
	struct CONSOLE *cons = (struct CONSOLE *) *((int *) 0x0fec);
	struct SHEETCTL *sheetctl = (struct SHEETCTL *) *((int *) 0x0fe4);
	int *reg = &eax + 1;
	// reg[i]: EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX

	char s[12];

	if (edx == 1) {			// console char
		cons_putchar(cons, eax & 0xff, 1);
	} else if (edx == 2) {	// console str
		cons_putstr0(cons, (char*) ebx + ds_base);
	} else if (edx == 3) {	// console str
		cons_putstr1(cons, (char*) ebx + ds_base, ecx);
	} else if (edx == 4) {	// exit
		return &(task->tss.esp0);
	} else if (edx == 5) {	// make window
		struct SHEET *sheet = sheet_alloc(sheetctl);
		sheet->task = task;
		sheet_setbuf(sheet, (char *)ebx + ds_base, esi, edi, eax);
		make_window8((char *)ebx + ds_base, esi, edi, (char *)ecx + ds_base, 0);
		sheet_slide(sheet, 100, 50);
		sheet_updown(sheet, 3);
		reg[7] = (int)sheet;
	} else if (edx == 6) {	// draw string
		struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
		putfonts8_asc(sheet->buf, sheet->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if (!(ebx & 1)) {
			sheet_refresh(sheet, esi, edi, esi + ecx * 8, edi + 16);
		}
	} else if (edx == 7) {	// draw box
		struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
		boxfill8(sheet->buf, sheet->bxsize, ebp, eax, ecx, esi, edi);
		if (!(ebx & 1)) {
			sheet_refresh(sheet, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 8) {	// memalloc init
		memman_init((struct MEMMAN*) (ebx + ds_base));
		ecx &= 0xfffffff0;
		memman_free_4k((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {	// memalloc alloc
		ecx = (ecx + 0x0f) & 0xfffffff0;
		reg[7] = memman_alloc_4k((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {	// memalloc free
		ecx = (ecx + 0x0f) & 0xfffffff0;
		memman_free_4k((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 11) {
		struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
		sheet->buf[esi + edi * sheet->bxsize] = eax;
		if (!(ebx & 1)) {
			sheet_refresh(sheet, esi, edi, esi + 1, edi + 1);
		}
	} else if (edx == 12) {
		struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
		sheet_refresh(sheet, eax, ecx, esi, edi);
	} else if (edx == 13) {
		struct SHEET *sheet = (struct SHEET *) (ebx & 0xfffffffe);
		hrb_api_linewin(sheet, eax, ecx, esi, edi, ebp);
		if (!(ebx & 1)) {
			sheet_refresh(sheet, eax, ecx, esi + 1, edi + 1);
		}
	} else if (edx == 14) {
		sheet_free((struct SHEET *) ebx);
	} else if (edx == 15) {
		for (;;) {
			io_cli();
			if (!fifo32_status(&task->fifo)) {
				if (eax) {
					task_sleep(task);
				} else {
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			int d = fifo32_get(&task->fifo);
			io_sti();
			if (d <= 1) {
				timer_init(cons->timer, &task->fifo, 1);
				timer_settime(cons->timer, 50);
			}
			if (d == 2) {
				cons->cur_c = COL8_FFFFFF;
			}
			if (d == 3) {
				cons->cur_c = -1;
			}
			if (256 <= d && d < 512) {
				reg[7] = d - 256;
				return 0;
			}
		}
	}
	return 0;
}

int* inthandler0d(int *esp) {
	struct CONSOLE* cons = (struct CONSOLE *) *((int*) 0x0fec);
	struct TASK* task = task_now();
	char s[30];

	cons_putstr0(cons, "\nINT 0D: \n General Protected Exception.\n");
	sprintf(s, "EIP = %x\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);
}

int* inthandler0c(int *esp) {
	struct CONSOLE* cons = (struct CONSOLE *) *((int*) 0x0fec);
	struct TASK* task = task_now();
	char s[30];

	cons_putstr0(cons, "\nINT 0C: \n Stack Exception.\n");
	sprintf(s, "EIP = %x\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);
}
