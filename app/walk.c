#include "api.h"

void app_main() {
	api_initmalloc();
	char *buf = api_malloc(160 * 100);
	int win = api_openwin(buf, 160, 100, -1, "walk");
	api_boxfillwin(win, 4, 24, 155, 95, 0);

	int x = 76, y = 56;
	api_putstrwin(win, x, y, 3, 1, "*");

	for (;;) {
		int d = api_getkey(1);
		api_putstrwin(win, x, y, 0, 1, "*");
		if (d == '4' && x > 4) {
			x -= 8;
		}
		if (d == '6' && x < 148) {
			x += 8;
		}
		if (d == '8' && y > 24) {
			y -= 8;
		}
		if (d == '2' && y < 80) {
			y += 8;
		}
		if (d == 0x0a) {
			break;
		}
		api_putstrwin(win, x, y, 3, 1, "*");
	}
	api_closewin(win);
	api_end();
}