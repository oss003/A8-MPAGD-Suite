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
WINDOWTOP = 1
WINDOWLFT = 1
WINDOWHGT = 20
WINDOWWID = 30 ;a
MAPWID = 13
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,0,1,2,3,4,5,6,7,8,9,10,255,255,11,12,13,14,15,16,17,18,19,20,255,255
        .byte 255,255,255,255,255,255,255,255,255,255,255,255,255
stmap:  .byte 1

evnt00:
        jsr skobj	; DETECTOBJECT
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00040
        lda #2
        sta varg
a00040: lda #2
        cmp varg
        beq *+5
        jmp a00206
        ldy #8
        lda (z80_ix),y
        sta varj
        lda varj
        lsr a
        lsr a
        sta varj
        lda varj	; BEEP
        asl a
        sta sndtyp
        lda #16
        ldy #6
        sta (z80_ix),y
        lda #160
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00195
        lda #0
        cmp varc
        beq *+5
        jmp a00165
        lda #0		; ANIMATE
        jsr animsp
a00165: ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp a00206
a00195: lda #1 		; KILL
        sta deadf
a00206: lda #0
        cmp varg
        beq *+5
        jmp a01482
        lda #0
        cmp varl
        beq *+5
        jmp a00896
        lda joyval	; KEY
        and #1
        beq :+
        jmp a00459
:
        lda #1
        cmp vare
        beq *+5
        jmp a00318
        lda #0
        sta vare
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
a00318: lda #218
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00459
        lda #0
        cmp vark
        beq *+5
        jmp a00459
        ldy #9
        lda (z80_ix),y
        sta vara
        lda #30
        sta z80_c
        lda vara
        clc
        adc z80_c
        sta vara
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1
        sta vard
        lda #1
        sta vark
        lda #1
        ldy #6
        sta (z80_ix),y
a00459: lda joyval	; KEY
        and #2
        beq :+
        jmp a00679
:
        lda #1
        cmp vare
        beq *+5
        jmp a00538
        lda #0
        sta vare
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
a00538: lda #30
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00679
        lda #0
        cmp vark
        beq *+5
        jmp a00679
        ldy #9
        lda (z80_ix),y
        sta vara
        lda #36
        sta z80_c
        lda vara
        sec
        sbc z80_c
        sta vara
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #2
        sta vard
        lda #1
        sta vark
        lda #3
        ldy #6
        sta (z80_ix),y
a00679: lda joyval	; KEY
        and #4
        beq :+
        jmp a00787
:
        lda #0
        cmp vark
        beq *+5
        jmp a00787
        lda #0
        cmp vare
        beq *+5
        jmp a00787
        lda #1
        sta vare
        lda #4
        ldy #6
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
a00787: lda joyval	; KEY
        and #8
        beq :+
        jmp a00896
:
        lda #0
        cmp vark
        beq *+5
        jmp a00896
        lda #1
        cmp vare
        beq *+5
        jmp a00896
        lda #0
        sta vare
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
a00896: lda #1
        cmp vard
        beq *+5
        jmp a01036
        ldy #9
        lda (z80_ix),y
        cmp vara
        bcc *+5
        jmp a01007
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda vara
        clc
        adc #1
        sta vara
        jmp a01036
a01007: lda #0
        sta vard
        lda #0
        sta vark
        lda #0
        ldy #6
        sta (z80_ix),y
a01036: lda joyval	; KEY
        and #16
        beq :+
        jmp a01235
:
        lda #0
        cmp varb
        beq *+5
        jmp a01145
        lda #DEADLY	; DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01145
:
        lda #40
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #10
        sta varb
a01145: lda #0
        cmp varb
        beq *+5
        jmp a01235
        lda #CUSTOM	; CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01235
:
        lda #40
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #10
        sta varb
a01235: lda #2
        cmp vard
        beq *+5
        jmp a01359
        ldy #9
        lda (z80_ix),y
        cmp vara
        bcs *+5
        jmp a01330
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp a01359
a01330: lda #0
        sta vard
        lda #0
        sta vark
        lda #2
        ldy #6
        sta (z80_ix),y
a01359: lda #0
        cmp vard
        beq *+5
        jmp a01482
        lda #0
        cmp vare
        beq *+5
        jmp a01482
        jsr cangd	; CANGODOWN
        beq :+
        jmp a01474
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
        lda #1
        sta varl
        jmp a01482
a01474: lda #0
        sta varl
a01482: lda #1
        cmp varg
        beq *+5
        jmp a01639
        lda #114
        cmp varf
        bcc *+5
        jmp a01538
        jsr cls		; CLS
        lda #1	; NEXTLEVEL
        sta nexlev
        jmp a01639
a01538: lda #11
        sta chary
        lda #10
        sta charx
        lda #2  	; MESSAGE
        jsr dmsg
        lda #4 	; REPEAT
        sta loopa
a01576: dec loopa	; ENDREPEAT
        beq :+
        jmp a01576
:
        lda #100 	; DELAY
        jsr delay
        lda #0
        sta varg
        lda #0
        sta vark
        lda #0
        sta vard
        lda #1	; NEXTLEVEL
        sta nexlev
a01639: rts
evnt01:
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00035
:
        lda #2
        sta varg
b00035: jsr cangr	; CANGORIGHT
        beq :+
        jmp b00244
:
        lda #5
        cmp scno
        beq *+5
        jmp b00068
b00068: lda #6
        cmp scno
        beq *+5
        jmp b00085
b00085: lda #7
        cmp scno
        beq *+5
        jmp b00102
b00102: lda #9
        cmp scno
        bcc *+5
        jmp b00118
b00118: ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #5
        cmp scno
        beq *+5
        jmp b00161
b00161: lda #6
        cmp scno
        beq *+5
        jmp b00178
b00178: lda #7
        cmp scno
        beq *+5
        jmp b00195
b00195: lda #9
        cmp scno
        bcc *+5
        jmp b00211
b00211: lda #0
        cmp varc
        beq *+5
        jmp b00239
        lda #0		; ANIMATE
        jsr animsp
b00239: jmp b00271
b00244: lda #0
        ldy #9
        sta (z80_ix),y
        lda #4		; BEEP
        asl a
        sta sndtyp
b00271: rts
evnt02:
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c00035
:
        lda #2
        sta varg
c00035: jsr cangr	; CANGORIGHT
        beq :+
        jmp c00136
:
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp c00131
        lda #0		; ANIMATE
        jsr animsp
c00131: jmp c00163
c00136: lda #0
        ldy #9
        sta (z80_ix),y
        lda #4		; BEEP
        asl a
        sta sndtyp
c00163: rts
evnt03:
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00035
:
        lda #2
        sta varg
d00035: jsr cangl	; CANGOLEFT
        beq :+
        jmp d00110
:
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp d00105
        lda #0		; ANIMATE
        jsr animsp
d00105: jmp d00137
d00110: lda #4		; BEEP
        asl a
        sta sndtyp
        lda #240
        ldy #9
        sta (z80_ix),y
d00137: rts
evnt04:
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00035
:
        lda #2
        sta varg
e00035: lda #20
        cmp scno
        beq *+5
        jmp e00084
        lda #0
        cmp varc
        beq *+5
        jmp e00080
        lda #0		; ANIMATE
        jsr animsp
e00080: jmp e00213
e00084: jsr cangl	; CANGOLEFT
        beq :+
        jmp e00185
:
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp e00180
        lda #0		; ANIMATE
        jsr animsp
e00180: jmp e00213
e00185: lda #4		; BEEP
        asl a
        sta sndtyp
        lda #240
        ldy #9
        sta (z80_ix),y
e00213: rts
evnt05:
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00035
:
        lda #2
        sta varg
f00035: lda #150
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp f00116
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp f00111
        lda #0		; ANIMATE
        jsr animsp
f00111: jmp f00217
f00116: lda #0
        ldy #8
        sta (z80_ix),y
        lda #160
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        ldy #9
        sta (z80_ix),y
        lda #60
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp f00217
f00217: rts
evnt06:
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00035
:
        lda #2
        sta varg
g00035: lda #0
        cmp varc
        beq *+5
        jmp g00123
        lda #0		; ANIMATE
        jsr animsp
        jsr cangr	; CANGORIGHT
        beq :+
        jmp g00110
:
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp g00123
g00110: lda #0
        ldy #9
        sta (z80_ix),y
g00123: rts
evnt07:
        lda #0
        cmp varc
        beq *+5
        jmp h00030
        lda #0		; ANIMATE
        jsr animsp
h00030: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00081
:
        lda #4	; EXPLODE
        jsr explod
        lda varf
        clc
        adc #1
        sta varf
h00081: lda #114
        cmp varf
        beq *+5
        jmp h00129
        jsr cls		; CLS
        lda #250
        sta varb
        lda #115
        sta varf
        lda #1
        sta varg
h00129: lda #112
        cmp varf
        beq *+5
        jmp h00187
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #113
        sta varf
        lda #1
        sta varg
        lda #20
        sta varh
h00187: lda #111
        cmp varf
        beq *+5
        jmp h00218
        lda #136
        ldy #8
        sta (z80_ix),y
h00218: lda #110
        cmp varf
        beq *+5
        jmp h00249
        lda #54
        ldy #8
        sta (z80_ix),y
h00249: lda #109
        cmp varf
        beq *+5
        jmp h00279
        lda #16
        ldy #8
        sta (z80_ix),y
h00279: lda #108
        cmp varf
        beq *+5
        jmp h00324
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
h00324: lda #106
        cmp varf
        beq *+5
        jmp h00382
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #107
        sta varf
        lda #1
        sta varg
        lda #19
        sta varh
h00382: lda #105
        cmp varf
        beq *+5
        jmp h00426
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h00426: lda #104
        cmp varf
        beq *+5
        jmp h00471
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #134
        ldy #8
        sta (z80_ix),y
h00471: lda #103
        cmp varf
        beq *+5
        jmp h00515
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
h00515: lda #102
        cmp varf
        beq *+5
        jmp h00560
        lda #180
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
h00560: lda #101
        cmp varf
        beq *+5
        jmp h00604
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h00604: lda #99
        cmp varf
        beq *+5
        jmp h00662
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #100
        sta varf
        lda #1
        sta varg
        lda #18
        sta varh
h00662: lda #98
        cmp varf
        beq *+5
        jmp h00692
        lda #54
        ldy #8
        sta (z80_ix),y
h00692: lda #97
        cmp varf
        beq *+5
        jmp h00723
        lda #20
        ldy #9
        sta (z80_ix),y
