#include "api.h"

void app_main() {
	char buf[150 * 70];

	int win = api_openwin(buf, 150, 70, 255, "notrec");
	api_boxfillwin(win, 0, 50, 34, 69, 255);
	api_boxfillwin(win, 115, 50, 149, 69, 255);
	api_boxfillwin(win, 50, 30, 99, 49, 255);

	for (;;) {
		if (api_getkey(1) == 0x0a) {
			break;
		}
	}
	api_end();
}