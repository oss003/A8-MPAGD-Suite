;--------------------------------------------------------------
; Arcade Game Designer.
; (C) 2008 Jonathan Cauldwell.
; ZX Spectrum Next Engine v0.7.
; Acorn Atom port by K.v.Oss 2018
;--------------------------------------------------------------

; Conditional compilation flags
; Code is installed if flag is set
; Flags are set in commandline assembly or by the compiler

; Flags set by AGD compiler
;	mflag  			; MENU + INV
;	pflag 			; particle engine
;	sflag 			; scrollytext
;	dflag 			; digging mode
;	cflag			; collectable blocks
;	oflag			; objects
;	lflag			; ladders
;
; Flags set manually
;	aflag			; adventure mode
;	bflag			; big sprites (16x24)
;	iflag			; invert mode

.if mflag 
	.out "- MEN/INV enabled"  
.endif
.if pflag 
	.out "- Particles enabled" 
.endif
.if sflag 
	.out "- Scrolling enabled" 
.endif
.if dflag
	.out "- Digging enabled" 
.endif
.if cflag
	.out "- Collectable blocks enabled" 
.endif
.if oflag
	.out "- Objects enabled" 
.endif
.if lflag
	.out "- Ladders enabled" 
.endif
.if aflag
	.out "- Adventure mode enabled" 
.endif
.if bflag
	.out "- Big Sprites (16x24) enabled" 
.endif
.if iflag
	.out "- Invert mode enabled" 
.endif
.if rflag
	.out "- RAM saving mode enabled" 
.endif

; Global definitions ------------------------------------------------------------

	FONT = font		; Font address

	green	= $00		; Colour definition filters
	yellow	= $55
	blue	= $aa
	red	= $ff

; Block characteristics.

	PLATFM	= 1		; platform.
	WALL	= PLATFM + 1	; solid wall.
	LADDER	= WALL + 1	; ladder.
	FODDER	= LADDER + 1	; fodder block.
	DEADLY	= FODDER + 1	; deadly block.
	CUSTOM	= DEADLY + 1	; custom block.
	WATER	= CUSTOM + 1	; water block.
        COLECT	= WATER + 1	; collectable block.
        NUMTYP	= COLECT + 1	; number of types.

; Sprites.

.if bflag 
	SPR_HGT	= 24		; Sprite height
	SPR_WID = 16		; Sprite width
	NUMSPR	= 8		; number of sprites.
.else				; 16x16 sprites
	SPR_HGT = 16		; Sprite height
	SPR_WID = 16		; Sprite width
	NUMSPR	= 12		; number of sprites.
.endif

	TABSIZ = 17		; size of each entry.
	SPRBUF = NUMSPR * TABSIZ; size of entire table.
	NMESIZ = 4		; bytes stored in nmetab for each sprite.
	var_X  = 8		; new x coordinate of sprite.
	var_Y  = var_X + 1	; new y coordinate of sprite.
	PAM1ST = 5		; first sprite parameter, old x (ix+5).

; Particle engine.

	NUMSHR = 55		; pieces of shrapnel.
	SHRSIZ = 6		; bytes per particle.

; System constants

	ScrSelAddr  = $b000	; Screen selection address
	KeyRowAddr  = $b000	; PIA key matrix row
	KeyColAddr  = $b001	; PIA key matrix column
	SpeakerBit  = $b002	; Speaker bit
	CmdReg      = $b400	; Command register AtoMMC
	DatReg      = $b401	; Data register AtoMMC 
	Timer2_Low  = $b808	; Timer 2 low byte
	Timer2_High = $b809	; Timer 2 high byte
	ScreenAddr  = $e400	; Startaddress video RAM

	MAP 	    = $dd00		; properties map buffer (3x256 bytes)
	SCADTB_lb   = MAP+3*256		; screen address table lo-byte (256 bytes)
	SCADTB_hb   = SCADTB_lb+256	; screen address table hi-byte (256 bytes)
	SHRAPN      = SCADTB_hb+256	; shrapnel table (55x6 bytes)

; System calls

	SCRSYNC	 = $fe66	; wait for next CRT field flyback 60 Hz
	SCRSYNC1 = $fe6b	; wait for CRT field flyback 60 Hz
	OSRDCH   = $fe94	; get key subroutine
        READKEY  = $fe71	; scan key matrix subroutine
	OSWRCH	 = $fff4	; write character


.if iflag
	TxtInvert   = $ff	; Invert byte for character printing
	ScrFillByte = $ff	; Screen fill byte for CLS
.else
	TxtInvert   = $00	; Invert byte for character printing
	ScrFillByte = $00	; Screen fill byte for CLS
.endif

	ScrMode     = $f0	;  $f0 = CLEAR4  mono

;===============================================================
; Game starts here
;===============================================================

;.segment "CODE"

	jsr init

; Clear variables

	lda #0
	ldx #0
clrloop:
	sta 0,x
	inx
	bne clrloop

gameloop:
	jsr game	 	; start the game.
	jmp gameloop		; Start again

; Definition of variables
; Make sure pointers are arranged in the same order as the data itself.

msgptr:		.word msgdat	; messages
blkptr:		.word chgfx	; block graphics.
proptr:		.word bprop	; address of char properties.
sptptr:		.word sprgfx	; sprite graphics
frmptr:		.word frmlst	; sprite frames.
scrptr:		.word scdat	; address of screens.
nmeptr:		.word nmedat	; enemy start positions.
objptr:		.word objdta	; object graphics

;TxtFilter:	.byte $ff
txtcol:		.byte green,yellow,blue,red

; Don't change the order of these four.  
; Menu routine relies on winlft following wintop.

wintop:		.byte WINDOWTOP	; top of window.
winlft:		.byte WINDOWLFT	; left edge.
winhgt:		.byte WINDOWHGT	; window height.
winwid:		.byte WINDOWWID	; window width.
numob:		.byte NUMOBJ	; number of objects in game.

; Variables start here.
; Pixel versions of wintop, winlft, winhgt, winwid.

wntopx:		.byte (8 * WINDOWTOP)
wnlftx:		.byte (8 * WINDOWLFT)
wnbotx:		.byte ((WINDOWTOP * 8) + (WINDOWHGT * 8) - 16)
wnrgtx:		.byte ((WINDOWLFT * 8) + (WINDOWWID * 8) - 16)-2

; Assorted game routines which can go in contended memory.

;--------------------------------------------------------------
; Entry for inventory called by the INV command
;
; Input:
;  X   = message nr with objects seperated with ,
;
; Output:
;  OPT = selected objectnr of INV menu
;--------------------------------------------------------------

.if mflag
minve:
	lda #<(invdis)		; routine address.
	sta mod0+1		; set up menu routine.
	sta mod2+1		; set up count routine.
	lda #>(invdis)
	sta mod0+2
	sta mod2+2
	lda #<(fopt)		; find option from available objects.
	sta mod1+1		; set up routine.
	lda #>(fopt)
	sta mod1+1+1
	jmp dbox		; do menu routine.

;--------------------------------------------------------------
; Entry for menu called by the MENU command
;
; Input:
;  X   = message nr with menu items seperated with ,
;
; Output:
;  OPT = selected line nr of MENU menu
;--------------------------------------------------------------

mmenu:
	lda #<(always)		; routine address.
	sta mod0+1		; set up routine.
	sta mod2+1		; set up count routine.
	lda #>(always)
	sta mod0+2
	sta mod2+2

	lda #<(fstd)		; standard option selection.
	sta mod1+1		; set up routine.
	lda #>(fstd)
	sta mod1+2

; Drop through into box routine.

;--------------------------------------------------------------
; Work out size of box for message or menu.
;--------------------------------------------------------------

dbox:
	lda #<(msgdat)		; pointer to messages.
	sta z80_l
	lda #>(msgdat)
	sta z80_h
	
	jsr getwrd		; get message number.

	lda z80_h		; store pointer to message.
	sta TmpAddr
	lda z80_l
	sta TmpAddr+1

	lda #1			; height.
	sta z80_d
	lda #0			; start at object zero.
	sta combyt		; store number of object in combyt.
	sta z80_e		; maximum width.
dbox5:
	lda #0			; this line"s width.
	sta z80_b
mod2:
	jsr always		; item in player"s possession?
	cmp #255
	bne dbox6		; not in inventory, skip this line.
	inc z80_d		; add to tally.
dbox6:
	ldy #0			; get character.
	lda (z80_hl),y
	sta z80_a
	inc z80_l		; next character.
	bne :+
	inc z80_h
:
	lda z80_a		; reached end of line?
	cmp #','
	beq dbox3		; yes.
	cmp #13
	beq dbox3		; yes.
	inc z80_b		; add to this line"s width.
	lda z80_a
	bmi dbox4		; end of message? yes, end count.
	jmp dbox6		; repeat until we find the end.
dbox3:
	lda z80_e		; maximum line width.
	cmp z80_b		; have we exceeded longest so far?
	bpl dbox5		; no, carry on looking.
	lda z80_b		; make this the widest so far.
	sta z80_e
	jmp dbox5		; keep looking.
dbox4:
	lda z80_e		; maximum line width.
	cmp z80_b		; have we exceeded longest so far?
	bpl dbox8		; no, carry on looking.
	lda z80_b		; final line is the longest so far.
	sta z80_e
dbox8:
	dec z80_d		; decrement items found.
	bne :+			; total was zero.
	lda #255
	sta varopt
	jmp dbox15		
:
	lda z80_e		; longest line.
	bne :+			; was it zero?
	jmp dbox15		; total was zero.
:
	sta bwid		; set up size.
	lda z80_d
	sta blen

;--------------------------------------------------------------
; That's set up our box size.
;--------------------------------------------------------------

	lda winhgt		; window height in characters.
	sec
	sbc z80_d		; subtract height of box.
	lsr a			; divide by 2.
	clc
	adc wintop		; add top edge of window.
	sta btop		; set up box top.

	lda winwid		; window width in characters.
	sec	
	sbc z80_e		; subtract box width.
	lsr a			; divide by 2.
	clc
	adc winlft		; add left edge of window.
	sta blft		; box left.

	lda #<(FONT-256)		; font.
	sta grbase		; set up for text display.
	lda #>(FONT-256)
	sta grbase+1

	lda TmpAddr+1		; restore message pointer.
	sta z80_l
	lda TmpAddr
	sta z80_h

	lda btop		; box top.
	sta dispy		; set display coordinate.
	lda #0			; start at object zero.
	sta combyt		; store number of object in combyt.
dbox2:
	lda combyt		; get object number.
	sta z80_a
mod0:
	jsr always		; check inventory for display.
	cmp #255
	beq :+			
	jmp dbox13		; not in inventory, skip this line.
:
	lda blft		; box left.
	sta dispx		; set left display position.
	lda bwid		; box width.
	sta z80_b		; store width.
dbox0:
	ldy #0
	lda (z80_hl),y		; get character.
	cmp #','		; end of line?
	beq dbox1		; yes, next one.
	cmp #13			; end of line?
	beq dbox1		; yes, next one.

	cmp #141			; end of line?
	bne :+
	dec bwid
	jmp dbox7		; yes, next one.
:
	dec z80_b		; one less to display.
	and #127		; remove terminator.

	jsr pchr		; display on screen.

	ldy #0
	lda (z80_hl),y		; get character.
	sta z80_a
	inc z80_l		; next character.
	bne :+
	inc z80_h
:
	lda z80_a
	cmp #128		; end of message?
	bmi :+
	jmp dbox7		; yes, job done.
:
	lda z80_b		; chars remaining.
	beq :+			; are any left?
	jmp dbox0		; yes, continue.
:
;---------------------------------------------------
; Reached limit of characters per line.
;---------------------------------------------------

dbox9:
	ldy #0
	lda (z80_hl),y		; get character.
	inc z80_l		; next one.
	bne :+
	inc z80_h
:
	cmp #','		; another line?
	beq dbox10		; yes, do next line.
	cmp #13			; another line?
	beq dbox10		; yes, do next line.
	cmp #128		; end of message?
	bcs :+
	jmp dbox11		; yes, finish message.
:
	jmp dbox9

;---------------------------------------------------
; Fill box to end of line.
;---------------------------------------------------

dboxf:
	lda #32			; space character.
	jsr pchr		; display character.
	dec z80_b
	beq :+
	jmp dboxf		; repeat for remaining chars on line.
:
	rts
dbox1:
	inc z80_l		; skip character.
	bne :+
	inc z80_h
:
	jsr dboxf		; fill box out to right side.
dbox10:
	inc dispy		; y coordinate down a line next position.
	jmp dbox2		; next line.
dbox7:
	lda z80_b		; chars remaining.
	bne :+			; are any left?
	jmp dbox11		; no, nothing to draw.
:
	jsr dboxf		; fill message to line.

;------------------------------------------------------
; Drawn the box menu, now select option.
;------------------------------------------------------

dbox11:
	lda btop		; box top.
	sta dispy		; set bar position.
dbox14:
	jsr joykey		; get controls.
	cmp #$7f		; anything pressed?
	bne dbox14		; yes, debounce it.
	jsr dbar		; draw bar.
dbox12:
	jsr joykey		; get controls.
	cmp #$7f		; anything pressed?
	beq dbox12		; no, nothing.
	and #16			; fire button pressed?
	bne :+
mod1:
	jmp fstd		; yes, job done.
:
	jsr dbar		; delete bar.

	lda joyval		; joystick reading.
	and #8			; going up?
	beq dboxu		; yes, go up.

	ldx dispy		; vertical position of bar.
	inx			; look down.
	txa
	sec
	sbc btop		; find distance from top.
	cmp blen		; top of box.
	bne :+
	jmp dbox14		; yes, go no further.
:
	inc dispy		; move bar.
	jmp dbox14		; continue.
dboxu:
	lda dispy		; vertical position of bar.
	cmp btop		; are we at the top?
	bne :+
	jmp dbox14		; yes, go no further.
:
	dec dispy		; move bar.
	jmp dbox14		; continue.
fstd:
	lda dispy		; bar position.
	sec
	sbc btop		; find selected option.
	sta varopt		; store the option.
	jmp redraw		; redraw the screen.

;------------------------------------------------------
; Option not available.  Skip this line.
;------------------------------------------------------

dbox13:
	ldy #0
	lda (z80_hl),y		; get character.
	inc z80_l		; next one.
	bne :+
	inc z80_h
:
	cmp #','		; another line?
	bne :+
	jmp dbox2		; yes, do next line.
:
	cmp #13			; another line?
	bne :+
	jmp dbox2		; yes, do next line.
:

	bpl :+			; end of message?
	jmp dbox11		; yes, finish message.
:
	jmp dbox13
dbox15:
	lda TmpAddr		; pop message pointer from the stack.
	sta z80_h
	lda TmpAddr+1
	sta z80_l
	rts

;------------------------------------------------------
; Invert bar
;------------------------------------------------------

dbar:
	lda blft		; box left.
	sta dispx		; set display coordinate.
	jsr gprad		; get printing address.

	lda bwid		; box width.
	sta z80_c		; loop counter in c.
	lda z80_h		
	sta z80_d		; store screen address high byte.
dbar1:
	ldx #7			; pixel height in b.
dbar0:
	ldy scrtab,x
	lda (scraddr),y		; get screen byte.
	eor #255		; reverse all bits.
	sta (scraddr),y		; write back to screen.
	dex			; next line down.
	bpl dbar0		; draw rest of character.

	inc scraddr		; one char right.
	dec z80_c		; decrement character counter.
	bne dbar1		; repeat for whole line.
	rts

;------------------------------------------------------
; Point to object
;
; Input:
;  combyt
;
; Output:
;  A = object number, A=255 if already in possession
;------------------------------------------------------

invdis:
	lda z80_l		; store message text pointer.
	pha
	lda z80_h
	pha
	lda combyt		; object number.
	inc combyt		; ready for next one.
	jsr gotob		; check if we have object.
	tay
	pla
	sta z80_h
	pla
	sta z80_l
	tya
	rts

;------------------------------------------------------
; Find option selected.
;
; Input:
;  -
;
; Output:
;  OPT = selected object
;------------------------------------------------------

fopt:
	lda dispy
	sec
	sbc btop		; find selected option.
	sta tmp+2		; option selected in b register.
	inc tmp+2

	lda #0			; set to first item.
	sta combyt		; object number.
fopt0:
	jsr fobj		; find next object in inventory.
	dec tmp+2
	bne fopt0		; repeat for relevant steps down the list.

	lda combyt		; get option.
	sta varopt		; store the option.
	dec varopt		; one less, due to where we increment combyt.
	jmp redraw		; redraw the screen.
fobj:
	ldy combyt		; object number.
	inc combyt		; ready for next item.
	tya
	jsr gotob		; do we have this item?
	cmp #255
	bne :+
	rts
:
	jmp fobj		; yes, it's on the list.

;bwid:	.byte 0              ; box/menu width.
;blen:	.byte 0              ; box/menu height.
;btop:	.byte 0              ; box coordinates.
;blft:	.byte 0
.endif

;----------------------------------------------------
; Delay routine 1/50 sec
;
; Wait 1/60 sec = 16666 usec
; Wait 208 x 16 =  3328 usec
;                 19994 usec
; rts           =     6 usec
; Total         = 20000 usec
;----------------------------------------------------

delay:
  rts
	sta xtmp
del_loop:
	jsr SCRSYNC		; wait for flyback

	ldy #208		; wait 208 x 16 = 3328 usec
delay1:
	adc ($80,x)		;	 6 usec
	adc ($80,x)		;	 6 usec
	dey			;	 2 usec
	bne delay1		;	 2 usec
	nop			; 2 usec
	nop			; 2 usec
				; tot: 20000 usec
	dec xtmp
	bne del_loop
	rts

;----------------------------------------------------
; Clear sprite table.
;
; sprtab[0] - sprtab[SPRBUF-1] = 255
;----------------------------------------------------

xspr:
	lda #255		; clear byte.
	ldx #0			; length of table.
xspr0:
	sta sprtab,x		; sprite table.
	inx			; move to next byte.
	cpx #SPRBUF
	bne xspr0		; repeat for rest of table.
	rts

;-------------------------------------------------------------
; Initialise all objects.
;
; Reset current room,y,x to start room,y,x for all objects
;-------------------------------------------------------------

.if oflag
iniob:
	lda #<objdta 		; objects table.
	sta z80_x
	lda #>objdta
	sta z80_i

	ldx numob 		; number of objects in the game.
iniob0:
	ldy #35
	lda (z80_ix),y 		; start screen.
	ldy #32
	sta (z80_ix),y 		; set start screen.

	ldy #36
	lda (z80_ix),y 		; find start y.
	ldy #33
	sta (z80_ix),y 		; set start y.

	ldy #37
	lda (z80_ix),y 		; get initial x.
	ldy #34
	sta (z80_ix),y 		; set x coord.

	clc 			; point to next object.
	lda z80_x
	adc #38			; distance between objects.
	sta z80_x
	bcc :+
	inc z80_i
:
	dex 			; repeat.
	bne iniob0

	rts
.endif

;-------------------------------------------------------------
; Screen synchronisation.
;
;  - read joystick/keyboard
;  - handle sound
;  - sync framerate with clock
;  - handle shrapnel every even frame
;-------------------------------------------------------------

vsync:
	pha
	tya
	pha
	txa
	pha
	jsr joykey		; read joystick/keyboard.
  jmp sndskip

vsync1:
	lda Timer2_High		; sync framerate with clock
	cmp #$ff
	bne vsync1
	lda #>50000
	sta Timer2_High
	lda #<50000
	sta Timer2_Low

	lda clock
	and #1
	bne:+
	jsr proshr		; handle shrapnel every even frame
:
	lda sndtyp
	beq sndskip
sndloop:
	lda SpeakerBit		; handle sound
	ldy sndtyp
sndwait:
	dey
	bne sndwait
	eor #4
	sta SpeakerBit
	dec sndtyp
	bne sndloop
sndskip: 
	pla
	tax
	pla
	tay
	pla
	rts

sndtyp:	.byte 0

;-----------------------------------------------
; Redraw the screen.
;
; Remove old copy of all sprites for redraw.
;-----------------------------------------------

redraw:
	lda z80_i 		; place sprite pointer on stack.
	pha
	lda z80_x
	pha

	jsr droom		; show screen layout.
.if oflag
	jsr shwob		; draw objects.
.endif
numsp0:
	lda #NUMSPR		; sprites to draw.
	sta tmp

	lda #<sprtab		; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
redrw0:
	ldy #0
	lda (z80_ix),y		; old sprite type.
	cmp #255		; is it enabled?
	beq redrw1 		; no, find next one.

	ldy #3
	lda (z80_ix),y 		; sprite y.
	cmp #177		; beyond maximum?
	bcs redrw1		; yes, nothing to draw.

	jsr sspria		; show single sprite.
redrw1:
	clc			; next sprite.
	lda z80_x
	adc #TABSIZ		; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec tmp			; repeat for remaining sprites.
	bne redrw0
rpblc1:
;	jsr dshrp		; redraw shrapnel.

.if aflag
	jsr rbloc		; draw blocks for this screen
.endif
	pla			; retrieve sprite pointer.
	sta z80_x
	pla
	sta z80_i

	rts

;----------------------------------------------------------------------
; Clear screen routine.
;
; Fill screenmem $e400-$fbff with ScrFillByte
;----------------------------------------------------------------------

cls:
	lda #>ScreenAddr		; screen address.
	sta clsloop+2
	lda #ScrFillByte
.if iflag
	eor #$ff
.endif
	ldy #0
clsloop:
	sta ScreenAddr,y
	iny
	bne clsloop
	inc clsloop+2
	ldx clsloop+2
	cpx #>(ScreenAddr+$1800)
	bne clsloop
	rts

;----------------------------------------------------------------------
; Atomic palette is static 
;----------------------------------------------------------------------

setpal:
	rts

;----------------------------------------------------------------------
; FODDER check
;----------------------------------------------------------------------

.if pflag .or dflag
fdchk:
	cmp #FODDER 		; is it fodder?
	beq :+
	rts 			; no.
:
	lda #0			; wipe fodder in MAP
	ldy #0
	sta (bufaddr),y 	; rewrite block type.

	lda dispx		; x=x/8
	pha
	lsr a
	lsr a
	lsr a
	sta dispx

	lda dispy		; y=y/8
	pha
	lsr a
	lsr a
	lsr a
	sta dispy

	lda #0 			; block to write.
	jsr pattr 		; write block.

	pla
	sta dispy
	pla
	sta dispx
	rts
.endif

;----------------------------------------------------
; Scrolly text and puzzle variables.
;----------------------------------------------------

.if sflag
txtbit:	.byte 128		; bit to write.
txtwid:	.byte 16		; width of ticker message.
txtpos:	.word msgdat
txtini:	.word msgdat
txtscr:	.word ScreenAddr
.endif

;----------------------------------------------------
; Specialist routines.
; Process shrapnel.
;----------------------------------------------------
proshr:
.if pflag
	lda #<SHRAPN		; table.
	sta z80_x
	lda #>SHRAPN
	sta z80_i

	lda #NUMSHR		; shrapnel pieces to process.
	sta shrctr
prosh0:
	ldy #0
	lda (z80_ix),y		; on/off marker.
	asl a
proshx:
	bcs :+
	jsr prosh1 		; on, so process it.
:
	clc
	lda z80_x
	adc #SHRSIZ
	sta z80_x
	bcc :+
	inc z80_i
:
	dec shrctr		; round again.
	bne prosh0
.endif
.if sflag
	jsr scrly
.endif
	rts

.if pflag
;shrctr:	.byte 0

;----------------------------------------------------
; Proces shrapnel piece
;----------------------------------------------------

prosh1:
	jsr plot 		; delete the pixel.

	lda #<shrptr		; shrapnel routine pointers.
	sta z80_l
	lda #>shrptr
	sta z80_h

	ldy #0
	lda (z80_ix),y		; restore shrapnel type.
	jsr prosh2 		; run the routine.
	jsr chkxy		; check x and y are good before we redisplay.

	lda #<SHRSIZ 		; distance to next.
	sta z80_e
	lda #>SHRSIZ
	sta z80_d
	rts

;----------------------------------------------------
; Run the routine
;----------------------------------------------------

prosh2:
	asl a 			; 2 bytes per address.
	tay
	lda shrptr,y
	sta z80_l
	lda shrptr+1,y 		; fetch high byte from table.
	sta z80_h
	jmp (z80_hl) 		; jump to routine.

;----------------------------------------------------
; Paricle routine table
;----------------------------------------------------

shrptr:	.word laser		; laser.
	.word trail		; vapour trail.
	.word shrap		; shrapnel from explosion.
	.word dotl		; horizontal starfield left.
	.word dotr		; horizontal starfield right.
	.word dotu		; vertical starfield up.
	.word dotd		; vertical starfield down.
	.word ptcusr		; user particle.

;----------------------------------------------------
; Explosion shrapnel.
;----------------------------------------------------

shrap:
	ldy #1
	lda (z80_ix),y 		; get the angle.
	clc
	adc #<shrsin		; shrapnel sine table.
	sta z80_l
	lda #>shrsin
	adc #0
	sta z80_h

	ldy #0
	lda (z80_hl),y 		; fetch value from table.
	sta z80_e
	inc z80_l 		; next byte of table.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y		; fetch value from table.
	sta z80_d
	inc z80_l		; next byte of table.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y 		; fetch value from table.
	sta z80_c
	inc z80_l 		; next byte of table.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y 		; fetch value from table.
	sta z80_b

	ldy #2
	lda (z80_ix),y 		; x coordinate in hl.
	clc
	adc z80_e		; add sine lb
	sta (z80_ix),y		; store new coordinate lb.
	ldy #3
	lda (z80_ix),y
	adc z80_d		; add sine hb
	sta (z80_ix),y		; store new coordinate hb.

	ldy #4
	lda (z80_ix),y	 	; y coordinate in hl.
	clc
	adc z80_c		; add cosine lb
	sta (z80_ix),y		; store new coordinate lb.
	ldy #5
	lda (z80_ix),y
	adc z80_b		; add cosine lb
	sta (z80_ix),y		; store new coordinate hb.

	rts

;----------------------------------------------------
; Move dots
;----------------------------------------------------