h00723: lda #96
        cmp varf
        beq *+5
        jmp h00754
        lda #134
        ldy #8
        sta (z80_ix),y
h00754: lda #95
        cmp varf
        beq *+5
        jmp h00784
        lda #16
        ldy #8
        sta (z80_ix),y
h00784: lda #93
        cmp varf
        beq *+5
        jmp h00842
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #94
        sta varf
        lda #1
        sta varg
        lda #17
        sta varh
h00842: lda #92
        cmp varf
        beq *+5
        jmp h00886
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h00886: lda #91
        cmp varf
        beq *+5
        jmp h00930
        lda #180
        ldy #9
        sta (z80_ix),y
        lda #134
        ldy #8
        sta (z80_ix),y
h00930: lda #90
        cmp varf
        beq *+5
        jmp h00974
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h00974: lda #89
        cmp varf
        beq *+5
        jmp h01005
        lda #180
        ldy #9
        sta (z80_ix),y
h01005: lda #87
        cmp varf
        beq *+5
        jmp h01063
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #88
        sta varf
        lda #1
        sta varg
        lda #16
        sta varh
h01063: lda #86
        cmp varf
        beq *+5
        jmp h01107
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
h01107: lda #85
        cmp varf
        beq *+5
        jmp h01151
        lda #100
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
h01151: lda #84
        cmp varf
        beq *+5
        jmp h01195
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h01195: lda #83
        cmp varf
        beq *+5
        jmp h01239
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
h01239: lda #82
        cmp varf
        beq *+5
        jmp h01270
        lda #16
        ldy #8
        sta (z80_ix),y
h01270: lda #80
        cmp varf
        beq *+5
        jmp h01327
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #81
        sta varf
        lda #1
        sta varg
        lda #15
        sta varh
h01327: lda #79
        cmp varf
        beq *+5
        jmp h01372
        lda #180
        ldy #9
        sta (z80_ix),y
        lda #134
        ldy #8
        sta (z80_ix),y
h01372: lda #78
        cmp varf
        beq *+5
        jmp h01416
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h01416: lda #77
        cmp varf
        beq *+5
        jmp h01460
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #134
        ldy #8
        sta (z80_ix),y
h01460: lda #76
        cmp varf
        beq *+5
        jmp h01491
        lda #220
        ldy #9
        sta (z80_ix),y
h01491: lda #75
        cmp varf
        beq *+5
        jmp h01535
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
h01535: lda #73
        cmp varf
        beq *+5
        jmp h01593
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #74
        sta varf
        lda #1
        sta varg
        lda #14
        sta varh
h01593: lda #72
        cmp varf
        beq *+5
        jmp h01623
        lda #16
        ldy #8
        sta (z80_ix),y
h01623: lda #71
        cmp varf
        beq *+5
        jmp h01667
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
h01667: lda #70
        cmp varf
        beq *+5
        jmp h01698
        lda #54
        ldy #8
        sta (z80_ix),y
h01698: lda #69
        cmp varf
        beq *+5
        jmp h01742
        lda #180
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
h01742: lda #67
        cmp varf
        beq *+5
        jmp h01800
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #68
        sta varf
        lda #1
        sta varg
        lda #13
        sta varh
h01800: lda #66
        cmp varf
        beq *+5
        jmp h01844
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h01844: lda #65
        cmp varf
        beq *+5
        jmp h01888
        lda #140
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
h01888: lda #64
        cmp varf
        beq *+5
        jmp h01932
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
h01932: lda #63
        cmp varf
        beq *+5
        jmp h01976
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
h01976: lda #61
        cmp varf
        beq *+5
        jmp h02034
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #62
        sta varf
        lda #1
        sta varg
        lda #12
        sta varh
h02034: lda #60
        cmp varf
        beq *+5
        jmp h02078
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h02078: lda #59
        cmp varf
        beq *+5
        jmp h02122
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
h02122: lda #58
        cmp varf
        beq *+5
        jmp h02152
        lda #16
        ldy #8
        sta (z80_ix),y
h02152: lda #57
        cmp varf
        beq *+5
        jmp h02196
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
h02196: lda #56
        cmp varf
        beq *+5
        jmp h02227
        lda #16
        ldy #8
        sta (z80_ix),y
h02227: lda #54
        cmp varf
        beq *+5
        jmp h02284
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #1
        sta varg
        lda #55
        sta varf
        lda #11
        sta varh
h02284: lda #53
        cmp varf
        beq *+5
        jmp h02328
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h02328: lda #52
        cmp varf
        beq *+5
        jmp h02373
        lda #100
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
h02373: lda #51
        cmp varf
        beq *+5
        jmp h02403
        lda #16
        ldy #8
        sta (z80_ix),y
h02403: lda #50
        cmp varf
        beq *+5
        jmp h02447
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
h02447: lda #49
        cmp varf
        beq *+5
        jmp h02492
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
h02492: rts
evnt08:
        lda #0
        cmp varc
        beq *+5
        jmp i00030
        lda #0		; ANIMATE
        jsr animsp
i00030: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00081
:
        lda #4	; EXPLODE
        jsr explod
        lda varf
        clc
        adc #1
        sta varf
i00081: lda #48
        cmp varf
        beq *+5
        jmp i00138
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #49
        sta varf
        lda #1
        sta varg
        lda #10
        sta varh
i00138: lda #47
        cmp varf
        beq *+5
        jmp i00183
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
i00183: lda #46
        cmp varf
        beq *+5
        jmp i00227
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i00227: lda #45
        cmp varf
        beq *+5
        jmp i00271
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
i00271: lda #44
        cmp varf
        beq *+5
        jmp i00315
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i00315: lda #42
        cmp varf
        beq *+5
        jmp i00372
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #43
        sta varf
        lda #1
        sta varg
        lda #9
        sta varh
i00372: lda #41
        cmp varf
        beq *+5
        jmp i00416
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
i00416: lda #40
        cmp varf
        beq *+5
        jmp i00461
        lda #100
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
i00461: lda #39
        cmp varf
        beq *+5
        jmp i00505
        lda #140
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i00505: lda #38
        cmp varf
        beq *+5
        jmp i00536
        lda #20
        ldy #9
        sta (z80_ix),y
i00536: lda #36
        cmp varf
        beq *+5
        jmp i00593
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #37
        sta varf
        lda #1
        sta varg
        lda #8
        sta varh
i00593: lda #35
        cmp varf
        beq *+5
        jmp i00637
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i00637: lda #34
        cmp varf
        beq *+5
        jmp i00668
        lda #60
        ldy #9
        sta (z80_ix),y
i00668: lda #33
        cmp varf
        beq *+5
        jmp i00712
        lda #140
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
i00712: lda #32
        cmp varf
        beq *+5
        jmp i00756
        lda #100
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
i00756: lda #30
        cmp varf
        beq *+5
        jmp i00814
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #31
        sta varf
        lda #1
        sta varg
        lda #7
        sta varh
i00814: lda #29
        cmp varf
        beq *+5
        jmp i00858
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i00858: lda #28
        cmp varf
        beq *+5
        jmp i00902
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
i00902: lda #27
        cmp varf
        beq *+5
        jmp i00946
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
i00946: lda #26
        cmp varf
        beq *+5
        jmp i00990
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i00990: lda #24
        cmp varf
        beq *+5
        jmp i01048
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #25
        sta varf
        lda #1
        sta varg
        lda #6
        sta varh
i01048: lda #23
        cmp varf
        beq *+5
        jmp i01092
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i01092: lda #22
        cmp varf
        beq *+5
        jmp i01136
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
i01136: lda #21
        cmp varf
        beq *+5
        jmp i01180
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
i01180: lda #19
        cmp varf
        beq *+5
        jmp i01237
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #20
        sta varf
        lda #1
        sta varg
        lda #5
        sta varh
i01237: lda #18
        cmp varf
        beq *+5
        jmp i01281
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
i01281: lda #17
        cmp varf
        beq *+5
        jmp i01326
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i01326: lda #1
        cmp varf
        beq *+5
        jmp i01369
        lda #60
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
i01369: lda #2
        cmp varf
        beq *+5
        jmp i01400
        lda #220
        ldy #9
        sta (z80_ix),y
i01400: lda #3
        cmp varf
        beq *+5
        jmp i01444
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #16
        ldy #8
        sta (z80_ix),y
i01444: lda #4
        cmp varf
        beq *+5
        jmp i01500
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #1
        sta varg
        lda #5
        sta varf
        lda #2
        sta varh
i01500: lda #6
        cmp varf
        beq *+5
        jmp i01545
        lda #100
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
i01545: lda #7
        cmp varf
        beq *+5
        jmp i01575
        lda #220
        ldy #9
        sta (z80_ix),y
i01575: lda #8
        cmp varf
        beq *+5
        jmp i01605
        lda #20
        ldy #9
        sta (z80_ix),y
i01605: lda #9
        cmp varf
        beq *+5
        jmp i01662
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #10
        sta varf
        lda #1
        sta varg
        lda #3
        sta varh
i01662: lda #11
        cmp varf
        beq *+5
        jmp i01706
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #54
        ldy #8
        sta (z80_ix),y
i01706: lda #12
        cmp varf
        beq *+5
        jmp i01751
        lda #180
        ldy #9
        sta (z80_ix),y
        lda #92
        ldy #8
        sta (z80_ix),y
i01751: lda #13
        cmp varf
        beq *+5
        jmp i01795
        lda #20
        ldy #9
        sta (z80_ix),y
        lda #136
        ldy #8
        sta (z80_ix),y
i01795: lda #14
        cmp varf
        beq *+5
        jmp i01852
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #15
        sta varf
        lda #1
        sta varg
        lda #4
        sta varh
i01852: lda #16
        cmp varf
        beq *+5
        jmp i01883
        lda #20
        ldy #9
        sta (z80_ix),y
i01883: rts
evnt09:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00037
        lda #20
        ldy #9
        sta (z80_ix),y
j00037: lda #1
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00094
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
j00094: lda #2
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00151
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
j00151: lda #3
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00207
        lda #2
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
j00207: lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00264
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
j00264: rts
evnt10:
        lda #0
        sta chary
        lda #7
        sta charx
        lda varh	; DISPLAY
        jsr disply
        lda #0
        sta chary
        lda #30
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        rts
evnt11:
        lda #2
        cmp varc
        beq *+5
        jmp l00031
        lda #0
        sta varc
        jmp l00046
l00031: lda varc
        clc
        adc #1
        sta varc
l00046: lda #0
        cmp varb
        beq *+5
        jmp l00068
        jmp l00083
l00068: lda varb
        sec
        sbc #1
        sta varb
