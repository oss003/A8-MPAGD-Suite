;----------------------------------------------------------------------
; Atari 800 Platform Specific functions
;----------------------------------------------------------------------

;============================================================
; PLATFORM VARIABLES
;============================================================
;
; Memory map:
;
;	$0000-$01ff	 0.50kB	CPU WS
;	$0200-$05ff	 1,00kB	IOCB/Printer/Cassette RAM
;	$0600-$06ff	 0,25kB	User RAM
;	$0700-$1fff	 6,25kB	
;	$2000-$ffff	56,00kB	User RAM
;
; Keyboard matrix:
;
;		0	1	2	3	4	5	6	7
;       ----------------------------------------------------------------------
;	$00:	L	J	;:	F1	F2	K	+\	*^
;	$08:	O		P	U	Ret	I	-_	=|
;	$10:	V	Help	C	F3	F4	B	X	Z
;	$18:	4$		3#	6&	Esc	5%	2"	1!
;	$20:	,[	Space	.]	N		M	/?	Invert
;	$28:	R		E	Y	Tab	T	W	Q
;	$30:	9(		0)	7'	Bksp	8@	<	>
;	$38:	F	H	D		Caps	G	S	A
;
; Atari 800 constants:

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

; Atari 800 Addresses

	HPOSP0	= $D000		; HORIZONTAL POSITION P0
	HPOSP1	= $D001		; HORIZONTAL POSITION P1
	HPOSP2	= $D002		; HORIZONTAL POSITION P2	
	HPOSP3	= $D003		; HORIZONTAL POSITION P3
	HPOSM0	= $D004		; HORIZONTAL POSITION M0
	HPOSM1	= $D005		; HORIZONTAL POSITION M1
	HPOSM2	= $D006		; HORIZONTAL POSITION M2
	HPOSM3	= $D007		; HORIZONTAL POSITION M3

	SIZEP0	= $D008		; SIZE P0
	SIZEP1	= $D009		; SIZE P0
	SIZEP2	= $D00A		; SIZE P0
	SIZEP3	= $D00B		; SIZE P0
	SIZEM	= $D00C		; SIZE M

	GRAFP0	= $D00D
	GRAFP1	= $D00E
	GRAFP2	= $D00F
	GRAFP3	= $D010
	GRAFM	= $D011

	TRIG0	= $D010
	TRIG1	= $D011

	COLPM0	= $D012		; COLOR P0/M0
	COLPM1	= $D013		; COLOR P1/M1
	COLPM2	= $D014		; COLOR P2/M2
	COLPM3	= $D015		; COLOR P3/M3

	COLPF0	= $D016
	COLPF1	= $D017
	COLPF2	= $D018
	COLPF3	= $D019
	COLBK	= $D01A

	KBCODE	= $D209
	RANDOM	= $D20A
	IRQEN	= $D20E
	IRQST	= $D20E
	SKSTAT	= $D20F

	STICK	= $D300
	PORTB	= $D301
	PBCTL	= $D303

	DMACTL	= $D400
	DLISTL	= $D402
	DLISTH	= $D403
	WSYNC	= $D40A
	VCOUNT	= $D40B
	NMIEN	= $D40E
	NMIST	= $D40F
	NMIRES	= $D40F

	NMI	= $FFFA
	RESET	= $FFFC
	IRQ	= $FFFE

;----------------------------------------------------------------------
; Atari 800 hardware fns
;----------------------------------------------------------------------

init:

; Disable interrupts

	sei		
	lda #0
	sta IRQEN		; Disable IRQ
	sta NMIEN		; Disable NMI

; Switch BASIC + OS ROMS off

	lda PBCTL
	ora #%00000010
	sta PBCTL
	lda PORTB
	and #%01111100
	ora #%10000010
	sta PORTB

; Setup new NMI handler

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

; NMI interrupt handler

frame_counter:	.byte 0,0
frame_edge:	.byte 0

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

; Used for syncing to 25 Hz

	inc frame_counter
	bne skip_inc_frame_counter
	inc frame_counter+1
skip_inc_frame_counter:

