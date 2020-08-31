#include "bootpack.h"

void init_palette(void){
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,
		0xff, 0x00, 0x00,
		0x00, 0xff, 0x00,
		0xff, 0xff, 0x00,
		0x00, 0x00, 0xff,
		0xff, 0x00, 0xff,
		0x00, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0xc6, 0xc6, 0xc6,
		0x84, 0x00, 0x00,
		0x00, 0x84, 0x00,
		0x84, 0x84, 0x00,
		0x00, 0x00, 0x84,
		0x84, 0x00, 0x84,
		0x00, 0x84, 0x84,
		0x84, 0x84, 0x84
	};
	set_palette(0, 15, table_rgb);

	unsigned char table2[216 * 3];
	for (int b = 0; b < 6; b++) {
		for (int g = 0; g < 6; g++) {
			for (int r = 0; r < 6; r++) {
				table2[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
				table2[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
				table2[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
			}
		}
	}
	set_palette(16, 231, table2);
	return;
}

void set_palette(int start, int end, unsigned char *rgb) {
	int eflags = io_load_eflags();
	io_cli();
	io_out8(0x03c8, start);
	for (int i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);
	return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1){
	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++){
			vram[y * xsize + x] = c;
		}
	}
	return;
}

void init_screen8(unsigned char *vram, int xsize, int ysize){
	boxfill8(vram, xsize, COL8_008484, 0,	0,			xsize - 1,	ysize - 29);
	boxfill8(vram, xsize, COL8_C6C6C6, 0,	ysize - 28,	xsize - 1,	ysize - 28);
	boxfill8(vram, xsize, COL8_FFFFFF, 0,	ysize - 27,	xsize - 1,	ysize - 27);
	boxfill8(vram, xsize, COL8_C6C6C6, 0,	ysize - 26,	xsize - 1,	ysize - 1);

	boxfill8(vram, xsize, COL8_FFFFFF, 3,	ysize - 24,	59,	ysize - 24);
	boxfill8(vram, xsize, COL8_FFFFFF, 2,	ysize - 24,	2,	ysize - 4);
	boxfill8(vram, xsize, COL8_848484, 3,	ysize - 4,	59,	ysize - 4);
	boxfill8(vram, xsize, COL8_848484, 59,	ysize - 23,	59,	ysize - 5);
	boxfill8(vram, xsize, COL8_000000, 2,	ysize - 3,	59,	ysize - 3);
	boxfill8(vram, xsize, COL8_000000, 60,	ysize - 24,	60,	ysize - 3);

	boxfill8(vram, xsize, COL8_848484, xsize - 47,	ysize - 24,	xsize - 4,	ysize - 24);
	boxfill8(vram, xsize, COL8_848484, xsize - 47,	ysize - 23,	xsize - 47,	ysize - 4);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47,	ysize - 3,	xsize - 4,	ysize - 3);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 3,	ysize - 24,	xsize - 3,	ysize - 3);
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font){
	for (int i = 0; i < 16; i++) {
		char *p = vram + (y + i) * xsize + x;
		char d = font[i];
		if((d & 0x80)) { p[0] = c; }
		if((d & 0x40)) { p[1] = c; }
		if((d & 0x20)) { p[2] = c; }
		if((d & 0x10)) { p[3] = c; }
		if((d & 0x08)) { p[4] = c; }
		if((d & 0x04)) { p[5] = c; }
		if((d & 0x02)) { p[6] = c; }
		if((d & 0x01)) { p[7] = c; }
	}
	return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s){
	extern char hankaku[4096];
	struct TASK *task = task_now();
	char *japanese = (char *)*((int *) 0x0fe8);

	if (task->langmode == 0) {
		for (; *s != 0x00; s++){
			putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
			x += 8;
		}
	} else if (task->langmode == 1) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
					task->langbyte1 = *s;
				} else {
					putfont8(vram, xsize, x, y, c, japanese + *s * 16);
				}
			} else {
				int k, t;
				if (0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f) {
					k = (task->langbyte1 - 0x81) * 2;
				} else {
					k = (task->langbyte1 - 0xe0) * 2 + 62;
				}
				if (0x40 <= *s && *s <= 0x7e) {
					t = *s - 0x40;
				} else if (0x80 <= *s && *s <= 0x9e) {
					t = *s - 0x80 + 63;
				} else {
					t = *s - 0x9f;
					k++;
				}
				task->langbyte1 = 0;
				char *font = japanese + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font);
				putfont8(vram, xsize, x    , y, c, font + 16);
			}
			x += 8;
		}
	} else if (task->langmode == 2) {
		for (; *s != 0x00; s++) {
			if (task->langbyte1 == 0) {
				if (0x81 <= *s && *s <= 0xfe) {
					task->langbyte1 = *s;
				} else {
					putfont8(vram, xsize, x, y, c, japanese + *s * 16);
				}
			} else {
				int k = task->langbyte1 - 0xa1;
				int t = *s - 0xa1;
				task->langbyte1 = 0;
				char *font = japanese + 256 * 16 + (k * 94 + t) * 32;
				putfont8(vram, xsize, x - 8, y, c, font);
				putfont8(vram, xsize, x    , y, c, font + 16);
			}
		}
	}
	return;
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l) {
	struct TASK *task = task_now();
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);

	if (task->langmode && task->langbyte1) {
		putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
		sheet_refresh(sht, x - 8, y, x + l * 8, y + 16);
	} else {
		putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
		sheet_refresh(sht, x, y, x + l * 8, y + 16);
	}
	return;
}

void init_mouse_cursor8(char* mouse, char bc){
	static char cursor[16][16] = {
		"**************..",
		"*ooooooooooo*...",
		"*oooooooooo*....",
		"*ooooooooo*.....",
		"*oooooooo*......",
		"*ooooooo*.......",
		"*ooooooo*.......",
		"*oooooooo*......",
		"*oooo**ooo*.....",
		"*ooo*..*ooo*....",
		"*oo*....*ooo*...",
		"*o*......*ooo*..",
		"**........*ooo*.",
		"*..........*ooo*",
		".. .........*oo*",
		".............***",
	};
	for (int y = 0; y < 16; y++) {
		for (int x = 0; x < 16; x++){
			mouse[y * 16 + x] = (cursor[y][x] == '*')? COL8_000000 : (cursor[y][x] == 'o')? COL8_FFFFFF : bc;
		}
	}
	return;
}

void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize){
	for (int y = 0; y < pysize; y++) {
		for (int x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}