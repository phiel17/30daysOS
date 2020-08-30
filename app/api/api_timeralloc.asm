bits 32

GLOBAL	api_timeralloc

api_timeralloc:
	MOV		EDX, 16
	INT		0x40
	RET
