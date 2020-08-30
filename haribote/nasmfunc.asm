GLOBAL	io_hlt, io_cli, io_sti, io_stihlt
GLOBAL	io_in8, io_in16, io_in32
GLOBAL	io_out8, io_out16, io_out32
GLOBAL	io_load_eflags, io_store_eflags
GLOBAL	load_cr0, store_cr0
GLOBAL	load_gdtr, load_idtr
GLOBAL	asm_inthandler0c, asm_inthandler0d, asm_inthandler20, asm_inthandler21, asm_inthandler27, asm_inthandler2c
GLOBAL	memtest_sub
GLOBAL	load_tr, farjmp, farcall, start_app
GLOBAL	asm_hrb_api, asm_end_app

EXTERN	inthandler0c, inthandler0d, inthandler20, inthandler21, inthandler27, inthandler2c
EXTERN	hrb_api

io_hlt:
	HLT
	RET

io_cli:
	CLI
	RET

io_sti:
	STI
	RET

io_stihlt:
	STI
	HLT
	RET

io_in8:
	MOV		EDX, [ESP + 4]		; port
	MOV		EAX, 0
	IN		AL, DX
	RET

io_in16:
	MOV		EDX, [ESP + 4]		; port
	MOV		EAX, 0
	IN		AX, DX
	RET

io_in32:
	MOV		EDX, [ESP + 4]		; port
	IN		EAX, DX
	RET

io_out8:
	MOV		EDX, [ESP + 4]		; port
	MOV		AL, [ESP + 8]		; data
	OUT		DX, AL
	RET

io_out16:
	MOV		EDX, [ESP + 4]		; port
	MOV		AL, [ESP + 8]		; data
	OUT		DX, AX
	RET

io_out32:
	MOV		EDX, [ESP + 4]		; port
	MOV		AL, [ESP + 8]		; data
	OUT		DX, EAX
	RET

io_load_eflags:
	PUSHFD
	POP		EAX
	RET

io_store_eflags:
	MOV		EAX, [ESP + 4]
	PUSH	EAX
	POPFD
	RET

load_cr0:
	MOV		EAX, CR0
	RET

store_cr0:
	MOV		EAX, [ESP + 4]
	MOV		CR0, EAX
	RET

load_gdtr:
	MOV		AX, [ESP + 4]
	MOV		[ESP + 6], AX
	LGDT	[ESP + 6]
	RET

load_idtr:
	MOV		AX, [ESP + 4]
	MOV		[ESP + 6], AX
	LIDT	[ESP + 6]
	RET

asm_inthandler0c:
	STI

	PUSH	ES
	PUSH	DS
	PUSHAD
	MOV		EAX, ESP
	PUSH	EAX
	MOV		AX, SS
	MOV		DS, AX
	MOV		ES, AX
	CALL	inthandler0c
	CMP		EAX, 0
	JNE		asm_end_app			; end when illegal access

	POP		EAX
	POPAD
	POP		DS
	POP		ES
	ADD		ESP, 4
	IRETD

asm_inthandler0d:
	STI

	PUSH	ES
	PUSH	DS
	PUSHAD
	MOV		EAX, ESP
	PUSH	EAX
	MOV		AX, SS
	MOV		DS, AX
	MOV		ES, AX
	CALL	inthandler0d
	CMP		EAX, 0
	JNE		asm_end_app			; end when illegal access

	POP		EAX
	POPAD
	POP		DS
	POP		ES
	ADD		ESP, 4
	IRETD

asm_inthandler20:
	PUSH	ES
	PUSH	DS
	PUSHAD
	MOV		EAX,ESP
	PUSH	EAX
	MOV		AX,SS
	MOV		DS,AX
	MOV		ES,AX
	CALL	inthandler20
	POP		EAX
	POPAD
	POP		DS
	POP		ES
	IRETD

asm_inthandler21:
	PUSH	ES
	PUSH	DS
	PUSHAD
	MOV		EAX,ESP
	PUSH	EAX
	MOV		AX,SS
	MOV		DS,AX
	MOV		ES,AX
	CALL	inthandler21
	POP		EAX
	POPAD
	POP		DS
	POP		ES
	IRETD

asm_inthandler27:
	PUSH	ES
	PUSH	DS
	PUSHAD
	MOV		EAX,ESP
	PUSH	EAX
	MOV		AX,SS
	MOV		DS,AX
	MOV		ES,AX
	CALL	inthandler27
	POP		EAX
	POPAD
	POP		DS
	POP		ES
	IRETD

asm_inthandler2c:
	PUSH	ES
	PUSH	DS
	PUSHAD
	MOV		EAX,ESP
	PUSH	EAX
	MOV		AX,SS
	MOV		DS,AX
	MOV		ES,AX
	CALL	inthandler2c
	POP		EAX
	POPAD
	POP		DS
	POP		ES
	IRETD

memtest_sub:
	PUSH	EDI
	PUSH	ESI
	PUSH	EBX
	MOV		ESI, 0xdeadbeef
	MOV		EDI, 0x21524110
	MOV		EAX, [ESP + 12 + 4];

	mts_loop:
	MOV		EBX, EAX
	ADD		EBX, 0xffc
	MOV		EDX, [EBX]
	MOV		[EBX], ESI
	XOR		DWORD [EBX], 0xffffffff
	CMP		EDI, [EBX]
	JNE		mts_fin
	XOR		DWORD [EBX], 0xffffffff
	CMP		ESI, [EBX]
	JNE		mts_fin
	MOV		[EBX], EDX
	ADD		EAX, 0x1000
	CMP		EAX, [ESP + 12 + 8]
	JBE		mts_loop
	POP		EBX
	POP		ESI
	POP		EDI
	RET

	mts_fin:
	MOV		[EBX], EDX
	POP		EBX
	POP		ESI
	POP		EDI
	RET

load_tr:
	LTR		[ESP + 4]
	RET

farjmp:
	JMP	FAR	[ESP + 4]
	RET

farcall:
	CALL FAR	[ESP + 4]
	RET

start_app:
	PUSHAD
	MOV		EAX, [ESP + 36]		; EIP of app
	MOV		ECX, [ESP + 40]		; CS of app
	MOV		EDX, [ESP + 44]		; ESP of app
	MOV		EBX, [ESP + 48]		; DS/SS of app
	MOV		EBP, [ESP + 52]		; tss.esp0
	MOV		[EBP], ESP			; ESP of OS
	MOV		[EBP + 4], SS		; SS of OS

	MOV		ES, BX
	MOV		DS, BX
	MOV		FS, BX
	MOV		GS, BX

	; set stack to go to app via RETF
	OR		ECX, 3
	OR		EBX, 3
	PUSH	EBX
	PUSH	EDX
	PUSH	ECX
	PUSH	EAX
	RETF

asm_hrb_api:
	STI

	; set OS segment
	PUSH	DS
	PUSH	ES
	PUSHAD					; for recovery
	PUSHAD					; for hrb_api
	MOV		AX, SS
	MOV		ES, AX			; copy OS segment
	MOV		DS, AX			; copy OS segment

	CALL	hrb_api

	; reset segment
	CMP		EAX, 0
	JNE		asm_end_app
	ADD		ESP, 32
	POPAD
	POP		ES
	POP		DS
	IRETD

asm_end_app:
	; EAX is addr of tss.esp
	MOV		ESP, [EAX]
	MOV		DWORD	[EAX + 4], 0
	POPAD
	RET						; to cmd_app