l00083: lda #20
        cmp scno
        beq *+5
        jmp l00262
        lda #10
        sta chary
        lda #5
        sta charx
        lda #11  	; MESSAGE
        jsr dmsg
        lda #12
        sta chary
        lda #10
        sta charx
        lda #12  	; MESSAGE
        jsr dmsg
        lda #3
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        clc
        adc #1
        sta varrnd
        lda #7
        sta chary
        lda #9
        sta charx
        lda #10  	; MESSAGE
        jsr dmsg
        lda #0
        cmp varb
        beq *+5
        jmp l00262
        lda #1		; ENDGAME
        sta gamwon
l00262: rts
evnt12:
        jsr cls		; CLS
        lda #3
        sta chary
        lda #11
        sta charx
        lda #1  	; MESSAGE
        jsr dmsg
        lda #5
        sta chary
        lda #8
        sta charx
        lda #5  	; MESSAGE
        jsr dmsg
        lda #8
        sta chary
        lda #4
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #9
        sta chary
        lda #4
        sta charx
        lda #7  	; MESSAGE
        jsr dmsg
        lda #10
        sta chary
        lda #4
        sta charx
        lda #8  	; MESSAGE
        jsr dmsg
        lda #19
        sta chary
        lda #5
        sta charx
        lda #9  	; MESSAGE
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
        rts
evnt13:
        lda #3
        sta numlif
        jsr cls		; CLS
        lda #24
        sta vara
        lda #0
        sta varb
        lda #0
        sta varc
        lda #0
        sta vard
        lda #0
        sta vare
        lda #0
        sta varf
        lda #0
        sta varg
        lda #1
        sta varh
        lda #0
        sta vari
        lda #0
        sta varj
        lda #0
        sta vark
        lda #0
        sta varl
        lda #0
        sta varm
        rts
evnt14:
        jsr cls		; CLS
        lda #0
        sta vara
        lda #0
        sta varg
        lda #0
        sta vard
        lda #0
        sta varj
        lda #0
        sta vark
        lda #114
        cmp varf
        beq *+5
        jmp o00069
        jmp o00152
o00069: lda #0
        sta chary
        lda #1
        sta charx
        lda #3  	; MESSAGE
        jsr dmsg
        lda #0
        sta chary
        lda #12
        sta charx
        lda #1  	; MESSAGE
        jsr dmsg
        lda #0
        sta chary
        lda #24
        sta charx
        lda #4  	; MESSAGE
        jsr dmsg
o00152: rts
evnt15:
        lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp p00034
        lda #1 		; KILL
        sta deadf
p00034: rts
evnt16:
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varg
        jsr cls		; CLS
        rts
evnt17:
        lda #10
        sta chary
        lda #12
        sta charx
        jsr cls		; CLS
        lda #0  	; MESSAGE
        jsr dmsg
        lda #12
        sta chary
        lda #4
        sta charx
        lda #13  	; MESSAGE
        jsr dmsg
        jsr prskey	; WAITKEY
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
        .byte "GAME OVE",210
        .byte "BALDY Z",216
        .byte "-NEXT LEVEL",173
        .byte "LEVEL",186
        .byte "LIVES",186
        .byte "SELECT CONTRO",204
        .byte "1.............KEYBOAR",196
        .byte "2....KEMPSTON JOYSTIC",203
        .byte "3....SINCLAIR JOYSTIC",203
        .byte ";PAUL JENKINSON 201",181
        .byte "CONGRATULATION",211
        .byte "BALDY HAS COLLECTED AL",204
        .byte "OF HIS GAME",211
        .byte "BALDY IS IN NEED OF WINE",161
nummsg:
        .byte 14
chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,175,191,0,183,181,0,91
        .byte 0,253,189,0,211,219,0,246
        .byte 87,0,23,23,0,0,0,0
        .byte 250,0,212,212,0,0,0,0
        .byte 247,0,239,239,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,170
        .byte 0,0,0,0,0,0,0,170
        .byte 0,255,165,231,165,231,165,255
        .byte 0,223,223,181,191,177,191,181
        .byte 0,255,251,245,245,59,223,223
        .byte 0,127,49,31,0,0,0,0
        .byte 0,253,195,223,0,0,0,0
        .byte 0,222,140,136,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,223,193,253,165,228,167,255
        .byte 0,0,0,0,0,0,0,0
        .byte 0,239,235,0,253,237,0,191
        .byte 0,125,187,188,81,43,21,29
        .byte 0,246,181,111,143,70,216,181
        .byte 0,242,85,47,15,70,240,184
        .byte 8,3,2,1,0,0,0,0
        .byte 186,48,0,0,0,0,0,0
        .byte 240,96,0,0,0,0,0,0
        .byte 0,247,111,109,143,70,216,181
        .byte 0,222,218,76,49,61,24,0
        .byte 0,247,215,99,156,239,198,56
        .byte 0,223,93,142,112,118,44,0
        .byte 60,90,90,126,44,0,52,24
        .byte 0,0,0,0,0,0,0,170
        .byte 0,213,122,213,171,101,188,104
        .byte 0,106,191,117,174,85,170,5
        .byte 0,174,117,174,117,170,85,160
        .byte 14,237,167,216,21,10,1,0
        .byte 113,119,165,90,21,170,145,0
        .byte 6,234,164,90,52,168,0,0
        .byte 0,255,255,0,118,118,6,22
        .byte 0,255,255,0,222,222,0,123
        .byte 0,255,255,0,222,222,0,246
        .byte 6,0,0,0,0,0,0,0
        .byte 123,120,120,0,0,0,0,0
        .byte 246,6,0,0,0,0,0,0
        .byte 0,222,222,222,0,189,189,189
        .byte 0,247,247,247,0,253,253,253
        .byte 0,247,247,247,0,221,221,221
        .byte 0,111,165,66,24,53,25,3
        .byte 0,111,165,66,24,53,153,195
        .byte 0,111,165,66,24,52,152,192
        .byte 0,111,181,254,54,164,4,32
        .byte 0,189,189,89,32,14,23,11
        .byte 0,173,239,231,200,158,54,92
        .byte 0,215,222,140,32,92,116,120
        .byte 0,255,255,169,175,169,255,255
        .byte 0,255,215,131,131,131,215,255
        .byte 0,255,165,191,161,161,191,255
        .byte 6,214,209,44,0,0,0,0
        .byte 208,214,38,1,0,0,0,0
        .byte 218,218,4,64,0,0,0,0
        .byte 0,255,0,85,74,85,74,85
        .byte 0,255,0,61,188,65,170,85
        .byte 0,255,0,82,170,82,170,82
        .byte 0,255,0,0,0,0,0,0
        .byte 68,32,68,64,0,0,0,0
        .byte 40,4,36,0,4,0,0,0
        .byte 0,255,0,111,111,55,27,13
        .byte 0,255,0,232,244,232,244,232
        .byte 0,255,0,94,190,92,188,88
        .byte 255,152,170,168,170,154,255,0
        .byte 255,136,171,170,170,168,255,0
        .byte 255,147,181,147,181,149,255,0
        .byte 0,8,0,8,42,8,42,42
        .byte 0,246,246,6,174,174,32,0
        .byte 0,221,221,0,247,247,3,0
        .byte 0,111,111,99,11,120,123,3
        .byte 96,147,148,100,3,8,20,8
        .byte 24,36,165,153,64,164,74,4
        .byte 6,201,41,38,200,20,8,0
        .byte 0,255,0,183,170,157,174,4
        .byte 0,255,0,119,187,85,238,68
        .byte 0,255,0,117,169,93,237,68
        .byte 0,0,16,25,141,77,38,22
        .byte 4,128,148,133,147,214,206,220
        .byte 8,0,16,50,36,101,234,212
        .byte 0,218,85,109,42,41,68,4
        .byte 0,154,81,77,21,84,4,16
        .byte 0,255,0,102,102,102,0,255
        .byte 0,255,0,126,126,126,0,255
        .byte 0,133,86,187,237,183,93,10
        .byte 0,162,213,106,221,115,88,160
        .byte 6,213,108,8,0,0,0,0
        .byte 208,106,181,40,8,0,0,0
        .byte 94,157,94,77,86,89,92,94
        .byte 30,14,14,38,26,45,30,46
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 6
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 0
sprgfx:
        .byte 15,192,30,160,3,96,7,224,30,32,15,192,0,0,30,208,53,224,43,232,29,220,24,8,37,224,88,192,60,184,116,116
        .byte 3,240,7,168,0,216,1,248,7,136,3,240,0,0,7,180,13,120,10,250,7,119,6,2,9,120,22,48,15,46,29,29
        .byte 0,252,1,234,0,54,0,126,1,226,0,252,0,0,1,237,3,94,130,190,193,221,129,128,2,94,5,140,131,203,71,71
        .byte 0,63,128,122,128,13,128,31,128,120,0,63,0,0,64,123,128,215,160,175,112,119,32,96,128,151,1,99,224,242,209,209
        .byte 15,192,30,160,3,96,7,224,30,32,15,194,0,7,62,218,93,236,235,224,65,192,0,48,237,251,232,119,168,10,192,28
        .byte 3,240,7,168,0,216,1,248,7,136,131,240,192,1,143,182,23,123,58,248,16,112,0,12,251,126,250,29,170,2,48,7
        .byte 0,252,1,234,0,54,0,126,1,226,32,252,112,0,163,237,197,222,14,190,4,28,0,3,190,223,126,135,170,128,204,1
        .byte 0,63,128,122,128,13,128,31,128,120,8,63,28,0,104,251,177,119,131,175,1,7,192,0,239,183,223,161,42,160,115,0
        .byte 3,240,5,120,6,192,7,224,4,120,3,240,0,0,11,120,7,172,23,212,59,184,16,24,7,164,3,26,29,60,46,46
        .byte 0,252,1,94,1,176,1,248,1,30,0,252,0,0,2,222,1,235,5,245,14,238,4,6,1,233,128,198,7,79,139,139
        .byte 0,63,128,87,0,108,0,126,128,71,0,63,0,0,128,183,192,122,65,125,131,187,129,1,64,122,160,49,193,211,226,226
        .byte 192,15,224,21,0,27,128,31,224,17,192,15,0,0,224,45,176,30,80,95,224,238,96,64,144,30,104,12,240,116,184,184
        .byte 3,240,5,120,6,192,7,224,4,120,67,240,224,0,91,124,55,186,7,215,3,130,12,0,223,183,238,23,80,21,56,3
        .byte 0,252,1,94,1,176,1,248,1,30,16,252,56,0,22,223,141,238,193,245,128,224,3,0,247,237,251,133,84,5,206,0
        .byte 0,63,128,87,0,108,0,126,128,71,4,63,14,0,197,183,163,123,112,125,32,56,0,192,125,251,126,225,85,1,51,128
        .byte 192,15,224,21,0,27,128,31,224,17,193,15,3,128,241,109,232,222,92,31,8,14,0,48,223,126,95,184,85,64,12,224
        .byte 5,160,15,240,14,112,15,240,183,237,219,219,76,50,218,91,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 1,104,3,252,3,156,3,252,109,251,246,246,147,12,246,150,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,90,0,255,0,231,0,255,219,126,189,189,36,195,189,165,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 128,22,192,63,192,57,192,63,182,223,111,111,201,48,111,105,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,8,2,255,253,8,2,0,4
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,130,0,127,255,130,0,0,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,32,128,223,255,32,128,64,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,8,32,247,255,8,32,16,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,6,15,253,248,6,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,190,1,67,255,190,1,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,111,128,208,255,111,128,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,224,244,63,27,224,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,240,0,8,0,15,255,8,0,240,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60,0,2,0,195,255,2,0,60,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,0,0,128,240,255,0,128,15,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,192,0,32,252,63,0,32,3,192
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,248,6,15,253,248,6,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,190,1,67,255,190,1,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,111,128,208,255,111,128,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,27,224,244,63,27,224,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,127,254,231,231,195,195,231,231,255,255,128,1,191,253,182,109,63,252,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,159,255,249,249,240,240,249,249,255,255,96,0,111,255,109,155,15,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,231,255,126,126,60,60,126,126,255,255,24,0,219,255,219,102,195,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,249,255,159,159,15,15,159,159,255,255,6,0,246,255,182,217,240,255,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,29,240,43,88,42,72,43,88,43,248,42,8,42,232,42,168,42,232,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,7,124,10,214,10,146,10,214,10,254,10,130,10,186,10,170,10,186,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,223,130,181,130,164,130,181,130,191,130,160,130,174,130,170,130,174,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,192,119,96,173,32,169,96,173,224,175,32,168,160,171,160,170,160,171,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,128,3,64,3,192,3,192,3,192,0,0,3,192,3,192,3,192,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,96,0,208,0,240,0,240,0,240,0,0,0,240,0,240,0,240,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,24,0,52,0,60,0,60,0,60,0,0,0,60,0,60,0,60,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,13,0,15,0,15,0,15,0,0,0,15,0,15,0,15,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,112,53,168,36,168,53,168,63,168,32,168,46,168,42,168,46,168,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,7,220,13,106,9,42,13,106,15,234,8,42,11,170,10,170,11,170,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,1,247,131,90,130,74,131,90,131,250,130,10,130,234,130,170,130,234,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,192,125,160,214,160,146,160,214,160,254,160,130,160,186,160,170,160,186,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,224,0,177,240,83,88,173,85,83,88,177,240,224,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,44,124,20,214,107,85,20,214,44,124,56,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,11,31,133,53,90,213,133,53,11,31,14,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,194,199,97,77,86,181,97,77,194,199,3,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,224,243,240,86,184,170,170,86,184,243,240,1,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,60,252,21,174,170,170,21,174,60,252,0,120
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,15,63,133,107,170,170,133,107,15,63,0,30
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,7,195,207,225,90,170,170,225,90,195,207,128,7
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,224,3,240,5,120,253,108,5,120,3,240,1,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,0,252,1,94,63,91,1,94,0,252,0,120
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,63,128,87,207,214,128,87,0,63,0,30
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,7,192,15,224,21,179,245,224,21,192,15,128,7
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,241,240,83,88,173,85,83,88,241,240,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60,124,20,214,107,85,20,214,60,124,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,31,133,53,90,213,133,53,15,31,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,195,199,97,77,86,181,97,77,195,199,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,135,32,205,223,186,63,205,31,135
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,199,225,72,51,183,238,79,243,199,225
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,113,248,210,12,173,251,211,252,113,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,126,52,131,235,126,52,255,28,126
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,128,63,207,255,186,63,207,31,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,224,207,243,191,238,207,243,7,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,248,243,252,175,251,243,252,1,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,126,60,255,235,254,60,255,0,126
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,128,63,192,223,191,32,192,31,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,224,15,240,247,239,8,48,7,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,248,3,252,253,251,2,12,1,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,126,0,255,255,126,0,131,0,126
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,128,63,207,192,186,63,207,31,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,224,207,243,176,46,207,243,7,224
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,248,243,252,172,11,243,252,1,248
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,126,60,255,235,2,60,255,0,126
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,6,79,248,32,6
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,136,1,19,254,136,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,98,0,132,255,98,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,128,225,63,24,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,111,240,0,12
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,27,252,0,3
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,192,0,6,255,192,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,48,0,193,191,48,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,24,111,224,0,24
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,27,248,0,6
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,1,6,254,128,1
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,96,0,129,191,96,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,12,79,240,32,12
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,3,19,252,8,3
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,194,0,4,255,194,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,48,128,193,63,48,128
        .byte 3,128,29,112,114,156,110,236,64,4,9,0,29,0,45,64,125,0,0,64,15,128,22,0,46,0,14,0,0,0,27,0
        .byte 0,224,7,92,28,167,27,187,16,1,2,64,7,64,11,80,31,64,0,16,3,224,5,128,11,128,3,128,0,0,6,192
        .byte 0,56,1,215,199,41,198,238,68,0,0,144,1,208,2,212,7,208,0,4,0,248,1,96,2,224,0,224,0,0,1,176
        .byte 0,14,192,117,113,202,177,187,17,0,0,36,0,116,0,181,1,244,0,1,0,62,0,88,0,184,0,56,0,0,0,108
        .byte 3,128,29,112,114,156,110,236,64,4,9,0,29,0,45,64,125,0,0,64,15,128,22,0,39,0,3,0,0,64,6,128
        .byte 0,224,7,92,28,167,27,187,16,1,2,64,7,64,11,80,31,64,0,16,3,224,5,128,9,192,0,192,0,16,1,160
        .byte 0,56,1,215,199,41,198,238,68,0,0,144,1,208,2,212,7,208,0,4,0,248,1,96,2,112,0,48,0,4,0,104
        .byte 0,14,192,117,113,202,177,187,17,0,0,36,0,116,0,181,1,244,0,1,0,62,0,88,0,156,0,12,0,1,0,26
        .byte 3,128,29,112,114,156,110,236,64,4,9,0,29,0,45,64,125,0,0,64,15,128,22,0,46,0,14,0,0,0,27,0
        .byte 0,224,7,92,28,167,27,187,16,1,2,64,7,64,11,80,31,64,0,16,3,224,5,128,11,128,3,128,0,0,6,192
        .byte 0,56,1,215,199,41,198,238,68,0,0,144,1,208,2,212,7,208,0,4,0,248,1,96,2,224,0,224,0,0,1,176
        .byte 0,14,192,117,113,202,177,187,17,0,0,36,0,116,0,181,1,244,0,1,0,62,0,88,0,184,0,56,0,0,0,108
        .byte 3,128,29,112,114,156,110,236,64,4,9,0,29,0,45,64,125,0,0,64,15,128,54,0,12,0,24,0,64,0,44,0
        .byte 0,224,7,92,28,167,27,187,16,1,2,64,7,64,11,80,31,64,0,16,3,224,13,128,3,0,6,0,16,0,11,0
        .byte 0,56,1,215,199,41,198,238,68,0,0,144,1,208,2,212,7,208,0,4,0,248,3,96,0,192,1,128,4,0,2,192
        .byte 0,14,192,117,113,202,177,187,17,0,0,36,0,116,0,181,1,244,0,1,0,62,0,216,0,48,0,96,1,0,0,176
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,116,109,174,0,0,222,219,0,0,122,238,45,188
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,221,155,107,0,0,247,182,0,0,158,187,11,111
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,67,119,230,218,0,0,189,237,0,0,231,174,194,219
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,221,185,182,0,0,111,123,0,0,185,235,240,182
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,116,109,174,0,0,183,119,222,219,0,0,122,238,45,188
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,221,155,107,0,0,237,221,247,182,0,0,158,187,11,111
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,67,119,230,218,0,0,123,119,189,237,0,0,231,174,194,219
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,221,185,182,0,0,222,221,111,123,0,0,185,235,240,182
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,116,109,174,0,0,183,119,222,219,0,0,122,238,45,188
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,221,155,107,0,0,237,221,247,182,0,0,158,187,11,111
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,67,119,230,218,0,0,123,119,189,237,0,0,231,174,194,219
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,221,185,182,0,0,222,221,111,123,0,0,185,235,240,182
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,55,116,109,174,0,0,222,219,0,0,122,238,45,188
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,221,155,107,0,0,247,182,0,0,158,187,11,111
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,67,119,230,218,0,0,189,237,0,0,231,174,194,219
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,221,185,182,0,0,111,123,0,0,185,235,240,182
        .byte 3,224,15,248,24,140,48,130,50,164,56,138,63,116,62,42,30,52,15,232,2,160,0,0,4,16,6,176,7,240,3,224
        .byte 0,248,3,254,6,35,140,32,12,169,142,34,15,221,143,138,7,141,3,250,0,168,0,0,1,4,1,172,1,252,0,248
        .byte 0,62,128,255,193,136,35,8,67,42,163,136,67,247,163,226,65,227,128,254,0,42,0,0,0,65,0,107,0,127,0,62
        .byte 128,15,224,63,48,98,8,194,144,202,40,226,208,253,168,248,208,120,160,63,128,10,0,0,64,16,192,26,192,31,128,15
        .byte 3,224,15,248,24,140,49,194,48,132,56,138,63,116,62,42,30,52,15,232,2,160,4,16,6,176,7,240,3,224,0,0
        .byte 0,248,3,254,6,35,140,112,12,33,142,34,15,221,143,138,7,141,3,250,0,168,1,4,1,172,1,252,0,248,0,0
        .byte 0,62,128,255,193,136,35,28,67,8,163,136,67,247,163,226,65,227,128,254,0,42,0,65,0,107,0,127,0,62,0,0
        .byte 128,15,224,63,48,98,8,199,16,194,40,226,208,253,168,248,208,120,160,63,128,10,64,16,192,26,192,31,128,15,0,0
        .byte 3,224,15,248,24,140,52,146,48,132,56,138,63,116,62,42,30,52,15,232,6,176,6,176,7,240,3,224,0,0,0,0
        .byte 0,248,3,254,6,35,141,36,12,33,142,34,15,221,143,138,7,141,3,250,1,172,1,172,1,252,0,248,0,0,0,0
        .byte 0,62,128,255,193,136,35,73,67,8,163,136,67,247,163,226,65,227,128,254,0,107,0,107,0,127,0,62,0,0,0,0
        .byte 128,15,224,63,48,98,72,210,16,194,40,226,208,253,168,248,208,120,160,63,192,26,192,26,192,31,128,15,0,0,0,0
        .byte 3,224,15,248,24,140,48,130,52,148,56,138,63,116,62,42,30,52,15,232,2,160,4,16,6,176,7,240,3,224,0,0
        .byte 0,248,3,254,6,35,140,32,13,37,142,34,15,221,143,138,7,141,3,250,0,168,1,4,1,172,1,252,0,248,0,0
        .byte 0,62,128,255,193,136,35,8,67,73,163,136,67,247,163,226,65,227,128,254,0,42,0,65,0,107,0,127,0,62,0,0
        .byte 128,15,224,63,48,98,8,194,80,210,40,226,208,253,168,248,208,120,160,63,128,10,64,16,192,26,192,31,128,15,0,0
        .byte 0,0,12,0,82,24,33,36,81,78,0,132,0,128,0,0,15,232,15,232,15,232,15,232,15,232,0,0,7,208,0,0
        .byte 0,0,3,0,20,134,8,73,148,83,0,33,0,32,0,0,3,250,3,250,3,250,3,250,3,250,0,0,1,244,0,0
        .byte 0,0,0,192,133,33,66,18,229,20,64,8,0,8,0,0,128,254,128,254,128,254,128,254,128,254,0,0,0,125,0,0
        .byte 0,0,0,48,97,72,144,132,57,69,16,2,0,2,0,0,160,63,160,63,160,63,160,63,160,63,0,0,64,31,0,0
        .byte 0,0,0,0,22,0,9,48,21,120,0,144,0,128,0,0,15,248,15,248,15,248,15,248,15,248,0,0,7,240,0,0
        .byte 0,0,0,0,5,128,2,76,5,94,0,36,0,32,0,0,3,254,3,254,3,254,3,254,3,254,0,0,1,252,0,0
        .byte 0,0,0,0,1,96,0,147,129,87,0,9,0,8,0,0,128,255,128,255,128,255,128,255,128,255,0,0,0,127,0,0
        .byte 0,0,0,0,0,88,192,36,224,85,64,2,0,2,0,0,224,63,224,63,224,63,224,63,224,63,0,0,192,31,0,0
        .byte 0,0,0,0,6,128,3,160,5,96,0,192,0,128,0,0,11,248,11,248,11,248,11,248,11,248,0,0,5,240,0,0
        .byte 0,0,0,0,1,160,0,232,1,88,0,48,0,32,0,0,2,254,2,254,2,254,2,254,2,254,0,0,1,124,0,0
        .byte 0,0,0,0,0,104,0,58,0,86,0,12,0,8,0,0,128,191,128,191,128,191,128,191,128,191,0,0,0,95,0,0
        .byte 0,0,0,0,0,26,128,14,128,21,0,3,0,2,0,0,224,47,224,47,224,47,224,47,224,47,0,0,192,23,0,0
        .byte 0,0,0,0,2,32,13,80,21,120,0,144,0,128,0,0,15,120,15,120,15,120,15,120,15,120,0,0,7,112,0,0
        .byte 0,0,0,0,0,136,3,84,5,94,0,36,0,32,0,0,3,222,3,222,3,222,3,222,3,222,0,0,1,220,0,0
        .byte 0,0,0,0,0,34,0,213,129,87,0,9,0,8,0,0,128,247,128,247,128,247,128,247,128,247,0,0,0,119,0,0
        .byte 0,0,0,0,128,8,64,53,224,85,64,2,0,2,0,0,224,61,224,61,224,61,224,61,224,61,0,0,192,29,0,0
        .byte 120,30,120,30,120,30,123,222,123,222,61,188,30,120,0,0,31,232,0,0,15,208,15,208,15,208,15,208,7,160,3,64
        .byte 158,7,158,7,158,7,158,247,158,247,15,111,7,158,0,0,7,250,0,0,3,244,3,244,3,244,3,244,1,232,0,208
        .byte 231,129,231,129,231,129,231,189,231,189,195,219,129,231,0,0,129,254,0,0,0,253,0,253,0,253,0,253,0,122,0,52
        .byte 121,224,121,224,121,224,121,239,121,239,240,246,224,121,0,0,160,127,0,0,64,63,64,63,64,63,64,63,128,30,0,13
        .byte 30,120,30,120,30,56,30,216,30,216,30,184,14,112,0,0,31,232,0,0,15,208,15,208,15,208,15,208,7,160,3,64
        .byte 7,158,7,158,7,142,7,182,7,182,7,174,3,156,0,0,7,250,0,0,3,244,3,244,3,244,3,244,1,232,0,208
        .byte 129,231,129,231,129,227,129,237,129,237,129,235,0,231,0,0,129,254,0,0,0,253,0,253,0,253,0,253,0,122,0,52
        .byte 224,121,224,121,224,120,96,123,96,123,224,122,192,57,0,0,160,127,0,0,64,63,64,63,64,63,64,63,128,30,0,13
        .byte 3,192,3,192,3,192,3,192,3,192,3,192,3,192,0,0,31,232,0,0,15,208,15,208,15,208,15,208,7,160,3,64
        .byte 0,240,0,240,0,240,0,240,0,240,0,240,0,240,0,0,7,250,0,0,3,244,3,244,3,244,3,244,1,232,0,208
        .byte 0,60,0,60,0,60,0,60,0,60,0,60,0,60,0,0,129,254,0,0,0,253,0,253,0,253,0,253,0,122,0,52
        .byte 0,15,0,15,0,15,0,15,0,15,0,15,0,15,0,0,160,127,0,0,64,63,64,63,64,63,64,63,128,30,0,13
        .byte 30,120,30,120,28,120,27,120,27,120,29,120,14,112,0,0,31,232,0,0,15,208,15,208,15,208,15,208,7,160,3,64
        .byte 7,158,7,158,7,30,6,222,6,222,7,94,3,156,0,0,7,250,0,0,3,244,3,244,3,244,3,244,1,232,0,208
        .byte 129,231,129,231,129,199,129,183,129,183,129,215,0,231,0,0,129,254,0,0,0,253,0,253,0,253,0,253,0,122,0,52
        .byte 224,121,224,121,224,113,224,109,224,109,224,117,192,57,0,0,160,127,0,0,64,63,64,63,64,63,64,63,128,30,0,13
        .byte 16,0,48,248,81,116,83,240,240,0,240,96,157,224,144,96,240,224,113,192,85,128,51,0,18,0,0,0,0,0,0,0
        .byte 4,0,12,62,20,93,20,252,60,0,60,24,39,120,36,24,60,56,28,112,21,96,12,192,4,128,0,0,0,0,0,0
        .byte 1,0,131,15,69,23,5,63,15,0,15,6,9,222,9,6,15,14,7,28,5,88,3,48,1,32,0,0,0,0,0,0
        .byte 0,64,224,195,209,69,193,79,3,192,131,193,130,119,130,65,131,195,1,199,1,86,0,204,0,72,0,0,0,0,0,0
        .byte 16,0,48,244,81,120,83,240,240,0,240,96,157,224,144,96,240,224,113,192,85,128,51,0,18,0,0,0,0,0,0,0
        .byte 4,0,12,61,20,94,20,252,60,0,60,24,39,120,36,24,60,56,28,112,21,96,12,192,4,128,0,0,0,0,0,0
        .byte 1,0,67,15,133,23,5,63,15,0,15,6,9,222,9,6,15,14,7,28,5,88,3,48,1,32,0,0,0,0,0,0
        .byte 0,64,208,195,225,69,193,79,3,192,131,193,130,119,130,65,131,195,1,199,1,86,0,204,0,72,0,0,0,0,0,0
        .byte 16,0,48,240,81,112,83,252,240,0,240,96,157,224,144,96,240,224,113,192,85,128,51,0,18,0,0,0,0,0,0,0
        .byte 4,0,12,60,20,92,20,255,60,0,60,24,39,120,36,24,60,56,28,112,21,96,12,192,4,128,0,0,0,0,0,0
        .byte 1,0,3,15,5,23,197,63,15,0,15,6,9,222,9,6,15,14,7,28,5,88,3,48,1,32,0,0,0,0,0,0
        .byte 0,64,192,195,193,69,241,79,3,192,131,193,130,119,130,65,131,195,1,199,1,86,0,204,0,72,0,0,0,0,0,0
        .byte 16,0,48,240,81,124,83,240,240,0,240,96,157,224,144,96,240,224,113,192,85,128,51,0,18,0,0,0,0,0,0,0
        .byte 4,0,12,60,20,95,20,252,60,0,60,24,39,120,36,24,60,56,28,112,21,96,12,192,4,128,0,0,0,0,0,0
        .byte 1,0,3,15,197,23,5,63,15,0,15,6,9,222,9,6,15,14,7,28,5,88,3,48,1,32,0,0,0,0,0,0
        .byte 0,64,192,195,241,69,193,79,3,192,131,193,130,119,130,65,131,195,1,199,1,86,0,204,0,72,0,0,0,0,0,0
        .byte 7,224,9,176,11,144,14,112,8,16,70,98,225,135,90,90,55,236,7,224,3,192,12,48,223,251,238,119,80,10,56,28
        .byte 1,248,2,108,2,228,3,156,2,4,145,152,248,97,150,150,13,251,1,248,0,240,3,12,247,254,251,157,148,2,14,7
        .byte 0,126,0,155,0,185,0,231,0,129,36,102,126,24,165,165,195,126,0,126,0,60,0,195,189,255,126,231,165,0,195,129
        .byte 128,31,192,38,64,46,192,57,64,32,137,25,31,134,105,105,176,223,128,31,0,15,192,48,239,127,223,185,41,64,112,224
        .byte 7,224,9,144,11,208,14,112,8,16,6,98,1,135,90,90,247,236,75,224,3,195,4,59,15,251,14,119,112,2,62,0
        .byte 1,248,2,100,2,244,3,156,2,4,129,152,192,97,150,150,61,251,18,248,192,240,193,14,195,254,195,157,156,0,15,128
        .byte 0,126,0,153,0,189,0,231,0,129,32,102,112,24,165,165,207,126,4,190,48,60,176,67,176,255,112,231,39,0,3,224
        .byte 128,31,64,38,64,47,192,57,64,32,136,25,28,6,105,105,179,223,129,47,12,15,236,16,236,63,220,57,9,192,0,248
        .byte 7,224,9,176,13,144,15,240,8,16,71,226,225,135,90,90,55,236,7,224,3,192,12,48,223,251,238,119,80,10,56,28
        .byte 1,248,2,108,3,100,3,252,2,4,145,248,248,97,150,150,13,251,1,248,0,240,3,12,247,254,251,157,148,2,14,7
        .byte 0,126,0,155,0,217,0,255,0,129,36,126,126,24,165,165,195,126,0,126,0,60,0,195,189,255,126,231,165,0,195,129
        .byte 128,31,192,38,64,54,192,63,64,32,137,31,31,134,105,105,176,223,128,31,0,15,192,48,239,127,223,185,41,64,112,224
        .byte 7,224,9,144,13,176,15,240,8,16,70,96,225,128,90,90,55,239,7,242,131,192,204,32,223,240,238,118,0,14,0,62
        .byte 1,248,2,100,3,108,3,252,2,4,17,152,56,96,150,150,205,251,129,252,32,240,51,8,55,252,187,157,128,3,128,15
        .byte 0,126,0,153,0,219,0,255,0,129,4,102,14,24,165,165,243,126,32,127,8,60,12,194,13,255,110,231,224,0,224,3
        .byte 128,31,64,38,192,54,192,63,64,32,129,25,3,134,105,105,188,223,200,31,2,15,131,48,195,127,219,185,56,0,248,0
        .byte 0,0,0,0,0,16,1,239,0,16,1,208,2,208,7,216,0,24,3,216,4,24,11,224,0,0,0,216,0,48,0,96
        .byte 0,0,0,0,0,4,192,123,0,4,0,116,0,180,1,246,0,6,0,246,1,6,2,248,0,0,0,54,0,12,0,24
        .byte 0,0,0,0,0,1,240,30,0,1,0,29,0,45,128,125,128,1,128,61,128,65,0,190,0,0,128,13,0,3,0,6
        .byte 0,0,0,0,64,0,188,7,64,0,64,7,64,11,96,31,96,0,96,15,96,16,128,47,0,0,96,3,192,0,128,1
        .byte 0,0,0,0,0,16,0,108,0,16,1,208,2,208,7,216,0,24,3,216,4,24,11,224,0,0,0,216,0,24,0,48
        .byte 0,0,0,0,0,4,0,27,0,4,0,116,0,180,1,246,0,6,0,246,1,6,2,248,0,0,0,54,0,6,0,12
        .byte 0,0,0,0,0,1,192,6,0,1,0,29,0,45,128,125,128,1,128,61,128,65,0,190,0,0,128,13,128,1,0,3
        .byte 0,0,0,0,64,0,176,1,64,0,64,7,64,11,96,31,96,0,96,15,96,16,128,47,0,0,96,3,96,0,192,0
        .byte 0,0,0,0,0,16,0,16,0,16,1,208,2,208,7,216,0,24,3,216,4,24,11,224,0,0,0,216,0,28,0,8
        .byte 0,0,0,0,0,4,0,4,0,4,0,116,0,180,1,246,0,6,0,246,1,6,2,248,0,0,0,54,0,7,0,2
        .byte 0,0,0,0,0,1,0,1,0,1,0,29,0,45,128,125,128,1,128,61,128,65,0,190,0,0,128,13,192,1,128,0
        .byte 0,0,0,0,64,0,64,0,64,0,64,7,64,11,96,31,96,0,96,15,96,16,128,47,0,0,96,3,112,0,32,0
        .byte 0,0,0,0,0,16,0,108,0,16,1,208,2,208,7,216,0,24,3,216,4,24,11,224,0,0,0,216,0,24,0,48
        .byte 0,0,0,0,0,4,0,27,0,4,0,116,0,180,1,246,0,6,0,246,1,6,2,248,0,0,0,54,0,6,0,12
        .byte 0,0,0,0,0,1,192,6,0,1,0,29,0,45,128,125,128,1,128,61,128,65,0,190,0,0,128,13,128,1,0,3
        .byte 0,0,0,0,64,0,176,1,64,0,64,7,64,11,96,31,96,0,96,15,96,16,128,47,0,0,96,3,96,0,192,0
