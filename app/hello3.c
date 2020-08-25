void api_putchar(int c);
void api_end();

void app_main(void) {
	api_putchar('h');
	api_putchar('e');
	api_putchar('l');
	api_putchar('l');
	api_putchar('o');
	api_end();
}