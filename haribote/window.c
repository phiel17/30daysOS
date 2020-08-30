#include "bootpack.h"

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

void change_wtitle8(struct SHEET *sheet, char act) {
	int tc_new = (act)? COL8_FFFFFF : COL8_C6C6C6;
	int tbc_new = (act)? COL8_000084 : COL8_848484;
	int tc_old = (act)? COL8_C6C6C6 : COL8_FFFFFF;
	int tbc_old = (act)? COL8_848484 : COL8_000084;

	int xsize = sheet->bxsize;

	for (int y = 3; y <= 20; y++) {
		for (int x = 3; x <= xsize - 4; x++) {
			char c = sheet->buf[y * xsize + x];
			if (c == tc_old && x <= xsize - 22) {
				c = tc_new;
			} else if (c == tbc_old) {
				c = tbc_new;
			}
			sheet->buf[y * xsize + x] = c;
		}
	}
	sheet_refresh(sheet, 3, 3, xsize, 21);
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