dotl:
	ldy #5
	lda (z80_ix),y
	sec
	sbc #1		 	; move left.
	sta (z80_ix),y
	rts
dotr:
	ldy #5
	lda (z80_ix),y
	clc
	adc #1		 	; move left.
	sta (z80_ix),y
	rts
dotu:
	ldy #3
	lda (z80_ix),y
	sec
	sbc #1		 	; move up.
	sta (z80_ix),y
	rts
dotd:
	ldy #3
	lda (z80_ix),y
	clc
	adc #1			; move down.
	sta (z80_ix),y
	rts

;----------------------------------------------------
; Check if coordinates are ok before redrawing at new position.
;
; left:   X>L		X=L	Ok
; right:  R+15>X	X=R	Ok
; top:    Y>T		Y=T	Ok
; bottom: B+15>Y	Y=B	Ok
;----------------------------------------------------

chkxy:

; top:    Y>T		Y=T	Ok

	ldy #3
	lda (z80_ix),y	 	; fetch shrapnel coordinate.
	cmp wntopx		; window top.
	bcs :+			; compare with top window limit.
	jmp kilshr		; out of window, kill shrapnel.
:
; left:   X>L		X=L	Ok

	ldy #5
	lda (z80_ix),y	 	; fetch shrapnel coordinate.
	cmp wnlftx		; left edge.
	bcs :+			; compare with left window limit.
	jmp kilshr		; out of window, kill shrapnel.
:
; bottom: B+15>Y	Y=B	Ok

	lda wnbotx		; point to bottom.
	clc
	adc #15
	ldy #3
	cmp (z80_ix),y	 	; fetch shrapnel coordinate.
	bcs :+			; compare with shrapnel x coordinate.
	jmp kilshr		; off screen, kill shrapnel..
:
; right:  R+15>X	X=R	Ok

	lda wnrgtx		; point to right edge.
	clc
	adc #15
	ldy #5
	cmp (z80_ix),y	 	; fetch shrapnel coordinate.
	bcs :+			; compare with window limit.
	jmp kilshr		; off screen, kill shrapnel.
:

;----------------------------------------------------
; Drop through.
; Display shrapnel.
;----------------------------------------------------

plot:
	ldy #3
	lda (z80_ix),y		; y integer.
	sta dispy	 	; workspace coordinates.
	ldy #5
	lda (z80_ix),y	 	; x integer.
	sta dispx 		; workspace coordinates.

	ldy #0
	lda (z80_ix),y 		; type.
	bne :+			; is it a laser?
	jmp plot1 		; yes, draw laser instead.
:
plot0:
	lda dispx		; which pixel within byte do we
	and #7			; want to set first?
	tay
	lda dots,y 		; table of small pixel positions.
	sta z80_e 		; get value.

	jsr scadd 		; screen address.
	ldy #0
	lda (scraddr),y		; see what's already there.
	eor z80_e
	sta (scraddr),y 	; put back on screen.
	rts

plot1:
	jsr scadd 		; screen address.
	ldy #0
	lda (scraddr),y 	; fetch byte there.
	eor #255 		; toggle all bits.
	sta (scraddr),y 	; new byte.
	rts

;----------------------------------------------------
; Switch off shrapnel
;----------------------------------------------------

kilshr:
	lda #128
	ldy #0
	sta (z80_ix),y	; switch off shrapnel.
	rts

;----------------------------------------------------
; Sine/cosine table
;----------------------------------------------------

shrsin:	.word 0,1024,391,946,724,724,946,391
	.word 1024,0,946,65144,724,64811,391,64589
	.word 0,64512,65144,64589,64811,64811,64589,65144
	.word 64512,0,64589,391,64811,724,65144,946

;----------------------------------------------------
; Create trail
;----------------------------------------------------

trail:
	ldy #1
	lda (z80_ix),y 	; time remaining.
	sec
	sbc #1
	sta (z80_ix),y
	bne :+
	jmp trailk		; time to switch it off.
:
	jsr qrand		; get a random number.
	lsr a 			; x or y axis?
	bcc :+
	jmp trailv		; use y.
:
; Trail horizontal

	lsr a 			; which direction?
	bcc :+
	jmp traill		; go left.
:
; Trail right

	ldy #5
	lda (z80_ix),y
	clc
	adc #1	 		; go right.
	sta (z80_ix),y
	rts

; Trail left

traill:
	ldy #5
	lda (z80_ix),y
	sec
	sbc #1 			; go left.
	sta (z80_ix),y
	rts

; Trail vertical

trailv:
	lsr a		 	; which direction?
	bcc :+
	jmp trailu		; go up.
:
; Trail down

	ldy #3
	lda (z80_ix),y
	clc
	adc #1 			; go down.
	sta (z80_ix),y
	rts

; Trail up

trailu:
	ldy #3
	lda (z80_ix),y
	sec
	sbc #1 			; go up.
	sta (z80_ix),y
	rts

; Kill trail

trailk:
	lda #200		; set off-screen to kill vapour trail.
	ldy #3
	sta (z80_ix),y
	rts

;----------------------------------------------------
; Create laser beam
;----------------------------------------------------

laser:
	ldy #1
	lda (z80_ix),y 		; direction.
	ror a 			; left or right?
	bcs :+
	jmp laserl		; move left.
:
; Laser right

	lda #8			; distance to travel.
	sta z80_b
	jmp laserm		; move laser.

; Laser left

laserl:
	lda #248		; distance to travel.
	sta z80_b
laserm:
	ldy #5
	lda (z80_ix),y		; x position.
	clc
	adc z80_b		; add distance.
	sta (z80_ix),y		; set new x coordinate.

; Test new block.

	sta dispx 		; set x for block collision detection purposes.
	ldy #3
	lda (z80_ix),y 		; get y.
	sta dispy		; set coordinate for collision test.
	jsr tstbl 		; get block type there.
	cmp #WALL		; is it solid?
	bne :+
	jmp trailk		; yes, it cannot pass.
:
.if pflag .or dflag
        cmp #FODDER             ; is it fodder?
        bne :+
        jsr fdchk               ; remove fodder block.
        jmp trailk              ; destroy laser.
:
.endif
        rts                     ; no, ignore it.

;----------------------------------------------------
; Dots mask
;----------------------------------------------------

dots:	.byte 128,64,32,16,8,4,2,1


;----------------------------------------------------
; Plot, preserving de.
;----------------------------------------------------

plotde:
	lda z80_d 		; put de on stack.
	pha
	lda z80_e
	pha

	jsr plot 		; plot pixel.

	pla			; restore de from stack.
	sta z80_e
	pla
	sta z80_d

	rts

;----------------------------------------------------
; Shoot a laser.
;----------------------------------------------------

shoot:
	sta z80_c		; store direction in c register.
	ldy #8
	lda (z80_ix),y 		; y coordinate.
	clc
shoot1:
	adc #(SPR_HGT/2-1)	; down 7 pixels.
	sta z80_l 		; puty y coordinate in l.

	ldy #9
	lda (z80_ix),y 		; x coordinate in h.
	sta z80_h

	lda z80_i		; store pointer to sprite.
	pha
	lda z80_x
	pha

	jsr fpslot 		; find particle slot.
	bcs :+
	jmp vapou2		; failed, restore ix.
:
	lda #0
	ldy #0
	sta (z80_ix),y 		; set up a laser.

	lda z80_c
	ldy #1
	sta (z80_ix),y 		; set the direction.

	lda z80_l
	ldy #3
	sta (z80_ix),y		; set y coordinate.

	ror z80_c		; check direction we want.
	bcc :+
	jmp shootr		; shoot right.
:
	lda z80_h		; X position.
shoot0:
	and #248		; align on character boundary.
	ldy #5
	sta (z80_ix),y		; set x coordinate.
	jmp vapou0 		; draw first image.
shootr:
	lda z80_h		; x position.
	clc
	adc #15			; look right.
	jmp shoot0		; align and continue.

;----------------------------------------------------
; Create a bit of vapour trail.
;----------------------------------------------------

vapour:
	lda z80_i		; store pointer to sprite.
	pha
	lda z80_x
	pha

	ldy #8
	lda (z80_ix),y 		; y coordinate.
	clc
vapou3:
	adc #(SPR_HGT/2-1)	; mid-point of sprite.
	sta z80_l

	ldy #9
	lda (z80_ix),y 		; x coordinate.
	adc #7
	sta z80_h

	jsr fpslot 		; find particle slot.
	bcc :+
	jmp vapou1		; no, we can use it.
:
vapou2:
	pla
	sta z80_x
	pla
	sta z80_i
	rts
vapou1:
	lda z80_l
	ldy #3
	sta (z80_ix),y		; set up y.

	lda z80_h
	ldy #5
	sta (z80_ix),y 		; set up x coordinate.

	jsr qrand		; get quick random number.
	and #15			; random time.
	clc
	adc #15			; minimum time on screen.
	ldy #1
	sta (z80_ix),y		; set time on screen.

	lda #1
	ldy #0
	sta (z80_ix),y		; define particle as vapour trail.
vapou0:
	jsr chkxy		; plot first position.
	jmp vapou2

;----------------------------------------------------
; Create a user particle.
;----------------------------------------------------

ptusr:
	sta z80_f		; store timer.

	ldy #8
	lda (z80_ix),y 		; y coordinate.
	clc
	adc #7			; mid-point of sprite.
	sta z80_l

	ldy #9
	lda (z80_ix),y 		; x coordinate.
	clc
	adc #7			; mid-point of sprite.
	sta z80_h

	jsr fpslot 		; find particle slot.
	bcs ptusr1
	rts 			; out of slots, can't generate anything.
ptusr1:
	lda z80_l
	ldy #3
	sta (z80_ix),y 		; set up y.

	lda z80_h
	ldy #5
	sta (z80_ix),y		; set up x coordinate.

	lda z80_f 		; restore timer.
	ldy #1
	sta (z80_ix),y		; set time on screen.

	lda #7
	ldy #0
	sta (z80_ix),y		; define particle as user particle.

	jmp chkxy		; plot first position.

;----------------------------------------------------
; Create a vertical or horizontal star.
;----------------------------------------------------

star:
	lda z80_i		; store pointer to sprite.
	pha
	lda z80_x
	pha

	jsr fpslot 		; find particle slot.
	bcs star7		; found one we can use.
star0:
	pla 			; restore sprite pointer.
	sta z80_x
	pla
	sta z80_i
	rts 			; out of slots, can't generate anything.
star7:
	lda z80_c		; direction.
	and #3 			; is it left?
	bne :+
	jmp star1 		; yes, it's left.
:
	cmp #1 			; is it right?
	bne :+
	jmp star2 		; yes, it's right.
:
	cmp #2 			; is it up?
	bne :+
	jmp star3 		; yes, it's up.
:
	ldy wntopx 		; get edge of screen.
	iny			; down one pixel.
	tya
star8:
	ldy #3
	sta (z80_ix),y 		; set y coord.
	jsr qrand 		; get quick random number.
star9:
	ldy #5
	sta (z80_ix),y		; set x position.

	lda z80_c		; direction.
	and #3			; zero to three.
	clc
	adc #3			; 3 to 6 for starfield.
	ldy #0
	sta (z80_ix),y		; define particle as star.
	jsr chkxy		; plot first position.
	jmp star0
star1:
	jsr qrand		; get quick random number.
	ldy #3
	sta (z80_ix),y 		; set y coord.

	lda wnrgtx 		; get edge of screen.
	clc
	adc #15			; add width of sprite minus 1.
	jmp star9
star2:
	jsr qrand 		; get quick random number.
	ldy #3
	sta (z80_ix),y		; set y coord.

	lda wnlftx		; get edge of screen.
	jmp star9
star3:
	lda wnbotx 		; get edge of screen.
	clc
	adc #15 		; height of sprite minus one pixel.
	jmp star8

;----------------------------------------------------
; Find particle slot for lasers or vapour trail.
; can't use alternate accumulator.
;----------------------------------------------------

fpslot:
	lda #<SHRAPN 		; shrapnel table.
	sta z80_x
	lda #>SHRAPN
	sta z80_i

	lda #NUMSHR		; number of pieces in table.
	sta z80_b
fpslt0:
	ldy #0
	lda (z80_ix),y		; get type.
	asl a  			; is this slot in use?
	bcc :+
	rts			; no, we can use it.
:
	clc			; point to more shrapnel.
	lda z80_x
	adc #SHRSIZ
	sta z80_x
	bcc :+
	inc z80_i
:
	dec z80_b		; repeat for all shrapnel.
	bne fpslt0

	clc
	rts 			; out of slots, can't generate anything.

;----------------------------------------------------
; Create an explosion at sprite position.
;----------------------------------------------------

explod:
	sta z80_c 		; particles to create.

	lda z80_i 		; store pointer to sprite.
	pha
	lda z80_x
	pha

	ldy #8
	lda (z80_ix),y 		; y coordinate.
	sta z80_l
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta z80_h

	lda #<SHRAPN		; shrapnel table.
	sta z80_x
	lda #>SHRAPN
	sta z80_i

	lda #NUMSHR		; number of pieces in table.
	sta explcnt
expld0:
	ldy #0
	lda (z80_ix),y		; get type.
	asl a 			; is this slot in use?
	bcs expld1		; no, we can use it.
expld2:
	clc
	lda z80_x
	adc #SHRSIZ
	sta z80_x
	bcc :+
	inc z80_i
:
	dec explcnt		; repeat for all shrapnel.
	bne expld0
expld3:
	pla			; restore sprite pointer.
	sta z80_x
	pla
	sta z80_i
	rts 			; out of slots, can't generate any more.

expld1:
	lda z80_c		; shrapnel counter.
	and #15			; 0 to 15.
	clc			; add to x.
	adc z80_l
	ldy #3
	sta (z80_ix),y		; y coord.

	lda seed3 		; crap random number.
	and #15			; 0 to 15.
	clc 			; add to y.
	adc z80_h
	ldy #5
	sta (z80_ix),y		; x coord.

	lda #2
	ldy #0
	sta (z80_ix),y		; switch it on.

	jsr chkxy		; plot first position.
	jsr qrand		; quick random angle.
	and #60 		; keep within range.
	ldy #1
	sta (z80_ix),y		; angle.

	dec z80_c		; one less piece of shrapnel to generate.
	bne expld2 		; back to main explosion loop.
	jmp expld3 		; restore sprite pointer and exit.

;explcnt:	.byte 0

;----------------------------------------------------
; Quick random
;----------------------------------------------------

qrand:
	jsr random		; r register.
	eor seed3		; combine with seed.
	sta seed3 		; new seed.
	rts

;seed3:	.byte 0

;----------------------------------------------------
; Display all shrapnel.
;----------------------------------------------------

dshrp:
	lda #<plotde		; display routine.
	sta proshx+1
	lda #>plotde
	sta proshx+2
	jsr proshr		; process shrapnel.

	lda #<prosh1		; processing routine.
	sta proshx+1
	lda #>prosh1
	sta proshx+2
	rts

;------------------------------------------------------
; Particle engine.
;
; Init particle data for 55 particles in SHRAPN table.
; Every particle has 6 bytes.
;
; global:	-
; local:	x,y,hl
; calls:	-
;------------------------------------------------------

inishr:
	lda #<SHRAPN 		; table.
	sta z80_l
	lda #>SHRAPN
	sta z80_h

	ldy #0
	ldx #NUMSHR		; shrapnel pieces to process.
inish0:
	lda #255 		; kill the shrapnel.
	sta (z80_hl),y

	clc 			; point there.
	lda z80_l
	adc #SHRSIZ		; distance to next.
	sta z80_l
	bcc :+
	inc z80_h
:
	dex
	bne inish0 		; round again.
	rts

;------------------------------------------------------
; Check for collision between laser and sprite.
;------------------------------------------------------

lcol:
	lda #<SHRAPN		; shrapnel table.
	sta z80_l
	lda #>SHRAPN
	sta z80_h

	lda #NUMSHR		; number of pieces in table.
	sta z80_b
lcol0:
	ldy #0
	lda (z80_hl),y 		; get type.
	beq lcol1		; yes, check collision.
lcol3:
	clc			; point to more shrapnel.
	lda z80_l
	adc #SHRSIZ
	sta z80_l
	bcc :+
	inc z80_h
:
	dec z80_b		; repeat for all shrapnel.
	bne lcol0
	rts 			; no collision, carry not set.
lcol1:
	ldy #3
	lda (z80_hl),y		; get y.
	sec
	ldy #8
	sbc (z80_ix),y		; subtract sprite y.
lcolh:
	cmp #SPR_HGT 		; within range?
	bcc :+
	jmp lcol2		; no, missed.
:
	ldy #5
	lda (z80_hl),y 		; get x.
	sec
	ldy #9
	sbc (z80_ix),y 		; subtract sprite y.
	cmp #16			; within range?
	bcs :+
	jmp lcol4 		; yes, collision occurred.
:
lcol2:
	jmp lcol3
lcol4:
	sec
	rts 			; return with carry set for collision.
.endif

;------------------------------------------------------
; Main game engine code starts here.
; After initialisation, mloop is the main loop
;------------------------------------------------------

game:

; Set up screen address table.

setsat:
	lda #<ScreenAddr		; start of screen.
	sta scraddr
	lda #>ScreenAddr
	sta scraddr+1

	ldy #0			; vertical lines on screen.
setsa0:
	lda scraddr
	sta SCADTB_lb,y		; write low byte.
	lda scraddr+1
	cpy #192		; vertical lines on screen.
	bcc :+
	ora #$F0		; plot sprites in rom if of screen
:
	sta SCADTB_hb,y		; write high byte.
	jsr nline		; next line down.
	iny			; next position in table.
	bne setsa0
	
; Init graphics mode

	lda #ScrMode		; graphics mode
	sta ScrSelAddr 		; screen selection address
	jsr setpal 		; set up palette.

; Init AtoMMC joystick
	jsr joyinit		; AtoMMC joystick on PORT B

rpblc2:
.if pflag
	jsr inishr 		; initialise particle engine.
.endif
evintr:
	jsr evnt12 		; call intro/menu event.

	lda #WALL 		; write default property.
	ldx #0
clrmap:
	sta MAP,x 		; block properties.
	sta MAP+256,x
	sta MAP+512,x
	inx			; next byte.
	bne clrmap
.if oflag
	jsr iniob 		; initialise objects.
.endif
	lda #0			; put zero in accumulator.
	sta gamwon		; reset game won flag.

	jsr inisc 		; init the score.
mapst:
	lda stmap 		; start position on map.
	sta roomtb		; set up position in table, if there is one.

inipbl:
.if aflag
	lda #<eop		; reset blockpointer
	sta pbptr
	lda #>eop
	sta pbptr+1
.endif
	jsr initsc 		; set up first screen.

	lda #<ssprit 		; default to spare sprite in table.
	sta z80_x
	lda #>ssprit
	sta z80_i
evini:
	jsr evnt13 		; initialisation.

; Two restarts.
; First restart - clear all sprites and initialise everything.

rstrt:
	jsr rsevt 		; restart events.
	jsr xspr 		; clear sprite table.
	jsr sprlst 		; fetch pointer to screen sprites.
	jsr ispr 		; initialise sprite table.

	jmp rstrt0

; Second restart - clear all but player, and don't initialise him.

rstrtn:
	jsr rsevt		; restart events.
	jsr nspr 		; clear all non-player sprites.
	jsr sprlst 		; fetch pointer to screen sprites.
	jsr kspr 		; initialise sprite table, no more players.

; Set up the player and/or enemy sprites.

rstrt0:
	lda #0 			; zero in accumulator.
	sta nexlev 		; reset next level flag.
	sta restfl 		; reset restart flag.
	sta deadf 		; reset dead flag.
	jsr droom 		; show screen layout.
rpblc0:
.if pflag
	jsr inishr 		; initialise particle engine.
.endif
.if aflag
	jsr rbloc		; draw blocks for this screen
.endif
.if oflag
	jsr shwob		; draw objects.
.endif

	lda #<sprtab 		; address of sprite table, even sprites.
	sta z80_x
	lda #>sprtab
	sta z80_i
	jsr dspr 		; display sprites.

	lda #<(sprtab+TABSIZ) 	; address of first odd sprite.
	sta z80_x
	lda #>(sprtab+TABSIZ)
	sta z80_i
	jsr dspr 		; display sprites.
mloop:
	jsr vsync 		; synchronise with display.
	lda #<sprtab 		; address of sprite table, even sprites.
	sta z80_x
	lda #>sprtab
	sta z80_i
	jsr dspr 		; display even sprites.

;	jsr plsnd 		; play sounds.

	jsr proshr
	lda #<(sprtab+TABSIZ) 	; address of first odd sprite.
	sta z80_x
	lda #>(sprtab+TABSIZ)
	sta z80_i
	jsr dspr 		; display odd sprites.

	lda #<(ssprit) 		; point to spare sprite for spawning purposes.
	sta z80_x
	lda #>(ssprit)
	sta z80_i
evlp1:
	jsr evnt10 		; called once per main loop.
	jsr pspr 		; process sprites.

; Main loop events.

	lda #<ssprit 		; point to spare sprite for spawning purposes.
	sta z80_x
	lda #>ssprit
	sta z80_i
evlp2:
	jsr evnt11 		; called once per main loop.
bsortx:
	jsr bsort 		; sort sprites.

	lda nexlev		; finished level flag.
	beq :+
	jmp newlev		; is set, go to next level.
:
	lda gamwon		; finished game flag.
	beq :+
	jmp evwon		; is set, finish the game.
:
	lda restfl 		; finished level flag.
	cmp #1			; has it been set?
	bne :+
	jmp rstrt		; yes, go to next level.
:
	cmp #2			; has it been set?
	bne :+
	jmp rstrtn		; yes, go to next level.
:
	lda deadf 		; dead flag.
	beq :+
	jmp pdead		; yes, player dead.
:
; back to start of main loop.

	inc frmno
	inc clock
	jmp mloop		; switched to a jmp mloop during test mode.

;----------------------------------------------------------
; Read blocks from list and update screen accordingly.
;----------------------------------------------------------

.if aflag
rbloc:
	lda #<eop		; reset temp blockpointer
	sta pbbuf
	lda #>eop
	sta pbbuf+1

rbloc2:
	lda pbbuf			; check for last block
	cmp pbptr
	bne rbloc1
	lda pbbuf+1
	cmp pbptr+1
	bne rbloc1
	rts
rbloc1:
	ldy #0
	lda (pbbuf),y		; check if block for this scno
	cmp scno
	bne rbloc0		; if not, skip
	iny
	lda (pbbuf),y		; get y
	sta dispy
	iny
	lda (pbbuf),y		; get x
	sta dispx
	iny
	lda (pbbuf),y		; get blocknr
	jsr pattr2		; draw block
rbloc0:
	clc			; point to next block
	lda pbbuf
	adc #4
	sta pbbuf
	bcc rbloc2
	inc pbbuf+1
	jmp rbloc2
.endif

;----------------------------------------------------------
; New level
;----------------------------------------------------------

;.repeat 270
;  .byte 0
;.endrep

newlev:
	lda scno 			; current screen.
	clc
	adc #1				; next screen.
	cmp numsc			; total number of screens.
	bcs evwon			; yes, game finished.
	sta scno			; set new level number.
	jmp rstrt			; restart, clearing all aliens.

evwon:
	jsr evnt18		 	; game completed.
	jmp tidyup			; tidy up and return to BASIC/calling routine.

;----------------------------------------------------------
; Player dead.
;----------------------------------------------------------

;.repeat 258
;  .byte 0
;.endrep

pdead:
	lda #0				; zeroise accumulator.
	sta deadf			; reset dead flag.
evdie:
	jsr evnt16 			; death subroutine.
	lda numlif			; number of lives.
	beq :+
	jmp rstrt 			; restart game.
:
evfail:
	jsr evnt17 			; failure event.
	rts			; restart game

;----------------------------------------------------------
; Tidy things up
;----------------------------------------------------------

;.repeat 291
;  .byte 0
;.endrep

tidyup:
	ldy #0				; digits to check.
tidyu2:
	lda score,y 			; get score digit.
	cmp hiscor 			; are we larger than high score digit?
	bcc tidyu0			; high score is bigger.
	bne tidyu1			; score is greater, record new high score.
	iny				; next digit of high score.
	cpy #6
	bne tidyu2			; repeat for all digits
tidyu0:
	lda #<score			; return pointing to score.
	sta z80_c
	lda #>score
	sta z80_b
	rts
tidyu1:
	ldy #5
tidyu3:
	lda score,y			; score.
	sta hiscor,y			; high score.
	dey
	bpl tidyu3 			; copy score to high score.
evnewh:
	jsr evnt19			; new high score event.
	jmp tidyu0			; tidy up.

;--------------------------------------------------
; Restart event.
;--------------------------------------------------

rsevt:
	lda #<ssprit 			; default to spare element in table.
	sta z80_x
	lda #>ssprit
	sta z80_i
evrs:
	jmp evnt14	 		; call restart event.

;------------------------------------------------------------------
; Copy number passed in a to string position bc, right-justified.
;
; Input:
;  A  = number
;  BC = string address
;
; Output:
;  BC = string with number
;-----------------------------------------------------------------

num2ch:
	sta z80_d		; Save number

	lda #0
	sta flag
numdg3:
	ldx #100		; hundreds column.
	stx z80_e
	jsr numdg		; show digit.
numdg2:
	ldx #10			; tens column.
	stx z80_e
	jsr numdg		; show digit.

	inc flag
	ldx #1			; units column.
	stx z80_e
numdg:
	lda #48			; clear digit.
	sta z80_a
numdg1:
	lda z80_d
	cmp z80_e
	bcc numdg0		; nothing to show.
	sec
	lda z80_d
	sbc z80_e		; subtract from column.
	sta z80_d
	inc z80_a		; increment digit.
	inc flag
	jmp numdg1		; repeat until column is zero.
numdg0:
	ldy #0
	lda z80_a
	sta (z80_bc),y		; write digit to buffer.
	lda flag
	beq :+
	inc z80_c		; next buffer position.
	bne :+
	inc z80_b
:
	rts
num2dd:
	sta z80_d		; Save number

	lda #1
	sta flag

	jmp numdg2