frmlst:
        .byte 0,1
        .byte 1,1
        .byte 2,1
        .byte 3,1
        .byte 4,1
        .byte 5,4
        .byte 9,4
        .byte 13,4
        .byte 17,4
        .byte 21,4
        .byte 25,4
        .byte 29,4
        .byte 33,4
        .byte 37,4
        .byte 41,4
        .byte 45,4
        .byte 49,4
        .byte 53,4,57,0
scdat:
        .word 331,324,312,207,312,324,208,194,207,317,297,279,201,202,181,326,166,147,143,218,132
        .byte 255,0,51,6,6,6,255,0,22,31,8,31,0,0,0,7,255,0,8,1,1,2,0,0,1,1,2,0,0,1,1,2,0,0
        .byte 1,19,2,0,0,1,1,2,0,0,1,1,2,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4
        .byte 0,0,3,5,4,0,0,3,5,4,255,0,42,6,6,6,255,0,17,31,8,31,255,0,8,7,0,0,0,31,9,31,0,0
        .byte 31,8,31,255,0,7,1,19,2,0,0,1,1,2,0,0,1,1,2,0,0,1,19,2,0,0,1,19,2,0,0,1,1
        .byte 2,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,255,0,47
        .byte 6,6,6,255,0,12,31,9,31,255,0,7,31,8,31,0,0,0,7,0,0,0,31,9,31,0,0,31,8,31,0,0,1
        .byte 19,2,0,0,1,1,2,0,0,1,19,2,0,0,1,1,2,0,0,1,19,2,0,0,1,19,2,0,0,3,5,4,0,0
        .byte 3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,255,0,72,31,9,31,255,0,12,31
        .byte 9,31,0,0,1,1,2,0,0,1,1,2,0,0,1,19,2,0,0,1,1,2,0,0,1,1,2,0,0,1,19,2,0,0
        .byte 3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0
        .byte 255,0,71,31,8,31,255,0,7,31,8,31,255,0,7,10,11,12,0,0,10,11,12,0,0,10,17,12,0,0,10,11,12
        .byte 0,0,10,17,12,0,0,10,11,12,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,0,0,13
        .byte 14,15,0,0,13,14,15,255,0,62,31,8,31,255,0,7,31,9,31,0,0,0,16,0,0,0,31,9,31,0,0,31,8
        .byte 31,0,0,10,17,12,0,0,10,11,12,0,0,10,17,12,0,0,10,11,12,0,0,10,17,12,0,0,10,17,12,0,0
        .byte 13,14,15,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,255,0,62,31,9
        .byte 31,0,0,31,8,31,0,0,0,16,255,0,8,31,8,31,0,0,31,9,31,0,0,10,17,12,0,0,10,17,12,0,0
        .byte 10,11,12,0,0,10,11,12,0,0,10,17,12,0,0,10,17,12,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15
        .byte 0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,255,0,67,31,9,31,255,0,8,16,0,0,0,31,9,31,255,0,7
        .byte 10,11,12,0,0,10,17,12,0,0,10,11,12,0,0,10,11,12,0,0,10,17,12,0,0,10,11,12,0,0,13,14,15
        .byte 0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,0,0,13,14,15,0
        .byte 255,0,71,31,8,31,255,0,12,31,8,31,0,0,20,21,22,0,0,20,21,22,0,0,20,26,22,0,0,20,21,22,0,0
        .byte 20,21,22,0,0,20,26,22,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25
        .byte 0,0,23,24,25,255,0,62,31,8,31,255,0,7,31,9,31,255,0,7,31,8,31,0,0,31,9,31,0,0,20,26,22
        .byte 0,0,20,21,22,0,0,20,26,22,0,0,20,21,22,0,0,20,26,22,0,0,20,26,22,0,0,23,24,25,0,0,23
        .byte 24,25,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25,255,0,62,31,9,31,0,0,31,8,31
        .byte 255,0,7,31,8,31,0,0,31,9,31,255,0,7,20,26,22,0,0,20,26,22,0,0,20,21,22,0,0,20,26,22,0,0
        .byte 20,26,22,0,0,20,21,22,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25
        .byte 0,0,23,24,25,255,0,67,31,9,31,255,0,7,31,9,31,255,0,12,20,21,22,0,0,20,26,22,0,0,20,21,22
        .byte 0,0,20,26,22,0,0,20,21,22,0,0,20,21,22,0,0,23,24,25,0,0,23,24,25,0,0,23,24,25,0,0,23
        .byte 24,25,0,0,23,24,25,0,0,23,24,25,0
        .byte 255,0,66,31,8,31,0,0,18,18,0,0,0,31,8,31,255,0,12,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0
        .byte 27,28,29,0,0,27,28,29,0,0,27,28,29,255,0,92,31,8,31,0,0,31,9,31,255,0,7,31,9,31,255,0,7
        .byte 31,8,31,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29
        .byte 255,0,92,31,9,31,0,0,31,8,31,0,0,18,18,0,0,0,31,8,31,255,0,7,31,9,31,0,0,27,28,29,0,0
        .byte 27,28,29,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29,255,0,97,31,9,31,0,0,18,18
        .byte 0,0,0,31,9,31,255,0,12,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0,27,28,29,0,0
        .byte 27,28,29,255,0,31
        .byte 255,0,66,31,8,31,255,0,12,31,8,31,255,0,7,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34
        .byte 0,0,32,33,34,0,0,32,33,34,0,0,35,36,37,0,0,35,36,37,0,0,35,36,37,0,0,35,30,37,0,0,35
        .byte 36,37,0,0,35,36,37,255,0,62,31,8,31,0,0,31,9,31,255,0,7,31,8,31,0,0,31,9,31,255,0,7,32
        .byte 33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,35,36,37,0,0
        .byte 35,36,37,0,0,35,30,37,0,0,35,36,37,0,0,35,36,37,0,0,35,36,37,255,0,62,31,9,31,255,0,7,31
        .byte 8,31,0,0,31,9,31,255,0,7,31,8,31,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34
        .byte 0,0,32,33,34,0,0,32,33,34,0,0,35,36,37,0,0,35,36,37,0,0,35,36,37,0,0,35,36,37,0,0,35
        .byte 30,37,0,0,35,36,37,255,0,72,31,9,31,255,0,12,31,9,31,0,0,32,33,34,0,0,32,33,34,0,0,32,33
        .byte 34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,35,36,37,0,0,35,36,37,0,0,35,36,37,0,0
        .byte 35,36,37,0,0,35,36,37,0,0,35,36,37,0
        .byte 255,0,61,31,8,31,0,0,0,18,255,0,13,31,8,31,255,0,7,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0
        .byte 38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43
        .byte 0,0,41,42,43,0,0,41,42,43,255,0,62,31,9,31,255,0,7,31,8,31,0,0,0,18,0,0,0,31,9,31,0,0
        .byte 31,8,31,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40
        .byte 0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,255,0,67
        .byte 31,8,31,0,0,31,9,31,0,0,0,18,0,0,0,31,8,31,0,0,31,9,31,0,0,38,39,40,0,0,38,39,40
        .byte 0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,41,42,43,0,0,41,42,43,0,0,41
        .byte 42,43,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,255,0,67,31,9,31,255,0,12,31,9,31,255,0,7,38
        .byte 39,40,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,38,39,40,0,0,41,42,43,0,0
        .byte 41,42,43,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,0,0,41,42,43,0
        .byte 255,0,61,31,8,31,255,0,12,31,8,31,255,0,12,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46
        .byte 0,0,44,45,46,0,0,44,45,46,255,0,92,31,9,31,0,0,31,8,31,0,0,6,6,0,0,0,31,9,31,255,0,7
        .byte 31,8,31,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46
        .byte 255,0,92,31,8,31,0,0,31,9,31,255,0,7,6,6,0,0,0,31,8,31,0,0,31,9,31,0,0,44,45,46,0,0
        .byte 44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46,255,0,92,31,9,31,255,0,7,6,6
        .byte 255,0,8,31,9,31,255,0,7,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0,44,45,46,0,0
        .byte 44,45,46,255,0,31
        .byte 255,0,66,31,8,31,255,0,17,31,8,31,0,0,47,48,49,0,0,47,48,49,0,0,47,50,49,0,0,47,48,49,0,0
        .byte 47,48,49,0,0,47,48,49,255,0,92,31,8,31,0,0,31,9,31,255,0,7,31,8,31,255,0,7,31,9,31,0,0
        .byte 47,48,49,0,0,47,48,49,0,0,47,48,49,0,0,47,48,49,0,0,47,48,49,0,0,47,48,49,255,0,92,31,9
        .byte 31,0,0,31,8,31,255,0,7,31,9,31,0,0,31,8,31,255,0,7,47,48,49,0,0,47,48,49,0,0,47,50,49
        .byte 0,0,47,48,49,0,0,47,48,49,0,0,47,48,49,255,0,97,31,9,31,255,0,12,31,9,31,255,0,7,47,48,49
        .byte 0,0,47,48,49,0,0,47,48,49,0,0,47,50,49,0,0,47,48,49,0,0,47,48,49,255,0,31
        .byte 255,0,61,31,8,31,255,0,12,18,18,255,0,8,31,8,31,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0
        .byte 51,52,53,0,0,51,52,53,0,0,51,52,53,255,0,92,31,9,31,0,0,31,8,31,255,0,7,18,18,0,0,0,31
        .byte 8,31,0,0,31,9,31,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0
        .byte 51,52,53,255,0,92,31,8,31,0,0,31,9,31,0,0,18,18,0,0,0,31,8,31,0,0,31,9,31,255,0,7,51
        .byte 52,53,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,255,0,92,31,9,31
        .byte 255,0,12,31,9,31,255,0,12,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0,51,52,53,0,0
        .byte 51,52,53,255,0,31
        .byte 255,0,71,31,8,31,0,0,0,6,0,0,0,31,8,31,255,0,7,54,55,56,0,0,54,55,56,0,0,54,55,56,0,0
        .byte 54,50,56,0,0,54,55,56,0,0,54,55,56,0,0,65,64,65,0,0,0,65,64,0,0,65,64,0,0,0,65,64,255,0,4
        .byte 64,65,0,0,64,65,64,255,0,67,31,8,31,0,0,31,9,31,255,0,7,31,9,31,0,0,31,8,31,0,0,54,55
        .byte 56,0,0,54,55,56,0,0,54,55,56,0,0,54,55,56,0,0,54,55,56,0,0,54,55,56,0,0,0,64,65,0,0
        .byte 64,65,255,0,4,64,64,0,0,0,65,64,0,0,0,64,65,0,0,0,64,64,255,0,62,31,8,31,0,0,31,9,31
        .byte 0,0,0,6,0,0,0,31,8,31,255,0,7,31,9,31,0,0,54,55,56,0,0,54,55,56,0,0,54,50,56,0,0
        .byte 54,55,56,0,0,54,55,56,0,0,54,55,56,0,0,64,65,64,0,0,65,64,0,0,0,64,0,64,0,0,0,65,65
        .byte 0,0,0,64,65,0,0,64,65,64,255,0,62,31,9,31,255,0,12,31,9,31,255,0,12,54,55,56,0,0,54,55,56
        .byte 0,0,54,55,56,0,0,54,55,56,0,0,54,55,56,0,0,54,55,56,0,0,65,64,255,0,4,65,64,0,0,64,0
        .byte 64,0,0,0,65,0,0,0,65,65,65,0,0,0,64,65,0
        .byte 255,0,81,31,8,31,255,0,7,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0
        .byte 32,33,34,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59
        .byte 255,0,67,31,8,31,255,0,12,31,9,31,255,0,7,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34
        .byte 0,0,32,33,34,0,0,32,33,34,0,0,57,58,59,0,0,57,58,59,0,0,58,58,59,0,0,57,58,59,0,0,57
        .byte 58,59,0,0,57,58,59,255,0,62,31,8,31,0,0,31,9,31,255,0,7,31,8,31,255,0,12,32,33,34,0,0,32
        .byte 33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,57,58,59,0,0,57,58,59,0,0
        .byte 57,58,59,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,255,0,62,31,9,31,255,0,7,18,18,18,0,0,31
        .byte 9,31,255,0,12,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34,0,0,32,33,34
        .byte 0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,0,0,57,58,59,0
        .byte 255,0,86,31,8,31,0,0,60,61,62,0,0,60,61,62,0,0,60,61,62,0,0,60,61,62,0,0,60,61,62,0,0
        .byte 60,61,62,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63
        .byte 255,0,62,31,8,31,255,0,22,31,9,31,0,0,60,61,62,0,0,60,61,62,0,0,60,61,62,0,0,60,61,62,0,0
        .byte 60,61,62,0,0,60,61,62,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63
        .byte 0,0,63,63,63,255,0,62,31,9,31,255,0,7,31,8,31,255,0,17,60,61,62,0,0,60,61,62,0,0,60,61,62
        .byte 0,0,60,61,62,0,0,60,61,62,0,0,60,61,62,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63
        .byte 0,0,63,63,63,0,0,63,63,63,255,0,72,31,9,31,255,0,17,60,61,62,0,0,60,61,62,0,0,60,61,62,0,0
        .byte 60,61,62,0,0,60,61,62,0,0,60,61,62,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63,0,0,63,63,63
        .byte 0,0,63,63,63,0,0,63,63,63,0
        .byte 255,0,71,31,8,31,255,0,17,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0
        .byte 66,67,68,255,0,68,72,255,0,23,31,8,31,0,0,69,70,71,0,0,31,9,31,0,0,31,8,31,255,0,12,66,67
        .byte 68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,255,0,92,31,9,31,0,0
        .byte 31,8,31,255,0,7,31,9,31,255,0,7,31,8,31,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66
        .byte 67,68,0,0,66,67,68,0,0,66,67,68,255,0,78,72,255,0,18,31,9,31,255,0,7,69,70,71,255,0,7,31,9
        .byte 31,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,0,0,66,67,68,255,0,31
        .byte 255,0,71,31,8,31,255,0,17,73,74,75,0,0,73,74,75,0,0,73,74,75,0,0,73,74,75,0,0,73,74,75,0,0
        .byte 73,74,75,255,0,24,64,255,0,77,31,9,31,255,0,7,31,8,31,255,0,7,73,74,75,0,0,73,74,75,0,0,73
        .byte 74,75,0,0,73,74,75,0,0,73,74,75,0,0,73,74,75,255,0,19,65,255,0,77,31,8,31,255,0,12,31,9,31
        .byte 0,0,31,8,31,0,0,73,74,75,0,0,73,74,75,0,0,73,74,75,0,0,73,74,75,0,0,73,74,75,0,0,73
        .byte 74,75,255,0,4,64,255,0,92,31,9,31,255,0,7,69,30,71,255,0,7,31,9,31,0,0,73,74,75,0,0,73,74
        .byte 75,0,0,73,74,75,0,0,73,1,75,0,0,73,74,75,0,0,73,74,75,255,0,4,65,255,0,9,65,0,0,0,59
        .byte 255,0,12
        .byte 255,0,61,31,8,31,255,0,27,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0
        .byte 76,77,78,255,0,92,31,9,31,255,0,22,31,8,31,0,0,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0,76
        .byte 77,78,0,0,76,77,78,0,0,76,77,78,255,0,97,31,8,31,0,0,69,70,71,0,0,69,70,71,0,0,31,8,31
        .byte 0,0,31,9,31,0,0,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0,76
        .byte 77,78,255,0,97,31,9,31,255,0,12,31,9,31,255,0,7,76,77,78,0,0,76,77,78,0,0,76,77,78,0,0,76
        .byte 77,78,0,0,76,77,78,0,0,76,77,78,255,0,31
        .byte 255,0,71,31,8,31,0,0,82,83,84,0,0,31,8,31,255,0,7,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0
        .byte 79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4
        .byte 0,0,3,5,4,0,0,3,5,4,255,0,62,31,8,31,255,0,7,31,9,31,0,0,31,8,31,0,0,31,9,31,255,0,7
        .byte 79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,3,5,4
        .byte 0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,255,0,62,31,9,31,0,0
        .byte 31,8,31,0,0,82,83,84,0,0,31,9,31,255,0,7,31,8,31,0,0,79,80,81,0,0,79,80,81,0,0,79,80
        .byte 81,0,0,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0
        .byte 3,5,4,0,0,3,5,4,0,0,3,5,4,255,0,67,31,9,31,255,0,7,82,83,84,255,0,7,31,9,31,0,0
        .byte 79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,79,80,81,0,0,3,5,4
        .byte 0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0,0,3,5,4,0
        .byte 255,0,66,31,8,31,255,0,22,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0
        .byte 85,86,85,255,0,97,31,9,31,255,0,17,31,8,31,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85
        .byte 86,85,0,0,85,86,85,0,0,85,86,85,255,0,92,82,83,84,0,0,31,8,31,255,0,17,31,9,31,0,0,85,50
        .byte 85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,255,0,97,31,9,31,255,0,22
        .byte 85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,0,0,85,86,85,255,0,31
        .byte 255,0,66,31,8,31,255,0,12,31,8,31,255,0,7,51,48,49,0,0,51,48,49,0,0,51,48,49,255,0,7,51,48
        .byte 49,0,0,51,48,49,255,0,112,31,9,31,255,0,7,51,48,49,255,0,7,51,48,49,0,0,51,48,49,0,0,51,48
        .byte 49,0,0,51,48,49,255,0,103,9,255,0,18,51,48,49,0,0,51,48,49,0,0,51,48,49,255,0,7,51,48,49,0,0
        .byte 51,48,49,255,0,92,31,9,31,255,0,23,9,0,0,0,51,48,49,0,0,51,48,49,0,0,51,48,49,0,0,51,48
        .byte 49,0,0,51,48,49,0,0,51,48,49,255,0,31
        .byte 255,0,91,87,88,87,0,0,87,88,87,0,0,87,88,87,0,0,87,88,87,255,0,7,87,88,87,255,0,107,31,9,31
        .byte 255,0,12,87,88,87,0,0,87,88,87,255,0,7,87,88,87,0,0,87,88,87,0,0,87,88,87,255,0,92,31,9,31
        .byte 255,0,22,31,9,31,0,0,87,88,87,0,0,87,88,87,0,0,87,88,87,0,0,87,88,87,255,0,7,87,88,87,255,0,92
        .byte 31,9,31,255,0,22,31,9,31,0,0,87,88,87,0,0,87,88,87,0,0,87,88,87,0,0,87,88,87,0,0,87,88
        .byte 87,0,0,87,88,87,255,0,31
        .byte 255,0,91,90,89,90,0,0,90,89,90,0,0,90,89,90,0,0,90,89,90,0,0,90,89,90,255,0,7,91,92,91,0,0
        .byte 91,92,91,0,0,91,92,91,0,0,91,92,91,0,0,91,92,91,255,0,102,90,89,90,0,0,90,89,90,0,0,90,89
        .byte 90,0,0,90,89,90,0,0,90,89,90,255,0,7,91,92,91,0,0,91,92,91,0,0,91,92,91,0,0,91,92,91,0,0
        .byte 91,92,91,255,0,92,90,89,90,0,0,90,89,90,0,0,90,89,90,0,0,90,89,90,0,0,90,89,90,255,0,7,91
        .byte 92,91,0,0,91,92,91,0,0,91,92,91,0,0,91,92,91,0,0,91,92,91,255,0,97,90,89,90,0,0,90,89,90
        .byte 0,0,90,89,90,0,0,90,89,90,0,0,90,89,90,0,0,90,89,90,0,0,91,92,91,0,0,91,92,91,0,0,91
        .byte 92,91,0,0,91,92,91,0,0,91,92,91,0,0,91,92,91,0
        .byte 255,87,30,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94
        .byte 255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94
        .byte 93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26
        .byte 93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,93,94,255,0,26,93,94,255,87,30
