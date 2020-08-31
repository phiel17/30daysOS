bits 32

GLOBAL	api_cmdline

api_cmdline:
	PUSH	EBX
	MOV		EDX, 26
	MOV		ECX, [ESP + 12]		; maxsize
	MOV		EBX, [ESP + 8]		; buf
	INT		0x40
	POP		EBX
	RET