num2td:
	sta z80_d		; Save number

	lda #1
	sta flag
	jmp numdg3

;flag:	.byte 0

;---------------------------------------------------------
; Reset score to "000000"
;---------------------------------------------------------

inisc:
	lda #'0'
	ldx #5			; digits to initialise.
inisc0:
	sta score,x 		; write zero digit.
	dex			; next column.
	bpl inisc0		; repeat for all digits.

	rts

;-----------------------------------------------------
; Multiply h by d and return in hl.
;
; Input:
;  H = first number
;  D = second number
;
; Output:
;  HL = result H x D
;-----------------------------------------------------

imul:
	lda z80_d		; HL = H * D
	sta z80_e
	lda z80_h
	sta z80_c		; make c first multiplier.
imul0:
	lda #0			; zeroise total.
	sta z80_l
	sta z80_h

	lda z80_h
	sta z80_d		; zeroise high byte.

	lda #8			; repeat 8 times.
	sta z80_b
imul1:
	lsr z80_c		; rotate rightmost bit into carry.
	bcc imul2		; wasn't set.
	clc			; bit was set, so add de.
	lda z80_l
	adc z80_e
	sta z80_l
	lda z80_h
	adc z80_d
	sta z80_h
	clc 			; reset carry.
imul2:
	asl z80_e 		; shift de 1 bit left.
	rol z80_d
	dec z80_b
	bne imul1		; repeat 8 times.

	rts

;-----------------------------------------------
; Divide d by e and return in d, remainder in a.
;
; Input:
;  D = first number
;  E = second number
;
; Output:
;  D = result D/E
;  A = remainder
;-----------------------------------------------

idiv:
	lda #0
	ldy #8		 	; bits to shift.
	asl z80_d
idiv0:
	rol a 			; multiply d by 2.
	cmp z80_e 		; test if e is smaller.
	bcc idiv1		; e is greater, no division this time.
	sbc z80_e		; subtract it.
idiv1:
	rol z80_d		; rotate into d.
	dey
	bne idiv0		; repeat
	rts

;---------------------------------------------------
; Play AY sound effect
;---------------------------------------------------

plsnd:
	rts

;---------------------------------------------------
; Objects handling.
; 32 bytes for image
; 3 for room, y and x
; 3 for starting room, y and x.
; 254 = disabled.
; 255 = object in player"s pockets.
;---------------------------------------------------

;---------------------------------------------------
; Show items present.
;---------------------------------------------------

.if oflag
shwob:
	lda #<objdta 			; objects table.
	sta z80_l
	lda #>objdta
	sta z80_h

	lda numob 			; number of objects in the game.
	sta sprcnt
shwob0:
	ldy #32 			; distance to room number.
	lda (z80_hl),y 			; same as an item?
	cmp scno 			; current location.
	bne :+
	jsr dobj 			; yes, display object.
:
	clc
	lda z80_l
	adc #38 			; distance to next item.
	sta z80_l
	lda z80_h
	adc #0
	sta z80_h	 		; point to it.
	dec sprcnt
	bne shwob0 			; repeat for others.
	rts

;---------------------------------------------------
; Display object.
; hl must point to object's start address.
;
; Input:
;  HL = object address
;---------------------------------------------------

dobj:
	ldy #33
	lda (z80_hl),y 			; point to y.
	sta dispy
	iny
	lda (z80_hl),y 			; point to x.
	sta dispx
dobj1:
	jmp sprite 			; draw this sprite.

;--------------------------------------
; Remove an object.
;
; Input:
;  A = object number
;--------------------------------------

remob:
	cmp numob			; number of objects in game.
	bcc :+				; are we checking past the end?
	rts				; yes, can't get non-existent item.
:
	pha				; remember object.
	jsr getob			; pick it up if we haven't already got it.
	pla				; retrieve object number.
	jsr gotob			; get its address.
	lda #254
	ldy #32
	sta (z80_hl),y			; remove it.
	rts

;---------------------------------------------------
; Pick up object number held in the accumulator.
;
; Input:
;  A = object number
;---------------------------------------------------

getob:
	cmp numob 		; number of objects in game.
	bcc :+			; are we checking past the end?
	rts			; yes, can't get non-existent item.
:
	jsr gotob 		; check if we already have it.
	cmp #255
	bne :+
	rts			; we already do.
:
	ldy #32
	lda (z80_hl),y		; is it on this screen?
	cmp scno 		; current screen.
	bne getob0		; not on screen, so nothing to delete.

	lda #255
	sta (z80_hl),y		; pick it up.
	iny 			; point to y coord.
getob1:
	ldy #33
	lda (z80_hl),y		; y coord.
	sta dispy
	ldy #34
	lda (z80_hl),y 		; x coord.
	sta dispx
	jmp dobj1 		; delete object sprite.
getob0:
	lda #255
	sta (z80_hl),y 		; pick it up.
	rts
.endif

;-----------------------------------------------------------------
; Got object check.
; Call with object in accumulator, returns zero set if in pockets.
;
; Input:
;  A = object number
;-----------------------------------------------------------------
.if oflag .or mflag
gotob:
	cmp numob 		; number of objects in game.
	bcc :+ 			; are we checking past the end?
	jmp gotob0 		; yes, we can't have a non-existent object.
:
	jsr findob		; find the object.
gotob1:
	rts

gotob0:
	lda #254 		; missing.
	jmp gotob1

findob:
	pha			; save object number
	lda #<objdta 		; objects.
	sta z80_l
	lda #>objdta
	sta z80_h
	pla			; retreive object number
	beq fndob1 		; is it zero? yes, skip loop.
	tax 			; loop counter
fndob2:
	clc
	lda z80_l
	adc #38 		; size of each object.
	sta z80_l
	bcc :+
	inc z80_h
:
	dex 			; repeat until we find address.
	bne fndob2
fndob1:
	ldy #32			; distance to room it's in.
	lda (z80_hl),y		; fetch status.
	rts
.endif

;---------------------------------------------
; Drop object number at (dispx, dispy).
;
; Input:
;  A = object number
;---------------------------------------------

.if oflag
drpob:
	cmp numob 		; are we checking past the end?
	bcc :+
	rts			; yes, can't drop non-existent item.
:
	jsr gotob		; make sure object is in inventory.
	cmp scno		; already on this screen?
	bne :+
	rts			; yes, nothing to do.
:
	ldy #32
	lda scno
	sta (z80_hl),y		; bring onto screen.
	lda dispy		; sprite y coordinate.
	iny 
	sta (z80_hl),y		; point to object y.
	lda dispx 		; sprite x coordinate.
	iny
	sta (z80_hl),y 		; point to object x
	jmp dobj		; draw the object sprite.

;-----------------------------------------------
; Seek objects at sprite position.
;
; Output:
;  A = object number, if not found A=255
;-----------------------------------------------

skobj:
	lda #<objdta 		; pointer to objects.
	sta z80_l
	lda #>objdta
	sta z80_h

	lda numob 		; number of objects in game.
	sta z80_b 		; set up the loop counter.
skobj0:
	lda scno		; current room number.
	ldy #32
	cmp (z80_hl),y		; is object in here?
	bne :+
	jsr skobj1		; yes, check coordinates.
:
	clc			; point to next object in table.
	lda z80_l
	adc #38			; size of each object.
	sta z80_l
	bcc :+
	inc z80_h
:
	dec z80_b
	bne skobj0		; repeat for all objects.

	lda #255		; end of list and nothing found, return 255.
	rts

skobj1:
	ldy #33			; point to y coordinate.
	lda (z80_hl),y		; point to y coordinate.
	sec
	ldy #8
	sbc (z80_ix),y 		; subtract sprite y.
	clc
	adc #15			; add sprite height minus one.
	cmp #31			; within range?
	bcs skobj2		; no, ignore object.

	ldy #34			; point to x coordinate now.
	lda (z80_hl),y 		; get coordinate.
	sec
	ldy #9
	sbc (z80_ix),y 		; subtract the sprite x.
	clc			; add sprite width minus one.
	adc #15
	cmp #31			; within range?
	bcs skobj2		; no, ignore object.

	pla			; remove return address from stack.
	pla

	lda numob 		; objects in game.
	sec
	sbc z80_b		; subtract loop counter.
skobj2:
	rts			; accumulator now points to object.
.endif

;---------------------------------------------------------------------
; Spawn a new sprite.
;---------------------------------------------------------------------

  offset = 20

;.repeat 343
;  .byte 0
;.endrep

spawn:
	lda #<sprtab		; sprite table.
	sta z80_l
	lda #>sprtab
	sta z80_h
numsp1:
	lda #NUMSPR		; number of sprites.
	sta spcnt
spaw0:
	ldy #0
	lda (z80_hl),y		; get sprite type.
	cmp #255		; is it an unused slot?
	beq spaw1 		; yes, we can use this one.

	clc 			; point to next sprite in table.
	lda z80_l
	adc #TABSIZ		; size of each entry.
	sta z80_l
	bcc :+
	inc z80_h
:
	dec spcnt		; one less iteration.
	bne spaw0		; keep going until we find a slot.

; Didn't find one but drop through and set up a dummy sprite instead.

spaw1:
	lda z80_i		; address of original sprite.
	pha
	lda z80_x
	pha

	lda z80_l		; store spawned sprite address.
	sta spptr
	lda z80_h
	sta spptr+1

	lda z80_c
	ldy #0
	sta (z80_hl),y 		; set the type.
	ldy #5
	sta (z80_hl),y		; copy

	lda z80_b
	ldy #1
	sta (z80_hl),y		; set the image.
	ldy #6
	sta (z80_hl),y		; copy

	lda #0 			; frame zero.
	ldy #2
	sta (z80_hl),y		; set frame.
	ldy #7
	sta (z80_hl),y		; copy

	ldy #8
	lda (z80_ix),y 		; x coordinate.
	ldy #3
	sta (z80_hl),y		; set sprite coordinate.
	ldy #8
	sta (z80_hl),y		; copy

	ldy #9
	lda (z80_ix),y 		; y coordinate.
	ldy #4
	sta (z80_hl),y		; set sprite coordinate.
	ldy #9
	sta (z80_hl),y		; copy

	ldy #10			; direction of original.
	lda (z80_ix),y
	sta (z80_hl),y		; direction

	lda #0
	ldy #11
	sta (z80_hl),y		; reset parameter.
	iny
	sta (z80_hl),y		; reset parameter.
	iny
	sta (z80_hl),y		; reset parameter.
	iny
	sta (z80_hl),y		; reset parameter.
rtssp:
	lda spptr		; address of new sprite.
	sta z80_x
	lda spptr+1
	sta z80_i
evis1:
	jsr evnt09 		; call sprite initialisation event.

	lda spptr 		; address of new sprite.
	sta z80_x
	lda spptr+1
	sta z80_i
	jsr sspria 		; display the new sprite.

	pla			; address of original sprite.
	sta z80_x
	pla
	sta z80_i

	rts

;spcnt:	.byte 0
;spptr:	.word 0			; spawned sprite pointer.
;seed:	.byte 0			; seed for random numbers.
score:	.byte "000000"		; player"s score.
hiscor:	.byte "000000"		; high score.
bonus:	.byte "000000"		; bonus.
grbase:	.word ScreenAddr	; graphics base address.

;----------------------------------------------------
; Check y-pos
;----------------------------------------------------

checkx:
	lda dispy		; y position.
	cmp #24			; off screen?
	bcs :+
	rts			; no, it's okay.
:
	pla			; remove return address from stack.
	sta z80_l
	pla
	sta z80_h
	rts

;-----------------------------------------------
; Displays the current high score.
;-----------------------------------------------

dhisc:
	lda #<hiscor 		; high score text.
	sta dscor3+1
	sta bscor3+1
	lda #>hiscor
	sta dscor3+2
	sta bscor3+2
	jmp dscor1		; check in printable range then show 6 digits.

;------------------------------------------------------
; Displays the current score.
;------------------------------------------------------

;.repeat 322
;  .byte 0
;.endrep

dscor:
	lda #<score		; score text.
	sta dscor3+1
	sta bscor3+1
	lda #>score
	sta dscor3+2
	sta bscor3+2
dscor1:
	sty tmpscr
	jsr preprt		; set up font and print position.
	jsr checkx		; make sure we're in a printable range.

;	lda #6			; digits to display.
;	sta z80_b
	lda prtmod		; get print mode.
	beq :+			; standard size text?
	jmp bscor0		; no, show double-height.
:
dscor0:
;	ldy #0
;	lda (z80_hl),y 		; fetch character.
	ldy tmpscr
dscor3:
	lda score,y
	jsr pchar 		; display character.
	inc dispx		; move along x coordinate

;	inc z80_l		; next score column.
;	bne :+
;	inc z80_h
;:
	inc tmpscr
	dec z80_b
	bne dscor0 		; repeat for all digits.
dscor2:
	lda dispx 		; set up display coordinates.
	sta charx
	lda dispy
	sta chary
	rts

tmpscr:	.byte 0
;------------------------------------------------------
; Displays the current score in double-height characters.
;
; Input:
;  B  = digit number
;  HL = score string
;------------------------------------------------------

bscor0:
;	ldy #0

;	lda (z80_hl),y 		; fetch character.
	ldy tmp
bscor3:
	lda score,y
	jsr bchar 		; display big char.

;	inc z80_l 		; next score column.
;	bne :+
;	inc z80_h
;:
	inc tmp
	dec z80_b
	beq :+
	jmp bscor0 		; repeat for all digits.
:
	jmp dscor2 		; tidy up line and column variables.

;-----------------------------------------------------
; Adds number in the hl pair to the score.
;-----------------------------------------------------

addsc:
	lda #<(score+1) 	; ten thousands column.
	sta z80_e
	lda #>(score+1)
	sta z80_d
	lda #<10000		; amount to add each time.
	sta z80_c
	lda #>10000
	sta z80_b
	jsr incsc		; add to score.

	inc z80_e		; thousands column.
	bne :+
	inc z80_d
:
	lda #<1000		; amount to add each time.
	sta z80_c
	lda #>1000
	sta z80_b
	jsr incsc 		; add to score.

	inc z80_e		; hundreds column.
	bne :+
	inc z80_d
:
	lda #<100		; amount to add each time.
	sta z80_c
	lda #>100
	sta z80_b
	jsr incsc		; add to score.

	inc z80_e 		; tens column.
	bne :+
	inc z80_d
:
	lda #<10		; amount to add each time.
	sta z80_c
	lda #>10
	sta z80_b
	jsr incsc 		; add to score.

	inc z80_e		; units column.
	bne :+
	inc z80_d
:
	lda #<1			; units.
	sta z80_c
	lda #>1
	sta z80_b
incsc:
	lda z80_h		; store amount to add.
	pha
	lda z80_l
	pha

	sec			; subtract from amount to add.
	lda z80_l
	sbc z80_c
	sta z80_l
	lda z80_h
	sbc z80_b
	sta z80_h
	bcc incsc0		; too much, restore value.

	pla			; delete the previous amount from the stack.
	pla

	lda z80_d 		; store column position.
	pha
	lda z80_e
	pha
	jsr incsc2		; do the increment.

	pla			; restore column.
	sta z80_e
	pla
	sta z80_d
	jmp incsc		; repeat until all added.

incsc0:
	pla			; restore previous value.
	sta z80_l
	pla
	sta z80_h
	rts
incsc2:
	ldy #0
	lda (z80_de),y 		; get amount.
	clc
	adc #1			; add one to column.
	sta (z80_de),y		; write new column total.
	cmp #'9'+1		; gone beyond range of digits?
	bcs :+
	rts			; no, carry on.
:
	lda #'0'		; make it zero.
	sta (z80_de),y		; write new column total.
	dec z80_e		; back one column.
	bne :+
	dec z80_d
:
	jmp incsc2

;------------------------------------
; Add bonus to score and reset bonus
;------------------------------------

addbo:
	ldx #5			; last digit.
	clc
addbo0:
	lda score,x		; get score.
	adc bonus,x		; add bonus.
	sec			; 0 to 18.
	sbc #48
	pha
	lda #'0'
	sta bonus,x
	pla
	cmp #58
	bcc addbo1
	sec
	sbc #10
addbo1:
	sta score,x		; zeroise bonus.
	dex			; next digit.
	bpl addbo0		; repeat for all 6 digits.
	rts

;------------------------------------
; Swap score and bonus.
;------------------------------------

swpsb:
	ldx #5			; digits to add.
swpsb0:
	lda score,x 		; get score digits.
	pha			; save digit
	lda bonus,x 		; get bonus digits.
	sta score,x		; switch score-bonus
	pla
	sta bonus,x
	dex 			; repeat for all 6 digits.
	bpl swpsb0
	rts

;----------------------------------------------------
; Get print address.
;----------------------------------------------------

gprad:
	lda dispx		; get x
	sta scraddr		; store lb address

	rol a
	rol tmp
	rol a
	rol tmp
	rol a
	rol tmp
	lda tmp
	and #7
	sta tmp
	lda prtmod
	beq :+
	asl tmp
:
	lda tmp
	clc
	adc dispy		; get y
	adc #>ScreenAddr		; add $80
	sta scraddr+1		; store hb address
	rts

;--------------------------------------------------------------
; Get property buffer address of char at (dispx, dispy) in hl.
; 
; Output:
;  bufaddr = MAP + dispy*32 + dispx
;--------------------------------------------------------------

pradd:
	lda dispy 		; y coordinate.
	sta bufaddr
	lda #0
	sta bufaddr+1
	asl bufaddr  		; multiply char by 32
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	clc			; add address of MAP graphics.
	lda bufaddr
	adc dispx
	adc #<MAP
	sta bufaddr
	lda bufaddr+1
	adc #>MAP
	sta bufaddr+1
	rts

;----------------------------------------------------
; Display character in A at dispx,dispy.
;
; Input:
;  A 	   = character to print
;----------------------------------------------------

pchar:
	sta fntaddr
	lda #0
	sta fntaddr+1
	asl fntaddr  		; multiply char by 8.
	rol fntaddr+1
	asl fntaddr
	rol fntaddr+1
	asl fntaddr
	rol fntaddr+1

	lda fntaddr
	clc
	adc #<(FONT-256)
	sta fntaddr		; that's the low byte.
	lda fntaddr+1
	adc #>(FONT-256)
	sta fntaddr+1		; add displacement.
pchark:
	jsr gprad		; get screen address.
	ldx #7			; lines to write.
pchar0:
	ldy #0
	lda (fntaddr),y 	; get image byte.
	ldy scrtab,x		; Get rowoffset
.if iflag
	eor #$ff
.endif
	sta (scraddr),y 	; copy to screen.
	inc fntaddr		; next image byte.
	bne :+
	inc fntaddr+1
:
	dex			; next screen row down.
	bpl pchar0		; repeat.
	rts

scrtab:		.byte $e0,$c0,$a0,$80,$60,$40,$20,$00

;----------------------------------------------

; Print attributes, properties and pixels.
;
; Input:
;  A	= tile number
;----------------------------------------------

;.repeat 294
;  .byte 0
;.endrep

pbpattr:			; entry PUTBLOCK command
	ldy charx
	sty dispx
	ldy chary
	sty dispy
pattr:
.if aflag
	pha
	jsr wbloc		; save blockinfo
	pla
.endif

pattr2:
	sta z80_b		; store cell in b register for now.
	tax
	lda bprop,x 		; block properties.
	sta z80_c
	cmp #COLECT
	bne :+
	lda z80_b
	sta colpatt
:
	jsr pradd 		; get property buffer address.
	lda z80_c
	ldy #0
	sta (bufaddr),y 	; write property.
	lda z80_b 		; restore cell.
	jmp panp
; Print attributes, no properties.

;.repeat 263
; .byte 0
;.endrep

colpatt:	.byte 0

panp:
	sta z80_e		; displacement in e.
	lda #0
	sta z80_d		; no high byte.
	asl z80_e  		; multiply char by 8.
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	clc
	lda z80_e
	adc #<chgfx 		; address of graphics.
	sta tileaddr
	lda z80_d
	adc #>chgfx
	sta tileaddr+1
	jsr gprad 		; get screen address.
	ldx #7			; number of pixel rows to write.
panp0:
	ldy #0
	lda (tileaddr),y 	; get image byte.
.if iflag
	eor #$ff		; Invert
.endif
	ldy scrtab,x
	sta (scraddr),y 	; copy to screen.
	inc tileaddr 		; next image byte.
	bne :+
	inc tileaddr+1
:
	dex	 		; repeat for 8 pixel rows.
	bpl panp0
	inc dispx 		; move along one.
	inc charx
	rts

;----------------------------------------------
; Write block
;----------------------------------------------

.if aflag
wbloc:
	ldy #3
	sta (pbptr),y		; store block number
	dey
	lda dispx
	sta (pbptr),y		; write x position of block.
	dey
	lda dispy
	sta (pbptr),y		; write y position of block.
	dey
	lda scno
	sta (pbptr),y		; write screen.
	clc			; point to next free location
	lda pbptr
	adc #4
	sta pbptr
	bcc :+
	inc pbptr+1
:
	rts
.endif

;----------------------------------------------
; Print character pixels, no more.
;
; Input:
;  A	= character to print
;----------------------------------------------

pchr:
	jsr pchar 		; show character in accumulator.
	inc dispx		; move along one.
	rts

;----------------------------------------------------
; Shifter sprite routine for objects.
;----------------------------------------------------

.if oflag
sprit7:
	lda z80_b
	beq sprit0
	sta z80_a
sprit3:
	lsr spr			; shift into position.
	ror spr+1
	ror spr+2
	dec z80_a		; one less iteration.
	bne sprit3
sprit0:
	rts 			; now apply to screen.

;----------------------------------------------------
; Draw sprite
;----------------------------------------------------

sprite:
	stx xtmp		; Save X-reg
	jsr scadd 		; get screen address in scraddr.

	lda dispx 		; x position.
	and #7 			; position straddling cells.
	sta z80_b		; store in b register.

	lda z80_l		; store sprite graphic address.
	sta sprit1+1
	sta sprit2+1
	lda z80_h
	sta sprit1+2
	sta sprit2+2

	ldx #0			; pixel height.
	ldy #0
sprit1:
	lda objdta,x		; fetch first byte.
	sta spr
	inx
sprit2:
	lda objdta,x
	sta spr+1

	lda #0
	sta spr+2
	jsr sprit7		; shift sprite

	dex
	lda spr			; fetch graphic.
	ldy spritetab,x
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.
	inx			; next screen byte.

	lda spr+1		; fetch graphic.
	ldy spritetab,x
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.
	iny

	lda spr+2		; fetch graphic.
	eor (scraddr),y		; merge with screen image.
	sta (scraddr),y		; write to screen.

	inx			; next source byte.
	cpx #16
	bne :+
	inc scraddr+1
:
	cpx #32
	bne sprit1		; repeat

	ldx xtmp		; retreive X-reg
	rts

spritetab:
	.byte $00,$01,$20,$21,$40,$41,$60,$61
	.byte $80,$81,$a0,$a1,$c0,$c1,$e0,$e1
	.byte $00,$01,$20,$21,$40,$41,$60,$61
	.byte $80,$81,$a0,$a1,$c0,$c1,$e0,$e1

spr:	.byte 0,0,0

;xtmp:	.byte 0
.endif

;-----------------------------------------------------------
; Get room address.
;-----------------------------------------------------------

;.repeat 273
;  .byte 0
;.endrep

groom:
	ldx scno 		; screen number.
	ldy #0
groomx:
	lda #<scdat 		; pointer to screens.
	sta z80_l
	lda #>scdat
	sta z80_h
groom1:
	cpx #0			; is it the first one?
	beq groom0 		; no more screens to skip.

	clc
	lda z80_l
	adc scdat,y 		; low byte of screen size.
	sta z80_l
	iny			; point to high byte.
	lda z80_h
	adc scdat,y 		; high byte of screen size.
	sta z80_h
	iny			; next address.

	dex 			; one less iteration.
	jmp groom1 		; loop until we reach the end.
groom0:
	lda numsc 		; add displacement.
	asl a
	clc			; add double displacement to address.
	adc z80_l
	sta z80_l
	lda z80_h
	adc #0
	sta z80_h
	rts

;-----------------------------------------------------------
; Draw present room.
;-----------------------------------------------------------

droom:
	lda wintop 		; window top.
	sta dispy		; set cursor y position.
droom2:
	jsr groom 		; get address of current room.
	lda #0	 		; zero in accumulator.
	sta comcnt 		; reset compression counter.
	lda winhgt 		; height of window.
	sta rrow		; set row counter
droom0:
	lda winlft 		; window left edge.
	sta dispx 		; set cursor x position.
	lda winwid 		; width of window.
	sta rcol		; set column counter
droom1:
	jsr flbyt 		; decompress next byte on the fly.
	jsr pattr2 		; show attributes and block.
	dec rcol		; one less column.
	bne droom1 		; repeat for entire line.
	inc dispy		; move down one line.
	dec rrow 		; one less row.
	bne droom0 		; repeat for all rows.
	rts

;rcol:	.byte 0
;rrow:	.byte 0

;----------------------------------------------
; Decompress bytes on-the-fly.
;----------------------------------------------

flbyt:
	lda comcnt 		; compression counter.
	bne flbyt1		; any more to decompress?  yes.

	ldy #0
	lda (z80_hl),y 		; fetch next byte.
	inc z80_l 		; point to next cell.
	bne :+
	inc z80_h
:
	cmp #255 		; is this byte a control code?
	beq :+
	rts 			; no, this byte is uncompressed.
:
	lda (z80_hl),y 		; fetch byte type.
	sta combyt 		; set up the type.
	inc z80_l 		; point to quantity.
	bne :+
	inc z80_h
:
	lda (z80_hl),y 		; get quantity.
	inc z80_l 		; point to next byte.
	bne :+
	inc z80_h
:
flbyt1:
	sta comcnt 		; store new quantity.
	dec comcnt		; one less.
	lda combyt 		; byte to expand.
	rts

;combyt:	.byte 0			; byte type compressed.
;comcnt:	.byte 0			; compression counter.

;------------------------------------------
; Ladder down check.
;
; Input:
;  IX = sprite pointer
;------------------------------------------

