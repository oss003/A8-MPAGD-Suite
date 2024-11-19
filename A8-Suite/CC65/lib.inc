; Atari 800 hardware specific routines

systeminit:

	sei			; Disable interrupts

;disable_irq_and_dma:
    lda #0
    sta IRQEN
    sta NMIEN

; Switch ROMS off

	lda PBCTL
	ora #%00000010
	sta PBCTL
	lda PORTB
	and #%01111100
	ora #%10000010
	sta PORTB

; Setup_nmi
    lda #<nmi_handler
    sta NMI
    lda #>nmi_handler
    sta NMI+1
    lda #%11000000
    sta NMIEN

; Init video registers

	lda #<BITMAP_DLIST
	sta DLISTL
	lda #>BITMAP_DLIST
	sta DLISTH

; Screenwidth 32 bytes

	lda #%00110001
	sta DMACTL

	jsr cls

	rts

nmi_handler:
    bit NMIST
    bmi dli_handler
    bpl nmi_not_dli
nmi_not_vbi:
    lda #%00100000
    bit NMIST
    bne nmi_not_reset
    sta NMIRES
    rti
nmi_not_reset:
    pla
    rti
nmi_not_dli:
    pha
    bvc nmi_not_vbi
    txa
    pha
    tya
    pha

    lda #$FF
    sta COLBK
    sta WSYNC
    sta WSYNC
    lda #0
    sta COLBK

    sta NMIRES
    pla
	tay
	pla
	tax
	pla
	rti

dli_handler:
	rti

PORTB		= $D301
PBCTL		= $D303
DMACTL		= $D400
DLISTL		= $D402
DLISTH		= $D403
GFX_MEM		= $E000

NMIEN       = $D40E
NMIST       = $D40F
NMIRES      = $D40F
NMI         = $FFFA
RESET       = $FFFC
IRQ         = $FFFE
RANDOM      = $D20A
IRQEN       = $D20E
IRQST       = $D20E

COLPF0      = $D016
COLPF1      = $D017
COLPF2      = $D018
COLPF3      = $D019
COLBK       = $D01A

VCOUNT      = $D40B
WSYNC       = $D40A

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


.align $100

BITMAP_DLIST:
	.byte BLANK8
	.byte BLANK8
	.byte BLANK8

	.byte LMS+MODEF
	.word ScreenAddr
	.res 95,MODEF

 	.byte LMS+MODEF
	.word ScreenAddr+96*32
	.res 95,MODEF

	.byte DLISTENDJUMP
	.word BITMAP_DLIST
