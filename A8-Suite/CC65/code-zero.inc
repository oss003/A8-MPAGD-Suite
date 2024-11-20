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
WINDOWLFT = 2
WINDOWHGT = 21
WINDOWWID = 28 ;a
MAPWID = 8
        .byte 255,255,255,255,255,255,255,255
mapdat:
        .byte 255,25,24,23,22,5,0,255,255,21,20,19,18,4,0,255,255,17,16,15,14,3,0,255,255,13,12,11,10,2,0,255,255,9,8,7,6,1,0,255
        .byte 255,255,255,255,255,255,255,255
stmap:  .byte 37

evnt00:
        lda #DEADLY	; DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00042
:
        lda #1		; ENDGAME
        sta gamwon
a00042: lda #0
        cmp vari
        beq *+5
        jmp a00117
        lda #0
        sta numlif
        lda #7
        sta chary
        lda #10
        sta charx
        lda #12  	; MESSAGE
        jsr dmsg
        lda #100 	; DELAY
        jsr delay
        lda #1 		; KILL
        sta deadf
a00117: lda #1 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00153
:
        lda #2 	; DELAY
        jsr delay
a00153: lda #3 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00188
:
        lda #2 	; DELAY
        jsr delay
a00188: lda #4 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00224
:
        lda #2 	; DELAY
        jsr delay
a00224: lda #5 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00259
:
        lda #2 	; DELAY
        jsr delay
a00259: lda #7 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp a00381
:
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a00346
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        jmp a00381
a00346: lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
a00381: jsr skobj	; DETECTOBJECT
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00462
        lda varobj	; GET
        jsr getob
        lda varl
        clc
        adc #1
        sta varl
        lda varobj
        sta vark
        lda vark
        clc
        adc #1
        sta vark
a00462: lda #2
        cmp vard
        bcs *+5
        jmp a00486
        lda #0
        sta vard
a00486: lda joyval	; KEY
        and #1
        beq :+
        jmp a00647
:
        lda #3
        ldy #6
        sta (z80_ix),y
        jsr cangr	; CANGORIGHT
        beq :+
        jmp a00625
:
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp a00589
        lda #0		; ANIMATE
        jsr animsp
a00589: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00625
        lda #7		; BEEP
        asl a
        sta sndtyp
a00625: lda #1
        sta vard
        lda #0
        ldy #10
        sta (z80_ix),y
a00647: lda joyval	; KEY
        and #2
        beq :+
        jmp a00807
:
        lda #4
        ldy #6
        sta (z80_ix),y
        jsr cangl	; CANGOLEFT
        beq :+
        jmp a00785
:
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp a00749
        lda #0		; ANIMATE
        jsr animsp
a00749: lda #1
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00785
        lda #7		; BEEP
        asl a
        sta sndtyp
a00785: lda #2
        sta vard
        lda #2
        ldy #10
        sta (z80_ix),y
a00807: lda joyval	; KEY
        and #8
        beq :+
        jmp a00952
:
        jsr laddu	; CANGOUP
        beq :+
        jmp a00944
:
        lda #6
        ldy #6
        sta (z80_ix),y
        ldy #8 		; SPRITEUP
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp a00944
        lda #0		; ANIMATE
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a00944
        lda #3		; BEEP
        asl a
        sta sndtyp
a00944: lda #3
        sta vard
a00952: lda joyval	; KEY
        and #4
        beq :+
        jmp a01099
:
        jsr laddd	; LADDERBELOW
        beq :+
        jmp a01091
:
        lda #6
        ldy #6
        sta (z80_ix),y
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp varc
        beq *+5
        jmp a01091
        lda #0		; ANIMATE
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01091
        lda #3		; BEEP
        asl a
        sta sndtyp
a01091: lda #4
        sta vard
a01099: lda #0
        cmp vard
        beq *+5
        jmp a01163
        ldy #10
        lda (z80_ix),y
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp varj
        beq *+5
        jmp a01163
        lda #0		; ANIMATE
        jsr animsp
a01163: lda #224
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp a01207
        lda #18
        ldy #9
        sta (z80_ix),y
        jsr scrr	; SCREENRIGHT
a01207: lda #16
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a01251
        lda #222
        ldy #9
        sta (z80_ix),y
        jsr scrl	; SCREENLEFT
a01251: jsr cangd	; CANGODOWN
        beq :+
        jmp a01275
:
        jsr ifall	; TABLEFALL
a01275: lda #5
        cmp scno
        bcs *+5
        jmp a01344
        lda #1
        cmp varh
        beq *+5
        jmp a01344
        lda #104
        ldy #8
        sta (z80_ix),y
        lda #220
        ldy #9
        sta (z80_ix),y
        lda #0
        sta varh
a01344: lda #0
        cmp scno
        beq *+5
        jmp a01388
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #220
        ldy #9
        sta (z80_ix),y
a01388: jmp grav
evnt01:
        lda #0
        cmp varc
        beq *+5
        jmp b00030
        lda #0		; ANIMATE
        jsr animsp
b00030: lda #110
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00085
        lda #30
        ldy #8
        sta (z80_ix),y
        lda #12		; BEEP
        asl a
        sta sndtyp
        jmp b00110
b00085: ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
b00110: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b00193
:
        lda #0
        cmp varj
        beq *+5
        jmp b00193
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #50		; BEEP
        asl a
        sta sndtyp
        lda #5 	; DELAY
        jsr delay
b00193: rts
evnt02:
        lda #0
        cmp varj
        beq *+5
        jmp c00030
        lda #0		; ANIMATE
        jsr animsp
c00030: rts
evnt03:
        lda #180
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp d00052
        lda #0
        ldy #10
        sta (z80_ix),y
        lda #13		; BEEP
        asl a
        sta sndtyp
d00052: lda #40
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp d00102
        lda #1
        ldy #10
        sta (z80_ix),y
        lda #13		; BEEP
        asl a
        sta sndtyp
d00102: lda #0
        cmp varc
        beq *+5
        jmp d00197
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp d00171
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp d00197
d00171: ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
d00197: lda #0		; ANIMATE
        jsr animsp
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00291
:
        lda #0
        cmp varj
        beq *+5
        jmp d00291
        lda #50		; BEEP
        asl a
        sta sndtyp
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #5 	; DELAY
        jsr delay
d00291: rts
evnt04:
        lda #110
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00052
        lda #30
        ldy #8
        sta (z80_ix),y
        lda #10		; BEEP
        asl a
        sta sndtyp
e00052: lda #0
        cmp varc
        beq *+5
        jmp e00094
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
e00094: lda #0
        cmp varj
        beq *+5
        jmp e00122
        lda #0		; ANIMATE
        jsr animsp
e00122: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00205
:
        lda #0
        cmp varj
        beq *+5
        jmp e00205
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #50		; BEEP
        asl a
        sta sndtyp
        lda #5 	; DELAY
        jsr delay
e00205: rts
evnt05:
        lda #0		; ANIMATE
        jsr animsp
        lda #30
        cmp varm
        beq *+5
        jmp f00049
        ldy #11
        lda (z80_ix),y
        ldy #9
        sta (z80_ix),y
f00049: lda #59
        cmp varm
        beq *+5
        jmp f00085
        ldy #12
        lda (z80_ix),y
        ldy #9
        sta (z80_ix),y
f00085: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp f00168
:
        lda #0
        cmp varj
        beq *+5
        jmp f00168
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #20		; BEEP
        asl a
        sta sndtyp
        lda #5 	; DELAY
        jsr delay
f00168: rts
evnt06:
        rts
evnt07:
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00900
:
        lda #3
        cmp scno
        beq *+5
        jmp h00106
        lda #0		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00106
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00106: lda #1
        cmp scno
        beq *+5
        jmp h00185
        lda #1		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00185
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00185: lda #10
        cmp scno
        beq *+5
        jmp h00265
        lda #2		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00265
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00265: lda #4
        cmp scno
        beq *+5
        jmp h00344
        lda #3		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00344
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00344: lda #8
        cmp scno
        beq *+5
        jmp h00423
        lda #4		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00423
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00423: lda #19
        cmp scno
        beq *+5
        jmp h00503
        lda #5		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00503
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00503: lda #15
        cmp scno
        beq *+5
        jmp h00582
        lda #6		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00582
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00582: lda #5
        cmp scno
        beq *+5
        jmp h00661
        lda #7		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00661
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00661: lda #14
        cmp scno
        beq *+5
        jmp h00741
        lda #8		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00741
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00741: lda #24
        cmp scno
        beq *+5
        jmp h00820
        lda #9		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00820
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00820: lda #20
        cmp scno
        beq *+5
        jmp h00900
        lda #10		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp h00900
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varl
        clc
        adc #1
        sta varl
        lda #0
        sta vark
h00900: lda #1
        cmp scno
        beq *+5
        jmp h00950
        lda #3
        cmp varl
        bcc *+5
        jmp h00950
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h00950: lda #3
        cmp scno
        beq *+5
        jmp h00999
        lda #1
        cmp varl
        bcc *+5
        jmp h00999
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h00999: lda #10
        cmp scno
        beq *+5
        jmp h01049
        lda #5
        cmp varl
        bcc *+5
        jmp h01049
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01049: lda #4
        cmp scno
        beq *+5
        jmp h01098
        lda #7
        cmp varl
        bcc *+5
        jmp h01098
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01098: lda #8
        cmp scno
        beq *+5
        jmp h01148
        lda #9
        cmp varl
        bcc *+5
        jmp h01148
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01148: lda #19
        cmp scno
        beq *+5
        jmp h01198
        lda #11
        cmp varl
        bcc *+5
        jmp h01198
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01198: lda #15
        cmp scno
        beq *+5
        jmp h01248
        lda #13
        cmp varl
        bcc *+5
        jmp h01248
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01248: lda #5
        cmp scno
        beq *+5
        jmp h01298
        lda #15
        cmp varl
        bcc *+5
        jmp h01298
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01298: lda #14
        cmp scno
        beq *+5
        jmp h01348
        lda #17
        cmp varl
        bcc *+5
        jmp h01348
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01348: lda #24
        cmp scno
        beq *+5
        jmp h01398
        lda #19
        cmp varl
        bcc *+5
        jmp h01398
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
h01398: rts
evnt08:
        lda #0
        cmp varg
        beq *+5
        jmp i00515
        lda #1
        cmp varf
        beq *+5
        jmp i00049
        lda #88
        ldy #8
        sta (z80_ix),y
i00049: lda #2
        cmp varf
        beq *+5
        jmp i00079
        lda #64
        ldy #8
        sta (z80_ix),y
i00079: lda #3
        cmp varf
        beq *+5
        jmp i00109
        lda #40
        ldy #8
        sta (z80_ix),y
i00109: lda #4
        cmp varf
        beq *+5
        jmp i00139
        lda #16
        ldy #8
        sta (z80_ix),y
i00139: lda joyval	; KEY
        and #8
        beq :+
        jmp i00236
:
        lda #112
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp i00236
        lda #16
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp i00236
        lda #24
        sta z80_c
        lda vare
        sec
        sbc z80_c
        sta vare
        lda #1
        sta varg
i00236: lda joyval	; KEY
        and #4
        beq :+
        jmp i00310
:
        lda #111
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp i00310
        lda #24
        sta z80_c
        lda vare
        clc
        adc z80_c
        sta vare
        lda #2
        sta varg
i00310: lda joyval	; KEY
        and #16
        beq :+
        jmp i00515
:
        lda #0
        cmp varf
        beq *+5
        jmp i00366
        lda #1
        sta scno
        jsr nwscr
        lda #1
        sta varh
i00366: lda #1
        cmp varf
        beq *+5
        jmp i00403
        lda #2
        sta scno
        jsr nwscr
        lda #1
        sta varh
i00403: lda #2
        cmp varf
        beq *+5
        jmp i00441
        lda #3
        sta scno
        jsr nwscr
        lda #1
        sta varh
i00441: lda #3
        cmp varf
        beq *+5
        jmp i00478
        lda #4
        sta scno
        jsr nwscr
        lda #1
        sta varh
i00478: lda #4
        cmp varf
        beq *+5
        jmp i00515
        lda #5
        sta scno
        jsr nwscr
        lda #1
        sta varh
i00515: lda #1
        cmp varg
        beq *+5
        jmp i00638
        ldy #8
        lda (z80_ix),y
        cmp vare
        bcs *+5
        jmp i00601
        lda #0
        cmp varc
        beq *+5
        jmp i00596
        ldy #8 		; SPRITEUP
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
i00596: jmp i00638
i00601: lda vare
        ldy #8
        sta (z80_ix),y
        lda #0
        sta varg
        lda varf
        clc
        adc #1
        sta varf
i00638: lda #2
        cmp varg
        beq *+5
        jmp i00762
        ldy #8
        lda (z80_ix),y
        cmp vare
        bcc *+5
        jmp i00725
        lda #0
        cmp varc
        beq *+5
        jmp i00720
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
i00720: jmp i00762
i00725: lda vare
        ldy #8
        sta (z80_ix),y
        lda #0
        sta varg
        lda varf
        sec
        sbc #1
        sta varf
i00762: rts
evnt09:
        lda #5
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00382
        lda #3
        cmp scno
        beq *+5
        jmp j00068
        lda #140
        ldy #11
        sta (z80_ix),y
        lda #68
        ldy #12
        sta (z80_ix),y
j00068: lda #9
        cmp scno
        beq *+5
        jmp j00113
        lda #188
        ldy #11
        sta (z80_ix),y
        lda #150
        ldy #12
        sta (z80_ix),y
j00113: lda #11
        cmp scno
        beq *+5
        jmp j00158
        lda #116
        ldy #11
        sta (z80_ix),y
        lda #68
        ldy #12
        sta (z80_ix),y