.if lflag
laddd:
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta dispx

	ldy #8
	lda (z80_ix),y		; y coordinate.
	and #254		; make it even.
	sta (z80_ix),y 		; reset it.
	clc 			; look down 16 pixels.
numsp5:
	adc #SPR_HGT
	sta dispy		; coords in dispx,dispy.
	jmp laddv

;------------------------------------------
; Ladder up check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;------------------------------------------

laddu:
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta dispx

	ldy #8
	lda (z80_ix),y		; y coordinate.
	and #254 		; make it even.
	sta (z80_ix),y		; reset it.
	clc 			; look 2 pixels above feet.
numsp6:
	adc #SPR_HGT-2
	sta dispy		; coords in dispx,dispy.
laddv:
	jsr tstbl 		; get map address.
	jsr ldchk 		; standard ladder check.
	beq :+
	rts 			; no way through.
:
	inc bufaddr 		; look right one cell.
	bne :+
	inc bufaddr+1
:
	jsr ldchk 		; do the check.
	beq :+
	rts 			; impassable.
:
	lda dispx 		; y coordinate.
	and #7 			; position straddling block cells.
	bne :+
	rts 			; no more checks needed.
:
	inc bufaddr 		; look to third cell.
	bne :+
	inc bufaddr+1
:
	jsr ldchk 		; do the check.
	rts  			; return with zero flag set accordingly.
.endif

;---------------------------------------------------------
; Can go up check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

cangu:
	ldy #9
	lda (z80_ix),y		; x coordinate.
	sta dispx
	ldy #8
	lda (z80_ix),y 		; y coordinate.
	sec
	sbc #2
	sta dispy		; coords in dispx,dispy.
	jsr tstbl 		; get map address.
	jsr lrchk 		; standard left/right check.
	beq :+
	rts			; no way through.
:
	inc bufaddr		; look right one cell.
	bne :+
	inc bufaddr+1
:
	jsr lrchk 		; do the check.
	beq :+
	rts			; impassable.
:
	lda dispx		; x coordinate.
	and #7			; position straddling block cells.
	bne :+
	rts			; no more checks needed.
:
	inc bufaddr		; look to third cell.
	bne :+
	inc bufaddr+1
:
	jsr lrchk		; do the check.
	rts 			; return with zero flag set accordingly.

;---------------------------------------------------------
; Can go down check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

;.repeat 264
;  .byte 0
;.endrep

cangd:
	ldy #9
	lda (z80_ix),y 		; x coordinate.
	sta dispx
	ldy #8
	lda (z80_ix),y		; y coordinate.
	clc
numsp3:
	adc #SPR_HGT 		; look down 16 pixels.
	sta dispy		; coords in dispx,dispy.
	jsr tstbl 		; get map address.
	jsr plchk 		; block, platform check.
	beq :+
	rts			; no way through.
:
	inc bufaddr		; look right one cell.
	bne :+
	inc bufaddr+1
:
	jsr plchk		; block, platform check.
	beq :+
	rts			; impassable.
:
	lda dispx		; x coordinate.
	and #7			; position straddling block cells.
	bne :+
	rts			; no more checks needed.
:
	inc bufaddr		; look to third cell.
	bne :+
	inc bufaddr+1
:
	jsr plchk		; block, platform check.
	rts			; return with zero flag set accordingly.

;---------------------------------------------------------
; Can go left check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

cangl:
	ldy #8
	lda (z80_ix),y 		; y coordinate.
	sta dispy
	ldy #9
	lda (z80_ix),y 		; x coordinate.
	sec
	sbc #2			; look left 2 pixels.
	sta dispx		; coords in dispx,dispy.
	jmp cangh		; test if we can go there.

;---------------------------------------------------------
; Can go right check.
;
; Input:
;  IX = sprite pointer
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;---------------------------------------------------------

.repeat 272
  .byte 0
.endrep

cangr:
	ldy #8
	lda (z80_ix),y		; y coordinate.
	sta dispy
	ldy #9
	lda (z80_ix),y		; x coordinate.
	clc
	adc #16			; look right 16 pixels.
	sta dispx		; coords in dispx,dispy.
cangh:
cangh2:
	lda #(SPR_HGT/8+1)	; default rows to write.
	sta z80_b
	lda dispy		; y position.
	and #7			; does x straddle cells?
	bne cangh0		; yes, loop counter is good.
	dec z80_b		; one less row to write.
cangh0:
	jsr tstbl		; get map address.
cangh1:
	jsr lrchk		; standard left/right check.
	beq :+
	rts			; no way through.
:
	pha
	clc
	lda bufaddr
	adc #32			; look down.
	sta bufaddr
	bcc :+
	inc bufaddr+1
:
	pla

	dec z80_b
	bne cangh1
	rts

;-------------------------------------
; Check left/right movement is okay.
;
; Input:
;  bufaddr = MAP + x/8 + y/8*32
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;-------------------------------------

lrchk:
	ldy #0
	lda (bufaddr),y		; fetch map cell.
	cmp #WALL 		; is it passable?
	beq lrchkx		; no.

	cmp #FODDER		; fodder has to be dug.
	beq lrchkx		; not passable.
	lda #0
	rts

;--------------------------------------------------------------
; Called by mmenu
;--------------------------------------------------------------

always:
	lda #255		; report it as okay.
	rts

lrchkx:
	lda #1 			; reset all bits.
	rts


;--------------------------------------------------------------
; Check platform or solid item is not in way.
;
; Input:
;  bufaddr = MAP + x/8 + y/8*32
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;--------------------------------------------------------------

plchk:
	ldy #0
	lda (bufaddr),y 	; fetch map cell.
	cmp #WALL 		; is it passable?
	beq lrchkx		; no.
.if pflag .or dflag
	cmp #FODDER		; fodder has to be dug.
	beq lrchkx		; not passable.
.endif
	cmp #PLATFM		; platform is solid.
	beq plchkx		; not passable.
.if lflag
	cmp #LADDER		; is it a ladder?
	beq lrchkx		; on ladder, deny movement.
.endif
plchk0:
	lda #0			; report as ok
	rts
plchkx:
	lda dispy		; x coordinate.
	and #7			; position straddling blocks.
	beq lrchkx		; on platform, deny movement.
	jmp plchk0

;--------------------------------------------------------------
; Check ladder is available.
;
; Input:
;  bufaddr = MAP + x/8 + y/8*32
;
; Output:
;  A  = 0 is ok, A <>0 is not ok
;--------------------------------------------------------------

.if lflag
ldchk:
	ldy #0
	lda (bufaddr),y 	; fetch cell.
	cmp #LADDER 		; is it a ladder?
	beq :+
	lda #1
	rts  			; return with zero flag set accordingly.
:
	lda #0
	rts
.endif

;--------------------------------------------------------------
; Get collectables.
;--------------------------------------------------------------

.if cflag
getcol:
        lda #COLECT             ; collectable blocks.
        sta z80_b
        jsr tded                ; test for collectable blocks.
        cmp z80_b               ; did we find one?
        beq :+
        rts                     ; none were found, job done.
:
        jsr gtblk               ; get block.
        jsr evnt20              ; collected block event.
        jmp getcol              ; repeat until none left.

; Get collectable block.

gtblk:
	ldy #0
	lda (bufaddr),y
	sta z80_a
        lda #0
        sta (bufaddr),y		; make it empty now.
       
	lda bufaddr		; set dispx
	and #31
	sta dispx

	lda bufaddr+1		; Set dispy
	sec
	sbc #>MAP
	sta bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	lda bufaddr+1
	sta dispy
 
	lda colpatt		; get blocknr
	sta z80_e		; displacement in e.
	lda #0
	sta z80_d		; no high byte.
	asl z80_e  		; multiply char by 8.
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d
	clc
	lda z80_e
	adc #<chgfx 		; address of graphics.
	sta tileaddr
	lda z80_d
	adc #>chgfx
	sta tileaddr+1
	jsr gprad 		; get screen address.
	ldx #7			; number of pixel rows to write.
gtblk0:
	ldy #0
	lda (tileaddr),y 	; get image byte.
	ldy scrtab,x
	eor (scraddr),y 	; XOR tile on screen
	sta (scraddr),y 	; copy to screen.
	inc tileaddr 		; next image byte.
	bne :+
	inc tileaddr+1
:
	dex	 		; repeat for 8 pixel rows.
	bpl gtblk0
	rts
.endif

;--------------------------------------------------------------
; Touched deadly block check.
; returns with DEADLY (must be non-zero) in accumulator if true.
;
; Input:
;  IX = sprite address
;
; Output:
;  A  = 0 is ok, A=5 is not ok
;--------------------------------------------------------------

tded:
	ldy #8
	lda (z80_ix),y 		; y coordinate.
	sta dispy
	iny
	lda (z80_ix),y 		; x coordinate.
	sta dispx		; coords in dispx,dispy.
	jsr tstbl		; get map address.
	pha
	lda #31			; default distance to next line down.
	sta z80_e
	pla
	cmp z80_b		; is this the required block?
	bne :+
	rts			; yes.
:
	inc bufaddr 		; next cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch type.
	cmp z80_b 		; is this deadly/custom?
	bne :+
	rts			; yes.
:
	lda dispx		; horizontal position.
	sta z80_c 		; store column in c register.
	and #7			; is it straddling cells?
	bne :+
	jmp tded0		; no.
:
	inc bufaddr 		; last cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch type.
	cmp z80_b		; is this the block?
	bne :+
	rts			; yes.
:
	dec z80_e		; one less cell to next row down.
tded0:
	clc 			; point to next row.
	lda bufaddr
	adc z80_e
	sta bufaddr
	bcc :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch left cell block.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	inc bufaddr 		; next cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch type.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	lda z80_c		; horizontal position.
	and #7			; is it straddling cells?
	bne :+
	jmp tded1 		; no.
:
	inc bufaddr		; last cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch type.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
tded1:
	lda dispy		; vertical position.
	and #7 			; is it straddling cells?
	bne :+
	rts			; no, job done.
:
	clc			; point to next row.
	lda bufaddr
	adc z80_e
	sta bufaddr
	bcc :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch left cell block.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	inc bufaddr		; next cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y 	; fetch type.
	cmp z80_b		; is this fatal?
	bne :+
	rts			; yes.
:
	lda z80_c		; horizontal position.
	and #7			; is it straddling cells?
	bne :+
	rts			; no.
:
	inc bufaddr		; last cell.
	bne :+
	inc bufaddr+1
:
	ldy #0
	lda (bufaddr),y		; fetch final type.
	rts 			; return with final type in accumulator.

;---------------------------------------------------
; Fetch block type at (dispx, dispy).
;
; Output:
;  A = block type
;---------------------------------------------------

tstbl:
	lda dispy 		; fetch y coord.
	lsr a			; bufaddr = y/8
	lsr a
	lsr a
	sta chary

	sta bufaddr
	lda #0
	sta bufaddr+1

	asl bufaddr  		; bufaddr = y/8 * 32
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1
	asl bufaddr
	rol bufaddr+1

	lda dispx		; x/8
	lsr a
	lsr a
	lsr a
	sta charx

	clc			; bufaddr = MAP + x/8 + y/8*32
	adc bufaddr
	adc #<MAP
	sta bufaddr
	lda bufaddr+1
	adc #>MAP
	sta bufaddr+1

	ldy #0
	lda (bufaddr),y 	; fetch byte there.
	rts


;-------------------------------------------------------------------
; Jump - if we can.
; Requires initial speed to be set up in accumulator prior to call.
;
; Input:
;  IX = sprite address
;-------------------------------------------------------------------

jump:
	ldy #13
	lda (z80_ix),y		; jump table low.
	ldy #14
	ora (z80_ix),y		; jump table high.
	beq :+
	rts			; already in the air.
:
	lda #>jtab
	ldy #14
	sta (z80_ix),y		; set jump high.
	lda #<jtab		; jump table start.
	ldy #13
	sta (z80_ix),y		; set jump low.
	rts

; Jump table.

;jtab:
;	.byte 248,250,252
;	.byte 254,254,255
;	.byte 255,255,0,0
;	.byte 0,1,1,1,2,2
;	.byte 4,6,8,8,8,99

;------------------------------------------------
; Random numbers code.
; Pseudo-random number generator, 8-bit.
;
; Output:
;  RND = random number
;------------------------------------------------

random:
	lda seed
	beq doEor
	asl a
	beq noEor
	bcc noEor
doEor:	eor #$1d
noEor:	sta seed
	rts

	lda seed		; get last random number.
	asl a
	asl a
	clc
	adc seed
	clc
	adc #$45
	sta seed		; store new seed.
	sta varrnd		; return number in variable.
	rts

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

;              X   Z   .   ;  SPC  Q   P
;keys:   .byte $35,$15,$93,$22,$90,$04,$14       ; Keys defined by game designer.
;        .byte $21,$11,$01,$92                   ; menu options.
jkeys:  .byte $03,$01,$02,$04,$00,$04,$14       ; Joykey keys
        .byte $21,$11,$01,$92                   ; menu options.

;-------------------------------------------------------
; Joystick and keyboard reading routines.
;
; contrl = 0, Keyboard
;          1, JoyKeyb
;          2, JoyMMC
;-------------------------------------------------------

joykey:
	lda contrl 		; control flag.
	cmp #1
	bne :+
	jmp joyjoy 		; read keyboard joystick
:
	cmp #2
	bne :+
	jmp joysin 		; read MMC joystick.
:
; Keyboard controls.

	lda #0		 	; zero reading.
	sta z80_e

	ldy #6	 		; address of last key.
joyke0:
	lda keys,y 		; get key from table.
	jsr ktest		; being pressed?
	rol z80_e 		; rotate into reading.

	dey		 	; next key.
	bpl joyke0 		; repeat for all keys.
	jmp joyjo1 		; store the value.

; Keyboard joystick controls.

joyjoy:
	lda #0		 	; zero reading.
	sta z80_e

	ldy #6	 		; address of last key.
joyjo3:
	lda jkeys,y 		; get key from table.
	jsr ktest		; being pressed?
	rol z80_e 		; rotate into reading.

	dey		 	; next key.
	bpl joyjo3 		; repeat for all keys.
joyjo1:
	lda z80_e 		; copy e register to accumulator.
joyjo2:
	sta joyval		; remember value.
	rts


; AtoMMC joystick controls.

; Set PORTB direction

joyinit:
	rts

	lda #$ff   		; Write value to latch, bits 0-7 input
	sta DatReg
	jsr interwritedelay
	lda #$a1   		; Write direction in latch to PORTB
	sta CmdReg

wait_until_not_busy:
	lda CmdReg
	bmi wait_until_not_busy
	rts

; Get PORTB value

joysin:
	lda #$a2   		; Read value PORTB in latch
	sta CmdReg
	jsr interwritedelay

	lda DatReg		; Read joystick status
	and #$7f
	sta joyval
	sta $f0

	lda keys+5		; check fire2
	jsr ktest
	bcs :+
	lda joyval
	and #$df
	sta joyval
:
	lda keys+6		; check fire3
	jsr ktest
	bcs :+
	lda joyval
	and #$bf
	sta joyval
:
	rts

; Short delay
; Enough to intersperse 2 writes to the FATPIC.

interwritedelay:
	lda  #8
	sec
loop:
	sbc  #1
	bne  loop
	rts

;tmp_byte:   .byte 0

;---------------------------------------------------------------
; Display message.
;
; Input:
;  A = message number
;---------------------------------------------------------------

dmsg:
	tax
	lda #<msgdat		; pointer to messages.
	sta z80_l
	lda #>msgdat
	sta z80_h
	jsr getwrd		; get message number.
dmsg3:
	jsr preprt		; pre-printing stuff.
	jsr checkx		; make sure we"re in a printable range.
	lda prtmod		; print mode.
	bne bmsg1		; no, double-height text.
dmsg0:
	lda z80_h		; store string pointer.
	pha
	lda z80_l
	pha

	ldy #0
	lda (z80_hl),y		; fetch byte to display.
	and #127		; remove any end marker.
	cmp #13
	beq dmsg1
	jsr pchar		; display character.
	jsr nexpos 		; display position.
	bne dmsg2		; not on a new line.
	jsr nexlin		; next line down.
dmsg2:
	pla			; retrieve string pointer
	sta z80_l
	pla
	sta z80_h

	ldy #0
	lda (z80_hl),y		; fetch last character.
	asl a  			; was it the end?
	bcc :+
	jmp dscor2		; yes, job done.
:
	inc z80_l		; next character to display.
	bne :+
	inc z80_h
:
	jmp dmsg0
dmsg1:
	inc dispy
	lda dispy
	cmp #24
	bcc dmsg4
	lda #0
	sta dispy
dmsg4:
	lda #0
	sta dispx
	jmp dmsg2

;prtmod:	.byte 0            	; print mode, 0 = standard, 1 = double-height.

;----------------------------------------------------------
; Display message in big text.
;
; Input:
;  HL = string pointer
;----------------------------------------------------------

bmsg1:
	ldy #0
	lda (z80_hl),y 		; get character to display.
	and #127		; only want 7 bits.
	cmp #13
	beq bmsg2
	jsr bchar 		; display big char.
bmsg3:
	ldy #0
	lda (z80_hl),y 		; look at last character.
	pha
	inc z80_l 		; next character in list.
	bne :+
	inc z80_h
:
	pla
	asl a  			; was terminator flag set?
	bcc bmsg1		; no, keep going.
	rts
bmsg2:
	lda #0
	sta dispx
	inc dispy
	inc dispy
	lda dispy
	cmp #23
	bcc bmsg3
	lda #0
	sta dispy
	jmp bmsg3

;----------------------------------------------------------
; Big character display.
;
; Input:
;  A = character
;----------------------------------------------------------

bchar:
	sta z80_e		; save char in lb
	lda #0
	sta z80_d		; reset hb

	asl z80_e 		; multiply char by 8.
	rol z80_d
	asl z80_e
	rol z80_d
	asl z80_e
	rol z80_d		; de = a*8

	clc			; de = Fontpointer + a*8
	lda z80_e
	adc #<(FONT-256) 		; address of font.
	sta z80_e
	lda z80_d
	adc #>(FONT-256)
	sta z80_d

	jsr gprad 		; get screen address.

	ldx #0			; height of character in font.
bchar0:
	ldy #0
	lda (z80_de),y 		; get a bit of the font.

.if iflag
	eor #$ff
.endif

	sta (scraddr),y
	pha
	jsr nline 		; next line down.
	pla
	sta (scraddr),y
	jsr nline 		; next line down.

	clc
	inc z80_e 		; next line of font.
	bne :+
	inc z80_d
:
	inx
	cpx #8
	bne bchar0
	
	jsr nexpos		; display position.
	bne bchar2 		; not on a new line.
bchar3:
	inc dispy
	jsr nexlin 		; next line check.
bchar2:
	jmp dscor2		; tidy up line and column variables.


;-------------------------------------------------
; Display a character.
;
; Input:
;  A = character
;-------------------------------------------------

achar:
	sta z80_b 		; copy to b.
	jsr preprt 		; get ready to print.
	lda z80_b		; character in accumulator.
	ldx prtmod 		; print mode.
	beq :+
	jmp bchar 		; no, double-height text.
:
	jsr pchar 		; display character.
	jsr nexpos 		; display position.
	beq bchar3		; next line down.
	jmp bchar2 		; tidy up.


;-------------------------------------------------
; Get next print column position.
;-------------------------------------------------

nexpos:
	inc dispx		; move along one position.
	lda dispx 		; get coordinate.
;	and #31
	rts 			; return with status in zero flag.

;-------------------------------------------------
; Get next print line position.
;-------------------------------------------------

nexlin:
	inc dispy 		; newline.
	lda dispy		; vertical position.
	cmp #24			; past screen edge?
	bcs :+
	rts			; no, still okay.
:
	lda #0			; restart at top.
	sta dispy
	rts

;--------------------------------------------------------
; Pre-print preliminaries.
;--------------------------------------------------------

preprt:
	lda #<(FONT-256)		; font pointer.
	sta grbase		; set up graphics base.
	lda #>(FONT-256)
	sta grbase+1
prescr:
	lda charx 		; display coordinates.
	sta dispx		; set up general coordinates.
	lda chary
	sta dispy
	rts

;--------------------------------------------------------------
; Get messagenr x in hl
;
; Input:
;  HL = pointer to message list
;  X  = message number.
;--------------------------------------------------------------

getwrd:
	cpx #0
	bne:+ 			; first word in list?
	rts 			; yep, don't search.
:
	ldy #0
getwd0:
	lda (z80_hl),y
	pha
	inc z80_l
	bne :+
	inc z80_h
:
	pla
	cmp #128		; found end?
	bmi getwd0		; no, carry on.
	dex			; until we have right number.
	bne getwd0
	rts

;-----------------------------------------------------------
; Bubble sort.
;-----------------------------------------------------------

bsort:
	lda #NUMSPR - 1		; sprites to swap.
	sta qscnt

	lda #<sprtab 		; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
bsort0:
	ldy #0
	lda (z80_ix),y 		; first sprite type.
	cmp #255 		; is it switched off?
	beq swemp		; yes, may need to switch another in here.

	ldy #TABSIZ
	lda (z80_ix),y 		; check next slot exists.
	cmp #255 		; is it enabled?
	beq bsort2 		; no, nothing to swap.

	ldy #TABSIZ+3
	lda (z80_ix),y 		; fetch next sprite's coordinate.
	ldy #3
	cmp (z80_ix),y 		; compare with this x coordinate.
	bcc bsort1		; next sprite is higher - may need to switch.
bsort2:
	clc
	lda z80_x
	adc #TABSIZ 		; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec qscnt 
	bne bsort0		; repeat for remaining sprites.
	rts

;qscnt:	.byte 0

bsort1:
	ldy #TABSIZ
	lda (z80_ix),y		; sprite on/off flag.
	cmp #255		; is it enabled?
	beq bsort2		; no, nothing to swap.
	jsr swspr		; swap positions.
	jmp bsort2
swemp:
	ldy #TABSIZ
	lda (z80_ix),y		; next table entry.
	cmp #255		; is that one on?
	beq bsort2		; no, nothing to swap.
	jsr swspr		; swap positions.
	jmp bsort2

; Swap sprites.

swspr:
	lda z80_x		; table address 
	sta z80_e		; copy to de pair.
	sta z80_l		; copy to hl pair.
	lda z80_i
	sta z80_h
	sta z80_d

	clc
	lda z80_l
	adc #TABSIZ		; distance to second entry.
	sta z80_l
	bcc :+
	inc z80_h
:
	lda #TABSIZ		; bytes to swap.
	sta z80_b
	ldy #0
swspr0:
	lda (z80_hl),y		; fetch second byte.
	pha
	lda (z80_de),y 		; fetch first byte.
	sta (z80_hl),y 		; copy to second.
	pla
	sta (z80_de),y 		; copy to first sprite entry.

	inc z80_e 		; next byte.
	bne :+	
	inc z80_d
:
	inc z80_l 		; next byte.
	bne :+
	inc z80_h
:
	dec z80_b
	bne swspr0 		; swap all bytes in table entry.
	rts

;----------------------------------------------------
; Process sprites.
;----------------------------------------------------

pspr:
	lda #NUMSPR		; sprites to process.
	sta sprptr

	lda #<sprtab 		; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
pspr1:
	ldy #0
	lda (z80_ix),y		; fetch sprite type.
	cmp #9 			; within range of sprite types?
	bcs :+
	jsr pspr2 		; yes, process this one.
:
	clc
	lda z80_x
	adc #TABSIZ 		; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i		; next sprite.
:
	dec sprptr 		; repeat for remaining sprites.
	bne pspr1
	rts

;sprptr:	.byte 0

pspr2:
	lda z80_x 		; store original sprite pointer.
	sta ogptr
	lda z80_i
	sta ogptr+1
	jsr pspr3		; do the routine.
rtorg:
	lda ogptr 		; restore original pointer to sprite.
	sta z80_x
	lda ogptr+1
	sta z80_i
rtorg0:
	rts

pspr3:
	lda #<evtyp0		; sprite type events list.
	sta z80_l
	lda #>evtyp0
	sta z80_h
pspr4:
	lda (z80_ix),y
	asl a			; double accumulator.
	clc
	adc z80_l
	sta z80_l
	bcc :+
	inc z80_h
:
	lda (z80_hl),y
	sta z80_e 		; copy to de.
	pha

	inc z80_l 		; next byte of address.
	bne :+
	inc z80_h
:
	lda (z80_hl),y 		; address high.
	sta z80_d

	pha	 		; swap address into hl.
	lda z80_h
	sta z80_d
	pla
	sta z80_h
	pla
	sta z80_l
	lda z80_l
	sta z80_e
	
	jmp (z80_hl) 		; go there.

;ogptr:	.word 0			; original sprite pointer.

; Address of each sprite type's routine.

evtyp0:	.word evnt00
evtyp1:	.word evnt01
evtyp2:	.word evnt02
evtyp3:	.word evnt03
evtyp4:	.word evnt04
evtyp5:	.word evnt05
evtyp6:	.word evnt06
evtyp7:	.word evnt07
evtyp8:	.word evnt08

;--------------------------------------------------------------
; Display sprites.
;
; Input:
;  IX = sprite table
;--------------------------------------------------------------

dspr:
	lda #(NUMSPR/2)		; number of sprites to display.
	sta sprcnt
dspr0:
	ldy #0
	lda (z80_ix),y 		; get sprite type.
	cmp #255 		; is it enabled?
	bne dspr1 		; yes, it needs deleting.
dspr5:
	ldy #5
	lda (z80_ix),y 		; new type.
	cmp #255		; is it enabled?
	bne dspr3 		; yes, it needs drawing.
dspr2:
	ldy #5
	lda (z80_ix),y 		; copy new type.
	ldy #0
	sta (z80_ix),y
	ldy #6
	lda (z80_ix),y 		; copy new image number.
	ldy #1
	sta (z80_ix),y
	ldy #7
	lda (z80_ix),y 		; copy new frame.
	ldy #2
	sta (z80_ix),y
	ldy #8
	lda (z80_ix),y 		; copy new y.
	ldy #3
	sta (z80_ix),y
	ldy #9
	lda (z80_ix),y 		; copy new x.
	ldy #4
	sta (z80_ix),y

	clc
	lda z80_x
	adc #(TABSIZ*2)		; distance to next odd/even entry.
	sta z80_x
	lda z80_i
	adc #0
	sta z80_i 		; next sprite.
	dec sprcnt
	bne dspr0		; repeat for remaining sprites.
	rts
