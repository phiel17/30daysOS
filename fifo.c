#include "bootpack.h"

#define FLAGS_OVERRUN	(0x0001)

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf) {
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size;
	fifo->flags = 0;
	fifo->p = 0;
	fifo->q = 0;
	return;
}

int fifo8_put(struct FIFO8 *fifo, unsigned char data) {
	if (!fifo->free) {
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p = (fifo->p + 1) % fifo->size;
	fifo->free--;
	return 0;
}

int fifo8_get(struct FIFO8 *fifo) {
	if (fifo->free == fifo->size) {
		return -1;
	}
	int data = fifo->buf[fifo->q];
	fifo->q = (fifo->q + 1) % fifo->size;
	fifo->free++;
	return data;
}

int fifo8_status(struct FIFO8 *fifo) {
	return fifo->size - fifo->free;
}
