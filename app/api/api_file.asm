bits 32

GLOBAL	api_fopen, api_fclose, api_fseek, api_fsize, api_fread

api_fopen:
	PUSH	EBX
	MOV		EDX, 21
	MOV		EBX, [ESP + 8]		; fname
	INT		0x40
	POP		EBX
	RET

api_fclose:
	MOV		EDX, 22
	MOV		EAX, [ESP + 4]		; fhandle
	INT		0x40
	RET

api_fseek:
	PUSH	EBX
	MOV		EDX, 23
	MOV		EAX, [ESP + 8]		; fhandle
	MOV		ECX, [ESP + 16]		; mod
	MOV		EBX, [ESP + 12]		; offset
	INT		0x40
	POP		EBX
	RET

api_fsize:
	MOV		EDX, 24
	MOV		EAX, [ESP + 4]		; fhandle
	MOV		ECX, [ESP + 8]		; mode
	INT		0x40
	RET

api_fread:
	PUSH	EBX
	MOV		EDX, 25
	MOV		EAX, [ESP + 16]		; fhandle
	MOV		ECX, [ESP + 12]		; maxsize
	MOV		EBX, [ESP + 8]		; buf
	INT		0x40
	POP		EBX
	RET
