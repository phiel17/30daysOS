#include "bootpack.h"

#define EFLAGS_AC_BIT		(0x00040000)
#define CR0_CACHE_DISABLE	(0x60000000)


unsigned int memtest(unsigned int start, unsigned int end) {
	char flag486 = 0;
	unsigned int eflag = io_load_eflags();
	eflag |= EFLAGS_AC_BIT;
	io_store_eflags(eflag);
	if (eflag & EFLAGS_AC_BIT) {		// AC-BIT will be 0 on 386 / 1 on 486
		flag486 = 1;
	}
	eflag &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflag);

	if (flag486) {
		unsigned int cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	unsigned int temp = memtest_sub(start, end);

	if (flag486) {
		unsigned int cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	return temp;
}

void memman_init(struct MEMMAN *man) {
	man->frees = 0;
	man->maxfrees = 0;
	man->lostsize = 0;
	man->losts = 0;
	return;
}

unsigned int memman_total(struct MEMMAN *man) {
	unsigned int t = 0;
	for (unsigned int i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
	for (unsigned int i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			unsigned int a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1];
				}
			}
			return a;
		}
	}
	return 0;
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size) {
	size = (size + 0xfff) & 0xfffff000;
	return memman_alloc(man, size);
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
	int i;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}

	if (i > 0) {	// prev exists
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			man->free[i - 1].size += size;	// extend prev

			if(i < man->frees) {	// next exists
				if (addr + size == man->free[i].addr) {	// extend prev
					man->free[i - 1].size += man->free[i].size;
					
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1];
					}
				}
			}
			return 0;
		}
	}

	if(i < man->frees) {	// next exists
		if (addr + size == man->free[i].addr) {		// extend next
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}

	if(man->frees < MEMMAN_FREES) {
		for (int j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees;
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}

	man->losts++;
	man->lostsize += size;
	return -1;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size) {
	size = (size + 0xfff) & 0xfffff000;
	return memman_free(man, addr, size);
}