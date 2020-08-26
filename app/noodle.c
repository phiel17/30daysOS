#include "api.h"

#pragma GCC diagnostic ignored "-Wvarargs"

#include <stdarg.h>

//10進数からASCIIコードに変換
int dec2asc (char *str, int dec) {
	int len = 0, len_buf; //桁数
	int buf[10];
	while (1) { //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
		buf[len++] = dec % 10;
		if (dec < 10) break;
		dec /= 10;
	}
	len_buf = len;
	while (len) {
		*(str++) = buf[--len] + 0x30;
	}
	return len_buf;
}

//16進数からASCIIコードに変換
int hex2asc (char *str, int dec) { //10で割れた回数（つまり桁数）をlenに、各桁をbufに格納
	int len = 0, len_buf; //桁数
	int buf[10];
	while (1) {
		buf[len++] = dec % 16;
		if (dec < 16) break;
		dec /= 16;
	}
	len_buf = len;
	while (len) {
		len --;
		*(str++) = (buf[len]<10)?(buf[len] + 0x30):(buf[len] - 9 + 0x60);
	}
	return len_buf;
}

void sprintf (char *str, char *fmt, ...) {
	va_list list;
	int i, len;
	int size = 2;
	va_start (list, size);

	while (*fmt) {
		if(*fmt=='%') {
			fmt++;
			switch(*fmt){
				case 'd':
					len = dec2asc(str, va_arg (list, int));
					break;
				case 'x':
					len = hex2asc(str, va_arg (list, int));
					break;
			}
			str += len; fmt++;
		} else {
			*(str++) = *(fmt++);
		}   
	}
	*str = 0x00; //最後にNULLを追加
	va_end (list);
}


void app_main() {
	api_initmalloc();
	char *buf = api_malloc(150 * 50);
	int win = api_openwin(buf, 150, 50, -1, "noodle");

	int timer = api_timeralloc();
	api_timerinit(timer, 128);

	char s[12];
	int hou = 0, min = 0, sec = 0;
	for (;;) {
		sprintf(s, "%d:%d:%d", hou, min, sec);
		api_boxfillwin(win, 28, 27, 115, 41, 7);
		api_putstrwin(win, 28, 27, 0, 11, s);
		api_timerset(timer, 100);
		if (api_getkey(1) != 128) {
			break;
		}

		sec++;
		if (sec == 60) {
			sec = 0;
			min++;
			if (min == 60) {
				min = 0;
				hou++;
			}
		}
	}
	api_end();
}
