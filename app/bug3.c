void api_putchar(int c);
void api_end();

void app_main() {
	for(int i = 0;;) {
		api_putchar('0' + i);
		i++;
		i %= 10;
	}
}