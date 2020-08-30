bits 32

GLOBAL api_linewin

api_linewin:
	PUSH	EDI
	PUSH	ESI
	PUSH	EBP
	PUSH	EBX
	MOV		EDX, 13
	MOV		EBX, [ESP + 20]	; win
	MOV		EAX, [ESP + 24]	; x0
	MOV		ECX, [ESP + 28]	; y0
	MOV		ESI, [ESP + 32]	; x1
	MOV		EDI, [ESP + 36]	; y1
	MOV		EBP, [ESP + 40]	; col
	INT		0x40
	POP		EBX
	POP		EBP
	POP		ESI
	POP		EDI
	RET