j00158: lda #12
        cmp scno
        beq *+5
        jmp j00202
        lda #100
        ldy #11
        sta (z80_ix),y
        lda #52
        ldy #12
        sta (z80_ix),y
j00202: lda #15
        cmp scno
        beq *+5
        jmp j00247
        lda #150
        ldy #11
        sta (z80_ix),y
        lda #190
        ldy #12
        sta (z80_ix),y
j00247: lda #17
        cmp scno
        beq *+5
        jmp j00292
        lda #165
        ldy #11
        sta (z80_ix),y
        lda #124
        ldy #12
        sta (z80_ix),y
j00292: lda #20
        cmp scno
        beq *+5
        jmp j00337
        lda #140
        ldy #11
        sta (z80_ix),y
        lda #190
        ldy #12
        sta (z80_ix),y
j00337: lda #24
        cmp scno
        beq *+5
        jmp j00382
        lda #117
        ldy #11
        sta (z80_ix),y
        lda #165
        ldy #12
        sta (z80_ix),y
j00382: rts
evnt10:
        lda #19
        sta chary
        lda #9
        sta charx
        lda #3  	; MESSAGE
        jsr dmsg
        lda #19
        sta chary
        lda #14
        sta charx
        lda vari	; DISPLAY
        jsr disply
        lda #19
        sta chary
        lda #22
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        lda #19
        sta chary
        lda #5
        sta charx
        lda vark	; DISPLAY
        jsr disply
        rts
evnt11:
        lda #18
        sta chary
        lda #25
        sta charx
        lda #13  	; MESSAGE
        jsr dmsg
        lda #19
        sta chary
        lda #25
        sta charx
        lda #14  	; MESSAGE
        jsr dmsg
        lda #0
        cmp numlif
        beq *+5
        jmp l00086
        lda #1 		; KILL
        sta deadf
l00086: lda #0
        cmp scno
        beq *+5
        jmp l00185
        lda #3
        sta chary
        lda #3
        sta charx
        lda #1  	; MESSAGE
        jsr dmsg
        lda #5
        sta chary
        lda #3
        sta charx
        lda #2  	; MESSAGE
        jsr dmsg
        lda #8
        sta chary
        lda #3
        sta charx
        lda #4  	; MESSAGE
        jsr dmsg
l00185: lda #1
        cmp varc
        beq *+5
        jmp l00215
        lda #0
        sta varc
        jmp l00230
l00215: lda varc
        clc
        adc #1
        sta varc
l00230: lda #5
        cmp varj
        beq *+5
        jmp l00259
        lda #0
        sta varj
        jmp l00275
l00259: lda varj
        clc
        adc #1
        sta varj
l00275: lda #60
        cmp varm
        beq *+5
        jmp l00304
        lda #0
        sta varm
        jmp l00320
l00304: lda varm
        clc
        adc #1
        sta varm
l00320: rts
evnt12:
        jsr cls		; CLS
        lda #5
        sta chary
        lda #11
        sta charx
        lda #5  	; MESSAGE
        jsr dmsg
        lda #7
        sta chary
        lda #8
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #9
        sta chary
        lda #11
        sta charx
        lda #7  	; MESSAGE
        jsr dmsg
        lda #10
        sta chary
        lda #6
        sta charx
        lda #8  	; MESSAGE
        jsr dmsg
        lda #18
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
        lda #9
        sta numlif
        jsr cls		; CLS
        lda #0
        sta vara
        lda #0
        sta varb
        lda #0
        sta varc
        lda #0
        sta vard
        lda #112
        sta vare
        lda #0
        sta varf
        lda #0
        sta varg
        lda #0
        sta varh
        lda #181
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
        lda #0
        cmp vari
        bcc *+5
        jmp o00034
        lda vari
        sec
        sbc #1
        sta vari
o00034: rts
evnt15:
        rts
evnt16:
        lda #99
        sta vard
        lda #200		; BEEP
        asl a
        sta sndtyp
        rts
evnt17:
        jsr cls		; CLS
        lda #10
        sta chary
        lda #11
        sta charx
        lda #0  	; MESSAGE
        jsr dmsg
        lda #200 	; DELAY
        jsr delay
        rts
evnt18:
        lda #99
        sta vard
        lda #80		; BEEP
        asl a
        sta sndtyp
        lda #4
        sta chary
        lda #4
        sta charx
        lda #10  	; MESSAGE
        jsr dmsg
        lda #7
        sta chary
        lda #12
        sta charx
        lda #11  	; MESSAGE
        jsr dmsg
        lda #20 	; DELAY
        jsr delay
        lda #250		; BEEP
        asl a
        sta sndtyp
        jsr prskey	; WAITKEY
        rts
evnt19:
        rts
evnt20:
        lda #1	; GET
        jsr getob
        rts
ptcusr: rts
msgdat:
        .byte "-GAME OVER",173
        .byte "THE LIF",212
        .byte "USE UP AND",13
        .byte "   DOW",206
        .byte "TIM",197
        .byte "SPACE TO",13
        .byte "   EXI",212
        .byte "code zer",239
        .byte "SELECT  CONTRO",204
        .byte "1 KEYBOAR",196
        .byte "2 KEMPSTON JOYSTIC",203
        .byte " PAUL JENKINSON 201",183
        .byte "WITH THE VIRUS UPLOADED",13
        .byte "    YOUR MISSION IS COMPLET",197
        .byte "*WELL DONE",170
        .byte "OUT OF TIM",197
        .byte "cod",229
        .byte "zer",239
nummsg:
        .byte 15
chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,221,0,119,0,221,0,238
        .byte 31,32,79,159,191,191,191,191
        .byte 255,0,255,255,255,255,255,255
        .byte 254,1,249,245,249,245,249,245
        .byte 64,64,64,64,64,64,64,64
        .byte 0,0,0,0,0,0,0,0
        .byte 128,255,192,96,255,240,240,255
        .byte 22,10,22,10,22,10,22,10
        .byte 191,191,191,191,191,159,128,127
        .byte 255,255,255,255,170,85,0,255
        .byte 233,245,233,213,171,86,12,248
        .byte 0,255,0,0,255,255,255,255
        .byte 1,255,11,22,255,23,23,255
        .byte 0,127,64,74,85,74,85,74
        .byte 0,255,0,170,85,170,85,170
        .byte 0,255,1,171,85,171,85,171
        .byte 85,74,85,74,85,74,85,127
        .byte 85,170,85,170,85,170,85,255
        .byte 85,171,85,171,85,171,85,255
        .byte 0,255,255,0,0,255,255,0
        .byte 52,181,181,52,52,181,181,52
        .byte 0,126,102,90,90,90,102,126
        .byte 0,255,150,182,150,182,178,255
        .byte 0,255,34,170,170,170,34,255
        .byte 0,254,62,166,62,102,190,254
        .byte 0,126,102,118,118,118,66,126
        .byte 0,126,70,122,70,94,66,126
        .byte 0,126,70,122,98,122,70,126
        .byte 0,126,90,90,66,122,122,126
        .byte 1,0,1,0,1,0,1,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,255,0,255,255,170,85,0
        .byte 60,129,60,189,189,188,1,60
        .byte 0,6,24,54,46,106,84,0
        .byte 0,96,120,60,92,94,94,0
        .byte 0,122,122,116,108,120,96,0
        .byte 0,122,122,60,62,30,6,0
        .byte 90,58,90,58,90,58,90,58
        .byte 4,212,4,116,4,212,4,228
        .byte 32,45,32,39,32,45,32,46
        .byte 128,0,128,0,128,0,128,0
        .byte 0,0,0,0,0,239,239,0
        .byte 70,175,202,6,48,104,57,128
        .byte 0,221,0,119,0,221,0,238
        .byte 64,48,88,58,90,58,90,58
        .byte 63,23,11,23,32,0,112,248
        .byte 252,232,208,232,4,0,14,31
        .byte 32,35,63,0,63,0,43,0
        .byte 4,196,252,0,252,0,212,0
        .byte 15,16,32,32,33,32,35,32
        .byte 240,8,36,20,132,4,196,4
        .byte 0,0,128,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 86,128,128,0,128,0,85,0
        .byte 84,2,0,0,2,2,84,0
        .byte 0,63,0,63,0,63,0,255
        .byte 0,244,0,244,0,244,0,255
        .byte 197,202,197,202,197,224,255,255
        .byte 83,171,83,171,83,7,255,255
        .byte 31,0,255,255,224,192,197,202
        .byte 240,0,255,255,7,3,83,163
        .byte 0,0,21,36,85,36,149,96
        .byte 0,244,0,244,0,244,0,244
        .byte 6,6,10,246,246,10,6,6
        .byte 224,240,248,220,206,198,198,198
        .byte 255,0,0,119,0,221,0,238
        .byte 252,4,4,116,4,212,4,244
        .byte 2,2,2,2,2,2,2,2
        .byte 44,44,44,44,44,44,44,44
        .byte 0,0,255,0,255,255,0,0
        .byte 0,0,120,12,100,116,52,0
        .byte 0,44,38,48,62,30,0,0
        .byte 0,44,76,28,124,120,0,0
        .byte 32,40,44,44,44,44,44,44
        .byte 44,44,44,44,44,0,94,94
        .byte 127,127,127,63,63,31,15,3
        .byte 0,0,0,0,0,0,0,0
        .byte 250,250,242,228,196,136,176,192
        .byte 5,5,5,5,5,5,5,5
        .byte 128,128,128,128,128,128,128,128
        .byte 0,0,0,0,0,0,0,0
        .byte 8,20,226,241,226,132,72,48
        .byte 28,254,7,107,61,30,60,0
        .byte 0,122,120,126,62,62,30,14
        .byte 0,122,122,122,116,108,120,112
        .byte 0,255,0,187,221,238,0,255
        .byte 120,58,90,42,82,106,112,122
        .byte 0,0,14,16,46,46,44,0
        .byte 56,0,16,0,16,0,16,0
        .byte 16,56,56,116,116,56,16,0
        .byte 85,128,0,128,0,128,85,0
        .byte 84,2,0,2,0,2,84,0
        .byte 191,160,163,160,191,128,255,255
        .byte 250,10,138,10,250,2,254,254
        .byte 191,128,191,160,163,160,191,128
        .byte 250,2,250,10,138,10,250,2
        .byte 0,255,128,191,160,160,163,160
        .byte 0,254,2,250,10,10,138,10
        .byte 16,56,56,100,64,0,0,0
        .byte 54,54,54,54,54,54,54,0
        .byte 205,205,205,205,217,217,217,0
        .byte 179,179,179,179,155,155,155,0
        .byte 255,255,0,24,24,0,0,0
        .byte 0,255,255,0,223,223,0,251
        .byte 251,0,223,170,81,170,81,0
        .byte 251,248,251,251,0,223,223,0
        .byte 63,63,0,63,63,0,63,127
        .byte 40,40,0,40,40,0,52,26
        .byte 0,255,0,63,0,63,63,0
        .byte 0,254,0,40,0,40,40,0
        .byte 0,127,64,74,85,74,85,74
        .byte 0,254,2,170,86,170,86,170
        .byte 85,74,85,74,85,74,85,127
        .byte 86,170,86,170,86,170,86,254
        .byte 85,74,85,74,85,74,85,74
        .byte 86,170,86,170,86,170,86,170
        .byte 0,255,0,170,85,170,85,170
        .byte 85,170,85,170,85,170,85,255
        .byte 85,170,85,170,85,170,85,170
        .byte 85,162,69,138,85,170,85,170
        .byte 0,0,0,0,0,0,0,0
        .byte 24,24,24,24,24,0,24,52
        .byte 0,255,255,255,0,24,24,24
        .byte 0,255,255,255,0,0,0,0
        .byte 32,32,32,63,0,127,75,127
        .byte 4,4,4,252,0,254,242,254
        .byte 0,63,32,42,32,45,32,32
        .byte 0,252,4,4,4,68,4,4
        .byte 3,3,3,3,1,1,1,1
        .byte 255,0,0,0,0,0,0,0
        .byte 192,192,192,192,128,128,128,128
        .byte 0,0,0,0,0,0,0,0
        .byte 11,11,11,11,11,11,11,11
        .byte 208,208,208,208,208,208,208,208
        .byte 255,255,0,255,0,0,0,0
        .byte 0,0,0,12,108,110,110,255
        .byte 0,223,223,0,187,0,238,0
        .byte 110,0,187,0,247,0,109,0
        .byte 8,16,32,0,0,2,4,8
        .byte 0,255,177,187,187,145,255,0
        .byte 0,255,145,187,155,187,255,0
        .byte 14,28,63,223,239,239,103,64
        .byte 0,255,129,189,129,129,255,0
