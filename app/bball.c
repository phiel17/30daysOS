#include "api.h"

struct POINT {
	int x, y;
};

void app_main() {
	char buf[216 * 237];
	static struct POINT table[16] = {
		{204, 129},
		{195, 90},
		{172, 58},
		{137, 38},
		{98, 34},
		{61, 46},
		{31, 73},
		{15, 110},
		{15, 148},
		{31, 185},
		{61, 212},
		{98, 224},
		{137, 220},
		{172, 200},
		{195, 168},
		{204, 129},
	};

	int win = api_openwin(buf, 216, 237, -1, "bball");
	api_boxfillwin(win, 8, 29, 207, 228, 0);

	for (int i = 0; i < 15; i++) {
		for (int j = i + 1; j < 16; j++) {
			int dist = j - i;
			if (dist >= 8) {
				dist = 15 - dist;
			}
			if (dist != 0) {
				api_linewin(win, table[i].x, table[i].y, table[j].x, table[j].y, 8 - dist);
			}
		}
	}
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break;
		}
	}
	api_end();
}