numsc:
        .byte 21
nmedat:
        .byte 0,0,16,16,1,5,80,16,8,6,136,16,255
        .byte 0,0,16,16,2,5,40,120,1,5,120,24,8,6,96,136,255
        .byte 0,0,16,16,2,7,48,64,1,5,80,192,8,6,136,216,255
        .byte 0,0,16,16,8,6,136,216,2,7,40,40,1,5,80,112,3,8,120,184,255
        .byte 0,0,16,16,8,6,16,216,3,8,8,192,1,7,120,48,4,9,80,120,255
        .byte 0,0,16,16,8,6,16,96,5,10,24,120,1,11,128,104,255
        .byte 0,0,16,16,8,6,136,216,3,8,8,208,1,11,120,80,5,12,48,120,255
        .byte 0,0,16,16,8,6,136,216,5,13,8,120,5,13,56,80,255
        .byte 0,0,16,16,4,8,8,232,3,8,128,112,1,5,40,96,8,6,16,176,255
        .byte 0,0,16,16,7,6,136,216,5,13,48,80,6,7,120,16,5,10,72,160,255
        .byte 0,0,16,16,5,14,48,120,5,14,64,160,3,17,120,120,1,5,40,24,7,6,136,216,255
        .byte 0,0,16,16,5,10,16,160,7,6,136,216,4,15,88,144,255
        .byte 0,0,16,16,5,13,16,120,1,5,80,104,3,8,88,184,7,6,136,176,255
        .byte 0,0,16,16,7,6,136,16,5,12,64,80,1,11,40,16,3,11,80,184,255
        .byte 0,0,16,16,7,6,136,224,5,10,48,80,3,17,128,160,4,8,40,120,255
        .byte 0,0,16,16,7,6,136,16,6,11,48,32,2,5,40,152,3,8,88,152,255
        .byte 0,0,16,16,7,6,136,216,6,11,48,72,5,14,16,120,6,7,128,152,3,17,80,144,255
        .byte 0,0,16,16,7,6,136,144,3,17,40,160,1,7,88,40,3,17,120,64,255
        .byte 0,0,16,16,7,6,136,64,3,8,120,144,3,8,128,40,1,11,88,56,255
        .byte 0,0,16,16,3,8,48,200,2,5,40,40,3,9,80,120,7,6,136,24,3,17,128,152,3,11,128,72,255
        .byte 4,6,48,32,4,6,48,208,4,13,136,32,4,13,136,208,255
