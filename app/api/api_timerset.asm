bits 32

GLOBAL	api_timerset

api_timerset:
	PUSH	EBX
	MOV		EDX, 18
	MOV		EBX, [ESP + 8]
	MOV		EAX, [ESP + 12]
	INT		0x40
	POP		EBX
	RET
