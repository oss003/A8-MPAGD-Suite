;----------------------------------------------
; Common AGD engine
; Z80 conversion by Kees van Oss 2017
;----------------------------------------------
	.DEFINE asm_code $2000
	.include "game.cfg" 

.segment "ZEROPAGE"
	.include "z80-zp.inc"
	.include "engine-zp.inc"

.segment "CODE"
.org asm_code-6
	.word $ffff
	.word start_asm
	.word eind_asm - 1



exec:
start_asm:
	.include "game.inc"
	.include "z80.asm"
eind_asm:
eop:					; End Of Program

.out .sprintf("Gamecode size = %d bytes, free space = %d bytes", (eind_asm - start_asm),(32000-eind_asm + start_asm))
