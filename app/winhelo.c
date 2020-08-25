int api_openwin(char *buf, int xsize, int ysize, int col_transparent, char *title);
void api_end();

char buf[150 * 50];

void app_main() {
	int win = api_openwin(buf, 150, 50, -1, "hello");
	api_end();
}