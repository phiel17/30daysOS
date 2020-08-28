#include "api.h"

void app_main() {
	int timer = api_timeralloc();
	api_timerinit(timer, 128);

	for (int i = 20000000; i >= 20000; i -= i / 100) {
		api_beep(i);
		api_timerset(timer, 1);
		if (api_getkey(1) != 128) {
			break;
		}
	}
	api_beep(0);
	api_end();
}