dspr1:
	ldy #5
	lda (z80_ix),y 		; type of new sprite.
	cmp #255		; is this enabled?
	bne dspr4 		; yes, display both.
dspr6:
	jsr sspria 		; show single sprite.
	jmp dspr2

; Displaying two sprites.  Don't bother redrawing if nothing has changed.

dspr4:
	ldy #4
	lda (z80_ix),y		; old x.
	ldy #9
	cmp (z80_ix),y 		; compare with new value.
	bne dspr7 		; they differ, need to redraw.

	ldy #3
	lda (z80_ix),y		; old y.
	ldy #8
	cmp (z80_ix),y 		; compare against new value.
	bne dspr7		; they differ, need to redraw.

	ldy #2
	lda (z80_ix),y 		; old frame.
	ldy #7
	cmp (z80_ix),y 		; compare against new value.
	jmp dspr7 		; they differ, need to redraw.

	ldy #1
	lda (z80_ix),y 		; old image.
	ldy #6
	cmp (z80_ix),y 		; compare against new value.
	beq dspr2		; everything is the same, don't redraw.
dspr7:
	jsr sspric 		; delete old sprite, draw new one simultaneously.
	jmp dspr2
dspr3:
	jsr ssprib 		; show single sprite.
	jmp dspr2

;sprcnt:	.byte 0

;-----------------------------------------
; Get sprite address calculations.
; gspran = new sprite, gsprad = old sprite.
;
; Input:
;  IX = sprite address
;-----------------------------------------

gspran:
	ldy #8
	lda (z80_ix),y 		; new y coordinate.
	sta dispy
	ldy #9
	lda (z80_ix),y 		; new x coordinate.
	sta dispx
	ldy #6
	lda (z80_ix),y 		; new sprite image.
	jsr gfrm		; fetch start frame for this sprite.

	ldy #0
	lda (z80_hl),y 		; frame in accumulator.
	ldy #7
	clc
	adc (z80_ix),y 		; new add frame number.
	jmp gspra0

;-----------------------------------------
; Calculate old sprite address
;
; Input:
;  IX = sprite address
;
; Output:
;  B  = right byte mask
;  C  = left byte mask
;  DE = spriteframe address
;  scraddr = screenaddress(dispx,dispy)
;-----------------------------------------

gsprad:
	ldy #3
	lda (z80_ix),y		; y coordinate.
	sta dispy
	ldy #4
	lda (z80_ix),y		; x coordinate.
	sta dispx
	ldy #1
	lda (z80_ix),y 		; sprite image.
	jsr gfrm 		; fetch start frame for this sprite.

	ldy #0
	lda (z80_hl),y 		; frame in accumulator.
	ldy #2
	clc
	adc (z80_ix),y 		; add frame number.
gspra0:
	lsr a	  		; multiply by 128.
	sta z80_d 		; store in d.
	sta tmp1
	lda #0
	ror a
	sta z80_e 		; got low byte.
	sta tmp2
.if bflag
	lsr tmp1
	ror tmp2
	clc
	lda tmp2
	adc z80_e
	sta z80_e
	lda tmp1
	adc z80_d
	sta z80_d
.endif
	clc 			; address of play sprites.
	lda z80_e
	adc #<sprgfx
	sta z80_e
	lda z80_d
	adc #>sprgfx
	sta z80_d

	lda dispx 		; y coordinate.
	and #6 			; position within byte boundary.
	tax	 		; low byte of table displacement.

	asl a	  		; multiply by 32.
	asl a  			; already a multiple
	asl a  			; of 2, so just 4
.if bflag
	sta tmp1
	asl a  			; shifts needed.
	clc
	adc tmp1
.else
	asl a  			; shifts needed.
.endif
	clc 			; add to sprite address.
	adc z80_e
	sta z80_e
	bcc :+
	inc z80_d
:
	lda spmask,x		 ; pointer to mask table.
	sta z80_c 		; left mask.
	lda spmask+1,x
	sta z80_b 		; right mask.

;------------------------------------------------------------------
; Drop into screen address routine.
; This routine returns a screen address for (dispx, dispy) in scraddr.
;------------------------------------------------------------------

scadd:
	ldx dispy
	cpx #192
	bcc :+
	ldx #192
:
	lda dispx
	lsr a
	lsr a
	lsr a
	clc
	adc SCADTB_lb,x
	sta scraddr
	lda SCADTB_hb,x
	sta scraddr+1
	rts

spmask:	.byte $ff,$00
	.byte $3f,$c0
	.byte $0f,$f0
	.byte $03,$fc

tmp1:	.byte 0
tmp2:	.byte 0

;-----------------------------------------------------------------
; These are the sprite routines.
; sspria = single sprite, old (ix).
; ssprib = single sprite, new (ix+5).
; sspric = both sprites, old (ix) and new (ix+5).
;-----------------------------------------------------------------

sspria:
	jsr gsprad		; get old sprite address.
sspri2:
	lda z80_e
	sta dline1+1		; Set spritedata address
	sta dline2+1
	lda z80_d
	sta dline1+2
	sta dline2+2
	stx xtmp
	ldx #0			; vertical lines.
sspri0:
	jsr dline		; draw a line.
	cpx #16			; detect block boundery
	bne :+			; skip if not
	inc scraddr+1		; jump to next blockline
:
	cpx #32			; check finished
.if bflag
	bne :+			; skip if not
	inc scraddr+1		; jump to next blockline
	inc z80_hlp+1		; jump to next blockline
:
	cpx #48			; check finished
.endif
	bne sspri0		; no, repeat
	ldx xtmp
	rts

;-----------------------------------------------------------------

ssprib:
	jsr gspran 		; get new sprite address.
	jmp sspri2
	
;-----------------------------------------------------------------

sspric:
	jsr gsprad 		; get old sprite address.
	lda z80_e
	sta ddline1+1		; Set spritedata address
	sta ddline2+1
	lda z80_d
	sta ddline1+2
	sta ddline2+2
	jsr exx  		; store addresses.

	jsr gspran 		; get new sprite addresses.
	lda z80_e
	sta dline1+1		; Set spritedata address
	sta dline2+1
	lda z80_d
	sta dline1+2
	sta dline2+2

	stx xtmp
	ldx #0			; vertical lines.
lloop:
	jsr dline 		; draw a line.
	dex
	dex
	jsr ddline 		; delete a line.
	cpx #16			; detect block boundery
	bne :+			; skip if not
	inc scraddr+1		; jump to next blockline
	inc z80_hlp+1		; jump to next blockline
:
	cpx #32			; check finished
.if bflag
	bne :+			; skip if not
	inc scraddr+1		; jump to next blockline
	inc z80_hlp+1		; jump to next blockline
:
	cpx #48			; check finished
.endif
	bne lloop		; no, repeat
	ldx xtmp
	rts


;-----------------------------------------------------------------

dline:
	ldy sprline,x		; point to screenleft
dline1:
	lda objdta,x		; fetch spriteleft byte
	sta z80_a		; save spriteright byte
	lda z80_a
	and z80_c		; mask left
	eor (scraddr),y		; merge with screenleft
	sta (scraddr),y		; write screenleft
sline:
	inx			; next spritebyte
	iny			; point to screenmiddle
dline2:
	lda objdta,x		; fetch spritemiddle byte
	eor (scraddr),y		; merge with screenmiddle
	sta (scraddr),y		; write screenmiddle
sline2:
	iny			; point to screenright
	lda z80_a		; fetch spriteright byte
	and z80_b		; mask right
	eor (scraddr),y		; merge with screenright
	sta (scraddr),y		; write screenright
sline3:
	inx			; next spritebyte

	rts

;-----------------------------------------------------------------

ddline:
	ldy sprline,x		; point to screenleft
ddline1:
	lda objdta,x		; fetch spriteleft byte
	sta z80_a		; save spriteright byte
	lda z80_a
	and z80_cp		; mask left
	eor (z80_hlp),y		; merge with screenleft
	sta (z80_hlp),y		; write screenleft
dsline:
	inx			; next spritebyte
	iny			; point to screenmiddle
ddline2:
	lda objdta,x		; fetch spritemiddle byte
	eor (z80_hlp),y		; merge with screenmiddle
	sta (z80_hlp),y		; write screenmiddle
dsline2:
	iny			; point to screenright
	lda z80_a		; fetch spriteright byte
	and z80_bp		; mask right
	eor (z80_hlp),y		; merge with screenright
	sta (z80_hlp),y		; write screenright
dsline3:
	inx			; next spritebyte

	rts

sprline:
	.byte $00,$01,$20,$21,$40,$41,$60,$61
	.byte $80,$81,$a0,$a1,$c0,$c1,$e0,$e1
	.byte $00,$01,$20,$21,$40,$41,$60,$61
	.byte $80,$81,$a0,$a1,$c0,$c1,$e0,$e1
	.byte $00,$01,$20,$21,$40,$41,$60,$61
	.byte $80,$81,$a0,$a1,$c0,$c1,$e0,$e1

;----------------------------------------------------------------------
; Line drawn, now work out next target address.
;----------------------------------------------------------------------

nline:
	clc
	lda scraddr 		; get low byte of address.
	adc #$20
	sta scraddr 		; new low byte of address.
	bcc :+
	inc scraddr+1 		; new high byte of address.
:
	rts

;-------------------------------------------------------------
; Line drawn, now work out next target address.
;
; Input:
;  BP  = right mask
;  CP  = left mask
;  DEP = spriteaddress
;  z80_hlp = screen address
;-------------------------------------------------------------


;-----------------------------------------------------------
; Animates a sprite.
;
; Input:
;  IX = sprite address
;  HL = last sprite address
;-----------------------------------------------------------

animsp:
	and frmno
	beq :+
	rts
:
	ldy #6
	lda (z80_ix),y		; sprite image
	jsr gfrm		; get frame data.

	inc z80_l		; point to frames.
	bne :+
	inc z80_h
:
	ldy #7
	lda (z80_ix),y		; sprite frame.
	clc
	adc #1			; next one along.
	ldy #0
	cmp (z80_hl),y		; reached the last frame?
	bcc anims0		; no, not yet.
	lda #0			; start at first frame.
anims0:
	ldy #7
	sta (z80_ix),y		; new frame.
	rts

;--------------------------------------------------------------
; Animate back
;
; Input:
;  IX = sprite address
;  HL = last sprite address
;--------------------------------------------------------------

animbk:
	and frmno
	beq :+
	rts
:
	ldy #6
	lda (z80_ix),y		; sprite image.
	jsr gfrm		; get frame data.

	inc z80_l 		; point to frames.
	bne :+
	inc z80_h
:
	ldy #7
	lda (z80_ix),y 		; sprite frame.
	beq :+
	jmp rtanb0 		; yes, start at end.
:
	ldy #0
	lda (z80_hl),y 		; last sprite.
rtanb0:
	sec
	sbc #1			; next one along.
	jmp anims0		; set new frame.

;--------------------------------------------------------------
; Check for collision with other sprite, strict enforcement.
;
; Input:
;  b		= sprite to test for
;  ix		= current sprite pointer
;
; global:	b
; local:	x,y,hl,de,skptr
; calls:	-
;--------------------------------------------------------------

sktyp:
	lda #<sprtab				; sprite table.
	sta z80_l
	lda #>sprtab
	sta z80_h
numsp2:
	lda #NUMSPR				; number of sprites.
	sta sktptr
sktyp0:
	lda z80_l 				; store pointer to sprite.
	sta skptr
	lda z80_h
	sta skptr+1

	ldy #0
	lda (z80_hl),y 				; get sprite type.
	cmp z80_b				; is it the type we seek?
	beq coltyp				; yes, we can use this one.
sktyp1:
	clc
	lda skptr				; retrieve sprite pointer.
	adc #TABSIZ				; size of each entry.
	sta z80_l
	lda skptr+1
	adc #0
	sta z80_h
	dec sktptr					; one less iteration.
	bne sktyp0				; keep going until we find a slot.
	lda #0					; default to ROM address - no sprite.
	sta z80_l
	sta z80_h
	sta skptr				; store pointer to sprite.
	sta skptr+1

	clc					; don't return with zero flag set.
	rts 					; didn't find one.

;skptr:	.word 0					; search pointer.
;sktptr:	.byte 0

coltyp:
	ldy #0
	lda (z80_ix),y				; current sprite type.
	cmp z80_b				; seeking sprite of same type?
	beq colty1				; yes, need to check we're not detecting ourselves.
colty0:
	ldy #9					; distance to x position in table.
	lda (z80_hl),y				; fetch x coordinate.
	sta z80_e
	dey
	lda (z80_hl),y				; fetch y coordinate.
	sta z80_d

; Drop into collision detection.

colc16:
	ldy #9
	lda (z80_ix),y			 	; x coord.
	sec					; subtract x.
	sbc z80_e
	bcs  colc1a 				; result is positive.
	eor #$ff				; make negative positive.
	clc
	adc #1
colc1a:
	cmp #16 				; within x range?
	bcs sktyp1				; no - they"ve missed.
	sta z80_c				; store difference.

	ldy #8
	lda (z80_ix),y				; y coord.
	sec
	sbc z80_d				; subtract y.
	bcs colc1b				; result is positive.
	eor #$ff				; make negative positive.
	clc
	adc #1
colc1b:
	cmp #16					; within y range?
	bcs sktyp1 				; no - they've missed.
	clc					; add x difference.
	adc z80_c
	cmp #26					; only 5 corner pixels touching?
	bcs :+
	sec
	rts 					; carry set if there's a collision.
:
	jmp sktyp1				; try next sprite in table.
colty1:
	lda z80_x  				; compare the two.
	cmp z80_l
	bne end_col
	lda z80_i
	cmp z80_h
	bne end_col
	jmp sktyp1 				; addresses are identical.
end_col:
	jmp colty0

;-----------------------------------------------------------
; Display number, left aligned
; 
; Input:
;  a		= number
;
; global:	-
; local:	a,y,bc,hl,displ0
; calls:	num2ch,dmsg3
;-----------------------------------------------------------

disply:
	sta z80_a
	lda #<displ0				; display workspace.
	sta z80_c
	lda #>displ0
	sta z80_b
	lda z80_a
	jsr num2ch				; convert accumulator to string.
displ1:
	dec z80_c				; back one character.
	bne :+
	dec z80_b
:
	ldy #0
	lda (z80_bc),y				; fetch digit.
	ora #128				; insert end marker.
	sta (z80_bc),y				; new value.

	lda #<displ0				; display space.
	sta z80_l
	lda #>displ0
	sta z80_h
	jmp dmsg3				; display the string.

displ0:	.byte 0,0,0,13+128

;----------------------------------------------------------------
; Initialise screen.
;
; global:	roomtb,scno
; local:	-
; calls:	tstcs
;----------------------------------------------------------------

initsc:
	lda roomtb 			; whereabouts in the map are we?
	jsr tstsc 			; find displacement.
	cmp #255 			; is it valid?
	beq init_end 			; no, it's rubbish.
	sta scno			; store new room number.
init_end:
	rts

;----------------------------------------------------------------
; Test screen.
;
; global:	-
; local:	x
; calls:	-
;----------------------------------------------------------------

tstsc:
	sta tmproom
	clc
	adc #MAPWID 			; add width in case we"re negative.
	tax 				; add displacement to map data.
	lda mapdat-MAPWID,x 		; find room number there.
	rts

;tmproom:	.byte 0

;--------------------------
; Screen left.
;--------------------------

scrl:
	lda roomtb 			; present room table pointer.
	sec
	sbc #1				; room left.
scrl0:
	jsr tstsc			; test screen.
	cmp #255			; is there a screen this way?
	bne :+
	rts				; no, return to loop.
:
	lda tmproom			; restore room displacement.
	sta roomtb			; new room table position.
scrl1:
	jsr initsc 			; set new screen.
	lda #2
	sta restfl 			; set it.
	rts
scrr:
	lda roomtb 			; room table pointer.
	clc
	adc #1				; room right.
	jmp scrl0
scru:
	lda roomtb 			; room table pointer.
	sec
	sbc #MAPWID 			; room up.
	jmp scrl0
scrd:
	lda roomtb 			; room table pointer.
	clc
	adc #MAPWID 			; room down.
	jmp scrl0

;-----------------------------------------
; Jump to new screen.
;-----------------------------------------

nwscr:
	ldx #0				; start of map data.
nwscr0:
	cmp mapdat,x
	beq nwscr1			; have we found a match for screen?
	inx 				; next room.
	cpx #80				; zero room count, 80 to search.
	bne nwscr0			; keep looking.
	rts
nwscr1:
	stx roomtb			; set the map position.
	jmp scrl1			; draw new room.


;----------------------------------------------------------
; Gravity processing.
;----------------------------------------------------------

grav:
	ldy #13
	lda (z80_ix),y			; jump pointer low.
	sta z80_l
	ldy #14
	lda (z80_ix),y			; jump pointer high.
	sta z80_h
	ora z80_l			; merge in low byte.
	bne :+
	rts				; if neither is set, we're not in the air.
:
	ldy #0
	lda (z80_hl),y			; pixels to move.
	sta z80_a
	cmp #99				; reached the end?
	bne grav0			; no, continue.
grav2:
	dec z80_l			; go back to previous value.
	cmp #$ff
	bne :+
	dec z80_h
:
	lda (z80_hl),y			; fetch that from table.
	sta z80_a
grav0:
	inc z80_l			; point to next table entry.
	bne :+
	inc z80_h
:
	lda z80_l
	ldy #13
	sta (z80_ix),y			; store new pointer low.
	lda z80_h
	ldy #14
	sta (z80_ix),y			; store new pointer high.
grav1:
	lda z80_a
	bne :+				; any movement required?
	rts				; no, not this time.
:
	lda z80_a
	cmp #128			; is it up or down?
	bcs gravu			; it's up.
gravd:
	sta z80_b			; set pixels to move.
gravd0:
	jsr cangd			; can we go down?
	bne gravst			; can't move down, so stop.
	ldy #8
	lda (z80_ix),y			; adjust new x coord.
	clc
	adc #1
	sta (z80_ix),y
	dec z80_b
	bne gravd0
	rts
gravu:
	eor #$ff			; flip the sign so it's positive.
	clc
	adc #1
	sta z80_b			; set pixels to move.
gravu0:
	jsr cangu			; can we go up?
	bne ifalls			; can't move up, go down next.
	ldy #8
	lda (z80_ix),y
	sec
	sbc #1
	sta (z80_ix),y			; adjust new x coord.
	dec z80_b
	bne gravu0
	rts
gravst:
	ldy #13
	lda (z80_ix),y			; jump pointer low.
	sta z80_l
	ldy #14
	lda (z80_ix),y			; jump pointer high.
	sta z80_h

	lda #0				; null value in pointer.
	ldy #13
	sta (z80_ix),y			; store new pointer low.
	iny
	sta (z80_ix),y			; store new pointer high.

	ldy #0
	lda (z80_hl),y			; fetch byte from table.
	cmp #99				; is it the end marker?
evftf:
	beq :+				; yes, fallen too far.
	rts
:
	jmp evnt15			; EVENT FELLTOOFAR

;------------------------------------------------
; Initiate fall check.
;------------------------------------------------

ifall:
	ldy #13
	lda (z80_ix),y 			; jump pointer low.
	sta z80_l
	ldy #14
	lda (z80_ix),y 			; jump pointer high.
	sta z80_h			; high byte in accumulator.
	ora z80_l			; merge in low byte.
	beq :+
	rts				; if either is set, we're already in the air.
:
	ldy #9
	lda (z80_ix),y			; y coordinate.
	sta dispx
	ldy #8
	lda (z80_ix),y			; look x coordinate.
	clc
numsp7:
	adc #SPR_HGT			; add 16 pixels.
	sta dispy			; set up test coordinates.
	jsr tstbl			; get map address.
	jsr plchk			; block, platform check.
	beq :+
	rts				; it's solid, don't fall.
:
	inc bufaddr			; look right one cell.
	jsr plchk			; block, platform check.
	beq :+
	rts				; it's solid, don't fall.
:
	lda dispx			; y coordinate.
	and #7				; position straddling block cells.
	beq ifalls			; no more checks needed.
	inc bufaddr			; look to third cell.
	jsr plchk			; block, platform check.
	beq :+
	rts				; it's solid, don't fall.
:
ifalls:
	lda #<jtab			; jump table start.
	sta z80_l
	lda #>jtab
	sta z80_h
ifal0:
	inc z80_l			; point to next value.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y			; fetch value.
	beq ifal0			; no, get next value.
	cmp #99				; reached end of table?
	bne :+
	rts				; yes, don't fall.
:
	cmp #128			; is it going up?
	bcs ifal0			; yes, looking for first movement down.

	ldy #13
	lda z80_l
	sta (z80_ix),y 			; set jump low.
	ldy #14
	lda z80_h
	sta (z80_ix),y 			; set jump high.
	rts


;----------------------------------------------------
; Get frame data for a particular sprite.
; Input:
;  a		= framenumer
; Output:
;  hl		= frame address
;
; global:	hl,frmptr
; local:	-
; calls:	-
;----------------------------------------------------

gfrm:
	asl a	 		 	; multiple of 2.
	clc
	adc frmptr 			; frames used by game.
	sta z80_l
	lda frmptr+1
	adc #0
	sta z80_h 			; point to frame start.
	rts

;----------------------------------------------------
; Find sprite list for current room.
;
; global:	hl
; local:	x,y
; calls:	-
;----------------------------------------------------

sprlst:
	lda #<nmedat 			; list of enemy sprites.
	sta z80_l
	lda #>nmedat
	sta z80_h
	ldx scno 			; screen number.
	bne sprls2 			; is it the first screen?
	rts 				; yes, don't need to search data.
sprls2:
	ldy #0
sprls1:
	lda (z80_hl),y 			; fetch type of sprite.
	cmp #255			; is it an end marker?
	beq sprls0 			; yes, end of this room.

	clc 				; point to next sprite in list.
	lda z80_l
	adc #NMESIZ
	sta z80_l
	bcc :+
	inc z80_h
:
	jmp sprls1 			; continue until end of room.
sprls0:
	inc z80_l 			; point to start of next screen.s
	bne :+
	inc z80_h
:
	dex
	bne sprls1 			; continue until room found.
	rts


;----------------------------------------------------
; Clear all but a single player sprite.
;
; global:	-
; local:	x,y,ix
; calls:	-
;----------------------------------------------------

nspr:
	lda #NUMSPR			; sprite slots in table.
	sta sprcnt
	lda #<sprtab 			; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
nspr0:
	ldy #0 				; fetch sprite type.
	lda (z80_ix),y 			; is it a player?
	beq nspr1 			; yes, keep this one.

	lda #255
	ldy #0 				; fetch sprite type.
	sta (z80_ix),y 			; delete sprite.
	ldy #5
	sta (z80_ix),y 			; remove next type.

	clc	 			; next sprite.
	lda z80_x
	adc #TABSIZ 			; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt	 			; one less space in the table.
	bne nspr0
	rts
nspr1:
	lda #255
	ldy #0
	sta (z80_ix),y 			; delete sprite.

	clc	 			; point to next sprite.
	lda z80_x
	adc #TABSIZ 			; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt	 			; one less to do.
	bne nspr2
	rts
nspr2:
	lda #255
	ldy #0
	sta (z80_ix),y 			; delete sprite.
	ldy #5
	sta (z80_ix),y 			; remove next type.

	clc	 			; next sprite.
	lda z80_x
	adc #TABSIZ 			; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt	 			; one less space in table.
	bne nspr2
	rts

;----------------------------------------------------------
; Two initialisation routines.
; Initialise sprites - copy everything from list to table.
;
; global:	-
; local:	x,y,ix
; calls:	cpsp
;----------------------------------------------------------

ispr:
	lda #NUMSPR			; sprite slots in table.
	sta sprcnt
	lda #<sprtab			; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
ispr2:
	ldy #0
	lda (z80_hl),y 			; fetch byte.
	cmp #255 			; is it an end marker?
	bne :+
	rts 				; yes, no more to do.
:
ispr1:
	ldy #0
	lda (z80_ix),y 			; fetch sprite type.
	cmp #255 			; is it enabled yet?
	bne ispr4			; yes, try another slot.

	ldy #5
	lda (z80_ix),y		 	; next type.
	cmp #255 			; is it enabled yet?
	beq ispr3 			; no, process this one.
ispr4:
	clc 				; next sprite.
	lda z80_x
	adc #TABSIZ		 	; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dec sprcnt
	bne ispr1 			; repeat for remaining sprites.
	rts  				; no more room in table.
ispr3:
	jsr cpsp			; initialise a sprite.
	dec sprcnt			; one less space in the table.
	bne ispr2
	rts


;-----------------------------------------------------------------------
; Initialise sprites - but not player, we're keeping the old one.
;
; global:	-
; local:	x,y,ix
; calls:	cpsp
;-----------------------------------------------------------------------

kspr:
	ldx #NUMSPR			; sprite slots in table.
	lda #<sprtab 			; sprite table.
	sta z80_x
	lda #>sprtab
	sta z80_i
kspr2:
	ldy #0
	lda (z80_hl),y 			; fetch byte.
	cmp #255 			; is it an end marker?
	bne :+
	rts 				; yes, no more to do.
:
	cmp #0
	bne kspr1 			; no, add to table as normal.

	clc 				; next sprite.
	lda z80_l
	adc #NMESIZ		 	; distance to next odd/even entry.
	sta z80_l
	bcc :+
	inc z80_h
:
	jmp kspr2
kspr1:
	ldy #0 				; fetch sprite type.
	lda (z80_ix),y
	cmp #255 			; is it enabled yet?
	bne kspr4 			; yes, try another slot.

	ldy #5 				; next type.
	lda (z80_ix),y
	cmp #255 			; is it enabled yet?
	beq kspr3 			; no, process this one.
kspr4:
	clc 				; next sprite.
	lda z80_x
	adc #TABSIZ		 	; distance to next odd/even entry.
	sta z80_x
	bcc :+
	inc z80_i
