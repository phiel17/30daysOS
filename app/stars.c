#include "api.h"

unsigned int rand() {
	static unsigned int x = 123456789;
	static unsigned int y = 362436069;
	static unsigned int z = 521288629;
	static unsigned int w = 88675123;
	unsigned int t;
	t = x ^ (x<<11);
	x = y; y = z; z = w;
	w ^= t ^ (t>>8) ^ (w>>19);
	return w;
}

void app_main() {
	api_initmalloc();
	char *buf = api_malloc(150 * 100);
	int win = api_openwin(buf, 150, 100, -1, "stars");
	api_boxfillwin(win, 6, 26, 143, 93, 0);

	for (int i = 0; i < 50; i++) {
		int x = (rand() % 137) + 6;
		int y = (rand() % 67) + 26;
		api_pointwin(win, x, y, 3);
	}
	api_end();
}