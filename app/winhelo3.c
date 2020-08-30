#include "api.h"

void app_main() {
	api_initmalloc();
	char* buf = api_malloc(150 * 50);

	int win = api_openwin(buf, 150, 50, -1, "hello3");
	api_boxfillwin(win, 8, 36, 141, 43, 3);
	api_putstrwin(win, 28, 28, 0, 12, "Hello, World");
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break;
		}
	}
	api_end();
}