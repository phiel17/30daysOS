int api_openwin(char *buf, int xsize, int ysize, int col_transparent, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc();
char* api_malloc(int size);
void api_end();

void app_main() {
	api_initmalloc();
	char* buf = api_malloc(150 * 50);

	int win = api_openwin(buf, 150, 50, -1, "hello3");
	api_boxfillwin(win, 8, 36, 141, 43, 3);
	api_putstrwin(win, 28, 28, 0, 12, "Hello, World");
	api_end();
}