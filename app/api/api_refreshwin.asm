bits 32

GLOBAL	api_refreshwin

api_refreshwin:
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		EDX, 12
	MOV		EBX, [ESP + 16]		; win
	MOV		EAX, [ESP + 20]		; x0
	MOV		ECX, [ESP + 24]		; y0
	MOV		ESI, [ESP + 28]		; x1
	MOV		EDI, [ESP + 32]		; y1
	INT		0x40
	POP		EBX
	POP		ESI
	POP		EDI
	RET
