bits 32

GLOBAL	api_malloc

api_malloc:
	PUSH	EBX
	MOV		EDX, 9
	MOV		EBX, [CS:0x0020]
	MOV		ECX, [ESP + 8]		; size
	INT		0x40
	POP		EBX
	RET