:
	dex	 			; repeat for remaining sprites.
	bne kspr1
	rts  				; no more room in table.
kspr3:
	jsr cpsp 			; copy sprite to table.
	dex	 			; one less space in the table.
	bne kspr2
	rts

;----------------------------------------------
; Copy sprite from list to table.
;
; global:	hl,ix
; local:	y
; calls:	evnt09
;----------------------------------------------

cpsp:
	ldy #0				; fetch byte from table.
	lda (z80_hl),y
	sta (z80_ix),y			; set up type.
	ldy #PAM1ST
	sta (z80_ix),y 			; set up type.

	inc z80_l 			; move to next byte.
	bne :+
	inc z80_h
:
	ldy #0 				; fetch byte from table.
	lda (z80_hl),y
	ldy #6
	sta (z80_ix),y			; set up image.

	inc z80_l 			; move to next byte.
	bne :+
	inc z80_h
:
	ldy #0
	lda (z80_hl),y 			; fetch byte from table.
	ldy #8
	sta (z80_ix),y 			; set up coordinate.

	lda #200 			; set initial coordinate off screen.
	ldy #3
	sta (z80_ix),y

	inc z80_l 			; move to next byte.
	bne :+
	inc z80_h
:
	ldy #0 				; fetch byte from table.
	lda (z80_hl),y
	ldy #9
	sta (z80_ix),y 			; set up coordinate.

	inc z80_l 			; move to next byte.
	bne :+
	inc z80_h
:
	lda #0				; zeroes in accumulator.
	ldy #7 				; reset frame number.
	sta (z80_ix),y
	ldy #10 			; reset direction.
	sta (z80_ix),y
	ldy #13				; reset jump pointer low.
	sta (z80_ix),y
	ldy #14	 			; reset jump pointer high.
	sta (z80_ix),y

	lda #255 			; reset data pointer to auto-restore.
	ldy #16
	sta (z80_ix),y
evis0:
	lda z80_i
	pha
	lda z80_x
	pha
	lda z80_h
	pha
	lda z80_l
	pha

	jsr evnt09 			; perform event.

	pla
	sta z80_l
	pla
	sta z80_h
	pla
	sta z80_x
	pla
	sta z80_i

	clc
	lda z80_x 			; distance to next odd/even entry.
	adc #TABSIZ		 	; next sprite.
	sta z80_x
	bcc :+
	inc z80_i
:
	rts


;-------------------------------------
; Clear the play area window.
;-------------------------------------

clw:
	lda wintop			; get coordinates of window.
	sta dispy			; put into dispx for calculation.
	lda winlft
	sta dispx

	lda winhgt			; height of window.
	sta rrow			; copy to b register.
clw3:
	lda winwid 			; width of window.
	sta rcol
clw2:
	jsr gprad 			; get print address.
	lda #0				; zero byte to write.
	ldx #7				; pixel height of each cell.
clw1:
	ldy scrtab,x
	sta (scraddr),y 			; copy to screen.
	dex				; next screen row down.
	bpl clw1

	inc dispx			; next column.
	dec rcol			; one less to do.
	bne clw2			; repeat for remaining columns.

	lda winlft			; get left edge.
	sta dispx 			; reset x.
	inc dispy 			; next line down.

	dec rrow
	bne clw3			; repeat down the screen.

	lda wintop			; get coordinates of window.
	sta chary			; put into display position.
	lda winlft
	sta charx
	rts


;----------------------------------------------------------
; Effects code.
; Ticker routine is called 25 times per second.
;
; HL = txtscr = left text screen address
; DE = txtscr+txtwid-1 = right text screen address
; BC = txtpos = text scroller position
;
;----------------------------------------------------------

.if sflag
scrly:
	rts
	.word txtscr         	; get left screen address.
	sta scr_l
	lda txtscr+1
	sta scr_l+1
	sta scr_r+1
	
	stx xtmp

	clc         		; get right screen address.
	lda scr_l
	adc txtwid
	sta scr_r
	dec scr_r
scrly1:
	ldy txtwid		; set txtwide
	dey
	clc
scrly0:
	lda (scr_l),y		; scroll 1 line
	rol a
	sta (scr_l),y
	dey
	bpl scrly0

	clc			; point to next line
	lda scr_l
	adc #32
	sta scr_l
	bcc scrly1		; repeat 8 times

	lda txtpos 		; get text pointer.
	sta scr_txt
	lda txtpos+1
	sta scr_txt+1

	ldy #0
	lda (scr_txt),y 		; find character we're displaying.
	and #127 		; remove end marker bit if applicable.
	cmp #13			; is it newline?
	bne scrly5 		; no, it's okay.
	lda #32			; convert to a space instead.
scrly5:
	sta fntaddr		; calculate char address
	lda #0
	sta fntaddr+1
	asl fntaddr  		; multiply char by 8.
	rol fntaddr+1
	asl fntaddr
	rol fntaddr+1
	asl fntaddr
	rol fntaddr+1
	lda fntaddr
	clc
	adc #<(FONT-256)
	sta scrly3+1		; that's the low byte.
	lda fntaddr+1
	adc #>(FONT-256)
	sta scrly3+2		; add displacement.

	ldx #0
scrly3:
	lda $3333,x		; get image of char line.
	and txtbit
	beq scrly2		; don't plot pixel
	ldy scrline,x
	lda (scr_r),y
	clc
	ora #1
	sta (scr_r),y		; plot pixel
scrly2:
	inx			; next line of char.
	cpx #8
	bne scrly3

	lsr txtbit		; bit of text to display.
	bcs :+
	rts
:
	ldy #0
	lda (scr_txt),y 	; what was the character?
	asl a	  		; end of message?
	bcs scrly4
	inc txtpos
	bne :+
	inc txtpos+1
:
	jmp scrly6 		; not yet - continue.
scrly4:
	lda txtini 		; start of scrolling message.
	sta txtpos
	lda txtini+1
	sta txtpos+1
scrly6:
	lda #128
	sta txtbit
	ldx xtmp
	rts

scrline:	.byte $00,$20,$40,$60,$80,$a0,$c0,$e0

;-------------------------------------------------------
; Entry TICKER command
;
; Entry:
;  z80_b = message nr
;  z80_c = width
;-------------------------------------------------------

iscrly:
	jsr prescr 		; set up display position.

	lda #<msgdat 		; text messages.
	sta z80_l
	lda #>msgdat
	sta z80_h

	lda z80_c 		; width.
	sec
	sbc #1			; subtract one.
	cmp #32 		; is it between 1 and 32?
	bcc :+
	lda #$60
	jmp iscrl0		; no, disable messages.
:
	ldx z80_b		; message number.
	jsr getwrd 		; find message start.

	lda z80_l		; set initial text position.
	sta txtini
	lda z80_h
	sta txtini+1

	lda #$ad		; code for lda adrr
iscrl0:
	sta scrly		; enable/disable scrolling routine.

	jsr prescr 		; set up display position.
	jsr gprad 		; get print address.

	lda scraddr 		; set text screen address.
	sta txtscr
	lda scraddr+1
	sta txtscr+1

	lda z80_c		; width.
	sta txtwid		; set width in working storage.

	lda #128 		; start with leftmost bit.
	sta txtbit

	jmp scrly4
.endif

;------------------------------------------------------------------
; Dig routine, conditional assembly depending on dflag
;------------------------------------------------------------------
.if dflag
dig:
	and #3
	beq digr		; dig right
	cmp #1
	beq digl		; dig left
	cmp #2
	beq digd		; dig down

; Dig up.

digu:				; dig up
	ldy #8
	lda (z80_ix),y
	sec
	sbc #2
	sta dispy		; set y

	iny
	lda (z80_ix),y
	sta dispx		; set x
	jmp digv

; Dig down.

digd:
	ldy #9
	lda (z80_ix),y
	sta dispx		; set y

	dey
	clc
	lda (z80_ix),y
	adc #16
	sta dispy		; set y
	jmp digv

; Dig left.

digl:
	ldy #8
	lda (z80_ix),y
	sta dispy		; set y

	iny
	lda (z80_ix),y
	sec
	sbc #2			; x=x-2
	sta dispx		; set x
	jmp digh

; Dig right.

digr:
	ldy #8
	lda (z80_ix),y
	sta dispy		; set y

	iny
	lda (z80_ix),y
	clc
	adc #16
	sta dispx		; set x+16
	jmp digh

; Vertical digging

digv:
	jsr tstbl		; check blocktype in MAP
	jsr fdchk		; test if FODDER

	clc
	lda dispx		; look 1 cell down
	adc #8
	sta dispx
	jsr tstbl		; check blocktype in MAP
	jsr fdchk
	lda dispx
	and #7
	bne :+
	rts
:
	clc
	lda dispx		; look 1 cell down
	adc #8
	sta dispx
	jsr tstbl		; check blocktype in MAP
	jmp fdchk

; Horizontal digging

digh:
	jsr tstbl		; check blocktype in MAP
	jsr fdchk		; test if FODDER

	clc
	lda dispy		; look 1 cell down
	adc #8
	sta dispy
	jsr tstbl		; check blocktype in MAP
	jsr fdchk
	lda dispy
	and #7
	bne :+
	rts
:
	clc
	lda dispy		; look 1 cell down
	adc #8
	sta dispy
	jsr tstbl		; check blocktype in MAP
	jmp fdchk

digcnt:	.byte 0

.endif

;------------------------------------------------------------------
; Sprite table 
;------------------------------------------------------------------


; ix+0  = type.
; ix+1  = sprite image number.
; ix+2  = frame.
; ix+3  = y coord.
; ix+4  = x coord.

; ix+5  = new type.
; ix+6  = new image number.
; ix+7  = new frame.
; ix+8  = new y coord.
; ix+9  = new x coord.

; ix+10 = direction.
; ix+11 = parameter 1.
; ix+12 = parameter 2.
; ix+13 = jump pointer low.
; ix+14 = jump pointer high.
; ix+15 = data pointer low.
; ix+16 = data pointer high.

; block NUMSPR * TABSIZ,255

sprtab:	.res NUMSPR*TABSIZ,255
ssprit:	.byte 255,255,255,255,255,255,255,0,192,120,0,0,0,255,255,255,255
roomtb:	.byte 7                      ; start room map offset.

; User routine.  Put your own code in here to be called with USER instruction.
; if USER has an argument it will be passed in the accumulator.

user:
	.include "user.inc"

setfgcol:
	and #3
	tay
	lda codcol,y
	jmp calfgc

setbgcol:
	and #3
calbgc:
	tay
	lda codcol,y
	pha
	lda andeor
	eor andeor+1
	tax
	pla
	sta andeor+1
	txa
calfgc:
	eor andeor+1
	sta andeor
calex1:
	rts

andeor:	.byte 0,0
codcol:	.byte $00,$55,$aa,$ff

; Everything below here will be generated by the editors.

        rts
WINDOWTOP = 0
WINDOWLFT = 1
WINDOWHGT = 23
WINDOWWID = 30 ;a
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,1,2,3,4,5,6,7,8,9,10,255,255,11,13,14,16,15,255,255,255,255,255,255,255,255,12,255,255,255,255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        jsr skobj	; DETECTOBJECT
        sta varobj
        lda #255
        cmp varobj
        beq *+5
        jmp a00037
        jmp a00122
a00037: lda varobj	; GET
        jsr getob
        lda numlif
        clc
        adc #1
        sta numlif
        lda varc
        clc
        adc #1
        sta varc
        lda #23
        sta chary
        lda #7
        sta charx
        lda varc	; DISPLAY
        jsr disply
        lda #109		; BEEP
        asl a
        sta sndtyp
a00122: lda #1
        cmp vara
        beq *+5
        jmp a00544
        jsr ifall	; TABLEFALL
        lda joyval	; KEY
        and #1
        beq :+
        jmp a00334
:
        lda #228
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00266
        lda #9
        cmp scno
        beq *+5
        jmp a00236
        lda #10
        sta scno
        jsr nwscr
        lda #8
        ldy #9
        sta (z80_ix),y
        rts		; EXIT
a00236: jsr scrr	; SCREENRIGHT
        lda #8
        ldy #9
        sta (z80_ix),y
        rts		; EXIT
        jmp a00334
a00266: jsr cangr	; CANGORIGHT
        beq :+
        jmp a00334
:
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #0		; ANIMATE
        jsr animsp
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00334: lda joyval	; KEY
        and #2
        beq :+
        jmp a00464
:
        lda #5
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00398
        lda #5
        ldy #9
        sta (z80_ix),y
        rts		; EXIT
        jmp a00464
a00398: jsr cangl	; CANGOLEFT
        beq :+
        jmp a00464
:
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0		; ANIMATE
        jsr animsp
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a00464: lda joyval	; KEY
        and #16
        beq :+
        jmp a00491
:
        jsr jump	; TABLEJUMP
a00491: lda #DEADLY	; DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00531
:
        lda #1 		; KILL
        sta deadf
a00531: lda #0
        sta vara
        jmp a00552
a00544: lda #1
        sta vara
a00552: jmp grav
evnt01:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00102
        jsr cangu	; CANGOUP
        beq :+
        jmp b00070
:
        ldy #8 		; SPRITEUP
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp b00098
b00070: lda #1
        ldy #11
        sta (z80_ix),y
        lda #25		; BEEP
        asl a
        sta sndtyp
b00098: jmp b00177
b00102: jsr cangd	; CANGODOWN
        beq :+
        jmp b00149
:
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp b00177
b00149: lda #0
        ldy #11
        sta (z80_ix),y
        lda #45		; BEEP
        asl a
        sta sndtyp
b00177: lda #3
        cmp varb
        beq *+5
        jmp b00205
        lda #0		; ANIMATE
        jsr animsp
b00205: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00241
:
        lda #1 		; KILL
        sta deadf
b00241: rts
evnt02:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00173
        jsr cangl	; CANGOLEFT
        beq :+
        jmp c00141
:
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp c00169
c00141: lda #1
        ldy #11
        sta (z80_ix),y
        lda #70		; BEEP
        asl a
        sta sndtyp
c00169: jmp c00319
c00173: jsr cangr	; CANGORIGHT
        beq :+
        jmp c00291
:
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        jmp c00319
c00291: lda #0
        ldy #11
        sta (z80_ix),y
        lda #70		; BEEP
        asl a
        sta sndtyp
c00319: lda #1
        cmp varb
        beq *+5
        jmp c00347
        lda #0		; ANIMATE
        jsr animsp
c00347: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00382
:
        lda #1 		; KILL
        sta deadf
c00382: rts
evnt03:
        lda #0
        cmp vara
        beq *+5
        jmp d00030
        lda #0		; ANIMATE
        jsr animsp
d00030: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00162
        lda #3
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp d00162
        lda #4	; SPAWN
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
        lda varrnd
        sta z80_c
        ldy #11
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #11
        sta (z80_ix),y
d00162: ldy #11
        lda (z80_ix),y
        clc
        adc #1
        ldy #11
        sta (z80_ix),y
        lda #60
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00225
        lda #0
        ldy #11
        sta (z80_ix),y
d00225: rts
evnt04:
        lda #0
        cmp vara
        beq *+5
        jmp e00030
        lda #0		; ANIMATE
        jsr animsp
e00030: jsr cangd	; CANGODOWN
        beq :+
        jmp e00102
:
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp e00132
e00102: lda #2		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
e00132: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00168
:
        lda #1 		; KILL
        sta deadf
e00168: rts
evnt05:
        lda #0
        cmp vara
        beq *+5
        jmp f00030
        lda #0		; ANIMATE
        jsr animsp
f00030: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00194
:
        lda #230		; BEEP
        asl a
        sta sndtyp
        lda #5 	; DELAY
        jsr delay
        lda #240		; BEEP
        asl a
        sta sndtyp
        lda #5 	; DELAY
        jsr delay
        lda #180		; BEEP
        asl a
        sta sndtyp
        lda #5 	; DELAY
        jsr delay
        lda #75		; BEEP
        asl a
        sta sndtyp
        lda #10 	; DELAY
        jsr delay
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #12
        sta scno
        jsr nwscr
        lda #1	; RESTART
        sta restfl
f00194: rts
evnt06:
        rts
evnt07:
        rts
evnt08:
        rts
evnt09:
        lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00066
        lda vara
        clc
        adc #1
        sta vara
        lda #0
        ldy #11
        sta (z80_ix),y
        lda #69
        ldy #12
        sta (z80_ix),y
j00066: rts
evnt10:
        lda #0
        cmp scno
        beq *+5
        jmp k00181
        lda #22
        sta chary
        lda #9
        sta charx
        lda #4  	; MESSAGE
        jsr dmsg
        lda #21
        sta chary
        lda #2
        sta charx
        lda #2  	; MESSAGE
        jsr dmsg
        
rtcon:			; CONTROLMENU
        jsr vsync
        lda #0
        sta contrl
        lda keys+7
        jsr ktest
        bcc rtcon1
        lda #1
        sta contrl
        lda keys+8
        jsr ktest
        bcc rtcon1
        lda #2
        sta contrl
        lda keys+9
        jsr ktest
        bcs rtcon
        rtcon1:
        lda #1
        sta scno
        jsr nwscr
        lda #1	; RESTART
        sta restfl
k00181: lda #12
        cmp scno
        beq *+5
        jmp k00253
        lda #8
        sta chary
        lda #6
        sta charx
        lda #5  	; MESSAGE
        jsr dmsg
        lda #12
        sta chary
        lda #4
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
k00253: lda varb
        clc
        adc #1
        sta varb
        lda #4
        cmp varb
        beq *+5
        jmp k00293
        lda #0
        sta varb
k00293: rts
evnt11:
        rts
evnt12:
        rts
evnt13:
        lda #6
        sta numlif
        lda numlif
        sta varc
        lda varc
        sec
        sbc #1
        sta varc
        jsr cls		; CLS
        lda #23
        sta chary
        lda #1
        sta charx
        lda #1  	; MESSAGE
        jsr dmsg
        lda #23
        sta chary
        lda #7
        sta charx
        lda varc	; DISPLAY
        jsr disply
        lda #0
        sta vara
        lda #0
        sta varb
        rts
evnt14:
        lda #0
        sta vara
        lda #0
        sta varb
        lda #23
        sta chary
        lda #7
        sta charx
        lda varc	; DISPLAY
        jsr disply
        rts
evnt15:
        rts
evnt16:
        lda #128		; BEEP
        asl a
        sta sndtyp
        lda #2 	; DELAY
        jsr delay
        lda #140		; BEEP
        asl a
        sta sndtyp
        lda #2 	; DELAY
        jsr delay
        lda #232		; BEEP
        asl a
        sta sndtyp
        lda #30 	; DELAY
        jsr delay
        lda numlif
        sec
        sbc #1
        sta numlif
        lda varc
        sec
        sbc #1
        sta varc
        lda #0
        cmp numlif
        beq *+5
        jmp q00165
        lda #10
        sta chary
        lda #11
        sta charx
        lda #0  	; MESSAGE
        jsr dmsg
        lda #250 	; DELAY
        jsr delay
q00165: rts
evnt17:
        lda #50 	; DELAY
        jsr delay
        rts
evnt18:
        lda #50 	; DELAY
        jsr delay
        rts
evnt19:
        rts
evnt20:
        rts
ptcusr: rts
msgdat:
        .byte "GAME OVER",141
        .byte "LIVES",141
        .byte "1 QOP 2 KEMPSTON 3 SINCLAIR",141
        .byte "PUSH ANY KEY TO START",141
        .byte "(C)2018 SUNTEAM",141
        .byte "THE STORY CONTINUES...",141
        .byte "PLEASE LOAD THE NEXT PART",141
nummsg:
        .byte 7
chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 255,189,219,231,231,219,189,255
        .byte 129,0,24,52,44,24,0,129
        .byte 182,223,251,191,95,170,85,170
        .byte 0,0,0,0,0,0,0,255
        .byte 1,1,1,1,1,1,1,255
        .byte 1,1,1,1,1,1,1,1
        .byte 0,0,0,0,4,14,4,0
        .byte 0,0,32,0,0,0,0,0
        .byte 0,0,24,24,0,0,0,0
        .byte 255,241,102,15,7,96,113,48
        .byte 255,191,28,32,59,151,198,200
        .byte 6,70,224,64,9,129,0,32
        .byte 24,28,4,128,146,3,33,0
        .byte 127,252,184,153,131,160,178,216
        .byte 192,216,156,204,224,230,198,144
        .byte 132,142,196,224,225,204,134,160
        .byte 254,63,29,153,193,5,77,27
        .byte 3,27,57,51,7,103,99,9
        .byte 33,113,35,7,135,51,97,5
        .byte 0,0,0,24,124,255,255,255
        .byte 17,0,68,0,17,0,68,0
        .byte 128,192,192,96,176,76,178,25
        .byte 224,114,59,17,192,102,34,1
        .byte 8,132,32,48,8,128,68,0
        .byte 0,136,0,34,0,0,0,4
        .byte 3,2,24,112,196,0,0,0
        .byte 0,0,0,0,0,12,57,128
        .byte 3,2,12,8,34,64,208,128
        .byte 224,56,206,57,30,131,8,6
        .byte 0,0,0,192,112,56,78,115
        .byte 3,6,48,0,128,0,0,0
        .byte 0,0,0,0,1,0,24,128
        .byte 4,1,0,48,2,128,0,0
        .byte 8,12,6,4,5,6,2,1
        .byte 14,12,26,29,26,8,12,12
        .byte 136,208,116,40,52,24,8,12
        .byte 32,0,16,4,8,8,2,8
        .byte 48,80,160,96,32,160,64,128
        .byte 112,48,80,16,48,184,88,56
        .byte 3,5,18,44,28,8,80,48
        .byte 48,16,48,80,32,224,64,128
        .byte 0,4,32,0,8,40,0,16
        .byte 131,213,173,147,165,221,255,127
        .byte 179,221,173,51,125,253,255,255
        .byte 24,189,202,246,247,239,239,255
        .byte 9,181,203,213,241,235,239,254
        .byte 2,6,14,30,63,7,56,52
        .byte 0,128,192,224,112,64,48,112
        .byte 40,48,36,16,32,4,0,32
        .byte 48,80,32,144,32,0,16,0
        .byte 32,112,232,0,232,160,72,160
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 5
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
sprgfx:
        .byte 32,124,0,130,113,13,29,133,189,133,189,129,189,225,26,226,246,124,247,0,247,186,99,56,0,224,29,156,62,62,127,63
        .byte 8,31,128,32,92,67,71,97,111,97,111,96,111,120,134,184,61,159,61,192,189,238,24,206,0,56,7,103,143,143,223,207
        .byte 194,7,32,8,215,16,81,216,91,216,27,216,27,222,33,174,207,103,15,112,175,123,134,51,0,14,193,217,227,227,247,243
        .byte 240,129,8,2,53,196,20,118,22,246,6,246,134,247,136,107,243,217,3,220,235,222,225,140,128,3,112,118,248,248,253,252
        .byte 32,124,0,130,121,141,13,197,93,197,93,193,93,241,12,242,251,60,123,192,241,216,96,184,7,96,15,144,31,216,0,60
        .byte 8,31,128,32,94,99,67,113,87,113,87,112,87,124,131,60,62,207,30,240,60,118,24,46,1,216,3,228,7,246,0,15
        .byte 194,7,32,8,215,152,80,220,85,220,21,220,21,223,32,207,207,179,7,188,143,29,134,11,0,118,0,249,129,253,192,3
        .byte 240,129,8,2,53,230,20,55,21,119,5,119,197,119,200,51,243,236,1,239,99,199,225,130,128,29,64,62,96,127,240,0
        .byte 8,124,0,130,29,205,2,229,54,229,54,225,54,249,2,58,125,220,93,224,124,232,56,8,0,112,0,248,5,252,12,0
        .byte 2,31,128,32,71,115,64,185,77,185,77,184,77,190,128,142,31,119,23,120,31,58,14,2,0,28,0,62,1,127,3,0
        .byte 192,135,32,8,209,220,80,46,83,110,19,110,147,111,160,35,199,221,5,222,135,206,131,128,0,7,128,15,192,95,0,192
        .byte 240,33,8,2,52,119,148,11,148,219,132,219,228,219,232,8,113,247,129,119,161,243,32,224,192,1,224,3,240,23,0,48
        .byte 4,124,0,194,15,69,1,101,27,161,27,161,27,185,1,122,62,12,47,112,63,120,28,112,0,32,12,28,31,62,63,127
        .byte 1,31,128,48,67,209,64,89,70,232,70,232,70,238,128,94,15,131,11,220,15,222,7,28,0,8,3,7,135,207,207,223
        .byte 192,71,32,12,80,244,80,22,17,186,17,186,145,187,160,23,195,224,2,247,131,247,1,199,0,2,192,193,225,243,243,247
        .byte 240,17,8,3,20,61,148,5,132,110,132,110,228,110,232,5,48,248,192,189,224,253,192,113,128,0,112,48,248,124,252,253
        .byte 8,124,0,130,29,141,2,197,54,197,54,193,54,241,2,242,125,60,93,192,125,244,56,104,3,0,7,112,14,248,1,252
        .byte 2,31,128,32,71,99,64,177,77,177,77,176,77,188,128,188,31,79,23,112,31,125,14,26,0,192,1,220,3,190,0,127
        .byte 192,135,32,8,209,216,80,44,83,108,19,108,19,111,32,47,199,211,5,220,71,223,131,134,0,48,0,119,128,239,192,31
        .byte 240,33,8,2,52,118,20,11,20,219,4,219,196,219,200,11,241,244,1,119,209,247,160,225,0,12,192,29,224,59,240,7
        .byte 16,124,0,130,57,141,13,197,93,197,93,193,93,241,12,242,249,124,123,128,243,216,97,184,0,96,3,144,7,216,15,224
        .byte 4,31,128,32,78,99,67,113,87,113,87,112,87,124,131,60,62,95,30,224,60,246,24,110,0,24,0,228,1,246,3,248
        .byte 193,7,32,8,211,152,80,220,85,220,21,220,21,223,32,207,207,151,7,184,143,61,134,27,0,6,0,57,128,125,0,254
        .byte 240,65,8,2,52,230,20,55,21,119,5,119,197,119,200,51,243,229,1,238,99,207,225,134,128,1,64,14,96,31,128,63
        .byte 62,4,65,0,176,142,161,184,161,189,129,189,135,189,71,88,62,111,0,239,93,239,28,198,7,0,57,184,124,124,252,254
        .byte 15,129,16,64,172,35,40,110,104,111,96,111,97,239,17,214,207,155,192,59,215,123,135,49,1,192,14,110,31,31,191,63
        .byte 67,224,4,16,235,8,138,27,218,27,216,27,216,123,132,117,243,230,240,14,245,222,97,204,0,112,131,155,199,199,239,207
        .byte 16,248,1,4,58,194,226,134,246,134,246,6,246,30,97,29,188,249,188,3,189,119,24,115,0,28,224,230,241,241,251,243
        .byte 62,4,65,0,177,158,163,176,163,186,131,186,143,186,79,48,60,223,3,222,27,143,29,6,6,224,9,240,27,248,60,0
        .byte 15,129,16,64,172,103,40,236,168,238,160,238,163,238,19,204,207,55,128,247,198,227,135,65,1,184,2,124,6,254,15,0
        .byte 67,224,4,16,235,25,10,59,170,59,168,59,168,251,4,243,243,205,224,61,241,184,97,208,0,110,0,159,129,191,3,192
        .byte 16,248,1,4,122,198,194,142,234,142,234,14,234,62,193,60,124,243,120,15,60,110,24,116,128,27,192,39,224,111,0,240
        .byte 62,16,65,0,179,184,167,64,167,108,135,108,159,108,92,64,59,190,7,186,23,62,16,28,14,0,31,0,63,160,0,48
        .byte 15,132,16,64,44,238,41,208,41,219,33,219,39,219,23,16,142,239,129,238,133,207,4,7,3,128,7,192,15,232,0,12
        .byte 3,225,4,16,139,59,10,116,202,118,200,118,201,246,5,196,227,187,160,123,225,115,193,1,0,224,1,240,3,250,0,3
        .byte 64,248,1,4,226,206,2,157,178,157,178,29,178,125,1,113,248,238,232,30,248,92,112,64,0,56,0,124,128,254,192,0
        .byte 62,32,67,0,162,240,166,128,133,216,133,216,157,216,94,128,48,124,14,244,30,252,14,56,4,0,56,48,124,248,254,252
        .byte 15,136,16,192,40,188,41,160,33,118,33,118,39,118,23,160,12,31,3,189,7,191,3,142,1,0,14,12,31,62,63,191
        .byte 3,226,4,48,10,47,10,104,136,93,136,93,137,221,5,232,195,7,64,239,193,239,128,227,0,64,3,131,135,207,207,239
        .byte 128,248,1,12,194,139,2,154,98,23,98,23,98,119,1,122,240,193,208,59,240,123,224,56,0,16,192,224,225,243,243,251
        .byte 62,16,65,0,177,184,163,64,163,108,131,108,143,108,79,64,60,190,3,186,47,190,22,28,0,192,14,224,31,112,63,128
        .byte 15,132,16,64,44,110,40,208,40,219,32,219,35,219,19,208,143,47,128,238,139,239,5,135,0,48,3,184,7,220,15,224
        .byte 3,225,4,16,139,27,10,52,202,54,200,54,200,246,4,244,227,203,160,59,226,251,193,97,0,12,0,238,1,247,3,248
        .byte 64,248,1,4,226,198,2,141,178,141,178,13,178,61,1,61,248,242,232,14,248,190,112,88,0,3,128,59,192,125,0,254
        .byte 62,8,65,0,177,156,163,176,163,186,131,186,143,186,79,48,62,159,1,222,27,207,29,134,6,0,9,192,27,224,7,240
        .byte 15,130,16,64,44,103,40,236,168,238,160,238,163,238,19,204,207,167,128,119,198,243,135,97,1,128,2,112,6,248,1,252
        .byte 131,224,4,16,203,25,10,59,170,59,168,59,168,251,4,243,243,233,224,29,241,188,97,216,0,96,0,156,1,190,0,127
        .byte 32,248,1,4,114,198,194,142,234,142,234,14,234,62,193,60,124,250,120,7,60,111,24,118,0,24,0,39,128,111,192,31
        .byte 7,224,31,248,127,254,255,255,255,255,195,195,140,49,111,246,7,224,3,192,1,128,1,128,1,128,3,192,3,192,1,128
        .byte 1,248,7,254,159,255,255,255,255,255,240,240,99,12,155,253,1,248,0,240,0,96,0,96,0,96,0,240,0,240,0,96
        .byte 0,126,129,255,231,255,255,255,255,255,60,60,24,195,102,255,0,126,0,60,0,24,0,24,0,24,0,60,0,60,0,24
        .byte 128,31,224,127,249,255,255,255,255,255,15,15,198,48,217,191,128,31,0,15,0,6,0,6,0,6,0,15,0,15,0,6
        .byte 3,192,15,240,31,248,63,252,63,252,115,206,68,34,55,236,3,192,1,128,1,128,9,144,23,232,31,248,15,240,3,192
        .byte 0,240,3,252,7,254,15,255,15,255,156,243,145,8,13,251,0,240,0,96,0,96,2,100,5,250,7,254,3,252,0,240
        .byte 0,60,0,255,129,255,195,255,195,255,231,60,36,66,195,126,0,60,0,24,0,24,0,153,129,126,129,255,0,255,0,60
        .byte 0,15,192,63,224,127,240,255,240,255,57,207,137,16,176,223,0,15,0,6,0,6,64,38,160,95,224,127,192,63,0,15
        .byte 3,192,15,240,31,248,23,232,9,144,1,128,1,128,3,192,55,236,68,34,115,206,63,252,63,252,31,248,15,240,3,192
        .byte 0,240,3,252,7,254,5,250,2,100,0,96,0,96,0,240,13,251,145,8,156,243,15,255,15,255,7,254,3,252,0,240
        .byte 0,60,0,255,129,255,129,126,0,153,0,24,0,24,0,60,195,126,36,66,231,60,195,255,195,255,129,255,0,255,0,60
        .byte 0,15,192,63,224,127,160,95,64,38,0,6,0,6,0,15,176,223,137,16,57,207,240,255,240,255,224,127,192,63,0,15
        .byte 1,128,3,192,3,192,1,128,1,128,1,128,3,192,7,224,111,246,140,49,195,195,255,255,255,255,127,254,31,248,7,224
        .byte 0,96,0,240,0,240,0,96,0,96,0,96,0,240,1,248,155,253,99,12,240,240,255,255,255,255,159,255,7,254,1,248
        .byte 0,24,0,60,0,60,0,24,0,24,0,24,0,60,0,126,102,255,24,195,60,60,255,255,255,255,231,255,129,255,0,126
        .byte 0,6,0,15,0,15,0,6,0,6,0,6,0,15,128,31,217,191,198,48,15,15,255,255,255,255,249,255,224,127,128,31
        .byte 3,192,15,240,31,248,23,232,9,144,1,128,1,128,3,192,55,236,68,34,115,206,63,252,63,252,31,248,15,240,3,192
        .byte 0,240,3,252,7,254,5,250,2,100,0,96,0,96,0,240,13,251,145,8,156,243,15,255,15,255,7,254,3,252,0,240
        .byte 0,60,0,255,129,255,129,126,0,153,0,24,0,24,0,60,195,126,36,66,231,60,195,255,195,255,129,255,0,255,0,60
        .byte 0,15,192,63,224,127,160,95,64,38,0,6,0,6,0,15,176,223,137,16,57,207,240,255,240,255,224,127,192,63,0,15
        .byte 3,192,15,240,31,248,63,252,63,252,115,206,68,34,55,236,3,192,1,128,1,128,9,144,23,232,31,248,15,240,3,192
        .byte 0,240,3,252,7,254,15,255,15,255,156,243,145,8,13,251,0,240,0,96,0,96,2,100,5,250,7,254,3,252,0,240
        .byte 0,60,0,255,129,255,195,255,195,255,231,60,36,66,195,126,0,60,0,24,0,24,0,153,129,126,129,255,0,255,0,60
        .byte 0,15,192,63,224,127,240,255,240,255,57,207,137,16,176,223,0,15,0,6,0,6,64,38,160,95,224,127,192,63,0,15
        .byte 5,160,29,184,46,116,23,238,107,216,245,166,117,175,181,175,53,160,117,175,245,166,107,216,23,238,47,212,30,168,5,64
        .byte 1,104,7,110,11,157,133,251,26,246,189,105,221,107,237,107,13,104,221,107,189,105,26,246,133,251,11,245,7,170,1,80
        .byte 0,90,129,219,66,231,225,126,134,189,111,90,247,90,251,90,3,90,247,90,111,90,134,189,225,126,66,253,129,234,0,84
        .byte 128,22,224,118,208,185,184,95,97,175,155,214,189,214,190,214,128,214,189,214,155,214,97,175,184,95,80,191,160,122,0,21
        .byte 3,192,27,216,45,180,118,110,27,222,101,184,245,182,245,183,5,177,245,182,101,184,27,222,119,238,47,212,30,168,5,64
        .byte 0,240,6,246,11,109,157,155,134,247,25,110,189,109,253,109,65,108,189,109,25,110,134,247,157,251,11,245,7,170,1,80
        .byte 0,60,129,189,66,219,231,102,225,189,134,91,111,91,127,91,16,91,111,91,134,91,225,189,231,126,66,253,129,234,0,84
        .byte 0,15,96,111,208,182,185,217,120,111,225,150,219,214,223,214,196,22,219,214,225,150,120,111,185,223,80,191,160,122,0,21
        .byte 5,160,29,184,46,116,119,238,123,222,29,191,109,191,237,187,141,184,109,187,29,191,123,222,119,238,47,212,30,168,5,64
        .byte 1,104,7,110,11,157,157,251,158,247,199,111,219,111,251,110,35,110,219,110,199,111,158,247,157,251,11,245,7,170,1,80
        .byte 0,90,129,219,66,231,231,126,231,189,241,219,246,219,190,219,136,219,182,219,241,219,231,189,231,126,66,253,129,234,0,84
        .byte 128,22,224,118,208,185,185,223,121,239,252,118,253,182,239,182,226,54,237,182,252,118,121,239,185,223,80,191,160,122,0,21
        .byte 6,96,30,120,46,116,119,238,123,222,253,184,253,182,221,183,29,177,221,182,253,184,123,222,119,238,47,212,30,168,5,64
        .byte 1,152,7,158,11,157,157,251,158,247,63,110,191,109,247,109,71,108,183,109,63,110,158,247,157,251,11,245,7,170,1,80
        .byte 0,102,129,231,66,231,231,126,231,189,143,219,111,219,125,219,17,219,109,219,143,219,231,189,231,126,66,253,129,234,0,84
        .byte 128,25,224,121,208,185,185,223,121,239,227,246,219,246,223,118,196,118,219,118,227,246,121,239,185,223,80,191,160,122,0,21
        .byte 3,192,27,216,45,180,118,110,123,216,29,166,109,175,237,175,141,160,109,175,29,166,123,216,119,238,47,212,30,168,5,64
        .byte 0,240,6,246,11,109,157,155,30,246,135,105,219,107,251,107,35,104,219,107,135,105,30,246,157,251,11,245,7,170,1,80
        .byte 0,60,129,189,66,219,231,102,135,189,97,218,246,218,254,218,8,218,246,218,97,218,135,189,231,126,66,253,129,234,0,84
        .byte 0,15,96,111,208,182,185,217,97,239,152,118,189,182,191,182,130,54,189,182,152,118,97,239,185,223,80,191,160,122,0,21
        .byte 5,160,29,184,46,116,119,232,27,214,101,175,245,174,245,173,5,172,245,174,101,175,27,214,119,232,47,212,30,168,5,64
        .byte 1,104,7,110,11,157,29,250,134,245,217,107,189,107,125,107,1,107,189,107,217,107,134,245,29,250,11,245,7,170,1,80
        .byte 0,90,129,219,66,231,135,126,97,189,246,90,239,90,223,90,192,90,239,90,246,90,97,189,135,126,66,253,129,234,0,84
        .byte 128,22,224,118,208,185,161,223,88,111,189,150,187,214,183,214,176,22,187,214,189,150,88,111,161,223,80,191,160,122,0,21
        .byte 63,254,15,232,5,208,3,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 143,255,3,250,1,116,0,224,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 227,255,128,254,0,93,0,56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 248,255,160,63,64,23,0,14,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 31,236,15,216,7,160,3,192,1,64,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 7,251,3,246,1,232,0,240,0,80,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 193,254,128,253,0,122,0,60,0,20,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 176,127,96,63,128,30,0,15,0,5,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 15,232,7,208,3,176,2,192,1,160,0,192,0,128,0,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 3,250,1,244,0,236,0,176,0,104,0,48,0,32,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 128,254,0,125,0,59,0,44,0,26,0,12,0,8,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 160,63,64,31,192,14,0,11,128,6,0,3,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 31,236,15,216,7,160,3,192,1,64,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 7,251,3,246,1,232,0,240,0,80,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 193,254,128,253,0,122,0,60,0,20,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 176,127,96,63,128,30,0,15,0,5,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,128,0,0,0,128,1,192,1,192,3,160,2,32,7,144,12,8,31,4,30,36,29,76,14,136,5,80,3,32,1,192
        .byte 0,32,0,0,0,32,0,112,0,112,0,232,0,136,1,228,3,2,7,193,7,137,7,83,3,162,1,84,0,200,0,112
        .byte 0,8,0,0,0,8,0,28,0,28,0,58,0,34,0,121,128,192,65,240,65,226,193,212,128,232,0,85,0,50,0,28
        .byte 0,2,0,0,0,2,0,7,0,7,128,14,128,8,64,30,32,48,16,124,144,120,48,117,32,58,64,21,128,12,0,7
        .byte 0,0,0,128,0,0,0,0,0,128,3,224,6,48,31,28,48,2,127,17,127,137,125,17,58,170,29,92,7,240,0,0
        .byte 0,0,0,32,0,0,0,0,0,32,0,248,1,140,7,199,140,0,95,196,95,226,95,68,142,170,7,87,1,252,0,0
        .byte 0,0,0,8,0,0,0,0,0,8,0,62,0,99,193,241,35,0,23,241,151,248,23,209,163,170,193,213,0,127,0,0
        .byte 0,0,0,2,0,0,0,0,0,2,128,15,192,24,112,124,8,192,69,252,37,254,69,244,168,234,112,117,192,31,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,2,0,1,0,1,128,2,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,64,0,96,0,128,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,16,0,24,0,32,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,4,0,6,0,8,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,4,0,2,0,2,0,3,0,1,0,11,192,0,48,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,1,0,0,128,0,128,0,192,0,64,2,240,0,12,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,64,0,32,0,32,0,48,0,16,0,188,0,3,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,16,0,8,0,8,0,12,0,4,0,47,192,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,32,0,96,0,64,0,192,25,128,7,128,3,128,1,128,0,192,0,64,0,32,0,16,0,0,0,0,0,0
        .byte 0,0,0,8,0,24,0,16,0,48,6,96,1,224,0,224,0,96,0,48,0,16,0,8,0,4,0,0,0,0,0,0
        .byte 0,0,0,2,0,6,0,4,0,12,1,152,0,120,0,56,0,24,0,12,0,4,0,2,0,1,0,0,0,0,0,0
        .byte 0,0,128,0,128,1,0,1,0,3,0,102,0,30,0,14,0,6,0,3,0,1,128,0,64,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,4,8,0,96,2,192,1,128,1,0,1,0,1,0,2,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,1,2,0,24,0,176,0,96,0,64,0,64,0,64,0,128,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,128,64,0,6,0,44,0,24,0,16,0,16,0,16,0,32,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,32,16,128,1,0,11,0,6,0,4,0,4,0,4,0,8,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,80,0,128,1,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,84,0,32,0,64,0,0,1,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,21,0,8,0,16,0,0,0,64,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,64,5,0,2,0,4,0,0,0,16,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
frmlst:
        .byte 0,6
        .byte 6,6
        .byte 12,6
        .byte 18,6
        .byte 24,4
        .byte 28,2
        .byte 30,7,37,0
