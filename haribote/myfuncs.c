// mysprintf.c
// http://bttb.s1.valueserver.jp/wordpress/blog/2017/12/17/makeos-5-2/

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

int strcmp(const char *s1, const char *s2) {
	for (;;) {
		if (*s1 == *s2) {
			if (*s1 == 0){
				return 0;
			}
			s1++;
			s2++;
			continue;
		} else {
			return (*s1 - *s2);
		}
	}
}

int strncmp(const char *s1, const char *s2, int n) {
	while (*s2){
		if (*s1 != *s2) {
			return (*s1 - *s2);
		}
		s1++;
		s2++;
	}
	return 0;
}

int memcmp(const void *s1, const void *s2, int n)
{
	const unsigned char *p1 = (const unsigned char *)s1;
	const unsigned char *p2 = (const unsigned char *)s2;

	while (n-- > 0) {
		if (*p1 != *p2)
			return (*p1 - *p2);
		p1++;
		p2++;
	}
	return 0;
}
