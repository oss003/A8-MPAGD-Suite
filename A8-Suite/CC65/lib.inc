; Atari 800 hardware specific routines

systeminit:

	sei			; Disable interrupts

; Switch ROMS off

	lda PBCTL
	ora #%00000010
	sta PBCTL
	lda PORTB
	and #%01111100
	ora #%10000010
	sta PORTB

; Init video registers

	lda #<BITMAP_DLIST
	sta DLISTL
	lda #>BITMAP_DLIST
	sta DLISTH

; Screenwidth 32 bytes

	lda #%00110001
	sta DMACTL

	jsr cls

endtest:
	jmp endtest

.align $100

PORTB		= $D301
PBCTL		= $D303
DMACTL		= $D400
DLISTL		= $D402
DLISTH		= $D403
GFX_MEM		= $E000

LMS		= 64
HS		= 16
VS		= 32
MODE2		= 2
MODE3		= 3
MODE4		= 4
MODE5		= 5
MODEB		= $0B
MODED		= $0D
MODEE		= $0E
MODEF		= $0F

BLANK1		= $00
BLANK2		= $10
BLANK3		= $20
BLANK4		= $30
BLANK5		= $40
BLANK6		= $50
BLANK7		= $60
BLANK8		= $70

DLISTENDJUMP	= $41

BITMAP_DLIST:
	.byte BLANK8
	.byte BLANK8
	.byte BLANK8

	.byte LMS+MODEF
	.word ScreenAddr
	.res 95,MODEF

 	.byte LMS+MODEF
	.word $fc00
	.res 95,MODEF

	.byte DLISTENDJUMP
	.word BITMAP_DLIST