scdat:
        .word 400,315,343,354,324,301,352,379,272,272,359,296,186,397,385,404,422
        .byte 255,0,62,255,4,6,0,255,4,6,0,255,4,6,0,255,4,6,0,0,6,8,0,8,0,0,8,6,8,0,0,7,0
        .byte 9,6,7,0,0,0,8,0,6,9,8,0,9,7,0,0,0,6,0,7,8,9,0,7,6,0,7,9,0,8,0,6,0
        .byte 8,0,7,0,9,6,0,0,7,8,0,9,0,0,6,0,0,0,6,0,0,6,0,8,255,0,4,6,9,0,0,6,0
        .byte 8,6,7,0,0,6,0,8,0,0,6,8,0,0,6,0,8,6,7,9,255,0,4,6,0,0,0,6,9,0,6,8,9
        .byte 0,6,7,0,0,0,6,0,8,4,5,7,0,6,0,0,255,4,4,6,0,7,0,6,0,7,6,0,7,0,6,0,9
        .byte 0,0,6,9,0,8,0,0,0,6,9,0,8,0,0,7,6,8,0,0,6,8,0,6,9,0,0,6,8,0,0,0,6
        .byte 8,255,0,4,9,6,8,0,0,9,8,0,6,0,9,0,6,0,9,6,0,7,0,6,9,0,0,0,6,0,8,0,6
        .byte 8,0,6,255,0,6,6,7,0,0,6,9,0,6,0,8,0,6,0,7,0,0,6,0,0,0,6,0,7,6,7,255,0,5
        .byte 6,0,8,0,6,7,0,6,7,0,0,6,0,9,0,0,6,8,0,0,6,0,8,6,0,9,255,0,4,6,9,7,0
        .byte 6,0,8,6,0,9,0,6,0,8,0,0,6,0,9,0,6,7,0,6,0,8,255,4,4,6,0,0,4,5,0,0,6
        .byte 8,0,0,6,7,0,0,0,6,7,0,0,6,8,0,6,0,0,0,9,0,7,6,8,9,7,8,9,0,6,0,7,0
        .byte 6,9,0,0,0,6,0,8,0,6,0,9,6,9,0,7,0,8,0,6,0,8,0,9,0,7,6,9,8,0,6,0,7
        .byte 255,0,211
        .byte 255,0,6,8,0,0,7,255,0,8,9,255,0,5,7,0,0,0,8,0,0,8,32,31,29,30,255,0,4,27,26,23,22
        .byte 0,0,8,0,32,30,255,0,10,32,31,0,0,0,33,29,30,0,28,27,0,24,23,22,0,0,31,0,33,29,0,8,255,0,6
        .byte 7,255,0,6,33,33,28,0,0,0,25,23,23,22,255,0,8,32,31,29,30,0,28,255,0,7,28,255,0,5,25,24,23
        .byte 22,0,0,28,23,22,0,31,0,0,0,33,26,255,0,15,25,24,25,27,28,0,24,23,22,255,0,24,28,0,0,0,25
        .byte 24,255,0,157,10,11,10,11,10,11,10,11,10,11,10,11,17,0,0,0,14,10,11,17,255,0,6,14,10,11,10,12,13
        .byte 12,13,12,13,12,13,12,13,12,13,18,0,0,0,15,12,13,18,255,0,6,15,12,13,12,255,21,12,19,0,0,0,16
        .byte 21,21,13,10,11,17,0,0,0,16,255,21,15,18,0,0,0,15,21,21,21,12,13,18,0,0,0,15,255,21,15,19,0,0,0
        .byte 16,255,21,5,19,0,0,0,16,255,21,15,18,20,20,20,15,255,21,5,18,0,0,0,15,255,21,15,19,3,3,3,16
        .byte 255,21,5,19,20,20,20,16,255,21,15,13,10,11,10,13,255,21,5,18,3,3,3,15,255,21,16,12,13,12,255,21,6
        .byte 13,10,11,10,13,255,21,26,12,13,12,255,21,34
        .byte 0,7,255,0,4,9,255,0,5,8,0,0,0,7,255,0,5,8,0,0,28,25,23,22,0,0,27,27,26,23,22,0,0,0
        .byte 8,255,0,13,27,28,0,0,24,23,22,28,0,0,0,24,23,22,0,7,0,0,0,32,30,9,0,0,8,0,7,0,0
        .byte 28,255,0,4,25,24,24,255,0,4,25,23,23,22,0,0,32,31,33,31,29,30,255,0,4,27,28,0,27,255,0,5,25
        .byte 255,0,5,25,24,23,23,22,255,0,4,33,27,31,29,0,28,0,0,26,25,24,255,0,12,24,23,23,22,0,0,27,28
        .byte 0,23,22,28,255,0,19,25,24,24,25,28,0,0,25,24,23,22,255,0,22,28,255,0,4,25,24,25,255,0,131,10,11
        .byte 10,11,17,255,0,20,14,10,11,10,11,12,13,12,13,18,255,0,20,15,12,13,12,13,255,21,4,19,0,0,0,14,10
        .byte 11,10,11,10,11,10,11,10,11,10,11,17,0,0,0,16,255,21,8,18,0,0,0,15,12,13,12,13,12,13,12,13,12
        .byte 13,12,13,18,0,0,0,15,255,21,8,19,0,0,0,16,255,21,12,19,0,0,0,16,255,21,8,18,0,0,0,15,255,21,12
        .byte 18,0,0,0,15,255,21,8,19,0,0,0,16,255,21,12,19,20,20,20,16,255,21,8,18,20,20,20,15,255,21,12,18
        .byte 3,3,3,15,255,21,8,19,3,3,3,16,255,21,12,13,10,11,10,13,255,21,8,13,10,11,10,13,255,21,13,12,13
        .byte 12,255,21,10,12,13,12,255,21,22
        .byte 0,0,7,255,0,4,8,0,0,7,255,0,4,7,8,255,0,4,8,255,0,9,8,0,0,0,7,255,0,11,9,8,255,0,7
        .byte 7,0,0,0,22,255,0,5,32,32,31,29,30,0,8,255,0,6,27,27,255,0,7,8,32,23,22,0,9,32,31,255,0,4
        .byte 33,29,30,32,31,29,30,0,28,0,24,22,0,8,0,9,0,32,31,0,24,23,23,22,255,0,7,32,31,0,0,0,33
        .byte 28,0,0,25,24,23,22,0,0,31,255,0,4,24,25,24,25,255,0,10,27,26,255,0,4,25,24,23,22,255,0,6,25
        .byte 255,0,21,25,24,25,255,0,154,10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,11,17,0,0,0,14,10
        .byte 11,10,11,10,11,10,12,13,12,13,12,13,12,13,12,13,12,13,12,13,12,13,12,13,18,0,0,0,15,12,13,12,13
        .byte 12,13,12,255,21,18,19,0,0,0,16,255,21,25,18,0,0,0,15,255,21,12,14,11,10,11,17,255,21,8,19,0,0,0
        .byte 16,255,21,12,15,13,12,13,18,255,21,8,18,0,0,0,15,255,21,12,16,21,21,14,10,11,10,11,10,17,21,21,21
        .byte 19,0,0,0,16,255,21,12,15,21,21,15,12,13,12,13,12,18,21,21,21,18,0,0,0,15,255,21,12,16,21,21,16
        .byte 255,21,5,19,21,21,21,19,0,0,0,16,255,21,12,15,21,21,15,255,21,5,18,21,21,21,18,20,20,20,15,255,21,12
        .byte 16,21,21,16,255,21,5,19,21,21,21,19,3,3,3,16,255,21,7
        .byte 0,8,255,0,10,7,255,0,6,8,255,0,7,7,255,0,7,8,0,28,22,255,0,5,8,255,0,7,9,0,0,8,255,0,4
        .byte 29,30,0,9,0,27,26,25,24,23,22,255,0,5,8,255,0,4,7,255,0,9,33,29,30,28,255,0,4,24,23,22,0
        .byte 9,255,0,15,7,0,0,33,28,255,0,5,25,24,24,25,255,0,4,7,255,0,8,32,33,29,30,0,0,26,255,0,8
        .byte 25,0,25,255,0,5,32,31,29,30,8,32,31,0,0,33,33,255,0,18,31,0,0,0,33,29,33,255,0,4,33,255,0,150
        .byte 10,11,10,11,10,11,17,255,0,4,14,10,11,10,11,10,11,17,0,0,0,14,10,11,10,11,10,11,10,12,13,12,13
        .byte 12,13,18,255,20,4,15,12,13,12,13,12,13,18,0,0,0,15,12,13,12,13,12,13,12,255,21,6,19,255,3,4,15
        .byte 255,21,6,19,0,0,0,16,255,21,13,13,10,11,10,11,13,255,21,6,18,0,0,0,15,255,21,14,12,13,12,13,255,21,7
        .byte 19,0,0,0,16,255,21,25,18,0,0,0,15,255,21,19,14,11,10,17,21,21,19,0,0,0,16,255,21,19,15,12,13
        .byte 18,21,21,18,20,20,20,15,255,21,19,16,21,21,19,21,21,19,3,3,3,16,255,21,19,15,21,21,14,11,10,11,10
        .byte 11,10,11,17,255,21,18,16,21,21,15,13,12,13,12,13,12,13,18,255,21,6
        .byte 0,8,255,0,10,7,255,0,4,8,255,0,9,8,0,0,8,0,0,8,0,0,8,255,0,32,8,255,0,13,7,255,0,5
        .byte 8,255,0,8,8,0,7,255,0,4,27,255,0,4,8,255,0,12,9,255,0,9,27,26,25,23,22,255,0,4,28,22,0
        .byte 8,255,0,4,29,30,255,0,10,28,0,0,0,25,24,22,0,0,28,25,23,22,0,0,9,0,0,33,33,33,0,0,0
        .byte 8,0,0,9,0,28,255,0,6,25,24,28,0,0,25,24,25,255,0,13,27,26,255,0,9,25,255,0,17,26,255,0,111
        .byte 11,10,11,17,255,0,20,14,10,17,0,0,0,12,13,12,18,255,0,20,15,12,18,0,0,0,21,21,21,19,255,0,18
        .byte 14,10,13,21,13,10,11,10,21,21,21,18,255,0,18,15,12,21,21,21,12,13,12,21,21,21,19,255,0,15,14,10,11
        .byte 13,255,21,10,18,255,0,9,14,10,17,0,0,0,15,12,13,255,21,11,19,255,0,9,15,12,18,0,0,0,16,255,21,13
        .byte 18,255,0,5,14,10,11,10,13,21,19,20,20,20,15,255,21,13,19,255,0,5,15,12,13,12,21,21,18,3,3,3,16
        .byte 255,21,13,18,255,0,5,16,255,21,5,13,10,11,10,13,255,21,13,13,10,11,10,11,10,13,255,21,6,12,13,12,255,21,11
        .byte 0,0,7,0,8,255,0,5,9,255,0,9,8,0,0,7,255,0,14,7,0,0,27,27,27,8,255,0,12,8,0,7,9
        .byte 255,0,5,8,27,27,27,28,0,25,25,22,255,0,4,8,255,0,5,9,0,32,255,0,8,28,255,0,6,24,23,22,7
        .byte 255,0,4,32,31,29,30,32,31,33,29,30,0,0,8,0,27,28,255,0,7,25,24,23,23,22,9,0,31,0,0,33,31
        .byte 255,0,4,33,0,0,0,28,255,0,5,26,22,255,0,4,25,24,23,22,255,0,13,28,255,0,7,25,255,0,5,24,24
        .byte 23,22,255,0,11,26,255,0,15,24,24,25,25,255,0,27,25,255,0,111,14,11,17,255,0,27,15,12,18,255,0,17,11
        .byte 10,11,10,17,0,0,0,14,11,10,11,11,11,10,11,10,11,10,11,10,17,0,0,0,14,11,10,11,10,13,12,13,12
        .byte 18,20,20,20,15,13,12,13,12,13,12,13,12,13,12,13,12,18,0,0,0,15,13,12,13,12,255,21,4,19,3,3,3
        .byte 16,255,21,12,19,20,20,20,16,255,21,7,14,11,10,11,10,11,17,255,21,11,18,3,3,3,15,255,21,7,15,13,12
        .byte 13,12,13,18,255,21,8,14,11,10,11,10,11,10,11,10,17,255,21,5,16,255,21,5,19,255,21,8,15,13,12,13,12
        .byte 13,12,13,12,18,255,21,5,15,255,21,5,18,255,21,8,16,255,21,8,19,255,21,5,16,255,21,5,19,255,21,8,15
        .byte 255,21,8,18,255,21,5,15,255,21,5,18,255,21,8,16,255,21,8,19,21,21
        .byte 0,9,255,0,4,28,0,0,25,23,24,23,22,0,7,0,0,8,0,0,0,9,255,0,5,7,255,0,4,8,0,28,255,0,4
        .byte 25,23,24,23,22,255,0,19,28,255,0,7,25,24,24,23,22,0,9,0,7,0,0,0,8,255,0,4,9,8,0,27,28
        .byte 255,0,10,25,24,23,22,0,27,255,0,5,32,31,29,30,0,33,28,255,0,10,28,29,30,0,25,25,26,33,29,30,0,0
        .byte 31,0,0,0,33,33,26,255,0,9,27,26,0,0,33,29,33,28,0,0,33,33,29,30,255,0,10,28,29,30,255,0,8
        .byte 33,28,0,33,33,0,33,0,33,255,0,9,26,0,33,33,255,0,194,14,10,11,17,20,20,20,14,10,11,10,11,17,0,0,0
        .byte 14,10,11,10,17,0,0,0,14,10,17,0,0,0,15,12,13,18,3,3,3,15,12,13,12,13,18,0,0,0,15,12,13
        .byte 12,18,0,0,0,15,12,18,0,0,0,16,21,21,13,10,11,10,13,255,21,4,19,0,0,0,16,21,21,21,19,0,0,0
        .byte 16,21,19,0,0,0,15,21,21,21,12,13,12,255,21,5,18,0,0,0,15,21,21,21,18,0,0,0,15,21,18,0,0,0
        .byte 16,255,21,11,19,0,0,0,16,21,21,21,19,20,20,20,16,21,19,0,0,0,15,255,21,11,18,0,0,0,15,21,21,21
        .byte 18,3,3,3,15,21,18,20,20,20,16,255,21,11,19,20,20,20,16,21,21,21,14,10,11,10,17,21,19,3,3,3,15
        .byte 255,21,11,18,3,3,3,15,21,21,21,15,12,13,12,18,21,13,10,11,10,13,255,21,11,13,10,11,10,13,21,21,21
        .byte 16,21,21,21,19,255,21,14
        .byte 0,0,8,0,9,0,0,0,8,255,0,4,8,255,0,10,9,255,0,4,8,255,0,6,8,0,28,23,23,22,0,0,0
        .byte 8,255,0,4,7,0,8,255,0,7,7,255,0,6,28,25,24,23,23,22,0,7,0,26,23,22,255,0,4,31,29,30,0,0
        .byte 9,255,0,4,8,0,0,26,0,0,25,24,25,23,22,0,0,25,24,23,22,8,255,0,4,33,29,30,0,28,255,0,12
        .byte 24,23,22,0,0,0,25,25,255,0,6,27,27,26,25,0,8,255,0,11,25,25,255,0,10,28,255,0,181,14,10,11,255,0,27
        .byte 15,12,13,11,10,11,17,255,20,4,14,10,11,10,11,10,11,10,11,10,11,17,255,20,4,14,10,11,16,21,21,12,13
        .byte 12,18,255,3,4,15,12,13,12,13,12,13,12,13,12,13,18,255,3,4,15,12,13,15,255,21,5,13,10,11,10,11,13
        .byte 255,21,10,13,10,11,10,11,13,21,21,16,255,21,6,12,13,12,13,255,21,12,12,13,12,13,21,21,21,15,255,21,29
        .byte 16,255,21,29,15,255,21,26,14,10,11,10,17,255,21,25,15,12,13,12,18,255,21,25,16,21,21,21,19,255,21,25,15
        .byte 21,21,21,18,21
        .byte 0,0,8,0,0,7,0,0,0,27,0,0,8,255,0,4,9,0,0,7,0,8,0,0,8,0,0,0,7,7,255,0,6
        .byte 9,28,27,23,22,0,0,32,31,29,30,0,8,0,0,0,8,0,0,8,255,0,7,8,0,27,26,0,0,24,23,22,255,0,4
        .byte 33,0,0,26,23,22,0,0,8,255,0,4,23,22,255,0,9,24,23,22,255,0,6,25,24,23,22,255,0,5,8,25,24
        .byte 25,255,0,6,25,0,25,25,24,255,0,7,25,25,24,22,0,0,9,255,0,26,25,25,255,0,154,10,11,10,11,17,255,0,23
        .byte 14,10,12,13,12,13,18,255,0,23,15,255,21,5,19,255,0,21,14,10,11,10,255,21,4,18,255,0,21,15,12,13,12
        .byte 255,21,4,19,10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,16,255,21,7,18,12,13,12
        .byte 13,12,13,12,13,12,13,12,13,12,13,12,13,12,13,12,13,12,15,255,21,7,19,255,21,21,16,255,21,7,18,255,21,21
        .byte 15,255,21,7,19,255,21,21,16,255,21,7,18,255,21,21,15,255,21,7,19,255,21,21,16,255,21,7,18,255,21,21,15
        .byte 21,21,21
        .byte 255,0,28,42,255,0,10,28,23,22,0,27,26,23,23,22,255,0,8,37,0,39,255,0,4,27,26,22,0,0,28,25,24,24
        .byte 22,0,0,24,24,24,22,0,26,24,23,22,0,0,0,36,39,37,0,0,28,0,25,23,22,26,0,0,0,25,25,255,0,4
        .byte 25,23,22,0,25,25,24,22,0,0,35,39,35,0,26,0,24,25,24,23,22,255,0,11,24,255,0,4,25,0,0,35,39
        .byte 35,255,0,6,25,25,255,0,19,34,38,35,255,0,29,34,255,0,96,14,10,17,255,0,21,10,11,17,0,0,0,15,12
        .byte 18,255,0,21,12,13,18,0,0,0,16,21,19,255,0,9,14,10,17,255,0,9,21,21,19,0,0,0,15,21,18,0,0,0
        .byte 14,10,17,0,0,0,15,12,18,0,0,0,14,10,11,10,11,10,21,21,18,0,0,0,16,21,19,0,0,0,15,12,18
        .byte 0,0,0,16,21,19,0,0,0,15,12,13,12,13,12,21,21,19,0,0,0,15,21,18,0,0,0,16,21,19,0,0,0
        .byte 15,21,18,0,0,0,16,255,21,7,18,20,20,20,16,21,19,20,20,20,15,21,18,0,0,0,16,21,19,0,0,0,15
        .byte 255,21,7,19,3,3,3,15,21,18,3,3,3,16,21,19,20,20,20,15,21,18,20,20,20,16,255,21,7,12,10,11,10
        .byte 12,21,12,10,11,10,12,21,18,3,3,3,16,21,19,3,3,3,15,255,21,8,12,13,12,21,21,21,12,13,12,21,21
        .byte 12,10,11,10,12,21,12,10,11,10,12,255,21,20,12,13,12,21,21,21,12,13,12,255,21,66
        .byte 0,36,40,0,0,15,255,21,4,19,255,21,5,18,0,0,37,255,0,4,34,38,0,36,0,41,0,35,39,37,42,16,12
        .byte 13,12,13,18,255,21,5,19,0,0,0,36,0,0,42,0,0,0,34,40,0,0,35,39,34,38,43,44,45,44,45,46,255,21,5
        .byte 18,0,37,42,34,0,0,41,255,0,7,35,39,255,0,6,15,255,21,6,19,0,34,38,0,36,40,255,0,4,42,0,0,0
        .byte 35,39,255,0,6,16,12,13,12,13,12,13,18,255,0,4,35,39,0,37,0,0,41,0,0,0,34,38,255,0,6,43,44
        .byte 45,44,45,44,45,46,255,0,4,35,39,0,34,0,40,255,0,24,35,39,0,0,36,39,255,0,24,34,38,0,0,35,39
        .byte 255,0,28,34,38,255,0,123,10,11,10,11,10,17,255,0,24,12,13,12,13,12,18,255,0,24,255,21,5,19,255,0,24
        .byte 255,21,5,18,255,0,17,14,10,11,10,11,10,11,255,21,5,19,255,0,17,15,12,13,12,13,12,13,255,21,5,12,10
        .byte 11,10,11,10,11,10,11,10,17,0,0,0,14,10,11,10,12,255,21,12,12,13,12,13,12,13,12,13,12,18,20,20,20
        .byte 15,12,13,12,255,21,22,19,3,3,3,16,255,21,25,12,10,11,10,12,255,21,26,12,13,12,255,21,11
        .byte 0,8,255,0,20,8,255,0,19,8,255,0,4,7,255,0,48,8,0,0,0,7,255,0,16,9,255,0,24,8,255,0,7
        .byte 9,255,0,24,8,255,0,56,9,255,0,39,8,255,0,13,8,255,0,22,9,255,0,25,7,255,0,67,8,0,0,7,255,0,6
        .byte 8,255,0,29,7,0,0,0,8,0,0,0,9,255,0,47,8,255,0,18,7,255,0,11,8,255,0,12,8,255,0,11,8
        .byte 255,0,18,27,28,29,30,0,0,0,27,27,255,0,13,28,29,30,8,0,27,27,26,0,0,0,33,29,30,28,0,33,29
        .byte 30,255,0,4,8,255,0,5,28,0,33,33,29,26,255,0,7,33,28,255,0,4,33,29,30,255,0,6,27,26,255,0,4
        .byte 33,255,0,8,26,255,0,6,33,33,29,30,0,0,0,26,255,0,7
        .byte 0,0,42,0,16,21,19,0,35,40,0,0,16,21,21,19,255,0,4,37,39,0,15,21,18,0,37,42,0,0,0,41,42
        .byte 15,21,18,0,34,38,0,0,15,21,21,18,0,37,0,0,34,38,0,16,21,19,0,35,39,0,0,40,36,39,16,21,19
        .byte 0,0,42,0,42,16,21,21,19,0,35,37,0,0,42,0,15,21,18,0,34,38,0,0,39,34,38,15,13,18,0,0,41
        .byte 37,39,15,21,21,18,0,34,35,0,0,41,0,16,21,19,255,0,4,36,39,0,0,43,45,46,0,40,0,35,39,16,21,21
        .byte 19,0,0,36,0,40,0,0,15,12,18,0,0,42,0,35,39,255,0,4,37,40,0,0,34,38,15,13,12,18,0,0,35
        .byte 0,39,0,0,43,44,46,0,41,38,0,35,39,255,0,4,35,39,255,0,4,43,44,45,46,0,0,35,0,41,255,0,5
        .byte 36,40,0,0,34,38,255,0,4,34,38,255,0,10,35,40,255,0,6,34,41,255,0,20,34,38,255,0,163,14,10,11,17
        .byte 255,0,4,14,10,11,10,17,255,0,10,14,10,11,10,0,0,0,15,12,13,18,255,0,4,15,12,13,12,18,255,0,10
        .byte 16,12,13,12,10,11,10,12,21,21,19,255,0,4,16,21,21,21,19,255,0,8,14,10,12,21,21,21,12,13,12,21,21,21
        .byte 18,255,0,4,15,21,21,21,12,10,17,255,0,6,15,12,255,21,10,19,255,20,4,16,255,21,4,12,18,11,10,11,10
        .byte 11,10,16,255,21,11,18,255,3,4,15,255,21,5,19,12,12,13,12,13,12,15,255,21,11,12,10,11,10,11,12,255,21,5
        .byte 18,255,21,6,16,255,21,12,12,13,12,13,255,21,6,19,255,21,6,15,255,21,22,18,255,21,6,16,255,21,5
        .byte 37,0,0,41,0,16,19,0,16,21,19,0,0,42,0,16,19,37,0,0,34,38,15,21,18,255,0,4,42,34,0,40,36
        .byte 40,15,18,0,15,21,18,0,0,41,0,15,18,0,36,0,0,0,16,21,19,0,37,0,37,39,0,36,39,34,38,16,19
        .byte 0,16,21,19,0,40,0,42,16,19,0,34,0,0,42,15,21,18,0,36,0,34,38,0,35,39,0,0,15,18,0,15,21
        .byte 18,0,39,37,39,15,18,0,0,36,0,41,16,21,19,0,35,255,0,4,35,39,0,0,16,19,40,16,21,19,36,39,34
        .byte 38,16,19,0,0,35,40,0,15,21,18,0,35,0,0,40,0,34,38,0,0,15,18,0,15,21,18,35,39,0,0,15,18
        .byte 0,0,34,38,0,16,21,19,42,34,0,0,41,255,0,5,43,46,0,16,21,19,34,38,0,0,16,19,255,0,5,15,21
        .byte 18,41,0,36,40,255,0,9,15,12,18,255,0,4,15,18,255,0,5,16,21,19,0,0,34,41,255,0,9,43,45,46,255,0,4
        .byte 43,46,255,0,5,15,12,18,255,0,27,43,44,46,255,0,125,10,11,10,17,255,0,26,12,13,12,18,255,0,26,21,21,21
        .byte 19,255,0,22,14,10,11,10,21,21,21,18,255,0,22,15,12,13,12,21,21,21,19,255,0,19,14,10,11,16,255,21,6
        .byte 18,255,0,19,15,12,13,15,255,21,6,19,255,0,8,14,17,255,20,4,14,17,0,0,0,16,21,21,16,255,21,6,18
        .byte 10,11,10,11,10,11,10,11,15,18,255,3,4,15,18,10,11,10,15,21,21,15,255,21,6,19,12,13,12,13,12,13,12
        .byte 13,16,19,10,11,10,11,16,19,12,13,12,16,21,21,16,21,21,21
        .byte 0,37,0,37,0,42,0,0,37,40,0,0,37,255,0,8,37,39,0,37,0,0,0,15,21,0,35,42,0,36,39,0,0
        .byte 35,39,0,0,36,0,41,37,0,0,37,0,0,34,38,0,35,0,0,0,16,21,0,34,41,0,35,39,0,0,35,39,0,0
        .byte 34,40,0,0,36,0,34,255,0,5,34,0,0,42,15,21,255,0,4,35,39,0,0,34,38,255,0,6,35,0,41,36,0,0,0
        .byte 37,0,36,0,41,16,21,255,0,4,34,38,255,0,10,35,40,0,34,0,0,42,35,42,35,40,0,15,21,255,0,16,34
        .byte 38,0,0,36,0,41,35,39,35,39,0,16,21,255,0,20,35,40,0,34,38,35,39,0,15,21,255,0,20,34,38,0,0,0
        .byte 35,39,0,16,21,255,0,25,34,38,0,15,21,255,0,28,16,21,255,0,5,14,10,11,10,11,17,255,0,17,15,21,255,0,5
        .byte 15,12,13,12,13,18,255,0,17,43,45,0,0,0,14,11,12,255,21,4,19,255,0,15,51,0,0,16,0,0,0,15,255,21,6
        .byte 18,255,0,17,51,15,10,11,10,12,255,21,6,19,0,0,0,14,10,11,10,11,10,17,255,0,4,51,0,0,0,16,12
        .byte 13,12,255,21,7,18,0,0,0,15,12,13,12,13,12,18,255,0,5,47,48,0,15,255,21,10,19,0,0,0,16,255,21,5
        .byte 19,0,0,0,47,48,49,50,0,16,255,21,10,18,0,0,0,15,255,21,5,18,0,0,0,49,50,0,47,48,15,255,21,10
        .byte 19,0,0,0,16,255,21,5,12,10,11,10,11,10,11,10,11,12,255,21,10,18,0,0,0,15,255,21,6,12,13,12,13
        .byte 12,13,12,13,255,21,11,19,0,0,0,16,255,21,25,18,20,20,20,15,255,21,25,19,3,3,3,16,255,21,15
        .byte 0,37,255,0,5,15,12,13,15,21,18,12,13,19,0,0,0,42,255,0,4,16,12,13,18,0,0,0,34,0,0,42,0,0
        .byte 43,44,45,16,21,19,44,45,46,0,0,0,41,0,0,42,0,43,44,45,46,255,0,4,36,0,41,42,255,0,4,15,21
        .byte 18,0,0,0,37,0,40,0,37,0,39,255,0,6,42,0,0,35,40,37,39,255,0,4,16,13,19,0,0,0,34,0,39
        .byte 0,35,0,39,255,0,5,37,39,0,0,35,39,34,38,255,0,4,43,44,46,255,0,4,36,39,0,34,36,39,255,0,5
        .byte 35,39,0,0,34,38,255,0,13,35,39,0,0,34,38,255,0,5,35,39,255,0,17,34,38,255,0,9,34,38,255,0,183
        .byte 14,10,17,255,0,4,14,11,17,255,0,4,14,17,255,0,14,16,12,18,10,11,10,11,15,12,18,10,11,10,11,15,18
        .byte 255,0,9,14,10,0,0,0,15,21,19,12,13,12,13,16,21,19,12,13,12,13,16,19,255,0,9,15,12,11,10,11,16
        .byte 21,18,255,21,4,15,21,18,255,21,4,15,18,10,11,10,11,17,255,0,4,15,21,12,13,12,15,21,19,255,21,4,16
        .byte 21,19,255,21,4,16,19,12,13,12,13,18,20,20,20,14,13,255,21,4,16,21,18,255,21,4,15,21,18,255,21,4,15
        .byte 18,255,21,4,19,3,3,3,15,255,21,5,15,21,19,21,21,21,14,10,11,10,17,21,21,21,16,19,21,21,21,14,11
        .byte 10,11,10,11,17,255,21,4,16,21,18,21,21,21,15,12,13,12,18,21,21,14,10,11,10,17,21,15,13,12,13,12,13
        .byte 18,21,21,21,14,11,10,11,10,11,10,16,21,21,21,19,21,21,15,12,13,12,18,21,16,255,21,5,19,21,21,21,15
        .byte 12,13,12,13,12,13,15,21,21,21,18,21,21,16,21,21,21,19,21,15,255,21,5,18,21
numsc:
        .byte 17
nmedat:
        .byte 0,0,152,120,255
        .byte 0,0,80,8,1,2,96,120,1,2,56,192,255
        .byte 0,0,80,8,1,2,120,56,1,2,56,184,2,3,64,120,255
        .byte 0,0,80,8,2,3,80,200,255
        .byte 0,0,80,8,1,2,72,168,255
        .byte 0,0,80,8,1,2,96,144,2,3,96,64,255
        .byte 0,0,96,8,2,3,96,192,2,3,80,144,255
        .byte 0,0,96,8,1,2,104,40,1,2,64,104,1,2,88,160,255
        .byte 0,0,88,8,1,2,56,136,1,2,80,88,255
        .byte 0,0,72,8,1,2,48,88,1,2,96,168,2,3,104,136,255
        .byte 0,0,72,8,1,2,96,40,1,2,48,88,1,2,112,136,1,2,80,184,255
        .byte 0,0,88,8,2,3,128,152,3,4,48,120,3,4,24,56,255
        .byte 255,0,0,112,8,3,4,40,40,3,4,56,112,3,4,48,200,2,3,112,168,255
        .byte 0,0,96,8,3,4,56,48,3,4,72,128,1,2,88,80,1,2,112,184,255
        .byte 5,6,120,184,0,0,96,8,255
        .byte 0,0,112,8,1,2,64,56,1,2,40,128,2,3,80,192,3,4,40,96,3,4,16,208,255
NUMOBJ = 3
objdta:
        .byte 30,0,50,0,122,0,250,0,254,150,14,149,14,149,14,150,14,148,14,100,14,0,14,0,127,192,127,64,127,192,0,0,7,88,192,7,88,192
        .byte 30,0,50,0,122,0,250,0,254,150,14,149,14,149,14,150,14,148,14,100,14,0,14,0,127,192,127,64,127,192,0,0,11,112,208,11,112,208
        .byte 30,0,50,0,122,0,250,0,254,150,14,149,14,149,14,150,14,148,14,100,14,0,14,0,127,192,127,64,127,192,0,0,3,56,224,3,56,224
font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,48,48,48,48,0,48,0
        .byte 0,108,108,0,0,0,0,0
        .byte 0,108,254,108,108,254,108,0
        .byte 0,24,126,120,126,30,126,24
        .byte 0,230,236,24,48,110,206,0
        .byte 0,48,120,48,126,204,126,0
        .byte 0,24,48,0,0,0,0,0
        .byte 0,12,24,24,24,24,12,0
        .byte 0,96,48,48,48,48,96,0
        .byte 0,0,60,24,126,24,60,0
        .byte 0,0,24,24,126,24,24,0
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,56,56,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,124,206,222,246,230,124,0
        .byte 0,56,120,24,24,24,126,0
        .byte 0,124,198,6,124,192,254,0
        .byte 0,124,198,28,6,198,124,0
        .byte 0,24,56,120,216,254,24,0
        .byte 0,254,192,252,6,198,124,0
        .byte 0,124,192,252,198,198,124,0
        .byte 0,254,6,12,24,48,48,0
        .byte 0,124,198,124,198,198,124,0
        .byte 0,124,198,198,126,6,124,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,124,198,198,254,198,198,0
        .byte 0,252,198,252,198,198,252,0
        .byte 0,124,198,192,192,198,124,0
        .byte 0,248,204,198,198,204,248,0
        .byte 0,254,192,252,192,192,254,0
        .byte 0,254,192,252,192,192,192,0
        .byte 0,124,198,192,222,198,124,0
        .byte 0,198,198,254,198,198,198,0
        .byte 0,126,24,24,24,24,126,0
        .byte 0,6,6,6,198,198,124,0
        .byte 0,204,216,240,216,204,198,0
        .byte 0,192,192,192,192,192,254,0
        .byte 0,198,238,254,198,198,198,0
        .byte 0,198,230,246,222,206,198,0
        .byte 0,124,198,198,198,198,124,0
        .byte 0,252,198,198,252,192,192,0
        .byte 0,124,198,198,246,222,124,0
        .byte 0,252,198,198,252,204,198,0
        .byte 0,124,192,124,6,198,124,0
        .byte 0,254,48,48,48,48,48,0
        .byte 0,198,198,198,198,198,124,0
        .byte 0,198,198,198,198,108,56,0
        .byte 0,198,198,198,198,254,108,0
        .byte 0,198,108,56,56,108,198,0
        .byte 0,134,204,120,48,48,48,0
        .byte 0,254,12,24,48,96,254,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 0,0,120,12,124,204,124,0
        .byte 0,96,96,124,102,102,124,0
        .byte 0,0,60,96,96,96,60,0
        .byte 0,12,12,124,204,204,124,0
        .byte 0,0,120,204,248,192,124,0
        .byte 0,28,48,56,48,48,48,0
        .byte 0,0,124,204,204,124,12,120
        .byte 0,192,192,248,204,204,204,0
        .byte 0,48,0,112,48,48,120,0
        .byte 0,12,0,12,12,12,108,56
        .byte 0,96,120,112,112,120,108,0
        .byte 0,48,48,48,48,48,28,0
        .byte 0,0,248,252,252,252,252,0
        .byte 0,0,248,204,204,204,204,0
        .byte 0,0,120,204,204,204,120,0
        .byte 0,0,248,204,204,248,192,192
        .byte 0,0,124,204,204,124,12,14
        .byte 0,0,60,96,96,96,96,0
        .byte 0,0,120,192,120,12,248,0
        .byte 0,48,120,48,48,48,28,0
        .byte 0,0,204,204,204,204,120,0
        .byte 0,0,204,204,120,120,48,0
        .byte 0,0,204,252,252,252,120,0
        .byte 0,0,204,120,48,120,204,0
        .byte 0,0,204,204,204,124,12,120
        .byte 0,0,252,24,48,96,252,0
        .byte 0,30,24,112,24,24,30,0
        .byte 0,24,24,24,24,24,24,0
        .byte 0,240,48,28,48,48,240,0
        .byte 0,60,120,0,0,0,0,0
        .byte 124,198,187,227,227,187,198,124
jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,3,4,5,5,6,99
keys:   .byte 10,8,23,63,47,31,30,31,30,26,24