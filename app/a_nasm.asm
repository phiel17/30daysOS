bits 32

GLOBAL	api_end, api_getkey
GLOBAL	api_putchar, api_putstr0
GLOBAL	api_openwin, api_closewin
GLOBAL	api_putstrwin, api_boxfillwin, api_pointwin, api_linewin, api_refreshwin
GLOBAL	api_initmalloc, api_malloc, api_free
GLOBAL	api_timeralloc, api_timerinit, api_timerset, api_timerfree
GLOBAL	api_beep

api_end:
	MOV		EDX, 4
	INT		0x40

api_getkey:
	MOV		EDX, 15
	MOV		EAX, [ESP + 4]	; mode
	INT		0x40
	RET

api_putchar:
	MOV		EDX, 1
	MOV		AL, [ESP + 4]
	INT		0x40
	RET

api_putstr0:
	PUSH	EBX
	MOV		EDX, 2
	MOV		EBX, [ESP + 8]
	INT		0x40
	POP		EBX
	RET

api_openwin:
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		EDX, 5
	MOV		EBX, [ESP + 16]		; buf
	MOV		ESI, [ESP + 20]		; xsize
	MOV		EDI, [ESP + 24]		; ysize
	MOV		EAX, [ESP + 28]		; col_transparent
	MOV		ECX, [ESP + 32]		; title
	INT		0x40
	POP		EBX
	POP		ESI
	POP		EDI
	RET

api_closewin:
	PUSH	EBX
	MOV		EDX, 14
	MOV		EBX, [ESP + 8]
	INT		0x40
	POP		EBX
	RET

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

api_boxfillwin:
	PUSH	EDI
	PUSH	ESI
	PUSH	EBP
	PUSH	EBX
	MOV		EDX, 7
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

api_malloc:
	PUSH	EBX
	MOV		EDX, 9
	MOV		EBX, [CS:0x0020]
	MOV		ECX, [ESP + 8]		; size
	INT		0x40
	POP		EBX
	RET

api_free:
	PUSH	EBX
	MOV		EDX, 10
	MOV		EBX, [CS:0x0020]
	MOV		EAX, [ESP + 8]		; addr
	MOV		ECX, [ESP + 12]		; size
	INT		0x40
	POP		EBX
	RET

api_timeralloc:
	MOV		EDX, 16
	INT		0x40
	RET

api_timerinit:
	PUSH	EBX
	MOV		EDX, 17
	MOV		EBX, [ESP + 8]
	MOV		EAX, [ESP + 12]
	INT		0x40
	POP		EBX
	RET

api_timerset:
	PUSH	EBX
	MOV		EDX, 18
	MOV		EBX, [ESP + 8]
	MOV		EAX, [ESP + 12]
	INT		0x40
	POP		EBX
	RET

api_timerfree:
	PUSH	EBX
	MOV		EDX, 19
	MOV		EBX, [ESP + 8]
	INT		0x40
	POP		EBX
	RET

api_beep:
	MOV		EDX, 20
	MOV		EAX, [ESP + 4]
	INT		0x40
	RET
