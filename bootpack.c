#include "bootpack.h"

void HariMain(void){
	extern char hankaku[4096];
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADDR_BOOTINFO;
	char mcursor[256];
	int mx = binfo->scrnx / 2 - 8, my = binfo->scrny / 2 - 22;

	init_gdtidt();
	init_pic();
	io_sti();

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);

	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	char s[30];
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	io_out8(PIC0_IMR, 0xf9);	// enable PIC1 and keyboard
	io_out8(PIC1_IMR, 0xef);	// enable mouse

	for (;;) {
		io_hlt();
	}
}
