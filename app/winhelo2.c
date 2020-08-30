#include "api.h"

char buf[150 * 50];

void app_main() {
	int win = api_openwin(buf, 150, 50, -1, "hello2");
	api_boxfillwin(win, 8, 36, 141, 43, 3);
	api_putstrwin(win, 28, 28, 0, 12, "Hello, World");
	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break;
		}
	}
	api_end();
}