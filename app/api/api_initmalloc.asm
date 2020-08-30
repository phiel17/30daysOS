bits 32

GLOBAL	api_initmalloc

api_initmalloc:
	PUSH	EBX
	MOV		EDX, 8
	MOV		EBX, [CS:0x0020]	; addr for malloc
	MOV		EAX, EBX
	ADD		EAX, 32 * 1024
	MOV		ECX, [CS:0x0000]	; data segment size
	SUB		ECX, EAX
	INT		0x40
	POP		EBX
	RET