bprop:
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
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
        .byte 0
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
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 3
        .byte 3
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 3
        .byte 2
        .byte 2
        .byte 0
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
        .byte 2
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
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
sprgfx:
        .byte 0,0,3,224,7,248,15,0,14,200,5,252,0,248,3,96,13,120,30,128,31,160,61,160,59,168,61,172,26,206,7,5,6,227,9,240,30,240,60,88,120,56,96,48,88,14,60,31
        .byte 0,0,0,248,1,254,3,192,3,178,1,127,0,62,0,216,3,94,7,160,7,232,15,104,14,234,15,107,134,179,65,193,193,184,2,124,7,188,15,22,30,14,24,12,150,3,207,7
        .byte 0,0,0,62,128,127,0,240,128,236,192,95,128,15,0,54,128,215,1,232,1,250,3,218,131,186,195,218,225,172,80,112,48,110,0,159,1,239,131,197,135,131,6,3,229,128,243,193
        .byte 0,0,128,15,224,31,0,60,32,59,240,23,224,3,128,13,224,53,0,122,128,126,128,246,160,238,176,246,56,107,20,28,140,27,192,39,192,123,96,241,225,224,193,128,57,96,124,240
        .byte 0,0,7,192,15,240,31,0,30,144,13,248,3,240,1,192,26,240,61,0,63,176,123,176,119,180,123,182,53,165,30,2,12,224,3,240,30,240,60,88,120,56,96,48,88,14,60,31
        .byte 0,0,1,240,3,252,7,192,7,164,3,126,0,252,0,112,6,188,15,64,15,236,30,236,29,237,158,237,77,105,135,128,3,56,0,252,7,188,15,22,30,14,24,12,150,3,207,7
        .byte 0,0,0,124,0,255,1,240,1,233,128,223,0,63,0,28,1,175,3,208,3,251,7,187,71,123,103,187,83,90,33,224,0,206,0,63,1,239,131,197,135,131,6,3,229,128,243,193
        .byte 0,0,0,31,192,63,0,124,64,122,224,55,192,15,0,7,192,107,0,244,192,254,193,238,209,222,217,238,148,214,8,120,128,51,192,15,192,123,96,241,225,224,193,128,57,96,124,240
        .byte 0,0,7,192,15,240,31,0,30,144,13,248,3,240,1,192,26,240,61,0,63,176,123,176,119,180,123,182,53,165,30,2,12,224,3,240,30,240,60,88,120,56,96,48,88,14,60,31
        .byte 0,0,1,240,3,252,7,192,7,164,3,126,0,252,0,112,6,188,15,64,15,236,30,236,29,237,158,237,77,105,135,128,3,56,0,252,7,188,15,22,30,14,24,12,150,3,207,7
        .byte 0,0,0,124,0,255,1,240,1,233,128,223,0,63,0,28,1,175,3,208,3,251,7,187,71,123,103,187,83,90,33,224,0,206,0,63,1,239,131,197,135,131,6,3,229,128,243,193
        .byte 0,0,0,31,192,63,0,124,64,122,224,55,192,15,0,7,192,107,0,244,192,254,193,238,209,222,217,238,148,214,8,120,128,51,192,15,192,123,96,241,225,224,193,128,57,96,124,240
        .byte 0,0,3,224,7,248,15,0,14,200,5,252,0,248,3,96,13,120,30,128,31,160,61,160,59,168,61,172,26,206,7,5,6,227,9,240,30,240,60,88,120,56,96,48,88,14,60,31
        .byte 0,0,0,248,1,254,3,192,3,178,1,127,0,62,0,216,3,94,7,160,7,232,15,104,14,234,15,107,134,179,65,193,193,184,2,124,7,188,15,22,30,14,24,12,150,3,207,7
        .byte 0,0,0,62,128,127,0,240,128,236,192,95,128,15,0,54,128,215,1,232,1,250,3,218,131,186,195,218,225,172,80,112,48,110,0,159,1,239,131,197,135,131,6,3,229,128,243,193
        .byte 0,0,128,15,224,31,0,60,32,59,240,23,224,3,128,13,224,53,0,122,128,126,128,246,160,238,176,246,56,107,20,28,140,27,192,39,192,123,96,241,225,224,193,128,57,96,124,240
        .byte 0,0,3,224,7,248,15,0,14,200,5,252,0,248,3,96,13,120,30,128,31,160,61,160,59,168,61,172,26,206,7,5,6,227,9,240,30,240,60,88,120,56,96,48,88,14,60,31
        .byte 0,0,0,248,1,254,3,192,3,178,1,127,0,62,0,216,3,94,7,160,7,232,15,104,14,234,15,107,134,179,65,193,193,184,2,124,7,188,15,22,30,14,24,12,150,3,207,7
        .byte 0,0,0,62,128,127,0,240,128,236,192,95,128,15,0,54,128,215,1,232,1,250,3,218,131,186,195,218,225,172,80,112,48,110,0,159,1,239,131,197,135,131,6,3,229,128,243,193
        .byte 0,0,128,15,224,31,0,60,32,59,240,23,224,3,128,13,224,53,0,122,128,126,128,246,160,238,176,246,56,107,20,28,140,27,192,39,192,123,96,241,225,224,193,128,57,96,124,240
        .byte 0,0,3,224,7,248,15,0,14,200,5,252,0,248,3,96,13,120,30,128,31,160,61,160,59,168,61,172,26,206,7,5,6,227,9,240,30,240,60,88,120,56,96,48,88,14,60,31
        .byte 0,0,0,248,1,254,3,192,3,178,1,127,0,62,0,216,3,94,7,160,7,232,15,104,14,234,15,107,134,179,65,193,193,184,2,124,7,188,15,22,30,14,24,12,150,3,207,7
        .byte 0,0,0,62,128,127,0,240,128,236,192,95,128,15,0,54,128,215,1,232,1,250,3,218,131,186,195,218,225,172,80,112,48,110,0,159,1,239,131,197,135,131,6,3,229,128,243,193
        .byte 0,0,128,15,224,31,0,60,32,59,240,23,224,3,128,13,224,53,0,122,128,126,128,246,160,238,176,246,56,107,20,28,140,27,192,39,192,123,96,241,225,224,193,128,57,96,124,240
        .byte 0,0,0,0,64,2,168,21,88,26,24,24,63,252,127,254,64,2,95,250,95,234,95,218,87,250,79,250,95,250,95,250,95,234,91,218,87,250,95,250,64,2,127,254,32,4,192,3
        .byte 0,0,0,0,144,0,106,5,150,6,6,6,15,255,159,255,144,0,151,254,151,250,151,246,149,254,147,254,151,254,151,254,151,250,150,246,149,254,151,254,144,0,159,255,8,1,240,0
        .byte 0,0,0,0,36,0,90,129,165,129,129,129,195,255,231,255,36,0,165,255,165,254,165,253,165,127,164,255,165,255,165,255,165,254,165,189,165,127,165,255,36,0,231,255,66,0,60,0
        .byte 0,0,0,0,9,0,86,160,105,96,96,96,240,255,249,255,9,0,233,127,169,127,105,127,233,95,233,63,233,127,233,127,169,127,105,111,233,95,233,127,9,0,249,255,16,128,15,0
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,176,1,120,5,248,5,188,21,220,53,188,115,88,160,224,199,96,15,144,15,120,26,60,28,30,12,6,112,26,248,60
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,172,0,94,1,126,1,111,5,119,13,111,28,214,40,56,49,216,3,228,3,222,6,143,135,7,131,1,156,6,62,15
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,235,128,23,128,95,192,91,193,93,195,91,135,53,10,14,12,118,0,249,128,247,193,163,225,193,96,192,167,1,207,131
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,192,122,224,5,224,23,240,22,112,87,240,214,97,205,130,131,131,29,64,62,224,61,240,104,120,112,24,48,105,192,243,224
        .byte 0,0,3,224,15,240,0,248,9,120,31,176,15,192,3,128,15,88,0,188,13,252,13,222,45,238,109,222,165,172,64,120,7,48,15,192,15,120,26,60,28,30,12,6,112,26,248,60
        .byte 0,0,0,248,3,252,0,62,2,94,7,236,3,240,0,224,3,214,0,47,3,127,131,119,139,123,155,119,41,107,16,30,1,204,3,240,3,222,6,143,135,7,131,1,156,6,62,15
        .byte 0,0,0,62,0,255,128,15,128,151,1,251,0,252,0,56,128,245,192,11,192,223,224,221,226,222,230,221,202,90,132,7,0,115,0,252,128,247,193,163,225,193,96,192,167,1,207,131
        .byte 0,0,128,15,192,63,224,3,224,37,192,126,0,63,0,14,96,61,240,2,240,55,120,55,184,183,121,183,178,150,225,1,192,28,0,63,224,61,240,104,120,112,24,48,105,192,243,224
        .byte 0,0,3,224,15,240,0,248,9,120,31,176,15,192,3,128,15,88,0,188,13,252,13,222,45,238,109,222,165,172,64,120,7,48,15,192,15,120,26,60,28,30,12,6,112,26,248,60
        .byte 0,0,0,248,3,252,0,62,2,94,7,236,3,240,0,224,3,214,0,47,3,127,131,119,139,123,155,119,41,107,16,30,1,204,3,240,3,222,6,143,135,7,131,1,156,6,62,15
        .byte 0,0,0,62,0,255,128,15,128,151,1,251,0,252,0,56,128,245,192,11,192,223,224,221,226,222,230,221,202,90,132,7,0,115,0,252,128,247,193,163,225,193,96,192,167,1,207,131
        .byte 0,0,128,15,192,63,224,3,224,37,192,126,0,63,0,14,96,61,240,2,240,55,120,55,184,183,121,183,178,150,225,1,192,28,0,63,224,61,240,104,120,112,24,48,105,192,243,224
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,176,1,120,5,248,5,188,21,220,53,188,115,88,160,224,199,96,15,144,15,120,26,60,28,30,12,6,112,26,248,60
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,172,0,94,1,126,1,111,5,119,13,111,28,214,40,56,49,216,3,228,3,222,6,143,135,7,131,1,156,6,62,15
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,235,128,23,128,95,192,91,193,93,195,91,135,53,10,14,12,118,0,249,128,247,193,163,225,193,96,192,167,1,207,131
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,192,122,224,5,224,23,240,22,112,87,240,214,97,205,130,131,131,29,64,62,224,61,240,104,120,112,24,48,105,192,243,224
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,176,1,120,5,248,5,188,21,220,53,188,115,88,160,224,199,96,15,144,15,120,26,60,28,30,12,6,112,26,248,60
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,172,0,94,1,126,1,111,5,119,13,111,28,214,40,56,49,216,3,228,3,222,6,143,135,7,131,1,156,6,62,15
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,235,128,23,128,95,192,91,193,93,195,91,135,53,10,14,12,118,0,249,128,247,193,163,225,193,96,192,167,1,207,131
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,192,122,224,5,224,23,240,22,112,87,240,214,97,205,130,131,131,29,64,62,224,61,240,104,120,112,24,48,105,192,243,224
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,176,1,120,5,248,5,188,21,220,53,188,115,88,160,224,199,96,15,144,15,120,26,60,28,30,12,6,112,26,248,60
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,172,0,94,1,126,1,111,5,119,13,111,28,214,40,56,49,216,3,228,3,222,6,143,135,7,131,1,156,6,62,15
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,235,128,23,128,95,192,91,193,93,195,91,135,53,10,14,12,118,0,249,128,247,193,163,225,193,96,192,167,1,207,131
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,192,122,224,5,224,23,240,22,112,87,240,214,97,205,130,131,131,29,64,62,224,61,240,104,120,112,24,48,105,192,243,224
        .byte 0,0,0,0,3,224,7,248,15,0,14,200,5,252,2,248,13,96,30,120,31,128,61,160,59,168,61,172,26,204,7,10,6,230,9,240,30,240,60,88,120,58,96,54,88,14,60,28
        .byte 0,0,0,0,0,248,1,254,3,192,3,178,1,127,0,190,3,88,7,158,7,224,15,104,14,234,15,107,6,179,129,194,129,185,2,124,7,188,15,22,158,14,152,13,150,3,15,7
        .byte 0,0,0,0,0,62,128,127,0,240,128,236,192,95,128,47,0,214,129,231,1,248,3,218,131,186,195,218,193,172,160,112,96,110,0,159,1,239,131,197,167,131,102,3,229,128,195,193
        .byte 0,0,0,0,128,15,224,31,0,60,32,59,240,23,224,11,128,53,224,121,0,126,128,246,160,238,176,246,48,107,40,28,152,27,192,39,192,123,96,241,233,224,217,128,57,96,112,240
        .byte 0,0,3,224,7,248,15,0,14,200,5,252,0,248,3,96,5,120,6,128,15,160,15,32,14,160,14,40,13,140,5,212,1,152,8,64,30,224,61,224,217,112,224,192,96,176,48,120
        .byte 0,0,0,248,1,254,3,192,3,178,1,127,0,62,0,216,1,94,1,160,3,232,3,200,3,168,3,138,3,99,1,117,0,102,2,16,7,184,15,120,54,92,56,48,24,44,12,30
        .byte 0,0,0,62,128,127,0,240,128,236,192,95,128,15,0,54,128,87,0,104,0,250,0,242,0,234,128,226,192,216,64,93,128,25,0,132,1,238,3,222,13,151,14,12,6,11,131,7
        .byte 0,0,128,15,224,31,0,60,32,59,240,23,224,3,128,13,224,21,0,26,128,62,128,60,128,58,160,56,48,54,80,23,96,6,0,33,128,123,128,247,195,101,3,131,193,130,224,193
        .byte 0,0,3,224,7,248,15,0,14,200,5,252,0,248,3,96,5,120,14,128,15,160,15,32,6,160,7,0,3,160,5,112,6,96,41,128,55,160,55,96,40,192,3,0,2,192,1,224
        .byte 0,0,0,248,1,254,3,192,3,178,1,127,0,62,0,216,1,94,3,160,3,232,3,200,1,168,1,192,0,232,1,92,1,152,10,96,13,232,13,216,10,48,0,192,0,176,0,120
        .byte 0,0,0,62,128,127,0,240,128,236,192,95,128,15,0,54,128,87,0,232,0,250,0,242,0,106,0,112,0,58,0,87,0,102,2,152,3,122,3,118,2,140,0,48,0,44,0,30
        .byte 0,0,128,15,224,31,0,60,32,59,240,23,224,3,128,13,224,21,0,58,128,62,128,60,128,26,0,28,128,14,192,21,128,25,0,166,128,222,128,221,0,163,0,12,0,11,128,7
        .byte 0,0,3,224,7,248,15,0,14,200,5,252,0,248,3,96,1,120,2,128,3,160,11,160,27,32,51,192,53,232,38,220,48,24,6,96,7,176,7,144,1,96,4,240,11,0,7,128
        .byte 0,0,0,248,1,254,3,192,3,178,1,127,0,62,0,216,0,94,0,160,0,232,2,232,6,200,12,240,13,122,9,183,12,6,1,152,1,236,1,228,0,88,1,60,2,192,1,224
        .byte 0,0,0,62,128,127,0,240,128,236,192,95,128,15,0,54,128,23,0,40,0,58,0,186,1,178,3,60,131,94,194,109,131,1,0,102,0,123,0,121,0,22,0,79,0,176,0,120
        .byte 0,0,128,15,224,31,0,60,32,59,240,23,224,3,128,13,224,5,0,10,128,14,128,46,128,108,0,207,160,215,112,155,96,192,128,25,192,30,64,30,128,5,192,19,0,44,0,30
        .byte 0,0,0,0,3,224,7,248,15,0,14,200,5,252,2,248,1,96,6,120,15,128,14,160,14,160,15,0,3,160,5,112,6,96,1,128,5,248,14,216,31,54,48,44,44,24,30,0
        .byte 0,0,0,0,0,248,1,254,3,192,3,178,1,127,0,190,0,88,1,158,3,224,3,168,3,168,3,192,0,232,1,92,1,152,0,96,1,126,3,182,135,205,12,11,11,6,7,128
        .byte 0,0,0,0,0,62,128,127,0,240,128,236,192,95,128,47,0,22,128,103,0,248,0,234,0,234,0,240,0,58,0,87,0,102,0,24,128,95,128,237,97,243,195,2,130,193,1,224
        .byte 0,0,0,0,128,15,224,31,0,60,32,59,240,23,224,11,128,5,224,25,0,62,128,58,128,58,0,60,128,14,192,21,128,25,0,6,224,23,96,59,216,124,176,192,96,176,0,120
        .byte 0,0,0,0,3,224,7,248,15,0,14,200,5,252,2,248,1,96,14,120,15,128,30,160,29,160,30,160,15,64,2,200,0,200,1,48,13,248,30,253,62,27,36,7,24,2,60,0
        .byte 0,0,0,0,0,248,1,254,3,192,3,178,1,127,0,190,0,88,3,158,3,224,7,168,7,104,7,168,3,208,0,178,0,50,0,76,3,126,71,191,207,134,201,1,134,0,15,0
        .byte 0,0,0,0,0,62,128,127,0,240,128,236,192,95,128,47,0,22,128,231,0,248,1,234,1,218,1,234,0,244,128,44,128,12,0,19,128,223,209,239,179,225,114,64,33,128,3,192
        .byte 0,0,0,0,128,15,224,31,0,60,32,59,240,23,224,11,128,5,224,57,0,62,128,122,128,118,128,122,0,61,32,11,32,3,192,4,224,55,244,123,108,248,28,144,8,96,0,240
        .byte 0,0,0,0,7,192,31,224,0,240,19,112,63,160,31,64,6,176,30,120,1,248,5,188,21,220,53,188,51,88,80,224,103,96,15,144,15,120,26,60,92,30,108,6,112,26,56,60
        .byte 0,0,0,0,1,240,7,248,0,60,4,220,15,232,7,208,1,172,7,158,0,126,1,111,5,119,13,111,12,214,20,56,25,216,3,228,3,222,6,143,151,7,155,1,156,6,14,15
        .byte 0,0,0,0,0,124,1,254,0,15,1,55,3,250,1,244,0,107,129,231,128,31,192,91,193,93,195,91,131,53,5,14,6,118,0,249,128,247,193,163,229,193,102,192,167,1,195,131
        .byte 0,0,0,0,0,31,128,127,192,3,192,77,128,254,0,125,192,26,224,121,224,7,240,22,112,87,240,214,96,205,129,67,129,157,64,62,224,61,240,104,121,112,25,176,105,192,240,224
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,160,1,96,5,240,4,240,5,112,20,112,49,176,43,160,25,128,2,16,7,120,7,188,14,155,3,7,13,6,30,12
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,168,0,88,1,124,1,60,1,92,5,28,12,108,10,232,6,96,0,132,1,222,1,239,195,166,192,193,131,65,7,131
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,234,0,22,0,95,0,79,0,87,1,71,3,27,2,186,1,152,0,33,128,119,192,123,176,233,112,48,96,208,193,224
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,128,122,128,5,192,23,192,19,192,21,192,81,192,198,128,174,0,102,64,8,224,29,240,30,108,58,28,12,24,52,48,120
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,160,1,112,5,240,4,240,5,96,0,224,5,192,14,160,6,96,1,148,5,236,6,236,3,20,0,192,3,64,7,128
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,168,0,92,1,124,1,60,1,88,0,56,1,112,3,168,1,152,0,101,1,123,1,187,0,197,0,48,0,208,1,224
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,234,0,23,0,95,0,79,0,86,0,14,0,92,0,234,0,102,64,25,192,94,192,110,64,49,0,12,0,52,0,120
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,128,122,192,5,192,23,192,19,128,21,128,3,0,23,128,58,128,25,80,6,176,23,176,27,80,12,0,3,0,13,0,30
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,128,1,64,5,192,5,208,4,216,3,204,23,172,59,100,24,12,6,96,13,224,9,224,6,128,15,32,0,208,1,224
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,160,0,80,1,112,1,116,1,54,0,243,5,235,14,217,6,3,1,152,3,120,2,120,1,160,3,200,0,52,0,120
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,232,0,20,0,92,0,93,128,77,192,60,193,122,67,182,193,128,0,102,0,222,0,158,0,104,0,242,0,13,0,30
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,0,122,0,5,0,23,64,23,96,19,48,15,176,94,144,237,48,96,128,25,128,55,128,39,0,26,128,60,64,3,128,7
        .byte 0,0,0,0,7,192,31,224,0,240,19,112,63,160,31,64,6,128,30,96,1,240,5,112,5,112,0,240,5,192,14,160,6,96,1,128,31,160,27,112,108,248,52,12,24,52,0,120
        .byte 0,0,0,0,1,240,7,248,0,60,4,220,15,232,7,208,1,160,7,152,0,124,1,92,1,92,0,60,1,112,3,168,1,152,0,96,7,232,6,220,27,62,13,3,6,13,0,30
        .byte 0,0,0,0,0,124,1,254,0,15,1,55,3,250,1,244,0,104,1,230,0,31,0,87,0,87,0,15,0,92,0,234,0,102,0,24,1,250,1,183,134,207,195,64,65,131,128,7
        .byte 0,0,0,0,0,31,128,127,192,3,192,77,128,254,0,125,0,26,128,121,192,7,192,21,192,21,192,3,0,23,128,58,128,25,0,6,128,126,192,109,225,179,48,208,208,96,224,1
        .byte 0,0,0,0,7,192,31,224,0,240,19,112,63,160,31,64,6,128,30,112,1,240,5,120,5,184,5,120,2,240,19,64,19,0,12,128,31,176,191,120,216,124,224,36,64,24,0,60
        .byte 0,0,0,0,1,240,7,248,0,60,4,220,15,232,7,208,1,160,7,156,0,124,1,94,1,110,1,94,0,188,4,208,4,192,3,32,7,236,47,222,54,31,56,9,16,6,0,15
        .byte 0,0,0,0,0,124,1,254,0,15,1,55,3,250,1,244,0,104,1,231,0,31,128,87,128,91,128,87,0,47,1,52,1,48,0,200,1,251,139,247,205,135,78,2,132,1,192,3
        .byte 0,0,0,0,0,31,128,127,192,3,192,77,128,254,0,125,0,26,192,121,192,7,224,21,224,22,224,21,192,11,0,77,0,76,0,50,192,126,226,253,243,97,147,128,97,0,240,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,64,1,0,2,0,1,0,1,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,16,0,64,0,128,0,64,0,64,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,68,0,16,0,32,0,16,0,16,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,0,4,0,8,0,4,0,4,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0,0,0,128,0,0,2,0,0,16,0,128,8,0,2,32,0,128,8,64,0,0,1,0,0,128,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,0,0,0,32,0,0,0,128,0,4,0,32,2,0,0,136,0,32,2,16,0,0,0,64,0,32,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,8,0,0,0,32,0,1,0,8,0,128,0,34,0,8,0,132,0,0,0,16,0,8,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,0,0,0,2,0,0,0,8,64,0,0,2,0,32,128,8,0,2,0,33,0,0,0,4,0,2,0,0,0,0
        .byte 0,0,0,0,0,0,0,128,0,0,1,0,0,0,0,32,2,0,0,0,8,16,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,32,0,0,0,64,0,0,0,8,0,128,0,0,2,4,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,8,0,0,0,16,0,0,0,2,0,32,0,0,0,129,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,2,0,0,0,4,0,0,128,0,0,8,0,0,64,32,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,160,1,112,127,240,63,208,64,48,5,240,59,224,120,224,151,0,111,128,239,0,26,0,28,0,12,0,112,0,248,0
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,168,0,92,31,252,15,244,16,12,1,124,14,248,30,56,37,192,27,224,59,192,6,128,7,0,3,0,28,0,62,0
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,234,0,23,7,255,3,253,4,3,0,95,3,190,7,142,9,112,6,248,14,240,1,160,1,192,0,192,7,0,15,128
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,128,122,192,5,193,255,64,255,193,0,192,23,128,239,129,227,2,92,1,190,3,188,0,104,0,112,0,48,1,192,3,224
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,70,160,57,112,127,240,7,208,0,48,5,240,27,224,56,224,55,0,119,128,7,128,111,0,238,0,4,0,56,0,124,0
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,17,168,14,92,31,252,1,244,0,12,1,124,6,248,14,56,13,192,29,224,1,224,27,192,59,128,1,0,14,0,31,0
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,4,106,3,151,7,255,0,125,0,3,0,95,1,190,3,142,3,112,7,120,0,120,6,240,14,224,0,64,3,128,7,192
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,129,26,192,229,193,255,64,31,192,0,192,23,128,111,128,227,0,220,1,222,0,30,1,188,3,184,0,16,0,224,1,240
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,159,0,70,192,248,160,127,112,31,240,3,144,192,112,133,240,15,224,31,224,29,224,94,128,14,0,96,0,238,0,31,0,0,0,0,0
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,39,192,17,176,62,40,31,220,7,252,0,228,48,28,33,124,3,248,7,248,7,120,23,160,3,128,24,0,59,128,7,192,0,0,0,0
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,9,240,4,108,15,138,7,247,1,255,0,57,12,7,8,95,0,254,1,254,1,222,5,232,0,224,6,0,14,224,1,240,0,0,0,0
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,2,124,1,27,131,226,193,253,192,127,64,14,195,1,194,23,128,63,128,127,128,119,1,122,0,56,1,128,3,184,0,124,0,0,0,0
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,28,160,129,112,127,240,255,144,7,112,48,240,127,224,127,224,143,192,119,128,248,0,0,0,48,0,112,0,0,0,0,0
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,40,32,92,31,252,63,228,1,220,12,60,31,248,31,248,35,240,29,224,62,0,0,0,12,0,28,0,0,0,0,0
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,202,8,23,7,255,15,249,0,119,3,15,7,254,7,254,8,252,7,120,15,128,0,0,3,0,7,0,0,0,0,0
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,128,114,194,5,193,255,67,254,192,29,192,195,129,255,129,255,2,63,1,222,3,224,0,0,0,192,1,192,0,0,0,0
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,158,160,193,112,3,240,63,208,255,176,67,112,184,224,126,224,253,128,14,128,119,0,250,0,0,0,16,0,96,0,224,0
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,39,168,48,92,0,252,15,244,63,236,16,220,46,56,31,184,63,96,3,160,29,192,62,128,0,0,4,0,24,0,56,0
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,9,234,12,23,0,63,3,253,15,251,4,55,11,142,7,238,15,216,0,232,7,112,15,160,0,0,1,0,6,0,14,0
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,130,122,195,5,192,15,64,255,195,254,193,13,130,227,129,251,3,246,0,58,1,220,3,232,0,0,0,64,1,128,3,128
        .byte 0,0,7,192,31,224,0,240,19,112,63,160,31,0,6,192,30,160,1,112,3,240,15,208,126,48,33,240,95,224,63,224,123,224,125,192,13,128,114,0,248,0,0,0,0,0,0,0
        .byte 0,0,1,240,7,248,0,60,4,220,15,232,7,192,1,176,7,168,0,92,0,252,3,244,31,140,8,124,23,248,15,248,30,248,31,112,3,96,28,128,62,0,0,0,0,0,0,0
        .byte 0,0,0,124,1,254,0,15,1,55,3,250,1,240,0,108,1,234,0,23,0,63,0,253,7,227,2,31,5,254,3,254,7,190,7,220,0,216,7,32,15,128,0,0,0,0,0,0
        .byte 0,0,0,31,128,127,192,3,192,77,128,254,0,124,0,27,128,122,192,5,192,15,64,63,193,248,192,135,129,127,128,255,129,239,1,247,0,54,1,200,3,224,0,0,0,0,0,0
        .byte 48,24,32,8,51,24,34,136,50,152,35,8,48,24,33,8,50,152,34,136,49,24,32,8,49,24,34,136,50,152,33,8,48,24,35,8,50,152,35,8,50,152,32,8,48,24,32,8
        .byte 12,6,8,2,12,198,8,162,12,166,8,194,12,6,8,66,12,166,8,162,12,70,8,2,12,70,8,162,12,166,8,66,12,6,8,194,12,166,8,194,12,166,8,2,12,6,8,2
        .byte 131,1,130,0,131,49,130,40,131,41,130,48,131,1,130,16,131,41,130,40,131,17,130,0,131,17,130,40,131,41,130,16,131,1,130,48,131,41,130,48,131,41,130,0,131,1,130,0
        .byte 96,192,32,128,96,204,32,138,96,202,32,140,96,192,32,132,96,202,32,138,96,196,32,128,96,196,32,138,96,202,32,132,96,192,32,140,96,202,32,140,96,202,32,128,96,192,32,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,66,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,40,0,16,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,10,0,4,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,2,128,1,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,160,0,64,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,68,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,17,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,64,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,16,0,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,16,0,40,0,16,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,4,0,10,0,4,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,128,2,0,1,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,64,0,160,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,127,254,64,2,95,250,80,10,84,10,80,10,81,202,83,42,82,170,82,42,81,202,80,10,80,10,81,202,82,170,82,170,82,42,81,202,80,10,95,250,95,250
        .byte 0,0,0,0,0,0,159,255,144,0,151,254,148,2,149,2,148,2,148,114,148,202,148,170,148,138,148,114,148,2,148,2,148,114,148,170,148,170,148,138,148,114,148,2,151,254,151,254
        .byte 0,0,0,0,0,0,231,255,36,0,165,255,165,0,165,64,165,0,165,28,165,50,165,42,165,34,165,28,165,0,165,0,165,28,165,42,165,42,165,34,165,28,165,0,165,255,165,255
        .byte 0,0,0,0,0,0,249,255,9,0,233,127,41,64,41,80,41,64,41,71,169,76,169,74,169,72,41,71,41,64,41,64,41,71,169,74,169,74,169,72,41,71,41,64,233,127,233,127
        .byte 0,0,0,0,0,0,127,254,64,2,95,250,80,10,84,10,80,10,81,202,82,106,82,170,82,42,81,202,80,10,80,10,81,202,82,42,82,234,82,42,81,202,80,10,95,250,95,250
        .byte 0,0,0,0,0,0,159,255,144,0,151,254,148,2,149,2,148,2,148,114,148,154,148,170,148,138,148,114,148,2,148,2,148,114,148,138,148,186,148,138,148,114,148,2,151,254,151,254
        .byte 0,0,0,0,0,0,231,255,36,0,165,255,165,0,165,64,165,0,165,28,165,38,165,42,165,34,165,28,165,0,165,0,165,28,165,34,165,46,165,34,165,28,165,0,165,255,165,255
        .byte 0,0,0,0,0,0,249,255,9,0,233,127,41,64,41,80,41,64,41,71,169,73,169,74,169,72,41,71,41,64,41,64,41,71,169,72,169,75,169,72,41,71,41,64,233,127,233,127
        .byte 0,0,0,0,0,0,127,254,64,2,95,250,80,10,84,10,80,10,81,202,82,42,82,170,82,106,81,202,80,10,80,10,81,202,82,42,82,170,82,170,81,202,80,10,95,250,95,250
        .byte 0,0,0,0,0,0,159,255,144,0,151,254,148,2,149,2,148,2,148,114,148,138,148,170,148,154,148,114,148,2,148,2,148,114,148,138,148,170,148,170,148,114,148,2,151,254,151,254
        .byte 0,0,0,0,0,0,231,255,36,0,165,255,165,0,165,64,165,0,165,28,165,34,165,42,165,38,165,28,165,0,165,0,165,28,165,34,165,42,165,42,165,28,165,0,165,255,165,255
        .byte 0,0,0,0,0,0,249,255,9,0,233,127,41,64,41,80,41,64,41,71,169,72,169,74,169,73,41,71,41,64,41,64,41,71,169,72,169,74,169,74,41,71,41,64,233,127,233,127
        .byte 0,0,0,0,0,0,127,254,64,2,95,250,80,10,84,10,80,10,81,202,82,42,82,170,83,42,81,202,80,10,80,10,81,202,82,42,83,170,82,42,81,202,80,10,95,250,95,250
        .byte 0,0,0,0,0,0,159,255,144,0,151,254,148,2,149,2,148,2,148,114,148,138,148,170,148,202,148,114,148,2,148,2,148,114,148,138,148,234,148,138,148,114,148,2,151,254,151,254
        .byte 0,0,0,0,0,0,231,255,36,0,165,255,165,0,165,64,165,0,165,28,165,34,165,42,165,50,165,28,165,0,165,0,165,28,165,34,165,58,165,34,165,28,165,0,165,255,165,255
        .byte 0,0,0,0,0,0,249,255,9,0,233,127,41,64,41,80,41,64,41,71,169,72,169,74,169,76,41,71,41,64,41,64,41,71,169,72,169,78,169,72,41,71,41,64,233,127,233,127
        .byte 0,0,0,16,0,56,0,40,0,16,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,128,1,196,1,192,3,160,3,160,1,192,0,128,0,0
        .byte 0,0,0,4,0,14,0,10,0,4,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,32,0,113,0,112,0,232,0,232,0,112,0,32,0,0
        .byte 0,0,0,1,128,3,128,2,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,8,64,28,0,28,0,58,0,58,0,28,0,8,0,0
        .byte 0,0,64,0,224,0,160,0,64,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,2,16,7,0,7,128,14,128,14,0,7,0,2,0,0
        .byte 0,16,0,56,0,116,0,40,0,16,0,0,0,0,16,0,56,0,40,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,128,0,0
        .byte 0,4,0,14,0,29,0,10,0,4,0,0,0,0,4,0,14,0,10,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,32,0,0
        .byte 0,1,128,3,64,7,128,2,0,1,0,0,0,0,1,0,3,128,2,128,1,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,8,0,0
        .byte 64,0,224,0,208,1,160,0,64,0,0,0,0,0,0,64,0,224,0,160,0,64,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,2,0,0
        .byte 0,0,0,16,0,56,0,40,0,16,16,0,56,0,124,0,116,0,40,0,16,0,0,0,0,0,0,0,0,4,0,14,0,10,0,4,0,0,0,128,1,192,1,64,0,128,0,0
        .byte 0,0,0,4,0,14,0,10,0,4,4,0,14,0,31,0,29,0,10,0,4,0,0,0,0,0,0,0,0,1,128,3,128,2,0,1,0,0,0,32,0,112,0,80,0,32,0,0
        .byte 0,0,0,1,128,3,128,2,0,1,1,0,3,128,7,192,7,64,2,128,1,0,0,0,0,0,0,0,64,0,224,0,160,0,64,0,0,0,0,8,0,28,0,20,0,8,0,0
        .byte 0,0,64,0,224,0,160,0,64,0,0,64,0,224,1,240,1,208,0,160,0,64,0,0,0,0,0,0,16,0,56,0,40,0,16,0,0,0,0,2,0,7,0,5,0,2,0,0
        .byte 0,0,0,0,0,0,16,0,56,16,56,0,124,0,124,0,116,0,40,0,16,0,0,0,0,4,0,14,0,31,0,29,0,10,0,4,0,128,1,192,3,160,1,64,0,128,0,0
        .byte 0,0,0,0,0,0,4,0,14,4,14,0,31,0,31,0,29,0,10,0,4,0,0,0,0,1,128,3,192,7,64,7,128,2,0,1,0,32,0,112,0,232,0,80,0,32,0,0
        .byte 0,0,0,0,0,0,1,0,3,129,3,128,7,192,7,192,7,64,2,128,1,0,0,0,64,0,224,0,240,1,208,1,160,0,64,0,0,8,0,28,0,58,0,20,0,8,0,0
        .byte 0,0,0,0,0,0,0,64,64,224,0,224,1,240,1,240,1,208,0,160,0,64,0,0,16,0,56,0,124,0,116,0,40,0,16,0,0,2,0,7,128,14,0,5,0,2,0,0
        .byte 57,156,125,126,57,156,0,0,1,128,1,128,0,0,3,64,3,64,3,64,3,64,0,0,63,228,53,100,55,100,53,100,63,228,0,0,31,232,0,0,8,16,8,16,48,12,0,0
        .byte 14,103,159,95,14,103,0,0,0,96,0,96,0,0,0,208,0,208,0,208,0,208,0,0,15,249,13,89,13,217,13,89,15,249,0,0,7,250,0,0,2,4,2,4,12,3,0,0
        .byte 195,153,231,215,195,153,0,0,0,24,0,24,0,0,0,52,0,52,0,52,0,52,0,0,67,254,67,86,67,118,67,86,67,254,0,0,129,254,0,0,0,129,0,129,195,0,0,0
        .byte 112,230,249,245,112,230,0,0,0,6,0,6,0,0,0,13,0,13,0,13,0,13,0,0,144,255,144,213,144,221,144,213,144,255,0,0,160,127,0,0,64,32,64,32,48,192,0,0
        .byte 5,160,13,176,5,160,0,0,1,128,1,128,0,0,3,64,3,64,3,64,3,64,0,0,63,228,53,100,55,100,53,100,63,228,0,0,31,232,0,0,8,16,8,16,48,12,0,0
        .byte 1,104,3,108,1,104,0,0,0,96,0,96,0,0,0,208,0,208,0,208,0,208,0,0,15,249,13,89,13,217,13,89,15,249,0,0,7,250,0,0,2,4,2,4,12,3,0,0
        .byte 0,90,0,219,0,90,0,0,0,24,0,24,0,0,0,52,0,52,0,52,0,52,0,0,67,254,67,86,67,118,67,86,67,254,0,0,129,254,0,0,0,129,0,129,195,0,0,0
        .byte 128,22,192,54,128,22,0,0,0,6,0,6,0,0,0,13,0,13,0,13,0,13,0,0,144,255,144,213,144,221,144,213,144,255,0,0,160,127,0,0,64,32,64,32,48,192,0,0
        .byte 1,128,1,128,1,128,0,0,1,128,1,128,0,0,3,64,3,64,3,64,3,64,0,0,63,228,53,100,55,100,53,100,63,228,0,0,31,232,0,0,8,16,8,16,48,12,0,0
        .byte 0,96,0,96,0,96,0,0,0,96,0,96,0,0,0,208,0,208,0,208,0,208,0,0,15,249,13,89,13,217,13,89,15,249,0,0,7,250,0,0,2,4,2,4,12,3,0,0
        .byte 0,24,0,24,0,24,0,0,0,24,0,24,0,0,0,52,0,52,0,52,0,52,0,0,67,254,67,86,67,118,67,86,67,254,0,0,129,254,0,0,0,129,0,129,195,0,0,0
        .byte 0,6,0,6,0,6,0,0,0,6,0,6,0,0,0,13,0,13,0,13,0,13,0,0,144,255,144,213,144,221,144,213,144,255,0,0,160,127,0,0,64,32,64,32,48,192,0,0
        .byte 5,160,14,176,5,160,0,0,1,128,1,128,0,0,3,64,3,64,3,64,3,64,0,0,63,228,53,100,55,100,53,100,63,228,0,0,31,232,0,0,8,16,8,16,48,12,0,0
        .byte 1,104,3,172,1,104,0,0,0,96,0,96,0,0,0,208,0,208,0,208,0,208,0,0,15,249,13,89,13,217,13,89,15,249,0,0,7,250,0,0,2,4,2,4,12,3,0,0
        .byte 0,90,0,235,0,90,0,0,0,24,0,24,0,0,0,52,0,52,0,52,0,52,0,0,67,254,67,86,67,118,67,86,67,254,0,0,129,254,0,0,0,129,0,129,195,0,0,0
        .byte 128,22,192,58,128,22,0,0,0,6,0,6,0,0,0,13,0,13,0,13,0,13,0,0,144,255,144,213,144,221,144,213,144,255,0,0,160,127,0,0,64,32,64,32,48,192,0,0
        .byte 1,0,2,0,2,0,5,0,4,128,12,128,10,64,10,64,18,160,32,160,32,160,16,144,17,16,17,16,9,40,16,40,16,68,8,68,8,68,8,68,20,170,36,168,34,40,34,8
        .byte 0,64,0,128,0,128,1,64,1,32,3,32,2,144,2,144,4,168,8,40,8,40,4,36,4,68,4,68,2,74,4,10,4,17,2,17,2,17,2,17,133,42,9,42,8,138,8,130
        .byte 0,16,0,32,0,32,0,80,0,72,0,200,0,164,0,164,1,42,2,10,2,10,1,9,1,17,1,17,128,146,129,2,65,4,64,132,64,132,64,132,161,74,130,74,130,34,130,32
        .byte 0,4,0,8,0,8,0,20,0,18,0,50,0,41,0,41,128,74,128,130,128,130,64,66,64,68,64,68,160,36,160,64,16,65,16,33,16,33,16,33,168,82,160,146,160,136,32,136
        .byte 1,0,0,128,0,128,1,64,1,32,2,32,2,16,2,16,5,16,5,16,1,40,2,40,2,36,4,68,4,74,4,74,4,136,8,132,20,196,36,66,36,32,34,80,64,128,0,132
        .byte 0,64,0,32,0,32,0,80,0,72,0,136,0,132,0,132,1,68,1,68,0,74,0,138,0,137,1,17,129,18,129,18,1,34,2,33,5,49,137,16,9,8,8,148,16,32,0,33
        .byte 0,16,0,8,0,8,0,20,0,18,0,34,0,33,0,33,0,81,0,81,128,18,128,34,64,34,64,68,160,68,160,68,128,72,64,136,65,76,34,68,2,66,2,37,4,8,64,8
        .byte 0,4,0,2,0,2,0,5,128,4,128,8,64,8,64,8,64,20,64,20,160,4,160,8,144,8,16,17,40,17,40,17,32,18,16,34,16,83,8,145,128,144,64,137,1,2,16,2