NUMOBJ = 40
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,0,16,176,0,16,176
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,0,56,96,0,56,96
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,0,96,136,0,96,136
        .byte 0,0,0,0,0,0,0,0,0,66,0,231,0,231,0,66,0,0,0,189,0,60,0,189,0,60,0,189,0,60,0,189,1,56,136,1,56,136
        .byte 0,0,0,0,0,0,0,0,0,66,0,231,0,231,0,66,0,0,0,189,0,60,0,189,0,60,0,189,0,60,0,189,1,96,96,1,96,96
        .byte 0,0,0,0,0,0,0,0,0,66,0,231,0,231,0,66,0,0,0,189,0,60,0,189,0,60,0,189,0,60,0,189,1,136,136,1,136,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,16,0,84,0,84,0,68,0,40,1,41,0,170,2,16,136,2,16,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,16,0,84,0,84,0,68,0,40,1,41,0,170,2,56,56,2,56,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,16,0,84,0,84,0,68,0,40,1,41,0,170,2,136,96,2,136,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,0,68,0,40,0,146,1,85,0,186,3,16,96,3,16,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,0,68,0,40,0,146,1,85,0,186,3,96,96,3,96,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,0,68,0,40,0,146,1,85,0,186,3,136,96,3,136,96
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,4,16,136,4,16,136
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,4,56,96,4,56,96
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,4,96,176,4,96,176
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,0,68,0,40,0,146,1,85,0,186,5,16,56,5,16,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,0,68,0,40,0,146,1,85,0,186,5,56,136,5,56,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,0,68,0,40,0,146,1,85,0,186,5,96,136,5,96,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,56,0,56,0,56,0,186,1,215,0,238,6,56,96,6,56,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,56,0,56,0,56,0,186,1,215,0,238,6,96,136,6,96,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,56,0,56,0,56,0,186,1,215,0,238,6,136,96,6,136,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,73,0,73,0,127,0,54,0,62,0,20,0,42,0,28,7,16,96,7,16,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,73,0,73,0,127,0,54,0,62,0,20,0,42,0,28,7,96,96,7,96,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,62,0,73,0,73,0,127,0,54,0,62,0,20,0,42,0,28,7,136,136,7,136,136
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,8,16,136,8,16,136
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,8,56,136,8,56,136
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,8,96,96,8,96,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,16,0,84,0,84,0,68,0,40,1,41,0,170,9,16,136,9,16,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,16,0,84,0,84,0,68,0,40,1,41,0,170,9,96,96,9,96,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,130,0,68,0,40,0,146,1,85,0,186,10,136,96,10,136,96
        .byte 0,0,0,0,0,0,0,0,0,0,4,4,4,68,2,68,2,100,3,108,1,173,1,194,0,246,4,236,3,21,1,187,10,136,96,10,136,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,12,56,56,12,56,56
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,12,136,136,12,136,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,13,136,136,13,136,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,14,96,96,14,96,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,14,96,136,14,96,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,15,16,136,15,16,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,15,96,96,15,96,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,15,136,136,15,136,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,16,96,16,16,96,16
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
        .byte 0,0,0,24,36,24,0,0
        .byte 0,0,0,0,0,56,56,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,124,238,238,238,238,124,0
        .byte 0,56,60,28,28,126,126,0
        .byte 0,124,254,30,248,246,254,0
        .byte 0,124,222,60,30,254,124,0
        .byte 0,28,60,124,220,254,60,0
        .byte 0,254,246,248,62,222,124,0
        .byte 0,124,236,242,254,238,124,0
        .byte 0,254,254,14,60,120,120,0
        .byte 0,124,238,124,238,238,124,0
        .byte 0,124,254,246,222,62,124,0
        .byte 0,0,48,48,0,48,48,0
        .byte 0,124,130,186,162,186,130,124
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,124,238,254,238,238,108,0
        .byte 0,124,238,252,238,254,124,0
        .byte 0,124,254,224,254,254,124,0
        .byte 0,252,238,238,254,254,252,0
        .byte 0,126,240,252,240,254,126,0
        .byte 0,126,240,252,240,240,240,0
        .byte 0,124,254,240,238,246,124,0
        .byte 0,238,238,254,254,238,238,0
        .byte 0,126,126,60,60,126,126,0
        .byte 0,30,30,30,222,254,124,0
        .byte 0,238,252,248,248,252,238,0
        .byte 0,112,240,240,252,254,126,0
        .byte 0,214,254,254,238,238,238,0
        .byte 0,206,238,254,254,238,230,0
        .byte 0,124,254,238,238,254,124,0
        .byte 0,252,238,238,252,240,240,0
        .byte 0,124,254,254,238,242,124,0
        .byte 0,252,254,238,252,238,238,0
        .byte 0,124,240,252,30,254,124,0
        .byte 0,254,254,56,124,124,124,0
        .byte 0,238,238,238,238,254,124,0
        .byte 0,238,238,238,238,124,56,0
        .byte 0,238,238,238,254,254,108,0
        .byte 0,238,254,124,254,254,238,0
        .byte 0,238,238,254,62,254,252,0
        .byte 0,254,254,184,118,254,254,0
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
        .byte 99
keys:   .byte 10,8,63,47,33,1,57,31,30,26,24