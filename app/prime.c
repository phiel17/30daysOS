#include "api.h"

#define MAX		(10000)


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
	char *flag, s[8];

	api_initmalloc();
	flag = api_malloc(MAX);

	for (int i = 0; i < MAX; i++) {
		flag[i] = 0;
	}

	for (int i = 2; i < MAX; i++) {
		if (flag[i] == 0) {
			sprintf(s, "%d ", i);
			api_putstr0(s);
			for (int j = i * 2; j < MAX; j += i) {
				flag[j] = 1;
			}
		}
	}
	api_end();
}