void api_end();
int api_getkey(int mode);

void api_putchar(int c);
void api_putstr0(char *s);

int api_openwin(char *buf, int xsize, int ysize, int col_transparent, char *title);
void api_closewin(int win);

void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_pointwin(int win, int x, int y, int col);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);

void api_initmalloc();
char* api_malloc(int size);
void api_free(char *addr, int size);

int api_timeralloc();
void api_timerinit(int timer, int data);
void api_timerset(int timer, int time);
void api_timerfree(int timer);
