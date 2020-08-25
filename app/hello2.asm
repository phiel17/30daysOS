bits 32
GLOBAL	app_main

section .text
app_main:
	MOV		EDX, 2
	MOV		EBX, msg
	INT		0x40

	MOV		EDX, 4
	INT		0x40

section .data
msg:
	DB	"hello", 0