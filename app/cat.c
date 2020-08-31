#include "api.h"

void app_main() {
	char cmdline[30];
	api_cmdline(cmdline, 30);

	char *p;
	for (p = cmdline; *p > ' '; p++) { }
	for (; *p == ' '; p++) { }

	int fh = api_fopen(p);
	if (fh) {
		char c;
		for (;;) {
			if (!api_fread(&c, 1, fh)) {
				break;
			}
			api_putchar(c);
		}
	} else {
		api_putstr0("File not found\n");
	}
	api_end();
}