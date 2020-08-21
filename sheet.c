#include "bootpack.h"

#define SHEET_USE	(1)

struct SHEETCTL *sheetctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
	struct SHEETCTL *ctl = (struct SHEETCTL *)memman_alloc_4k(memman, sizeof(struct SHEETCTL));
	if (!ctl) {
		goto err;
	}

	ctl->map = (unsigned char *)memman_alloc_4k(memman, xsize * ysize);
	if (!ctl->map) {
		memman_free_4k(memman, (int)ctl, sizeof(struct SHEETCTL));
		goto err;
	}

	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1;		// no sheets
	for (int i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0;
		ctl->sheets0[i].ctl = ctl;
	}

err:
	return ctl;
}

struct SHEET *sheet_alloc(struct SHEETCTL *ctl) {
	for (int i = 0; i < MAX_SHEETS; i++) {
		if (ctl->sheets0[i].flags == 0) {
			struct SHEET *sht;
			sht = &ctl->sheets0[i];
			sht->flags = SHEET_USE;
			sht->height = -1;	// invisible
			return sht;
		}
	}
	return 0;	// no sheets available
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_transparent) {
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_transparent = col_transparent;
	return;
}

void sheet_refreshmap(struct SHEETCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

	for (int h = h0; h <= ctl->top; h++) {
		struct SHEET *sht = ctl->sheets[h];
		unsigned char sid = sht - ctl->sheets0;		// use address diff as sheet id

		int bx0 = vx0 - sht->vx0;
		int by0 = vy0 - sht->vy0;
		int bx1 = vx1 - sht->vx0;
		int by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0;}
		if (by0 < 0) { by0 = 0;}
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize;}
		if (by1 > sht->bysize) { by1 = sht->bysize;}

		for (int by = by0; by < by1; by++) {
			int vy = sht->vy0 + by;

			for (int bx = bx0; bx < bx1; bx++) {
				int vx = sht->vx0 + bx;
				if (sht->buf[by * sht->bxsize + bx] != sht->col_transparent) {
					ctl->map[vy * ctl->xsize + vx] = sid;
				}
			}
		}
	}
}

void sheet_refreshsub(struct SHEETCTL* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

	for (int h = h0; h <= h1; h++) {
		struct SHEET *sht = ctl->sheets[h];

		int bx0 = vx0 - sht->vx0;
		int by0 = vy0 - sht->vy0;
		int bx1 = vx1 - sht->vx0;
		int by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0;}
		if (by0 < 0) { by0 = 0;}
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize;}
		if (by1 > sht->bysize) { by1 = sht->bysize;}

		for (int by = by0; by < by1; by++) {
			int vy = sht->vy0 + by;

			for (int bx = bx0; bx < bx1; bx++) {
				int vx = sht->vx0 + bx;
				unsigned char sid = sht - ctl->sheets0;
				if (ctl->map[vy * ctl->xsize + vx] == sid) {
					ctl->vram[vy * ctl->xsize + vx] = sht->buf[by * sht->bxsize + bx];
				}
			}
		}
	}
}

void sheet_refresh(struct SHEET* sht, int bx0, int by0, int bx1, int by1) {
	if (sht->height >= 0) {
		sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
	}
	return;
}

void sheet_updown(struct SHEET *sht, int height) {
	int oldheight = sht->height;

	if (height > sht->ctl->top + 1) {
		height = sht->ctl->top + 1;
	}
	if(height < -1) {
		height = -1;
	}
	sht->height = height;

	if (oldheight > height) {		// lower
		if (height >= 0) {
			for (int h = oldheight; h > height; h--) {
				sht->ctl->sheets[h] = sht->ctl->sheets[h - 1];
				sht->ctl->sheets[h]->height = h;
			}
			sht->ctl->sheets[height] = sht;
			sheet_refreshmap(sht->ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
			sheet_refreshsub(sht->ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, oldheight);
		} else {	// invisible
			if (sht->ctl->top > oldheight) {
				for (int h = oldheight; h < sht->ctl->top; h++) {
					sht->ctl->sheets[h] = sht->ctl->sheets[h + 1];
					sht->ctl->sheets[h]->height = h;
				}
			}
			sht->ctl->top--;
		}
		sheet_refreshmap(sht->ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
		sheet_refreshsub(sht->ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, oldheight - 1);
	} else if (oldheight < height) {	// higher
		if (oldheight >= 0) {
			for (int h = oldheight; h < height; h++) {
				sht->ctl->sheets[h] = sht->ctl->sheets[h + 1];
				sht->ctl->sheets[h]->height = h;
			}
			sht->ctl->sheets[height] = sht;
		} else {
			for (int h = sht->ctl->top; h >= height; h--) {
				sht->ctl->sheets[h + 1] = sht->ctl->sheets[h];
				sht->ctl->sheets[h + 1]->height = h + 1;
			}
			sht->ctl->sheets[height] = sht;
			sht->ctl->top++;
		}
		sheet_refreshmap(sht->ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
		sheet_refreshsub(sht->ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
	}
	return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;

	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) {
		sheet_refreshmap(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
		sheet_refreshmap(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);

		sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
		sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
	}
	return;
}

void sheet_free(struct SHEET* sht) {
	if (sht->height >= 0) {
		sheet_updown(sht, -1);
	}
	sht->flags = 0;		// not used
	return;
}