frmlst:
        .byte 0,6
        .byte 6,1
        .byte 7,6
        .byte 13,6
        .byte 19,6
        .byte 25,4
        .byte 29,6
        .byte 35,1
        .byte 36,4
        .byte 40,4
        .byte 44,4
        .byte 48,4
        .byte 52,2,54,0
scdat:
        .word 339,398,356,286,301,298,308,334,358,382,379,354,331,346,309,310,357,365,339,332,328,331,329,305,309,337
        .byte 255,0,9,40,21,255,1,10,39,41,30,39,255,0,12,40,21,255,44,10,39,41,30,39,255,0,12,40,21,44,44,44,23
        .byte 24,25,29,44,44,44,39,41,30,39,255,0,12,40,21,255,44,10,39,41,30,39,255,0,12,40,21,255,1,10,39,41,30
        .byte 39,255,0,12,40,21,1,1,1,23,24,25,28,1,1,1,39,41,30,39,255,0,12,40,21,255,1,10,39,41,30,39,255,0,12
        .byte 40,21,255,44,10,39,41,30,39,255,0,12,40,21,44,44,44,23,24,25,27,44,44,44,39,41,30,39,0,0,0,255,31,4
        .byte 255,0,5,40,21,255,44,10,39,41,30,39,0,0,0,255,31,4,255,0,5,40,21,255,1,10,39,41,30,39,0,0,0
        .byte 255,31,4,255,0,5,40,21,1,1,1,23,24,25,26,1,1,1,39,41,30,39,255,31,7,255,0,5,40,21,255,1,10
        .byte 39,41,30,39,31,31,31,255,42,9,40,21,255,44,10,39,41,30,39,31,31,31,255,43,10,21,44,44,44,23,24,25,22
        .byte 44,44,44,39,41,30,39,31,31,31,255,43,10,21,255,44,10,39,41,30,39,42,42,42,12,7,13,255,12,4,7,13,255,12,4
        .byte 7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82
        .byte 6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10
        .byte 11,255,0,34
        .byte 21,21,72,70,38,70,71,38,69,55,69,55,54,38,54,55,69,55,34,32,33,32,14,15,16,14,15,16,32,32,33,255,32,4
        .byte 38,255,32,5,36,70,70,73,54,38,54,55,69,17,18,19,17,18,19,21,21,21,54,45,21,74,37,32,32,32,33,255,32,6
        .byte 36,55,54,75,54,55,54,55,45,55,21,21,21,0,0,21,72,70,73,55,0,0,54,55,54,55,0,0,54,55,0,54,55
        .byte 34,32,32,36,54,255,20,6,255,0,12,60,61,50,51,0,38,54,55,54,55,255,0,17,62,58,59,48,49,34,32,32,33
        .byte 32,32,255,0,17,63,56,57,46,47,38,45,54,55,54,55,255,0,8,65,255,0,8,63,255,32,4,38,255,32,5,255,66,7
        .byte 67,64,52,53,34,255,32,8,33,32,38,36,16,14,15,16,44,255,1,6,39,64,52,53,37,32,33,32,32,32,45,255,32,4
        .byte 36,18,19,17,18,19,44,255,1,6,39,64,52,53,55,54,37,32,32,32,36,54,55,54,55,54,55,0,0,140,141,44,255,1,6
        .byte 39,64,52,53,0,0,54,55,54,55,54,55,0,0,54,55,255,0,5,44,255,1,6,39,64,52,53,255,0,17,44,255,1,6
        .byte 39,64,52,53,255,0,17,44,255,1,6,39,64,52,53,255,0,17,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15
        .byte 16,14,15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8
        .byte 2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5
        .byte 83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,39,0,38,0,45,0,0,75,0,0,54,55,255,0,7,54,55,54,55,54,74,40,44,44,39,32,32,32
        .byte 36,255,0,6,54,55,255,0,4,62,255,70,6,73,40,44,44,39,0,45,0,0,0,87,0,0,87,0,0,87,255,0,5
        .byte 87,255,0,6,40,44,44,39,0,38,0,0,0,87,77,77,87,0,0,87,255,77,5,87,0,54,55,0,0,0,40,44,44
        .byte 39,70,38,70,70,70,87,81,81,87,70,70,87,255,81,5,87,255,0,4,50,51,40,44,44,39,32,36,54,55,0,84,86,86
        .byte 85,0,0,84,255,86,5,85,255,0,4,48,49,40,44,44,39,255,0,6,56,57,0,0,0,56,57,0,0,0,56,57,255,0,4
        .byte 46,47,40,44,44,39,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,40,44,44
        .byte 39,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,40,44,44,39,255,0,24,40
        .byte 44,20,20,255,0,24,140,141,255,0,84,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16
        .byte 14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0
        .byte 2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6
        .byte 9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 44,1,1,69,255,1,5,69,1,69,255,1,14,40,44,44,1,1,69,255,1,4,88,70,70,73,39,55,255,0,5,89,0
        .byte 54,55,0,0,0,40,44,44,1,1,72,255,70,4,69,73,1,1,39,0,0,0,54,55,91,90,91,92,54,0,0,0,40
        .byte 44,44,255,1,7,69,1,1,1,39,255,0,4,54,92,91,92,54,55,0,0,0,40,44,255,20,13,255,0,6,91,55,0
        .byte 50,51,63,63,40,44,255,0,22,46,47,97,98,40,44,255,0,22,97,98,95,96,40,44,255,0,22,93,94,93,94,40,44
        .byte 255,66,26,40,44,255,1,23,88,70,70,40,44,255,0,7,89,255,0,8,89,255,0,6,75,0,0,40,44,255,0,7,99
        .byte 255,0,8,99,255,0,9,140,141,255,0,84,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15
        .byte 16,14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4
        .byte 0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6
        .byte 9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 255,106,4,255,104,22,40,44,255,106,4,255,105,22,40,44,255,106,4,255,0,9,89,255,0,12,40,44,255,106,4,255,0,9
        .byte 90,255,0,8,111,117,117,112,40,44,255,106,5,255,0,8,121,255,0,4,63,111,117,117,115,120,109,110,40,44,255,0,12
        .byte 121,121,121,0,0,97,98,115,119,111,112,119,107,108,40,44,255,0,12,121,121,121,0,0,95,96,115,120,115,109,110,109,110
        .byte 40,44,255,0,8,65,0,0,255,121,5,0,93,94,113,118,113,107,108,107,108,40,44,255,104,8,64,52,53,255,104,15,40
        .byte 1,255,105,8,64,52,53,255,105,15,40,1,106,0,0,0,102,100,101,0,64,52,53,255,0,15,40,1,106,97,98,0,103,103,103
        .byte 0,64,52,53,255,0,15,140,141,106,95,96,255,0,5,64,52,53,255,0,17,106,95,96,117,112,0,0,0,64,52,53,255,0,17
        .byte 106,93,94,118,114,0,0,0,64,52,53,255,0,17,255,104,28,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12
        .byte 7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 138,0,0,0,133,255,81,5,134,0,0,133,255,81,5,134,255,0,7,138,138,0,0,0,133,81,81,81,0,81,134,0,139
        .byte 133,255,81,5,134,0,0,0,139,0,0,0,138,138,0,139,0,133,0,0,81,0,81,134,0,0,133,81,81,0,81,81,134
        .byte 255,0,7,138,138,0,0,0,133,0,0,136,0,0,134,0,0,133,136,0,0,136,0,134,255,0,7,138,138,0,0,0,255,135,7
        .byte 0,0,255,135,7,0,0,127,128,0,0,0,138,138,255,0,19,139,0,125,126,0,22,0,138,138,255,0,6,139,255,0,14
        .byte 123,124,124,123,0,138,138,255,0,11,65,255,0,9,122,0,0,122,0,138,138,255,137,11,64,52,53,255,137,13,255,138,12
        .byte 64,52,53,255,138,19,255,0,6,68,52,53,255,0,11,140,141,255,20,6,255,0,6,68,52,53,255,0,25,68,52,53,255,0,25
        .byte 68,52,53,255,0,25,68,52,53,255,0,13,255,137,28,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13
        .byte 255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 255,32,4,33,255,32,10,33,32,32,38,36,69,55,38,69,54,38,54,55,55,54,55,54,55,74,55,54,72,70,70,45,70
        .byte 74,74,70,74,70,38,70,73,54,37,32,33,255,32,5,33,255,32,4,33,32,32,32,36,54,75,75,55,69,55,37,32,33
        .byte 255,32,4,36,255,0,5,54,55,72,255,70,4,73,54,55,0,0,0,69,54,55,54,55,0,0,69,255,0,20,72,255,70,6
        .byte 73,255,0,88,14,15,16,32,32,33,255,32,7,33,255,32,5,33,32,32,35,255,66,5,17,18,19,255,0,16,60,61,0
        .byte 38,1,44,44,1,1,255,0,19,58,59,0,38,1,44,44,1,1,255,0,19,56,57,0,38,1,44,44,1,1,255,0,19
        .byte 60,61,0,38,1,44,44,1,1,255,0,19,58,59,0,38,1,44,44,1,1,255,0,19,56,57,0,38,1,44,44,1,1
        .byte 16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4
        .byte 7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
        .byte 14,15,16,55,69,55,54,34,32,32,38,255,32,7,36,69,74,34,255,32,6,17,18,19,70,69,70,70,38,70,74,38,70,70,70
        .byte 45,70,45,70,70,73,75,38,55,54,74,0,45,0,14,15,16,255,32,4,36,75,69,38,70,75,75,37,255,32,6,36,54
        .byte 55,69,0,37,32,17,18,19,54,69,54,55,0,0,75,255,0,5,32,36,54,75,54,55,0,0,0,75,0,0,0,39,60
        .byte 61,0,75,0,54,55,255,0,20,39,58,59,255,0,25,39,48,49,50,51,255,0,23,39,56,57,46,47,0,0,0,65,255,0,19
        .byte 14,15,16,14,15,16,14,15,64,52,53,14,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,17,18,19,17,18
        .byte 19,17,18,64,52,53,17,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,255,0,8,68,52,53,255,0,25,68
        .byte 52,53,255,0,25,68,52,53,255,0,25,68,52,53,255,0,25,68,52,53,255,0,17,16,14,15,16,14,15,16,14,15,16
        .byte 14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12
        .byte 7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 1,39,32,32,32,80,81,77,79,32,33,80,81,77,79,32,32,32,80,81,77,79,32,32,32,33,32,32,1,39,55,72,70
        .byte 80,81,77,79,74,55,80,81,77,79,70,71,54,80,81,77,79,74,54,45,54,40,69,1,39,33,32,32,80,81,77,79,69
        .byte 88,76,81,77,78,74,69,55,80,81,77,79,69,55,37,32,33,32,1,39,70,70,70,80,81,77,79,73,69,54,74,74,55
        .byte 75,69,0,80,81,77,79,69,54,55,54,40,69,1,39,54,55,54,76,81,77,78,0,69,55,75,75,54,55,69,55,80,81
        .byte 77,79,255,70,5,73,1,39,0,54,55,0,74,74,0,0,69,255,0,5,69,0,76,81,77,78,69,0,0,0,40,44,1
        .byte 39,255,0,4,72,70,70,70,73,255,0,5,72,255,70,5,73,0,0,0,40,44,1,39,255,0,5,69,255,0,11,74,74
        .byte 255,0,5,40,44,1,1,66,66,67,0,0,75,255,0,11,69,75,255,0,4,14,15,16,255,1,4,39,255,0,14,75,255,0,5
        .byte 17,18,19,255,1,4,39,255,0,23,255,20,5,255,0,107,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14
        .byte 15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2
        .byte 3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83
        .byte 6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 44,39,255,32,4,33,32,32,32,36,69,69,54,69,69,69,54,80,81,77,79,32,33,32,32,40,44,44,39,0,0,54,55
        .byte 54,55,54,74,54,69,69,55,69,72,70,70,80,81,77,79,74,70,45,70,40,44,44,39,255,70,7,73,55,75,72,70,70,70
        .byte 73,0,76,81,77,78,75,54,37,32,40,44,44,39,255,0,6,54,55,54,55,54,55,74,255,0,5,54,55,54,55,60,61
        .byte 40,44,44,39,255,0,12,75,255,0,9,58,59,40,44,44,39,50,51,255,0,20,60,61,40,44,44,39,48,49,255,0,19
        .byte 62,58,59,40,44,44,39,46,47,0,0,65,255,0,16,63,56,57,40,44,1,39,16,14,15,16,64,52,53,14,15,15,16
        .byte 14,15,16,14,15,16,14,15,16,14,15,16,14,40,44,44,39,19,17,18,19,64,52,53,17,18,18,19,17,18,19,17,18
        .byte 19,17,18,19,17,18,19,17,40,44,44,39,1,1,1,39,64,52,53,255,0,17,40,44,44,39,1,44,44,39,64,52,53
        .byte 255,0,17,20,20,44,39,1,44,44,39,64,52,53,255,0,19,44,39,1,44,44,39,64,52,53,255,0,19,44,39,1,44,44
        .byte 39,64,52,53,255,0,19,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,45,16,14,15,16,45,15,16,14
        .byte 15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8
        .byte 4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10
        .byte 11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,44,39,55,54,55,0,0,74,0,54,69,54,74,54,74,54,55,54,55,75,0,0,45,0,74,40
        .byte 44,44,44,39,54,55,54,55,0,72,70,70,69,70,73,0,69,0,54,55,255,0,4,37,32,32,40,44,44,44,39,255,0,5
        .byte 54,55,54,69,0,0,0,69,255,0,9,69,40,44,255,20,5,255,0,6,72,70,87,255,77,8,87,0,0,69,40,44,255,0,13
        .byte 87,255,81,8,87,32,35,73,40,44,255,0,13,84,255,86,8,85,69,37,32,40,44,255,0,8,65,255,0,5,56,57,0
        .byte 69,0,0,56,57,0,69,0,0,40,44,15,16,14,15,16,14,15,16,64,52,53,14,15,16,14,15,16,14,15,16,14,15
        .byte 16,14,15,16,40,44,18,19,17,18,19,17,18,19,64,52,53,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,40
        .byte 44,44,255,1,6,39,64,52,53,255,0,6,75,0,0,75,72,70,73,0,0,40,44,44,255,1,6,39,64,52,53,255,0,15
        .byte 20,20,44,255,1,6,39,64,52,53,255,0,17,44,255,1,6,39,64,52,53,255,0,17,44,255,1,6,39,64,52,53,255,0,17
        .byte 16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4
        .byte 7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,39,54,55,0,0,0,74,0,0,74,0,0,74,0,0,74,0,0,0,75,0,54,74,54,55,40
        .byte 44,44,39,255,70,11,69,70,70,73,255,0,4,54,55,69,55,54,40,44,44,39,54,55,54,55,54,69,54,55,75,0,0
        .byte 72,255,70,9,73,0,0,40,44,1,39,255,70,5,73,55,255,0,17,20,20,1,39,255,0,26,1,39,255,0,26,1,39
        .byte 255,0,15,65,255,0,10,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,64,52,53,14,15,16,14,15,16
        .byte 14,15,18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,64,52,53,17,18,19,17,18,19,17,18,255,0,7
        .byte 89,255,0,5,89,0,0,0,68,52,53,255,0,6,40,44,255,0,7,99,255,0,5,99,0,0,0,68,52,53,0,87,6,6,6
        .byte 87,40,44,255,0,17,68,52,53,0,87,10,10,10,87,40,44,255,0,17,68,52,53,0,84,86,86,86,85,40,44,255,0,17
        .byte 68,52,53,0,0,75,0,75,0,40,44,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16
        .byte 14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0
        .byte 2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6
        .byte 9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,39,54,55,54,55,0,0,74,55,74,55,54,55,74,74,0,0,74,0,74,0,0,0,74,0,40
        .byte 44,44,39,0,54,55,0,0,0,75,0,69,54,55,0,72,70,70,70,73,0,75,0,0,0,72,70,40,44,44,39,255,0,8
        .byte 72,255,70,4,73,255,0,10,40,44,20,20,255,0,15,87,255,77,4,87,70,70,70,40,44,255,0,16,88,87,255,81,4
        .byte 87,0,0,0,40,44,255,0,16,69,84,255,86,4,85,0,50,51,40,44,255,0,16,69,0,75,0,0,75,0,0,48,49
        .byte 40,44,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,18,19,17
        .byte 18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,255,0,5,89,255,0,5,89
        .byte 255,0,4,75,0,0,0,89,255,0,12,99,255,0,5,99,255,0,8,99,255,0,91,16,14,15,16,14,15,16,14,15,16
        .byte 14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12
        .byte 7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,39,54,55,75,55,0,0,88,255,70,6,73,72,70,74,70,70,73,255,0,4,40,44,44,39,87
        .byte 54,55,0,87,0,69,0,0,0,74,0,74,75,75,0,69,74,255,0,6,40,44,44,39,87,77,77,77,87,70,73,0,0,0
        .byte 72,255,70,5,73,75,255,0,6,40,44,44,39,87,81,81,81,87,255,0,7,75,255,0,11,20,20,44,39,87,81,81,81
        .byte 87,255,0,21,44,39,84,86,86,86,85,255,0,21,44,39,0,75,0,75,0,0,65,255,0,19,44,1,14,15,16,14,15
        .byte 16,64,52,53,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,44,1,17,18,19,17,18,19,64,52,53,17
        .byte 18,19,17,18,19,17,18,19,17,18,19,17,18,19,17,18,44,255,1,6,39,64,52,53,255,0,17,44,255,1,6,39,64
        .byte 52,53,255,0,17,44,255,1,6,39,64,52,53,255,0,17,44,255,1,6,39,64,52,53,255,0,17,44,255,1,6,39,64
        .byte 52,53,255,0,17,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16
        .byte 12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0
        .byte 2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11
        .byte 0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 1,1,69,255,1,8,69,1,1,88,70,73,255,1,7,69,1,40,44,44,1,69,1,54,55,0,89,91,55,0,69,54,55
        .byte 69,255,0,4,89,0,54,55,0,69,0,40,44,44,1,69,1,55,54,92,90,92,91,55,72,70,70,73,0,54,55,0,99
        .byte 0,55,54,0,72,70,40,44,44,1,69,1,54,92,0,0,91,92,255,0,7,54,255,0,4,55,0,0,0,40,44,255,20,5
        .byte 255,0,21,20,20,255,0,65,65,255,0,18,255,66,9,64,52,53,255,66,16,255,1,9,64,52,53,255,1,16,0,0,0
        .byte 54,55,54,55,0,0,68,52,53,0,0,0,54,55,54,55,0,0,0,54,55,255,0,6,54,55,54,55,0,0,0,68,52
        .byte 53,255,0,4,54,55,0,0,0,54,55,54,55,255,0,12,68,52,53,255,0,25,68,52,53,255,0,25,68,52,53,255,0,16
        .byte 16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4
        .byte 7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,39,54,55,54,55,0,89,54,55,0,55,54,74,74,74,0,0,0,89,0,54,55,0,0,0,40
        .byte 44,44,39,100,100,101,102,55,99,55,54,88,70,70,73,75,75,54,55,91,90,91,55,54,0,0,0,40,44,44,39,255,103,4
        .byte 0,0,54,55,75,255,0,6,91,92,0,0,91,92,0,0,0,40,44,44,39,76,78,255,0,22,20,20,44,39,97,98,63,63
        .byte 255,0,22,44,39,95,96,97,98,255,0,22,44,39,93,94,93,94,255,0,22,44,1,255,66,15,45,255,66,4,45,255,66,5
        .byte 44,1,1,34,255,32,11,33,32,36,255,1,4,38,255,1,5,44,1,1,38,1,1,54,55,0,0,0,54,55,54,55,37
        .byte 255,32,4,33,32,36,255,0,5,255,20,6,0,54,55,0,54,55,54,55,255,0,5,54,55,255,0,4,54,55,255,0,85
        .byte 16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4
        .byte 7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,39,54,55,54,55,0,89,54,55,0,55,54,255,0,6,89,0,54,55,102,101,100,40,44,44,39
        .byte 255,0,4,55,99,55,54,255,0,6,54,55,91,90,91,55,54,103,103,103,40,44,44,39,255,0,6,54,55,255,0,7,91
        .byte 92,0,0,91,92,0,0,0,40,44,20,20,255,0,24,40,44,255,0,18,50,51,0,0,97,98,97,98,40,44,255,0,18
        .byte 48,49,0,0,95,96,95,96,40,44,255,0,9,65,255,0,8,46,47,0,0,93,94,93,94,40,44,255,66,9,64,52,53
        .byte 255,66,14,40,44,70,70,70,255,71,5,1,64,52,53,255,1,14,40,44,44,39,0,255,69,5,0,68,52,53,0,0,0
        .byte 54,55,54,55,255,0,4,54,55,54,40,44,44,39,0,80,81,81,77,79,0,68,52,53,255,0,4,54,55,255,0,4,54
        .byte 55,0,0,20,20,44,39,0,80,81,81,77,79,0,68,52,53,255,0,16,44,39,0,76,81,81,77,78,0,68,52,53,255,0,16
        .byte 44,39,0,0,56,0,57,0,0,68,52,53,255,0,16,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15,16,14,15
        .byte 16,14,15,16,14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3
        .byte 4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8
        .byte 255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 44,255,1,25,40,44,44,39,54,55,74,55,0,89,54,55,0,55,54,55,0,74,0,0,0,89,0,54,55,0,0,0,40
        .byte 44,44,39,0,0,69,0,92,90,92,54,255,0,5,75,54,55,91,90,91,55,54,55,0,0,40,44,44,39,0,0,69,0,0,0
        .byte 91,92,255,0,7,91,92,0,0,91,92,0,0,0,40,44,44,39,0,0,75,255,0,21,20,20,44,39,87,0,0,0,87
        .byte 50,51,255,0,19,44,39,87,77,77,77,87,48,49,255,0,19,44,39,87,81,81,81,87,46,47,65,255,0,18,44,39,87
        .byte 81,81,81,87,66,66,64,52,53,255,66,16,44,39,87,81,81,81,87,1,1,64,52,53,255,1,13,88,70,70,44,39,87
        .byte 81,81,81,87,40,39,64,52,53,255,0,13,69,40,44,44,39,87,81,81,81,87,40,39,64,52,53,255,0,13,69,40,44,44
        .byte 39,87,81,81,81,87,40,39,64,52,53,255,0,13,69,40,44,44,39,84,86,86,86,85,40,39,64,52,53,255,0,13,69
        .byte 40,44,44,39,0,75,38,75,70,40,39,255,70,5,45,255,70,4,45,255,70,5,73,40,44,16,14,15,16,37,255,32,6
        .byte 33,32,32,36,16,14,15,16,38,15,16,14,15,16,14,15,16,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12
        .byte 7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 106,106,255,104,6,69,69,255,104,6,69,255,104,6,69,104,104,255,106,4,255,105,6,69,69,255,105,6,69,255,105,6,69
        .byte 105,105,255,106,4,255,0,4,54,55,69,75,54,55,54,89,54,55,69,55,54,72,70,70,70,73,54,55,255,106,4,0,54
        .byte 55,54,55,54,75,0,0,91,92,90,0,91,72,255,70,4,73,55,54,55,54,106,106,86,86,0,0,54,55,255,0,4,91
        .byte 92,0,121,0,0,54,55,54,55,54,55,54,55,54,55,86,86,255,0,12,121,121,121,255,0,25,121,121,121,255,0,24,255,121,5
        .byte 255,0,12,255,104,28,255,105,26,106,106,255,0,5,89,255,0,7,100,101,102,101,55,54,55,54,55,54,55,54,55,106,106
        .byte 255,0,5,90,255,0,7,255,103,4,54,55,111,117,112,109,110,55,54,106,106,255,0,4,121,121,121,255,0,7,54,55,54
        .byte 55,87,115,120,116,107,108,111,112,106,106,255,0,4,121,121,121,255,0,11,109,110,119,116,112,110,115,116,106,106,0,0,0
        .byte 255,121,5,255,0,6,87,0,0,0,107,108,118,114,114,108,113,114,106,106,255,104,26,106,106,12,7,13,255,12,4,7,13
        .byte 255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
        .byte 106,106,255,104,24,255,106,4,255,105,24,255,106,4,255,0,4,54,55,54,0,54,55,54,89,54,55,54,55,54,55,255,0,4
        .byte 54,55,255,106,4,0,111,117,112,55,54,55,0,0,91,92,90,0,91,92,54,55,0,0,54,55,54,55,54,255,106,4,111
        .byte 112,120,116,255,0,4,91,92,255,121,4,91,55,54,55,54,55,54,55,54,55,86,86,106,106,115,97,98,116,255,0,6,255,121,4
        .byte 255,0,12,106,106,115,95,96,109,110,255,0,5,255,121,4,255,0,12,106,106,115,93,94,107,108,0,0,65,0,255,121,6
        .byte 255,0,11,106,106,255,104,7,64,52,53,255,104,16,255,106,5,255,105,4,64,52,53,255,105,16,255,106,5,89,0,0,0
        .byte 68,52,53,0,0,54,55,0,0,54,55,54,55,89,55,54,55,54,55,255,86,5,90,0,0,0,68,52,53,0,54,55,54
        .byte 55,0,0,54,55,0,99,0,0,54,55,255,0,5,121,121,121,0,0,68,52,53,255,0,20,121,121,121,0,0,68,52,53
        .byte 255,0,20,121,121,121,0,0,68,52,53,255,0,16,255,104,28,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12
        .byte 7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 106,106,255,104,24,255,106,4,255,105,24,255,106,4,255,0,4,54,55,54,0,54,55,54,89,54,55,54,55,54,55,255,0,4
        .byte 54,55,255,106,4,255,0,4,55,54,55,0,0,91,92,90,0,91,92,54,55,0,0,54,55,54,109,110,106,106,86,86,255,0,8
        .byte 91,92,0,0,121,0,91,55,54,55,54,111,117,117,107,108,106,106,255,0,12,121,121,121,255,0,6,115,119,111,117,112,106,106
        .byte 255,0,11,255,121,5,255,0,5,115,109,110,119,116,106,106,255,0,9,65,0,255,121,5,255,0,5,113,107,108,118,114,106,106
        .byte 255,104,9,64,52,53,255,104,16,255,106,5,255,105,4,64,52,53,255,105,14,255,106,7,89,0,0,0,68,52,53,0,0
        .byte 54,55,89,0,54,55,54,55,89,55,54,55,106,106,255,86,5,90,0,0,0,68,52,53,0,54,55,54,99,0,0,54,55
        .byte 0,99,0,0,54,86,86,255,0,4,121,121,121,0,0,68,52,53,255,0,19,255,121,5,0,68,52,53,255,0,19,255,121,5
        .byte 0,68,52,53,255,0,16,255,104,28,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3
        .byte 4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8
        .byte 255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 106,106,255,104,24,255,106,4,255,105,24,255,106,4,255,0,4,54,55,54,0,54,55,54,89,54,55,54,55,54,55,255,0,4
        .byte 54,55,255,106,4,0,111,112,0,55,54,55,0,0,91,92,90,0,91,92,54,55,0,0,54,55,54,0,0,255,106,4,109
        .byte 110,116,112,75,0,0,0,91,92,0,0,121,0,91,55,54,55,54,255,0,5,86,86,106,106,107,108,111,117,117,112,255,0,4
        .byte 121,121,121,255,0,13,106,106,123,124,124,123,120,116,0,0,0,255,121,5,255,0,12,106,106,122,113,113,122,118,114,0,0,0
        .byte 255,121,5,255,0,12,106,106,255,104,26,106,106,255,105,24,255,106,4,255,0,12,54,55,0,0,54,55,54,55,129,132,131
        .byte 55,255,106,4,127,128,127,128,255,0,5,127,128,54,55,54,55,0,0,54,55,0,0,130,0,54,86,86,106,106,125,126,125
        .byte 126,0,121,53,121,0,125,126,255,0,15,106,106,123,124,124,123,0,121,0,121,123,124,123,143,143,255,0,13,106,106,122,0,0
        .byte 122,255,0,4,122,0,122,143,143,255,0,13,255,104,28,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13
        .byte 255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 138,138,0,0,133,255,81,5,134,0,0,133,255,81,5,134,255,0,6,255,138,4,0,0,133,255,81,4,0,134,0,139,133
        .byte 81,81,81,0,81,134,0,0,0,139,0,0,255,138,4,139,0,133,81,81,0,81,0,134,0,0,133,0,0,81,0,81,134
        .byte 255,0,6,255,138,4,0,0,133,136,255,0,4,134,0,0,133,255,0,5,134,255,0,6,138,138,20,20,0,0,255,135,7
        .byte 0,0,255,135,7,0,0,127,128,0,0,138,138,255,0,20,139,0,125,126,97,98,138,138,255,0,7,139,255,0,11,97,98
        .byte 0,123,124,95,96,138,138,255,0,9,65,255,0,9,93,94,0,122,0,93,94,138,138,255,137,9,64,52,53,255,137,14,255,138,11
        .byte 64,52,53,255,138,18,255,0,7,68,52,53,255,0,14,255,138,4,0,0,127,128,0,0,0,68,52,53,255,0,14,20,20
        .byte 138,138,0,0,125,126,0,97,98,68,52,53,255,0,16,138,138,0,123,124,124,123,95,96,68,52,53,255,0,16,138,138,0
        .byte 122,0,0,122,93,94,68,52,53,255,0,16,255,137,28,12,7,13,255,12,4,7,13,255,12,4,7,13,12,12,12,7,13
        .byte 255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6,5,142,82,6,8,0,5,255,6,8
        .byte 8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9,10,10,10,11,255,0,34
        .byte 138,138,0,0,133,255,81,5,134,0,0,133,255,81,5,134,255,0,6,255,138,4,0,0,133,81,0,81,81,81,134,0,139
        .byte 133,81,0,81,81,81,134,0,0,0,139,0,0,255,138,4,139,0,133,81,0,0,136,81,134,0,0,133,81,0,81,0,136
        .byte 134,255,0,6,255,138,4,0,0,133,255,0,5,134,0,0,133,136,255,0,4,134,255,0,6,255,138,4,0,0,255,135,7
        .byte 0,0,255,135,7,255,0,6,20,20,138,138,255,0,26,138,138,255,0,26,138,138,255,0,4,65,255,0,21,255,137,6,64
        .byte 52,53,255,137,19,255,138,6,64,52,53,255,138,21,255,0,4,64,52,53,255,0,17,138,138,20,20,255,0,4,68,52,53
        .byte 255,0,14,127,128,0,138,138,255,0,6,68,52,53,255,0,14,125,126,0,138,138,255,0,6,68,52,53,255,0,11,97,98
        .byte 123,124,124,123,138,138,255,0,6,68,52,53,255,0,11,93,94,122,0,0,122,138,138,255,137,28,12,7,13,255,12,4,7
        .byte 13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
        .byte 138,138,0,0,133,81,81,81,0,255,81,6,0,0,81,81,134,255,0,6,255,138,4,0,0,133,81,81,81,0,81,81,0,0
        .byte 81,81,0,0,81,81,134,0,0,0,139,0,0,255,138,4,139,0,133,81,81,81,0,81,81,0,0,81,81,0,0,0,81
        .byte 134,255,0,6,255,138,4,0,0,133,0,81,0,0,0,136,255,0,8,134,255,0,6,255,138,4,0,0,255,135,16,0,0
        .byte 127,128,0,0,255,138,4,97,98,255,0,18,125,126,0,0,255,138,4,95,96,255,0,15,97,98,123,124,124,123,0,255,138,4
        .byte 93,94,0,0,65,255,0,12,93,94,122,0,0,122,0,138,138,255,137,6,64,52,53,255,137,19,255,138,6,64,52,53,255,138,25
        .byte 64,52,53,255,0,4,89,255,0,5,89,255,0,6,138,138,255,20,6,64,52,53,255,0,4,99,255,0,5,99,255,0,6
        .byte 20,20,255,0,6,68,52,53,255,0,25,68,52,53,255,0,25,68,52,53,255,0,19,255,137,28,12,7,13,255,12,4,7
        .byte 13,255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
        .byte 138,138,0,0,133,81,81,81,134,0,133,255,81,6,134,0,133,81,81,81,134,0,0,255,138,4,0,0,133,81,0,81,134
        .byte 0,133,255,81,5,0,134,0,133,81,0,81,134,0,0,255,138,4,139,0,133,0,0,81,134,0,133,81,0,81,81,81,0
        .byte 134,0,133,81,0,0,134,0,0,255,138,4,0,0,133,0,0,0,134,0,133,0,0,81,0,0,0,134,0,133,81,0,0
        .byte 134,0,0,255,138,4,0,0,133,0,0,0,134,0,133,0,0,81,0,0,0,134,0,133,0,0,0,134,0,0,255,138,4
        .byte 0,0,133,0,0,0,134,0,133,255,0,6,134,0,133,0,0,0,134,0,0,255,138,4,0,0,255,135,5,0,255,135,8
        .byte 0,255,135,5,0,0,255,138,4,255,0,24,138,138,255,137,28,255,138,30,0,100,101,100,101,102,0,101,100,102,255,0,14
        .byte 255,138,4,0,255,103,5,0,103,103,103,255,0,14,20,20,138,138,0,97,98,255,0,4,102,101,100,255,0,16,138,138,0
        .byte 95,96,97,98,0,0,103,103,103,255,0,16,138,138,0,93,94,93,94,255,0,21,255,137,28,12,7,13,255,12,4,7,13
        .byte 255,12,4,7,13,12,12,12,7,13,255,12,8,2,3,3,3,4,0,2,255,3,8,4,0,2,3,3,3,4,255,0,6
        .byte 5,142,82,6,8,0,5,255,6,8,8,0,5,83,6,6,8,255,0,6,9,10,10,10,11,0,9,255,10,8,11,0,9
        .byte 10,10,10,11,255,0,34
numsc:
        .byte 26
nmedat:
        .byte 0,0,104,216,8,1,112,192,255
        .byte 0,0,104,216,7,7,48,48,255
        .byte 2,8,32,80,2,8,32,152,1,10,88,144,4,10,64,80,255
        .byte 7,7,48,96,5,12,104,72,255
        .byte 7,7,48,40,3,11,40,128,255
        .byte 7,7,104,40,3,11,96,128,255
        .byte 4,10,32,120,1,10,88,72,255
        .byte 4,10,32,168,3,11,96,128,255
        .byte 7,7,104,40,1,10,48,128,4,10,48,112,255
        .byte 1,10,32,96,5,5,104,152,255
        .byte 2,8,32,168,2,8,32,144,7,7,48,40,1,10,24,152,255
        .byte 5,12,104,120,255
        .byte 2,8,32,176,1,10,32,160,5,12,104,104,255
        .byte 2,8,32,40,1,10,40,136,255
        .byte 7,7,48,40,3,11,96,120,255
        .byte 7,7,104,40,5,5,48,192,255
        .byte 3,11,40,120,255
        .byte 2,8,88,40,2,8,56,48,5,5,96,168,255
        .byte 1,10,32,80,255
        .byte 7,7,104,40,3,11,96,136,255
        .byte 7,7,104,40,3,11,40,136,5,12,104,192,255
        .byte 2,9,104,64,2,9,104,80,255
        .byte 3,11,40,120,255
        .byte 3,11,96,128,255
        .byte 7,7,104,40,5,12,104,168,255
