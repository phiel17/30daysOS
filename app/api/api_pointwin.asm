bits 32

GLOBAL	api_pointwin

api_pointwin:
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		EDX, 11
	MOV		EBX, [ESP + 16]		; win
	MOV		ESI, [ESP + 20]		; x
	MOV		EDI, [ESP + 24]		; y
	MOV		EAX, [ESP + 28]		; col
	INT		0x40
	POP		EBX
	POP		ESI
	POP		EDI
	RET
