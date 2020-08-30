bits 32

GLOBAL	api_free

api_free:
	PUSH	EBX
	MOV		EDX, 10
	MOV		EBX, [CS:0x0020]
	MOV		EAX, [ESP + 8]		; addr
	MOV		ECX, [ESP + 12]		; size
	INT		0x40
	POP		EBX
	RET