NUMOBJ = 11
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,100,90,36,64,36,84,36,64,116,64,4,127,252,0,0,4,56,128,4,56,128
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,100,90,20,64,100,84,68,64,116,64,4,127,252,0,0,3,56,168,3,56,168
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,116,90,20,64,52,84,20,64,116,64,4,127,252,0,0,6,112,136,6,112,136
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,84,90,84,64,116,84,20,64,20,64,4,127,252,0,0,12,56,104,12,56,104
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,116,90,68,64,100,84,20,64,100,64,4,127,252,0,0,18,112,104,18,112,104
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,68,90,68,64,116,84,84,64,116,64,4,127,252,0,0,9,56,176,9,56,176
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,116,90,20,64,20,84,20,64,20,64,4,127,252,0,0,21,56,88,21,56,88
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,116,90,84,64,116,84,84,64,116,64,4,127,252,0,0,17,104,192,17,104,192
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,64,116,90,84,64,116,84,20,64,20,64,4,127,252,0,0,23,112,144,23,112,144
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,65,116,89,84,65,84,85,84,65,116,64,4,127,252,0,0,15,56,72,15,56,72
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,31,252,32,4,65,100,89,36,65,36,85,36,65,116,64,4,127,252,0,0,25,112,120,25,112,120
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
        .byte 0,0,36,24,102,24,36,0
        .byte 0,0,24,24,126,24,24,0
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,56,56,0
        .byte 0,0,6,12,24,48,96,0
        .byte 0,0,126,70,74,82,126,0
        .byte 0,0,120,8,8,8,126,0
        .byte 0,0,126,2,126,64,126,0
        .byte 0,0,126,2,14,2,126,0
        .byte 0,0,66,66,126,2,2,0
        .byte 0,0,126,64,126,2,126,0
        .byte 0,0,64,64,126,66,126,0
        .byte 0,0,126,2,2,2,2,0
        .byte 0,0,60,36,126,66,126,0
        .byte 0,0,126,66,126,2,2,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,0,126,66,126,66,66,0
        .byte 0,0,124,68,126,66,126,0
        .byte 0,0,126,66,64,66,126,0
        .byte 0,0,120,68,66,66,126,0
        .byte 0,0,126,64,112,64,126,0
        .byte 0,0,124,64,112,64,64,0
        .byte 0,0,126,64,94,66,126,0
        .byte 0,0,66,66,126,66,66,0
        .byte 0,0,124,16,16,16,124,0
        .byte 0,0,30,2,2,66,124,0
        .byte 0,0,68,68,126,66,66,0
        .byte 0,0,64,64,64,66,126,0
        .byte 0,0,126,74,74,66,66,0
        .byte 0,0,98,82,74,70,66,0
        .byte 0,0,126,66,66,66,126,0
        .byte 0,0,126,66,126,64,64,0
        .byte 0,0,126,66,90,70,126,0
        .byte 0,0,124,68,126,66,66,0
        .byte 0,0,126,64,126,2,126,0
        .byte 0,0,126,16,16,16,16,0
        .byte 0,0,66,66,66,66,126,0
        .byte 0,0,66,66,66,36,24,0
        .byte 0,0,66,66,74,74,126,0
        .byte 0,0,66,36,24,36,66,0
        .byte 0,0,66,66,126,2,62,0
        .byte 0,0,126,4,8,16,126,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 252,254,238,254,254,254,238,0
        .byte 252,252,238,254,238,254,254,0
        .byte 254,254,238,224,238,254,254,0
        .byte 240,248,252,222,206,254,254,0
        .byte 254,254,240,252,248,254,254,0
        .byte 254,254,248,254,254,240,240,0
        .byte 254,254,240,238,238,254,254,0
        .byte 238,238,254,254,254,238,238,0
        .byte 254,254,124,124,254,254,254,0
        .byte 254,254,254,60,252,252,252,0
        .byte 0,96,120,112,112,120,108,0
        .byte 0,48,48,48,48,48,28,0
        .byte 0,0,248,252,252,252,252,0
        .byte 0,0,248,204,204,204,204,0
        .byte 254,254,238,238,254,254,254,0
        .byte 0,0,248,204,204,248,192,192
        .byte 0,0,124,204,204,124,12,14
        .byte 252,252,236,254,254,238,238,14
        .byte 0,0,120,192,120,12,248,0
        .byte 0,48,120,48,48,48,28,0
        .byte 0,0,204,204,204,204,120,0
        .byte 0,0,204,204,120,120,48,0
        .byte 0,0,204,252,252,252,120,0
        .byte 0,0,204,120,48,120,204,0
        .byte 0,0,204,204,204,124,12,120
        .byte 254,254,62,124,248,254,254,0
        .byte 0,30,24,112,24,24,30,0
        .byte 0,24,24,24,24,24,24,0
        .byte 0,240,48,28,48,48,240,0
        .byte 0,60,120,0,0,0,0,0
        .byte 126,129,189,161,161,189,129,126
jtab:
        .byte 248,250,252,254,254,255,255,255,0,0,0,1,1,1,2,2,4,6,8,8,8,99
keys:   .byte 10,8,63,47,33,57,56,31,30,26,24