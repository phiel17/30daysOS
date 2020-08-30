bits 32

GLOBAL	api_end

api_end:
	MOV		EDX, 4
	INT		0x40
