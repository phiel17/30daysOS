#include "api.h"

void app_main() {
	int fh = api_fopen("ipl10.asm");
	if (fh) {
		char c;
		for (;;) {
			if (api_fread(&c, 1, fh) == 0) {
				break;
			}
			api_putchar(c);
		}
	}
	api_end();
}