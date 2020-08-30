bits 32

GLOBAL	api_timerinit

api_timerinit:
	PUSH	EBX
	MOV		EDX, 17
	MOV		EBX, [ESP + 8]
	MOV		EAX, [ESP + 12]
	INT		0x40
	POP		EBX
	RET