; Restore registers

	pla
	tay
	pla
	tax
	pla
	rti

dli_handler:
	rti

;========================================================
; KEY AND JOYSTICK ROUTINES
;--------------------------------------------------------
; Keys
;
; Out: joyval=x65FUDLR (bit cleared if key pressed)
;             ||||||||
;             |||||||+> Right    KEY 0  - X
;             ||||||+-> Left     KEY 1  - Z
;             |||||+--> Down     KEY 2  - .
;             ||||+---> Up       KEY 3  - ;
;             |||+----> Fire1    KEY 4  - SPC
;             ||+-----> Fire2    KEY 5  - Q
;             |+------> Fire3    KEY 6  - P
;             +-------> Not used
;
;                       Option1  KEY 7  - 1
;                       Option2  KEY 8  - 2
;                       Option3  KEY 9  - 3
;                       Option4  KEY 10 - 4
;--------------------------------------------------------
;----------------------------------------------------
; PRSKEY
; Wait for keypress.
;----------------------------------------------------

prskey:
	lda SKSTAT		; Check keypress
	cmp #255
	beq :+
	jmp prsend
:
	jsr joykey		; Check joystick move
	cmp #255
	beq prskey
prsend:
	rts

;--------------------------------------------------------
; KTEST
; Check if key in A is pressed,
;  carry clr = key pressed
;  carry set = key not pressed
;--------------------------------------------------------

ktest:
	sta tmp
	lda SKSTAT		; Check keypress
	and #4
	bne notpressed
	lda tmp
	cmp KBCODE
	beq pressed
notpressed:
	sec
	rts
pressed:
	clc			; key pressed
	rts

;---------------------------------------------------------------
; KGET
; Return keycode when a key is pressed and released
;---------------------------------------------------------------

kget:
	lda SKSTAT		; Check keypress
	and #4
	bne kget
	lda KBCODE		; If pressed, get keycode
	pha			; Save keycode pressed
kget2:
	lda SKSTAT		; Check keypress
	and #4
	beq kget2
	pla
	rts

;-------------------------------------------------------
; Joystick routines.
;
; contrl = 1, Joystick1
;          2, Joystick2
;-------------------------------------------------------

; Iitialize joysticks

joyinit:
	rts

; Joystick1 controls.

joy1:
	lda #0		 	; zero reading.
	sta joyval

	lda keys+6		; Check Fire3
	jsr ktest
	rol joyval
	lda keys+5		; Check Fire2
	jsr ktest
	rol joyval
	lda TRIG0		; Check Fire
	lsr
	rol joyval

	lda STICK
	sta tmp
	lsr tmp			; Check up
	rol joyval
	lsr tmp			; Check dwon
	rol joyval
	lsr tmp			; Check left
	rol joyval
	lsr tmp			; Check right
	rol joyval

	rts

; Joystick2 controls.

joy2:
	lda #0		 	; zero reading.
	sta joyval

	lda keys+6		; Check Fire3
	jsr ktest
	rol joyval
	lda keys+5		; Check Fire2
	jsr ktest
	rol joyval
	lda TRIG1		; Check Fire
	lsr
	rol joyval

	lda STICK
	lsr a
	lsr a
	lsr a
	lsr a
	sta tmp
	lsr tmp			; Check up
	rol joyval
	lsr tmp			; Check dwon
	rol joyval
	lsr tmp			; Check left
	rol joyval
	lsr tmp			; Check right
	rol joyval

	rts
	rts

;---------------------------------------------------------------
; Bitmap display list must be aligned at $100
;---------------------------------------------------------------

.align $100

.if gflag 
	GFXMODE = MODEE
.else
	GFXMODE = MODEF
.endif

BITMAP_DLIST:
	.byte BLANK8
	.byte BLANK8
	.byte BLANK8

	.byte LMS+GFXMODE
	.word ScreenAddr
	.res 95,GFXMODE

 	.byte LMS+GFXMODE
	.word ScreenAddr+96*32
	.res 95,GFXMODE

	.byte DLISTENDJUMP
	.word BITMAP_DLIST

