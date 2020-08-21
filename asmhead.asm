ORG	0xc200

BOTPAK	EQU	0x00280000
DSKCAC	EQU	0x00100000
DSKCAC0	EQU	0x00008000

; BOOT_INFO
CYLS	EQU	0x0ff0
LEDS	EQU	0x0ff1
VMODE	EQU	0x0ff2
SCRNX	EQU	0x0ff4
SCRNY	EQU	0x0ff6
VRAM	EQU	0x0ff8

	; change graphic mode
	MOV		AL, 0x13
	MOV		AH, 0x00
	INT		0x10

	MOV		BYTE [VMODE], 8
	MOV		WORD [SCRNX], 320
	MOV		WORD [SCRNY], 200
	MOV		DWORD [VRAM], 0x000a0000

	; keyboard led state
	MOV		AH, 0x02
	INT		0x16
	MOV		[LEDS], AL

	; disable PIC interrupt to initialize PIC
	MOV		AL, 0xff
	OUT		0x21, AL
	NOP
	OUT		0xa1, AL
	CLI					; disable PIC in CPU

	; enable CPU to access over 1MB memory
	CALL	waitkbdout
	MOV		AL, 0xd1
	OUT		0x64, AL
	CALL	waitkbdout
	MOV		AL, 0xdf	; enable A20 to use over 1MB
	OUT		0x60, AL
	CALL	waitkbdout	; ensure the precess is completed

; protect mode
; [INSTRSET "i486p"]
	LGDT	[GDTR0]
	MOV		EAX, CR0
	AND		EAX, 0x7fffffff
	OR		EAX, 0x00000001
	MOV		CR0, EAX			; set control register 0 to set protect mode without paging
	JMP		pipelineflush		; reflesh CPU pipeline

pipelineflush:
	MOV		AX, 1*8				; reset segment registers
	MOV		DS, AX
	MOV		ES, AX
	MOV		FS, AX
	MOV		GS, AX
	MOV		SS, AX

	; transfer bootpack
	MOV		ESI, bootpack	; src
	MOV		EDI, BOTPAK		; dst
	MOV		ECX, 512*1024/4
	CALL	memcpy

	; transfer bootsector
	MOV		ESI, 0x7c00
	MOV		EDI, DSKCAC
	MOV		ECX, 512/4
	CALL	memcpy

	; others
	MOV		ESI, DSKCAC0 + 512
	MOV		EDI, DSKCAC + 512
	MOV		ECX, 0
	MOV		CL, BYTE [CYLS]
	IMUL	ECX, 512*18*2/4	; cylinder -> byte/4
	SUB		ECX, 512/4		; ipl size
	CALL	memcpy

; start bootpack
	MOV		EBX, BOTPAK
	MOV		ECX, [EBX + 16]
	ADD		ECX, 3
	SHR		ECX, 2
	JZ		skip	; if no data to transfer

	MOV		ESI, [EBX + 20]	; src
	ADD		ESI, EBX
	MOV		EDI, [EBX + 12]	; dst
	CALL	memcpy
skip:
	MOV		ESP, [EBX + 12]	; initial stack
	JMP		DWORD 2*8:0x0000001b

waitkbdout:
	IN		AL, 0x64
	AND		AL, 0x02
	JNZ		waitkbdout
	RET

memcpy:
	MOV		EAX, [ESI]
	ADD		ESI, 4
	MOV		[EDI], EAX
	ADD		EDI, 4
	SUB		ECX, 1
	JNZ		memcpy
	RET

	ALIGNB	16
GDT0:
	RESB	8								; null selector
	DW		0xffff, 0x0000, 0x9200, 0x00cf	; read-write segment
	DW		0xffff, 0x0000, 0x9a28, 0x0047	; executable segment (bootpack)
	DW		0

GDTR0:
	DW		8*3 - 1
	DD		GDT0
	ALIGNB	16

bootpack: