bits 32

GLOBAL	api_putstrwin

api_putstrwin:
	PUSH	EDI
	PUSH	ESI
	PUSH	EBP
	PUSH	EBX
	MOV		EDX, 6
	MOV		EBX, [ESP + 20]	; win
	MOV		ESI, [ESP + 24]	; x
	MOV		EDI, [ESP + 28]	; y
	MOV		EAX, [ESP + 32]	; col
	MOV		ECX, [ESP + 36]	; len
	MOV		EBP, [ESP + 40]	; str
	INT		0x40
	POP		EBX
	POP		EBP
	POP		ESI
	POP		EDI
	RET
