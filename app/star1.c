#include "api.h"

void app_main() {
	api_initmalloc();
	char *buf = api_malloc(150 * 100);
	int win = api_openwin(buf, 150, 100, -1, "star1");
	api_boxfillwin(win, 6, 26, 143, 93, 0);
	api_pointwin(win, 75, 59, 3);
	api_end();
}
