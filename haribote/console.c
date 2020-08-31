#include "bootpack.h"

void cons_newline(struct CONSOLE *cons) {
	struct TASK *task = task_now();

	if (cons->cur_y < cons->sheet->bysize - 8 - 32 || !cons->sheet) {
		cons->cur_y += 16;
	} else {
		if (cons->sheet) {
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
	}
	cons->cur_x = 8;
	if (task->langmode == 1 && task->langbyte1 != 0) {
		cons->cur_x += 8;
	}
	return;
}

void cons_putchar(struct CONSOLE *cons, int chr, char move) {
	char s[2];
	s[0] = chr;
	s[1] = 0;

	if (s[0] == 0x09) {		// tab
		for (;;) {
			if (cons->sheet) {
				putfonts8_asc_sht(cons->sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			}
			cons->cur_x += 8;
			if (cons->cur_x >= cons->sheet->bxsize - 8 && cons->sheet) {
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
		if (cons->sheet) {
			putfonts8_asc_sht(cons->sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		}
		if (move){
			cons->cur_x += 8;
			if (cons->cur_x == cons->sheet->bxsize - 8 && cons->sheet) {
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

void cons_cmd_exit(struct CONSOLE* cons, int *fat) {
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct SHEETCTL *sheetctl = (struct SHEETCTL *)*((int *)0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *)*((int *)0x0fec);
	
	if (cons->sheet) {
		timer_cancel(cons->timer);
	}
	memman_free_4k(memman, (int)fat, 4 * 2880);
	io_cli();
	if (cons->sheet) {
		fifo32_put(fifo, cons->sheet - sheetctl->sheets0 + 768);
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);
	}
	io_sti();
	for (;;) {
		task_sleep(task);
	}
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
		int appsize = finfo->size;
		char *p = file_loadfile2(finfo->clustno, &appsize, fat);
		if (appsize >= 36 && !strncmp(p + 4, "Hari", 4) && *p == 0x00) {
			int segsize = *((int *) (p + 0x0000));
			int esp = *((int *) (p + 0x000c));
			int datsize = *((int *) (p + 0x0010));
			int dathrb = *((int *) (p + 0x0014));
			char *q = (char*) memman_alloc_4k(memman, segsize);
			task->ds_base = (int) q;

			set_segmdesc(task->ldt + 0, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsize - 1, (int) q, AR_DATA32_RW + 0x60);

			for (int i = 0; i < datsize; i++) {
				q[esp + i] = p[dathrb + i];
			}

			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));

			struct SHEETCTL* sheetctl = (struct SHEETCTL *) *((int *) 0x0fe4);
			for (int i = 0; i < MAX_SHEETS; i++) {
				struct SHEET* sheet = &(sheetctl->sheets0[i]);
				if ((sheet->flags & 0x11) == 0x11 && sheet->task == task) {
					sheet_free(sheet);
				}
			}
			for (int i = 0; i < 8; i++) {
				if (task->fhandle[i].buf) {
					memman_free_4k(memman, (int)task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int)q, segsize);
			task->langbyte1 = 0;
		} else {
			cons_putstr0(cons, ".hrb file format error.\n");
		}

		memman_free_4k(memman, (int)p, appsize);
		cons_newline(cons);
		return 1;
	}
	return 0;
}

void cons_cmd_start(struct CONSOLE* cons, char *cmdline, int memtotal) {
	struct SHEETCTL *sheetctl = (struct SHEETCTL *)*((int *)0x0fe4);
	struct SHEET *sheet = open_console(sheetctl, memtotal);
	struct FIFO32 *fifo = &sheet->task->fifo;
	sheet_slide(sheet, 32, 4);
	sheet_updown(sheet, sheetctl->top);
	for (int i = 0; cmdline[i]; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);
	cons_newline(cons);
	return;
}

void cons_cmd_ncst(struct CONSOLE* cons, char *cmdline, int memtotal) {
	struct TASK *task = open_cons_task(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	for (int i = 0; cmdline[i]; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);
	cons_newline(cons);
	return;
}

void cons_cmd_langmode(struct CONSOLE* cons, char *cmdline) {
	struct TASK *task = task_now();
	unsigned char mode = cmdline[0] - '0';
	if (mode <= 2) {
		task->langmode = mode;
	} else {
		cons_putstr0(cons, "mode error\n");
	}
	cons_newline(cons);
	return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
	if (!strcmp(cmdline, "mem") && cons->sheet) {
		cons_cmd_mem(cons, memtotal);
	} else if(!strcmp(cmdline, "clear") && cons->sheet) {
		cons_cmd_clear(cons);
	} else if(!strcmp(cmdline, "ls") && cons->sheet) {
		cons_cmd_ls(cons);
	} else if (!strcmp(cmdline, "exit")) {
		cons_cmd_exit(cons, fat);
	} else if (!strncmp(cmdline, "start ", 6)) {
		cons_cmd_start(cons, cmdline + 6, memtotal);
	} else if (!strncmp(cmdline, "ncst ", 5)) {
		cons_cmd_ncst(cons, cmdline + 5, memtotal);
	} else if (!strncmp(cmdline, "langmode ", 9)) {
		cons_cmd_langmode(cons, cmdline + 9);
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
	struct FILEHANDLE fhandle[8];

	cons.sheet = sheet;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;

	if (cons.sheet) {
		cons.timer = timer_alloc();
		timer_init(cons.timer, &task->fifo, 1);
		timer_settime(cons.timer, 50);
	}

	int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADDR_DISKIMG + 0x000200));

	for (int i = 0; i < 8; i++) {
		fhandle[i].buf = 0;
	}
	task->fhandle = fhandle;
	task->fat = fat;

	unsigned char *japanese = (char *)*((int *)0x0fe8);
	if (japanese[4096] != 0xff) {	// if japanese font is available
		task->langmode = 1;
	} else {
		task->langmode = 0;
	}
	task->langbyte1 = 0;

	cons_putchar(&cons, '>', 1);

	char cmdline[64];
	task->cmdline = cmdline;

	for (;;) {
		io_cli();

		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			int d = fifo32_get(&task->fifo);
			io_sti();
			if ((d == 0 || d == 1) && cons.sheet) {		// cursor blink
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
				if (cons.sheet) {
					boxfill8(cons.sheet->buf, cons.sheet->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				cons.cur_c = -1;
			}
			if (d == 4) {
				cons_cmd_exit(&cons, fat);
			}
			if (256 <= d && d < 512) {	// keyboard
				if (d == 8 + 256) {	// backspace
					if (cons.cur_x > 16) {
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				} else if (d == 10 + 256) {		// enter
					cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal);
					if (!cons.sheet) {
						cons_cmd_exit(&cons, fat);
					}
					cons_putchar(&cons, '>', 1);
				} else {
					if (cons.cur_x < cons.sheet->bxsize - 16 || !cons.sheet) {
						cmdline[cons.cur_x / 8 - 2] = d - 256;
						cons_putchar(&cons, d - 256, 1);
					}
				}
			}
			if (cons.sheet) {
				if (cons.cur_c >= 0) {
					boxfill8(cons.sheet->buf, cons.sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
			}
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
	struct TASK* task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct SHEETCTL *sheetctl = (struct SHEETCTL *) *((int *) 0x0fe4);
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
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
		sheet->flags |= 0x10;
		sheet_setbuf(sheet, (char *)ebx + ds_base, esi, edi, eax);
		make_window8((char *)ebx + ds_base, esi, edi, (char *)ecx + ds_base, 0);
		sheet_slide(sheet, ((sheetctl->xsize - esi) / 2) & ~3, ((sheetctl->ysize - edi) / 2) & ~3);
		sheet_updown(sheet, sheetctl->top);
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
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	} else if (edx == 9) {	// memalloc alloc
		ecx = (ecx + 0x0f) & 0xfffffff0;
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
	} else if (edx == 10) {	// memalloc free
		ecx = (ecx + 0x0f) & 0xfffffff0;
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
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
			if (eax > esi) {
				int temp = eax;
				eax = esi;
				esi = temp;
			}
			if (ecx > edi) {
				int temp = ecx;
				ecx = edi;
				edi = temp;
			}
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
			if (d == 4) {
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sheet - sheetctl->sheets0 + 2024);
				cons->sheet = 0;
				io_sti();
			}
			if (256 <= d) {
				reg[7] = d - 256;
				return 0;
			}
		}
	} else if (edx == 16) {
		reg[7] = (int) timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;
	} else if (edx == 17) {
		timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
	} else if (edx == 18) {
		timer_settime((struct TIMER *) ebx, eax);
	} else if (edx == 19) {
		timer_free((struct TIMER *) ebx);
	} else if (edx == 20) {
		if(eax == 0) {
			int d = io_in8(0x61);
			io_out8(0x61, d & 0x0d);
		} else {
			int d = 1193180000 / eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, d & 0xff);
			io_out8(0x42, d >> 8);
			d = io_in8(0x61);
			io_out8(0x61, (d | 0x03) & 0x0f);
		}
	} else if (edx == 21) {		// fopen
		struct MEMMAN* memman = (struct MEMMAN *) MEMMAN_ADDR;
		int index;
		for (index = 0; index < 8; index++) {
			if (!task->fhandle[index].buf) {
				break;
			}
		}
		struct FILEHANDLE *fh = &task->fhandle[index];
		reg[7] = 0;
		if (index < 8) {
			struct FILEINFO *finfo = file_search((char *) ebx + ds_base, (struct FILEINFO *) (ADDR_DISKIMG + 0x002600), 224);
			if (finfo != 0) {
				reg[7] = (int)fh;
				fh->buf = (char *)memman_alloc_4k(memman, finfo->size);
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &finfo->size, task->fat);
			}
		}
	} else if (edx == 22) {		// fclose
		struct MEMMAN* memman = (struct MEMMAN *) MEMMAN_ADDR;
		struct FILEHANDLE *fh = (struct FILEHANDLE *) eax;
		memman_free_4k(memman, (int) fh->buf, fh->size);
		fh->buf = 0;
	} else if (edx == 23) {		// fseek
		struct FILEHANDLE *fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			fh->pos = ebx;
		} else if (ecx == 1) {
			fh->pos += ebx;
		} else if (ecx == 2) {
			fh->pos = fh->size + ebx;
		}

		if (fh->pos < 0) {
			fh->pos = 0;
		}
		if (fh->pos > fh->size) {
			fh->pos = fh->size;
		}
	} else if (edx == 24) {		// fsize
		struct FILEHANDLE *fh = (struct FILEHANDLE *) eax;
		if (ecx == 0) {
			reg[7] = fh->size;
		} else if (ecx == 1) {
			reg[7] = fh->pos;
		} else if (ecx == 2) {
			reg[7] == fh->pos - fh->size;
		}
	} else if (edx == 25) {		// fread
		struct FILEHANDLE *fh = (struct FILEHANDLE *) eax;
		int index;
		for (index = 0; index < ecx; index++) {
			if (fh->pos == fh->size) {
				break;
			}
			*((char *)ebx + ds_base + index) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = index;
	} else if (edx == 26) {		// cmdline
		int index = 0;
		for (;;) {
			*((char *) ebx + ds_base + index) = task->cmdline[index];
			if (task->cmdline[index] == 0 || index >= ecx) {
				break;
			}
			index++;
		}
		reg[7] = index;
	} else if (edx == 27) {
		reg[7] = task->langmode;
	}
	return 0;
}

int* inthandler0d(int *esp) {
	struct TASK* task = task_now();
	struct CONSOLE* cons = task->cons;
	char s[30];

	cons_putstr0(cons, "\nINT 0D: \n General Protected Exception.\n");
	sprintf(s, "EIP = %x\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);
}

int* inthandler0c(int *esp) {
	struct TASK* task = task_now();
	struct CONSOLE* cons = task->cons;
	char s[30];

	cons_putstr0(cons, "\nINT 0C: \n Stack Exception.\n");
	sprintf(s, "EIP = %x\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0);
}
