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
WINDOWHGT = 22
WINDOWWID = 30 ;a
MAPWID = 6
        .byte 255,255,255,255,255,255
mapdat:
        .byte 0,1,2,255,255,255,255,255,3,4,5,6,7,8,9,255,255,255
        .byte 255,255,255,255,255,255
stmap:  .byte 0

evnt00:
        lda #0
        cmp varz
        beq *+5
        jmp a00302
        ldy #9
        lda (z80_ix),y
        sta varj
        lda #1
        sta chary
        lda #19
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        lda #0
        sta chary
        lda #16
        sta charx
        ldy #0		; SHOWSCORE
        lda #6
        sta z80_b
        jsr dscor
        lda #0
        sta chary
        lda #26
        sta charx
        lda #9  	; MESSAGE
        jsr dmsg
        lda #0
        sta chary
        lda #22
        sta charx
        lda #10  	; MESSAGE
        jsr dmsg
        lda #0
        sta chary
        lda #31
        sta charx
        lda #11  	; MESSAGE
        jsr dmsg
        lda #1
        sta chary
        lda #22
        sta charx
        lda #12  	; MESSAGE
        jsr dmsg
        lda #1
        sta chary
        lda #28
        sta charx
        lda varc	; DISPLAY
        jsr disply
        lda #1
        sta chary
        lda #23
        sta charx
        lda varl	; DISPLAY
        jsr disply
        lda #1
        sta chary
        lda #31
        sta charx
        lda #13  	; MESSAGE
        jsr dmsg
        lda #1
        sta varz
a00302: jsr laddu	; CANGOUP
        beq :+
        jmp a00420
:
        lda joyval	; KEY
        and #8
        beq :+
        jmp a00353
:
        lda #0		; ANIMATE
        jsr animsp
        jmp a00416
a00353: lda joyval	; KEY
        and #4
        beq :+
        jmp a00388
:
        lda #0		; ANIMATE
        jsr animsp
        jmp a00416
a00388: lda #0
        cmp varm
        beq *+5
        jmp a00416
        lda #0		; ANIMATE
        jsr animsp
a00416: jmp a00552
a00420: lda joyval	; KEY
        and #2
        beq :+
        jmp a00472
:
        lda #0
        cmp vara
        beq *+5
        jmp a00467
        lda #0		; ANIMATE
        jsr animsp
a00467: jmp a00552
a00472: lda joyval	; KEY
        and #1
        beq :+
        jmp a00524
:
        lda #0
        cmp vara
        beq *+5
        jmp a00519
        lda #0		; ANIMATE
        jsr animsp
a00519: jmp a00552
a00524: lda #0
        cmp varm
        beq *+5
        jmp a00552
        lda #0		; ANIMATE
        jsr animsp
a00552: lda #2
        cmp scno
        beq *+5
        jmp a00652
        lda #10
        sta chary
        lda #6
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #15
        sta chary
        lda #6
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #11
        sta chary
        lda #25
        sta charx
        lda #5  	; MESSAGE
        jsr dmsg
a00652: lda #DEADLY	; DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a00843
:
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #160
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00843
        lda #8
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
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
a00843: lda #168
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00905
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #8	; SPAWN
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
a00905: jsr laddu	; CANGOUP
        beq :+
        jmp a01260
:
        ldy #8
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ldy #8
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        asl a
        asl a
        asl a
        ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #32
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda joyval	; KEY
        and #8
        beq :+
        jmp a01190
:
        lda #CUSTOM	; CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01138
:
        jmp a01190
a01138: lda #0
        cmp vara
        beq *+5
        jmp a01190
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
a01190: lda joyval	; KEY
        and #4
        beq :+
        jmp a01260
:
        lda #0
        cmp vara
        beq *+5
        jmp a01260
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
a01260: lda #CUSTOM	; CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01373
:
        lda #4
        ldy #6
        sta (z80_ix),y
        lda joyval	; KEY
        and #4
        beq :+
        jmp a01373
:
        lda #0
        cmp vara
        beq *+5
        jmp a01373
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
a01373: lda joyval	; KEY
        and #1
        beq :+
        jmp a01479
:
        lda #0
        ldy #10
        sta (z80_ix),y
        jsr cangr	; CANGORIGHT
        beq :+
        jmp a01474
:
        lda #0
        cmp vara
        beq *+5
        jmp a01474
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
a01474: jmp a01579
a01479: lda joyval	; KEY
        and #2
        beq :+
        jmp a01579
:
        lda #1
        ldy #10
        sta (z80_ix),y
        jsr cangl	; CANGOLEFT
        beq :+
        jmp a01579
:
        lda #0
        cmp vara
        beq *+5
        jmp a01579
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
a01579: lda joyval	; KEY
        and #16
        beq :+
        jmp a01661
:
        jsr laddu	; CANGOUP
        beq :+
        jmp a01619
:
        jmp a01661
a01619: lda #CUSTOM	; CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01654
:
        jmp a01661
a01654: jsr jump	; TABLEJUMP
a01661: jsr ifall	; TABLEFALL
        lda #160
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a02432
        lda #0
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02062
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        jsr laddu	; CANGOUP
        beq :+
        jmp a01838
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp a01943
a01838: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
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
a01943: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jsr laddu	; CANGOUP
        beq :+
        jmp a02011
:
        lda #4
        ldy #6
        sta (z80_ix),y
        jmp a02062
a02011: lda joyval	; KEY
        and #1
        beq :+
        jmp a02048
:
        lda #0
        ldy #6
        sta (z80_ix),y
        jmp a02062
a02048: lda #1
        ldy #6
        sta (z80_ix),y
a02062: lda #1
        ldy #10
        cmp (z80_ix),y
        beq *+5
        jmp a02432
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        jsr laddu	; CANGOUP
        beq :+
        jmp a02209
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp a02313
a02209: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
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
a02313: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jsr laddu	; CANGOUP
        beq :+
        jmp a02382
:
        lda #4
        ldy #6
        sta (z80_ix),y
        jmp a02432
a02382: lda joyval	; KEY
        and #2
        beq :+
        jmp a02419
:
        lda #2
        ldy #6
        sta (z80_ix),y
        jmp a02432
a02419: lda #3
        ldy #6
        sta (z80_ix),y
a02432: lda #2
        cmp scno
        beq *+5
        jmp a02504
        lda #6
        sta chary
        lda #1
        sta charx
        lda #4  	; MESSAGE
        jsr dmsg
        lda #7
        sta chary
        lda #18
        sta charx
        lda #5  	; MESSAGE
        jsr dmsg
a02504: lda #4
        cmp scno
        bcs *+5
        jmp a02549
        lda #23
        sta chary
        lda #1
        sta charx
        lda #1  	; MESSAGE
        jsr dmsg
a02549: jmp grav
evnt01:
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00128
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
b00128: lda #9
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp b00219
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
b00219: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #32
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
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #148
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp b00666
        lda #2
        cmp scno
        beq *+5
        jmp b00417
        lda #32
        ldy #8
        sta (z80_ix),y
        jmp b00430
b00417: lda #64
        ldy #8
        sta (z80_ix),y
b00430: lda #96
        ldy #9
        sta (z80_ix),y
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #2
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp b00573
        lda #1	; SPAWN
        sta z80_c
        lda #8
        sta z80_b
        jsr spawn
        lda spptr	; SPAWNED
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #1
        ldy #11
        sta (z80_ix),y
        jmp b00666
b00573: lda #80
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #2	; SPAWN
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
b00666: lda #DEADLY	; DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp b00798
:
        lda #0
        cmp vara
        beq *+5
        jmp b00798
        lda #2
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp b00798
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
b00798: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp b00925
        jsr cangl	; CANGOLEFT
        beq :+
        jmp b00907
:
        lda #0
        cmp varm
        beq *+5
        jmp b00889
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
b00889: lda #9
        ldy #6
        sta (z80_ix),y
        jmp b00920
b00907: lda #1
        ldy #11
        sta (z80_ix),y
b00920: jmp b01025
b00925: jsr cangr	; CANGORIGHT
        beq :+
        jmp b01011
:
        lda #0
        cmp varm
        beq *+5
        jmp b00993
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
b00993: lda #8
        ldy #6
        sta (z80_ix),y
        jmp b01025
b01011: lda #0
        ldy #11
        sta (z80_ix),y
b01025: lda #0
        cmp vara
        beq *+5
        jmp b01053
        lda #0		; ANIMATE
        jsr animsp
b01053: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp b01145
:
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #1
        sta vard
        lda #8	; SPAWN
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
b01145: rts
evnt02:
        lda #41
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00121
        lda #0
        cmp vara
        beq *+5
        jmp c00052
        lda #0		; ANIMATE
        jsr animsp
c00052: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
c00121: lda #39
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp c00762
        lda #41
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp c00762
        lda #41
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00448
        lda #160
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp c00224
        lda #24
        ldy #8
        sta (z80_ix),y
c00224: lda #56
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00448
        lda #180
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp c00448
        lda #CUSTOM	; CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c00390
:
        lda #40
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        bcs *+5
        jmp c00385
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
c00385: jmp c00448
c00390: jsr cangd	; CANGODOWN
        beq :+
        jmp c00448
:
        lda #CUSTOM	; CUSTOM
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp c00440
:
        jmp c00448
c00440: jsr ifall	; TABLEFALL
c00448: lda #40
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00584
        lda #60
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp c00527
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
c00527: lda #176
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00584
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
c00584: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp c00687
        lda #56
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp c00668
        lda #3
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        jmp c00682
c00668: lda #1
        ldy #11
        sta (z80_ix),y
c00682: jmp c00762
c00687: lda #180
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp c00749
        lda #3
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp c00762
c00749: lda #0
        ldy #11
        sta (z80_ix),y
c00762: lda #40
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp c00812
        lda #0
        cmp vara
        beq *+5
        jmp c00812
        lda #0		; ANIMATE
        jsr animsp
c00812: lda #6
        cmp scno
        bne *+5
        jmp c01337
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #140
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp c01082
        lda #0
        cmp varm
        beq *+5
        jmp c01078
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
c01078: jmp c01309
c01082: lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #2
        cmp scno
        beq *+5
        jmp c01133
        lda #32
        ldy #8
        sta (z80_ix),y
        jmp c01147
c01133: lda #64
        ldy #8
        sta (z80_ix),y
c01147: lda #96
        ldy #9
        sta (z80_ix),y
        lda #2
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp c01269
        lda #1	; SPAWN
        sta z80_c
        lda #8
        sta z80_b
        jsr spawn
        lda spptr	; SPAWNED
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #1
        ldy #11
        sta (z80_ix),y
c01269: lda #0
        cmp varrnd
        beq *+5
        jmp c01309
        lda #2	; SPAWN
        sta z80_c
        lda #5
        sta z80_b
        jsr spawn
c01309: lda #0
        cmp vara
        beq *+5
        jmp c01337
        lda #0		; ANIMATE
        jsr animsp
c01337: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c01430
:
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #1
        sta vard
        lda #8	; SPAWN
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
c01430: jmp grav
evnt03:
        lda #42
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00320
        lda #132
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp d00081
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
d00081: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00292
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda numlif
        clc
        adc #1
        sta numlif
d00292: lda #0
        cmp varm
        beq *+5
        jmp d00320
        lda #0		; ANIMATE
        jsr animsp
d00320: lda #31
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00342
d00342: lda #5
        cmp scno
        beq *+5
        jmp d00480
        lda #39
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00414
        lda #0
        cmp vara
        beq *+5
        jmp d00409
        lda #0		; ANIMATE
        jsr animsp
d00409: jmp d00480
d00414: lda #0
        cmp varm
        beq *+5
        jmp d00442
        lda #0		; ANIMATE
        jsr animsp
d00442: lda joyval	; KEY
        and #16
        beq :+
        jmp d00480
:
        lda #3
        sta varl
        lda #1	; NEXTLEVEL
        sta nexlev
d00480: lda #22
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d00989
        jsr laddu	; CANGOUP
        beq :+
        jmp d00553
:
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
d00553: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00989
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #<100		; SCORE
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda #8	; SPAWN
        sta z80_c
        lda #32
        sta z80_b
        jsr spawn
        lda #8	; SPAWN
        sta z80_c
        lda #22
        sta z80_b
        jsr spawn
        lda #180
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda varrnd
        lsr a
        lsr a
        lsr a
        sta varrnd
        lda varrnd
        asl a
        asl a
        asl a
        sta varrnd
        lda #80
        ldy #8
        sta (z80_ix),y
        lda varrnd
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #10
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #8
        cmp varrnd
        bcs *+5
        jmp d00966
        lda #3	; SPAWN
        sta z80_c
        lda #22
        sta z80_b
        jsr spawn
        jmp d00989
d00966: lda #3	; SPAWN
        sta z80_c
        lda #42
        sta z80_b
        jsr spawn
d00989: lda #7
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01234
        lda #0
        cmp vare
        beq *+5
        jmp d01056
        lda #0
        cmp vara
        beq *+5
        jmp d01056
        lda #0		; ANIMATE
        jsr animsp
d01056: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d01234
:
        lda #0
        cmp vare
        beq *+5
        jmp d01234
        lda varc		; SCORE
        sta z80_l
        lda #0
        sta z80_h
        jsr addsc
        lda #6
        cmp scno
        bne *+5
        jmp d01154
        lda #1	; NEXTLEVEL
        sta nexlev
        jmp d01210
d01154: jsr cls		; CLS
        lda #10
        sta chary
        lda #1
        sta charx
        lda #30  	; MESSAGE
        jsr dmsg
        lda #80 	; DELAY
        jsr delay
        lda #1	; NEXTLEVEL
        sta nexlev
d01210: lda varl
        clc
        adc #1
        sta varl
        lda #1
        sta varb
d01234: lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d01617
        lda #7
        cmp scno
        beq *+5
        jmp d01328
        lda #6
        sta chary
        lda #5
        sta charx
        lda #26  	; MESSAGE
        jsr dmsg
        lda #7
        sta chary
        lda #5
        sta charx
        lda #28  	; MESSAGE
        jsr dmsg
d01328: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #6
        cmp scno
        beq *+5
        jmp d01554
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #32
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #40
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
d01554: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #0
        cmp varb
        beq *+5
        jmp d01617
        lda #0		; ANIMATE
        jsr animsp
d01617: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp d02176
        lda #0
        cmp varb
        beq *+5
        jmp d01667
        lda #0		; ANIMATE
        jsr animsp
d01667: lda #4
        cmp scno
        beq *+5
        jmp d01895
        lda #0
        cmp varm
        beq *+5
        jmp d01895
        lda #6
        sta chary
        lda #1
        sta charx
        lda #4  	; MESSAGE
        jsr dmsg
        lda #6
        sta chary
        lda #22
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #11
        sta chary
        lda #2
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #16
        sta chary
        lda #1
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #17
        sta chary
        lda #5
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #16
        sta chary
        lda #24
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #17
        sta chary
        lda #19
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
d01895: lda #6
        cmp scno
        beq *+5
        jmp d02176
        lda #5
        sta varo
        lda #2 	; REPEAT
        sta loopa
d01931: lda varo
        sta chary
        lda #1
        sta charx
        lda #25  	; MESSAGE
        jsr dmsg
        lda #8
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        dec loopa	; ENDREPEAT
        beq :+
        jmp d01931
:
        lda #9
        sta varo
        lda #2 	; REPEAT
        sta loopa
d02019: lda varo
        sta chary
        lda #27
        sta charx
        lda #25  	; MESSAGE
        jsr dmsg
        lda #8
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        dec loopa	; ENDREPEAT
        beq :+
        jmp d02019
:
        lda #5
        sta varo
        lda #5 	; REPEAT
        sta loopa
d02107: lda varo
        sta chary
        lda #7
        sta charx
        lda #27  	; MESSAGE
        jsr dmsg
        lda #4
        sta z80_c
        lda varo
        clc
        adc z80_c
        sta varo
        dec loopa	; ENDREPEAT
        beq :+
        jmp d02107
:
d02176: rts
evnt04:
        lda #8
        cmp scno
        beq *+5
        jmp e00522
        lda #13
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00145
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp e00145
        lda #0		; ANIMATE
        jsr animsp
        jsr jump	; TABLEJUMP
e00145: lda #23
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp e00360
        lda #26
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp e00360
        lda #120
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e00264
        lda #0
        cmp vara
        beq *+5
        jmp e00264
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
e00264: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00291
        jmp e00360
e00291: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
e00360: lda #26
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp e00522
        lda #29
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp e00522
        lda #136
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp e00511
        lda #0
        cmp vara
        beq *+5
        jmp e00479
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
e00479: lda #28
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00506
        jmp e00506
e00506: jmp e00522
e00511: lda #1	; NEXTLEVEL
        sta nexlev
e00522: lda #45
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e00678
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00581
:
        lda #1	; NEXTLEVEL
        sta nexlev
e00581: lda #0
        cmp vara
        beq *+5
        jmp e00609
        lda #0		; ANIMATE
        jsr animsp
e00609: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
e00678: lda #44
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01027
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00959
:
        lda #64
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp e00787
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        jmp e00959
e00787: lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #<200		; SCORE
        sta z80_l
        lda #>200
        sta z80_h
        jsr addsc
        lda vare
        sec
        sbc #1
        sta vare
        lda #3
        cmp vare
        beq *+5
        jmp e00959
        lda #64
        ldy #8
        sta (z80_ix),y
        lda #120
        ldy #9
        sta (z80_ix),y
        lda #1	; SPAWN
        sta z80_c
        lda #9
        sta z80_b
        jsr spawn
        lda spptr	; SPAWNED
        sta z80_x
        lda spptr+1
        sta z80_i
        lda #0
        ldy #11
        sta (z80_ix),y
e00959: lda #0
        cmp vare
        beq *+5
        jmp e01027
        lda #160
        ldy #8
        sta (z80_ix),y
        lda #120
        ldy #9
        sta (z80_ix),y
        lda #4	; SPAWN
        sta z80_c
        lda #45
        sta z80_b
        jsr spawn
e01027: lda #14
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp e01262
        lda #0
        cmp vara
        beq *+5
        jmp e01077
        lda #0		; ANIMATE
        jsr animsp
e01077: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e01195
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #8	; SPAWN
        sta z80_c
        lda #32
        sta z80_b
        jsr spawn
        lda #<100		; SCORE
        sta z80_l
        lda #>100
        sta z80_h
        jsr addsc
        lda vare
        sec
        sbc #1
        sta vare
e01195: lda #0
        cmp vare
        beq *+5
        jmp e01262
        lda #8
        ldy #8
        sta (z80_ix),y
        lda #88
        ldy #9
        sta (z80_ix),y
        lda #3	; SPAWN
        sta z80_c
        lda #7
        sta z80_b
        jsr spawn
e01262: jmp grav
evnt05:
        ldy #8
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ldy #8
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        asl a
        asl a
        asl a
        ldy #8
        sta (z80_ix),y
        lda #33
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00250
        lda #0
        cmp vara
        beq *+5
        jmp f00111
        lda #0		; ANIMATE
        jsr animsp
f00111: lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
f00250: lda #3
        cmp scno
        beq *+5
        jmp f00955
        lda #0
        cmp varb
        beq *+5
        jmp f00308
        lda #1
        cmp varg
        bcs *+5
        jmp f00308
        lda #2
        sta varg
f00308: lda #3
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp f00600
        lda #3
        cmp varg
        beq *+5
        jmp f00363
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
f00363: lda #16
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp f00482
        lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00412
        jmp f00482
f00412: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
f00482: lda #32
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp f00596
        lda #16
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00596
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
f00596: jmp f00628
f00600: lda #23
        sta chary
        lda #0
        sta charx
        lda #1  	; MESSAGE
        jsr dmsg
f00628: lda #2
        cmp varg
        bcs *+5
        jmp f00673
        lda #0
        cmp vara
        beq *+5
        jmp f00673
        lda #0		; ANIMATE
        jsr animsp
f00673: lda #3
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp f00806
        lda #2
        cmp varg
        beq *+5
        jmp f00806
        lda #0
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp f00790
        lda #0
        cmp varh
        beq *+5
        jmp f00785
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
f00785: jmp f00806
f00790: lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
f00806: lda #15
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00886
        lda #8
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp f00886
        lda #3
        sta varg
        lda #4
        sta chary
        lda #8
        sta charx
        lda #21  	; MESSAGE
        jsr dmsg
f00886: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f00955
        lda #3
        cmp varg
        beq *+5
        jmp f00955
        lda joyval	; KEY
        and #16
        beq :+
        jmp f00955
:
        lda #1	; NEXTLEVEL
        sta nexlev
f00955: lda #1
        cmp scno
        beq *+5
        jmp f01828
        lda #0
        cmp varg
        beq *+5
        jmp f01185
        lda #26
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp f01061
        lda #3
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp f01061
        lda #0
        cmp varh
        beq *+5
        jmp f01061
        lda #0		; ANIMATE
        jsr animsp
f01061: lda #3
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01111
        lda #0
        cmp vara
        beq *+5
        jmp f01111
        lda #0		; ANIMATE
        jsr animsp
f01111: lda #31
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01185
        lda #0
        cmp varh
        beq *+5
        jmp f01185
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
f01185: lda #1
        cmp varg
        beq *+5
        jmp f01252
        lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01252
        lda #0
        cmp varh
        beq *+5
        jmp f01252
        lda #0		; ANIMATE
        jsr animsp
f01252: lda #23
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp f01613
        lda #26
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp f01613
        lda #24
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f01375
        lda #0
        cmp varh
        beq *+5
        jmp f01370
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
f01370: jmp f01516
f01375: lda #1
        sta varg
        lda #12
        sta chary
        lda #9
        sta charx
        lda #20  	; MESSAGE
        jsr dmsg
        lda #23
        sta chary
        lda #11
        sta charx
        lda #21  	; MESSAGE
        jsr dmsg
        lda joyval	; KEY
        and #16
        beq :+
        jmp f01516
:
        jsr scrr	; SCREENRIGHT
        lda #136
        ldy #8
        sta (z80_ix),y
        lda #32
        ldy #9
        sta (z80_ix),y
        lda #0	; SPAWN
        sta z80_c
        lda #0
        sta z80_b
        jsr spawn
f01516: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01543
        jmp f01613
f01543: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
f01613: lda #26
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp f01828
        lda #29
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp f01828
        lda #40
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp f01731
        lda #0
        cmp varh
        beq *+5
        jmp f01731
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
f01731: lda #28
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp f01758
        jmp f01758
f01758: lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #16
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
f01828: rts
evnt06:
        lda #43
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g00414
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda joyval	; KEY
        and #2
        beq :+
        jmp g00288
:
        lda #12
        ldy #6
        sta (z80_ix),y
        lda #24
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #0	; SPAWN
        sta z80_c
        lda #2
        sta z80_b
        jsr spawn
        lda #24
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
g00288: lda joyval	; KEY
        and #1
        beq :+
        jmp g00414
:
        lda #12
        ldy #6
        sta (z80_ix),y
        lda #24
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #0	; SPAWN
        sta z80_c
        lda #0
        sta z80_b
        jsr spawn
        lda #24
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
g00414: lda #11
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp g00458
        lda #34
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp g00458
g00458: lda #43
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp g00811
        lda #34
        ldy #6
        cmp (z80_ix),y
        bne *+5
        jmp g00811
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00622
        lda #72
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp g00604
        lda #0
        cmp varh
        beq *+5
        jmp g00599
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
g00599: jmp g00617
g00604: lda #1
        ldy #11
        sta (z80_ix),y
g00617: jmp g00714
g00622: lda #120
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp g00701
        lda #0
        cmp varh
        beq *+5
        jmp g00696
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
g00696: jmp g00714
g00701: lda #0
        ldy #11
        sta (z80_ix),y
g00714: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00811
:
        lda #43
        ldy #6
        sta (z80_ix),y
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda ogptr	; ENDSPRITE
        sta z80_x
        lda ogptr+1
        sta z80_i
g00811: lda #34
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp g01340
        ldy #9
        lda (z80_ix),y
        sta varo
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00992
        jsr cangl	; CANGOLEFT
        beq :+
        jmp g00987
:
        lda #8
        cmp vark
        bcs *+5
        jmp g00974
        lda #0
        cmp varh
        beq *+5
        jmp g00969
        lda vark
        clc
        adc #1
        sta vark
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
g00969: jmp g00987
g00974: lda #1
        ldy #11
        sta (z80_ix),y
g00987: jmp g01110
g00992: lda #0
        cmp vark
        bcc *+5
        jmp g01097
        jsr cangr	; CANGORIGHT
        beq :+
        jmp g01092
:
        lda #0
        cmp varh
        beq *+5
        jmp g01092
        lda vark
        sec
        sbc #1
        sta vark
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
g01092: jmp g01110
g01097: lda #0
        ldy #11
        sta (z80_ix),y
g01110: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g01340
:
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda varo
        ldy #9
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp g01232
        lda #0		; ANIMATE
        jsr animsp
g01232: lda joyval	; KEY
        and #1
        beq :+
        jmp g01286
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
g01286: lda joyval	; KEY
        and #2
        beq :+
        jmp g01340
:
        lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
g01340: lda #0
        cmp varm
        beq *+5
        jmp g01368
        lda #0		; ANIMATE
        jsr animsp
g01368: rts
evnt07:
        lda #9
        cmp scno
        beq *+5
        jmp h00059
        lda #31
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00059
        jsr prskey	; WAITKEY
        lda #1		; ENDGAME
        sta gamwon
h00059: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp h00152
:
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #1
        sta vard
        lda #8	; SPAWN
        sta z80_c
        lda #6
        sta z80_b
        jsr spawn
h00152: lda varj
        ldy #9
        cmp (z80_ix),y
        beq *+5
        jmp h00274
        lda #0
        cmp varn
        beq *+5
        jmp h00274
        lda #10
        sta z80_d	; GETRANDOM
        jsr random
        sta z80_h
        jsr imul
        lda z80_h
        sta varrnd
        lda #1
        cmp varrnd
        beq *+5
        jmp h00274
        lda #8	; SPAWN
        sta z80_c
        lda #38
        sta z80_b
        jsr spawn
        lda #1
        sta varn
h00274: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00402
        lda varp
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp h00388
        lda #0
        cmp varm
        beq *+5
        jmp h00370
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
h00370: lda #10
        ldy #6
        sta (z80_ix),y
        jmp h00402
h00388: lda #36
        ldy #6
        sta (z80_ix),y
h00402: lda #36
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00655
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #88
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp h00641
        lda #0
        cmp varm
        beq *+5
        jmp h00637
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
h00637: jmp h00655
h00641: lda #33
        ldy #6
        sta (z80_ix),y
h00655: lda #10
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h00816
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
h00816: lda #33
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01070
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda varq
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp h01056
        lda #0
        cmp varm
        beq *+5
        jmp h01051
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
h01051: jmp h01070
h01056: lda #37
        ldy #6
        sta (z80_ix),y
h01070: lda #37
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp h01378
        lda #4
        cmp scno
        beq *+5
        jmp h01127
        lda #32
        ldy #11
        sta (z80_ix),y
        jmp h01141
h01127: lda #68
        ldy #11
        sta (z80_ix),y
h01141: lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        ldy #11
        lda (z80_ix),y
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp h01364
        lda #0
        cmp varm
        beq *+5
        jmp h01359
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
h01359: jmp h01378
h01364: lda #10
        ldy #6
        sta (z80_ix),y
h01378: lda #0
        cmp vara
        beq *+5
        jmp h01406
        lda #0		; ANIMATE
        jsr animsp
h01406: rts
evnt08:
        lda #38
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00401
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #156
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp i00155
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        jmp i00194
i00155: lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #0
        sta varn
        lda #20		; BEEP
        asl a
        sta sndtyp
i00194: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i00401
:
        lda #1
        sta vard
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        ldy #8
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ldy #8
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ldy #9
        sta (z80_ix),y
        lda #2
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sta chary
        ldy #9
        lda (z80_ix),y
        sta charx
        lda #29  	; MESSAGE
        jsr dmsg
        lda #40 	; DELAY
        jsr delay
        lda #1 		; KILL
        sta deadf
i00401: lda #35
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00615
        lda #5		; BEEP
        asl a
        sta sndtyp
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #0		; ANIMATE
        jsr animsp
        lda #0
        cmp varh
        beq *+5
        jmp i00615
        lda #1 		; KILL
        sta deadf
i00615: lda #11
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00709
        jsr cangu	; CANGOUP
        beq :+
        jmp i00693
:
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        jmp i00709
i00693: lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
i00709: lda #32
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00803
        jsr cangu	; CANGOUP
        beq :+
        jmp i00787
:
        lda #4
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        jmp i00803
i00787: lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
i00803: lda #23
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i00891
        lda #0
        cmp vara
        beq *+5
        jmp i00853
        lda #0		; ANIMATE
        jsr animsp
i00853: lda #2
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp i00891
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
i00891: lda #22
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01502
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #1 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i01124
:
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #8	; SPAWN
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda #<200		; SCORE
        sta z80_l
        lda #>200
        sta z80_h
        jsr addsc
        lda #1
        cmp scno
        bcc *+5
        jmp i01124
        lda #24
        ldy #8
        sta (z80_ix),y
        lda #88
        ldy #9
        sta (z80_ix),y
        lda #1	; SPAWN
        sta z80_c
        lda #8
        sta z80_b
        jsr spawn
i01124: lda #2 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i01300
:
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #8	; SPAWN
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda #<200		; SCORE
        sta z80_l
        lda #>200
        sta z80_h
        jsr addsc
        lda #1
        cmp scno
        bcc *+5
        jmp i01300
        lda #24
        ldy #8
        sta (z80_ix),y
        lda #88
        ldy #9
        sta (z80_ix),y
        lda #1	; SPAWN
        sta z80_c
        lda #8
        sta z80_b
        jsr spawn
i01300: lda #7 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp i01426
:
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #8	; SPAWN
        sta z80_c
        lda #11
        sta z80_b
        jsr spawn
        lda skptr	; OTHER
        sta z80_x
        lda skptr+1
        sta z80_i
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #<200		; SCORE
        sta z80_l
        lda #>200
        sta z80_h
        jsr addsc
i01426: lda #140
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp i01502
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #8	; SPAWN
        sta z80_c
        lda #23
        sta z80_b
        jsr spawn
        lda #20		; BEEP
        asl a
        sta sndtyp
i01502: lda #18
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp i01658
        lda #21
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp i01658
        lda #0
        cmp vara
        beq *+5
        jmp i01574
        lda #0		; ANIMATE
        jsr animsp
i01574: lda #3
        sta chary
        lda #10
        sta charx
        lda #14  	; MESSAGE
        jsr dmsg
        lda #5
        sta chary
        lda #12
        sta charx
        lda #15  	; MESSAGE
        jsr dmsg
        lda #5
        sta chary
        lda #18
        sta charx
        lda #16  	; MESSAGE
        jsr dmsg
i01658: lda #18
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01756
        lda #220
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp i01743
        lda #4
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp i01756
i01743: lda #16
        ldy #9
        sta (z80_ix),y
i01756: lda #26
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i01806
        lda #0
        cmp varh
        beq *+5
        jmp i01806
        lda #0		; ANIMATE
        jsr animsp
i01806: lda #12
        ldy #6
        cmp (z80_ix),y
        bcc *+5
        jmp i01879
        lda #17
        ldy #6
        cmp (z80_ix),y
        bcs *+5
        jmp i01879
        lda #0
        cmp vara
        beq *+5
        jmp i01879
        lda #0		; ANIMATE
        jsr animsp
i01879: lda #0
        cmp scno
        beq *+5
        jmp i02112
        lda #2
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i02112
        lda #0
        cmp vara
        beq *+5
        jmp i01946
        lda #0		; ANIMATE
        jsr animsp
i01946: lda #15
        sta chary
        lda #7
        sta charx
        lda #17  	; MESSAGE
        jsr dmsg
        lda joyval	; KEY
        and #16
        beq :+
        jmp i02112
:
        lda #15
        sta chary
        lda #4
        sta charx
        lda #18  	; MESSAGE
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
        jsr scrr	; SCREENRIGHT
i02112: lda #6
        ldy #6
        cmp (z80_ix),y
        beq *+5
        jmp i02640
        lda #168
        ldy #8
        cmp (z80_ix),y
        beq *+5
        jmp i02204
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #1
        sta varh
        lda #8	; SPAWN
        sta z80_c
        lda #35
        sta z80_b
        jsr spawn
i02204: jsr cangd	; CANGODOWN
        beq :+
        jmp i02277
:
        lda #0
        cmp vara
        beq *+5
        jmp i02272
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
i02272: jmp i02344
i02277: jsr laddu	; CANGOUP
        beq :+
        jmp i02344
:
        lda #0
        cmp vara
        beq *+5
        jmp i02344
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
i02344: ldy #8
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ldy #8
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        asl a
        asl a
        asl a
        ldy #8
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        lsr a
        lsr a
        lsr a
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y
        asl a
        asl a
        asl a
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #8
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #8
        sta (z80_ix),y
        lda #8
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        lda #0		; ANIMATE
        jsr animsp
        lda #0
        cmp vard
        beq *+5
        jmp i02640
        lda #1 		; KILL
        sta deadf
i02640: rts
evnt09:
        lda #6
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00037
        lda #0
        ldy #11
        sta (z80_ix),y
j00037: lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00074
        lda vare
        clc
        adc #1
        sta vare
j00074: rts
evnt10:
        lda varf
        cmp varm
        beq *+5
        jmp k00032
        lda #0
        sta varm
        jmp k00047
k00032: lda varm
        clc
        adc #1
        sta varm
k00047: lda #8
        cmp varh
        beq *+5
        jmp k00076
        lda #0
        sta varh
        jmp k00092
k00076: lda varh
        clc
        adc #1
        sta varh
k00092: lda #40
        cmp vard
        beq *+5
        jmp k00121
        lda #0
        sta vard
        jmp k00137
k00121: lda vard
        clc
        adc #1
        sta vard
k00137: lda #25
        cmp varb
        beq *+5
        jmp k00198
        lda #0
        sta varb
        lda #0
        cmp varc
        bcc *+5
        jmp k00194
        lda varc
        sec
        sbc #1
        sta varc
k00194: jmp k00214
k00198: lda varb
        clc
        adc #1
        sta varb
k00214: lda vari
        cmp vara
        beq *+5
        jmp k00244
        lda #0
        sta vara
        jmp k00259
k00244: lda vara
        clc
        adc #1
        sta vara
k00259: rts
evnt11:
        ldy #9 	; KEY
        lda keys,y
        jsr ktest
        bcc :+
        jmp l00048
:
        lda #1	; NEXTLEVEL
        sta nexlev
        lda #1	; RESTART
        sta restfl
l00048: rts
evnt12:
        rts
evnt13:
        lda #0
        sta varn
        lda #0
        sta vark
        lda #0
        sta varg
        lda #1
        sta varl
        lda #5
        sta numlif
        lda #0
        sta vare
        lda #250
        sta varc
        lda #0
        sta vara
        lda #0
        sta varz
        jsr cls		; CLS
        rts
evnt14:
        lda #0
        sta varz
        lda #0
        sta varn
        lda #0
        sta chary
        lda #0
        sta charx
        lda varn	; DISPLAY
        jsr disply
        lda #4
        cmp scno
        beq *+5
        jmp o00079
        lda #212
        sta varp
        lda #88
        sta varq
o00079: lda #7
        cmp scno
        beq *+5
        jmp o00113
        lda #180
        sta varp
        lda #32
        sta varq
o00113: lda #0
        sta vara
        lda #0
        sta varm
        jsr cls		; CLS
        lda #6
        cmp scno
        beq *+5
        jmp o00172
        lda #2
        sta varf
        lda #0
        sta vari
        jmp o00188
o00172: lda #4
        sta varf
        lda #1
        sta vari
o00188: lda #5
        cmp scno
        beq *+5
        jmp o00233
        lda #23
        sta chary
        lda #1
        sta charx
        lda #24  	; MESSAGE
        jsr dmsg
o00233: lda #11
        sta chary
        lda #31
        sta charx
        lda #23  	; MESSAGE
        jsr dmsg
        lda #0
        sta chary
        lda #0
        sta charx
        lda #22  	; MESSAGE
        jsr dmsg
        lda #1
        sta chary
        lda #31
        sta charx
        lda #23  	; MESSAGE
        jsr dmsg
        rts
evnt15:
        rts
evnt16:
        lda numlif
        sec
        sbc #1
        sta numlif
        lda #1
        sta chary
        lda #19
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        lda #0
        cmp numlif
        beq *+5
        jmp q00114
        lda #10
        sta chary
        lda #12
        sta charx
        lda #0  	; MESSAGE
        jsr dmsg
        lda #50 	; DELAY
        jsr delay
        lda #1		; ENDGAME
        sta gamwon
q00114: lda #0
        sta vark
        lda #0
        sta vare
        lda #250
        sta varc
        lda #0
        sta varz
        rts
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
        .byte "GAME OVER!",141
        .byte "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",13
        .byte 141
        .byte "bcccd",141
        .byte "e",141
        .byte "ffffffffffffffffff",141
        .byte "fffffff",141
        .byte "fff",141
        .byte "ffff",230
        .byte "8x",254
        .byte "BONU",211
        .byte "eLg",229
        .byte 231
        .byte "h i",232
        .byte 233
        .byte "ONKE",217
        .byte "ON",199
        .byte 242
        .byte "PRESS SPACE TO STAR",212
        .byte "1 KEYS 2 LEFT JOYSTICK    ",160
        .byte 204
        .byte "SAVE YOUR PAPA",161
        .byte "PRESS FIR",197
        .byte "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",235
        .byte 235
        .byte "KEEP GOING TO MARIO",39
        .byte "s HIDEOUT",161
        .byte "lmm",243
        .byte "nnnnnnnnnnnnnnnnnnnnn",238
        .byte "lmmmmmmmmmmmmmmmmmmmm",243
        .byte "ooooooooooooooooooooo",239
        .byte "OUCH",161
        .byte "GET READY FOR THE FINAL BATTL",197
nummsg:
        .byte 31
chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 68,212,180,204,232,40,98,202
        .byte 138,10,20,85,20,10,74,74
        .byte 42,234,213,85,87,106,234,174
        .byte 0,16,0,16,0,16,0,16
        .byte 255,68,222,65,88,0,33,0
        .byte 201,37,36,18,146,146,141,217
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 84,42,91,91,86,166,206,76
        .byte 255,68,222,65,88,0,33,0
        .byte 53,122,246,239,238,238,85,25
        .byte 92,238,238,221,73,183,239,102
        .byte 0,16,0,16,40,124,108,0
        .byte 0,16,0,16,40,124,108,0
        .byte 0,16,0,16,40,124,108,0
        .byte 0,16,0,16,0,16,0,16
        .byte 0,16,0,16,0,16,0,16
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 124,238,238,238,254,254,124,0
        .byte 0,0,0,0,0,0,0,0
        .byte 32,0,16,48,240,224,192,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,1,3,3,7,7,7
        .byte 15,127,255,255,255,255,255,255
        .byte 181,218,218,216,36,251,251,225
        .byte 123,129,6,6,7,7,3,3
        .byte 190,129,96,96,224,192,192,128
        .byte 173,91,91,3,60,223,207,135
        .byte 8,0,34,56,31,15,0,0
        .byte 15,15,79,31,255,255,127,15
        .byte 0,0,0,0,0,0,0,32
        .byte 170,85,170,85,170,85,170,85
        .byte 3,3,0,2,0,7,141,109
        .byte 15,26,31,15,0,0,0,1
        .byte 175,158,161,191,191,191,191,149
        .byte 138,128,128,128,128,128,128,127
        .byte 255,0,88,88,88,88,88,0
        .byte 245,121,133,253,253,253,249,81
        .byte 0,0,0,0,192,224,32,192
        .byte 255,255,126,0,36,128,252,219
        .byte 161,1,1,1,1,1,1,254
        .byte 255,255,126,0,68,1,127,187
        .byte 7,3,1,0,0,0,0,0
        .byte 128,128,0,128,0,192,97,110
        .byte 0,0,0,128,128,192,192,192
        .byte 0,239,247,0,0,0,0,0
        .byte 255,0,0,0,0,0,0,0
        .byte 128,128,128,128,128,128,128,128
        .byte 0,0,128,128,128,128,128,128
        .byte 0,0,1,1,1,1,1,1
        .byte 1,1,1,1,1,1,1,1
        .byte 255,0,2,2,2,2,2,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 2,3,3,2,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,170
        .byte 0,0,255,255,255,255,255,255
        .byte 0,0,0,0,0,0,0,0
        .byte 3,14,30,62,118,118,119,251
        .byte 26,20,154,133,63,127,62,0
        .byte 0,60,60,0,60,60,0,0
        .byte 63,64,95,95,95,95,95,95
        .byte 252,2,250,250,250,250,250,250
        .byte 95,95,95,47,23,11,4,3
        .byte 250,250,250,244,232,208,32,192
        .byte 96,96,96,96,96,96,96,96
        .byte 6,6,6,6,6,6,6,6
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 192,192,64,128,192,192,192,192
        .byte 96,241,159,14,96,241,159,14
        .byte 192,192,64,128,192,192,192,192
        .byte 0,0,0,0,0,0,0,0
        .byte 120,252,252,204,24,24,0,12
        .byte 0,0,0,0,0,0,0,0
        .byte 170,85,170,85,170,85,170,85
        .byte 170,255,170,255,170,255,170,255
        .byte 255,68,222,65,88,0,33,0
        .byte 16,0,16,0,16,0,16,0
        .byte 0,0,0,0,0,0,0,170
        .byte 224,224,224,192,128,0,0,0
        .byte 15,15,15,31,33,65,67,63
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 1,1,1,1,1,1,0,0
        .byte 128,128,128,128,128,128,0,0
        .byte 3,3,0,2,0,7,13,13
        .byte 92,94,83,81,89,95,64,95
        .byte 58,74,138,154,250,250,2,250
        .byte 255,255,0,255,0,0,0,255
        .byte 32,32,32,32,0,32,80,232
        .byte 85,170,85,170,85,170,85,170
        .byte 0,0,0,112,200,232,112,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 238,255,238,255,238,255,238,255
        .byte 128,128,0,128,0,192,96,96
        .byte 32,48,32,48,32,48,32,48
        .byte 255,0,255,199,108,56,56,56
        .byte 56,56,56,124,255,255,0,255
        .byte 192,128,192,128,192,128,192,128
        .byte 255,255,255,255,255,255,255,255
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 3
        .byte 2
        .byte 2
        .byte 3
        .byte 6
        .byte 5
        .byte 5
        .byte 3
        .byte 2
        .byte 2
        .byte 3
        .byte 3
        .byte 3
        .byte 3
        .byte 3
        .byte 0
        .byte 2
        .byte 0
        .byte 5
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
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 1
        .byte 1
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
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 2
        .byte 0
        .byte 2
        .byte 1
        .byte 0
        .byte 5
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 6
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 0
        .byte 1
        .byte 0
        .byte 2
        .byte 0
        .byte 6
        .byte 5
        .byte 0
        .byte 3
        .byte 2
        .byte 2
        .byte 3
        .byte 0
sprgfx:
        .byte 0,52,14,40,28,180,60,138,60,62,54,126,118,64,119,56,123,128,125,176,61,176,27,184,3,152,24,0,29,152,3,184
        .byte 0,13,3,138,7,45,143,34,143,15,141,159,29,144,29,206,30,224,31,108,15,108,6,238,0,230,6,0,7,102,0,238
        .byte 64,3,128,226,65,203,163,200,227,195,227,103,7,100,135,115,7,184,7,219,3,219,129,187,128,57,1,128,129,217,128,59
        .byte 208,0,160,56,208,114,40,242,248,240,248,217,1,217,225,220,1,238,193,246,192,246,224,110,96,14,0,96,96,118,224,14
        .byte 0,0,7,52,14,40,30,180,30,138,30,62,15,126,27,56,205,192,221,230,188,123,24,61,0,8,0,3,0,7,0,0
        .byte 0,0,1,205,3,138,7,173,135,162,135,143,131,223,6,206,51,112,183,121,239,30,70,15,0,2,192,0,192,1,0,0
        .byte 0,0,64,115,128,226,65,235,161,232,225,227,224,247,129,179,12,220,109,222,187,199,209,131,128,0,48,0,112,0,0,0
        .byte 0,0,208,28,160,56,208,122,40,122,248,120,248,61,224,108,3,55,155,119,238,241,244,96,32,0,12,0,28,0,0,0
        .byte 0,0,14,52,28,40,60,180,60,138,52,62,118,126,119,56,123,128,61,176,29,184,3,152,27,128,28,24,1,184,3,128
        .byte 0,0,3,141,7,10,15,45,143,34,141,15,157,159,29,206,30,224,15,108,7,110,0,230,6,224,7,6,0,110,0,224
        .byte 0,0,64,227,129,194,67,203,163,200,227,67,231,103,135,115,7,184,3,219,129,219,128,57,1,184,129,193,128,27,0,56
        .byte 0,0,208,56,160,112,208,242,40,242,248,208,249,217,225,220,1,238,192,246,224,118,96,14,0,110,96,112,224,6,0,14
        .byte 0,52,0,40,12,180,28,138,60,62,54,126,54,64,119,56,123,128,125,176,125,176,59,184,24,24,1,128,27,152,28,56
        .byte 0,13,0,10,3,45,135,34,143,15,141,159,13,144,29,206,30,224,31,108,31,108,14,238,6,6,0,96,6,230,7,14
        .byte 64,3,128,2,64,203,161,200,227,195,227,103,3,100,135,115,7,184,7,219,7,219,131,187,129,129,0,24,129,185,129,195
        .byte 208,0,160,0,208,50,40,114,248,240,248,217,0,217,225,220,1,238,193,246,193,246,224,238,96,96,0,6,96,110,224,112
        .byte 44,0,20,112,45,56,81,60,124,60,126,108,2,110,28,238,1,222,13,190,13,188,29,216,25,192,0,24,25,184,29,192
        .byte 11,0,5,28,11,78,20,79,31,15,31,155,128,155,135,59,128,119,131,111,3,111,7,118,6,112,0,6,6,110,7,112
        .byte 2,192,1,71,130,211,197,19,199,195,199,230,224,38,225,206,224,29,224,219,192,219,129,221,1,156,128,1,129,155,1,220
        .byte 0,176,192,81,224,180,241,68,241,240,177,249,184,9,184,115,120,7,248,54,240,54,96,119,0,103,96,0,224,102,0,119
        .byte 0,0,44,224,20,112,45,120,81,120,124,120,126,240,28,216,3,179,103,187,222,61,188,24,16,0,192,0,224,0,0,0
        .byte 0,0,11,56,5,28,11,94,20,94,31,30,31,188,7,54,192,236,217,238,119,143,47,6,4,0,48,0,56,0,0,0
        .byte 0,0,2,206,1,71,130,215,133,23,135,199,7,239,129,205,48,59,182,123,221,227,139,193,1,0,12,0,14,0,0,0
        .byte 0,0,128,179,192,81,224,181,225,69,225,241,193,251,96,115,204,14,237,158,247,120,98,240,0,64,3,0,3,128,0,0
        .byte 0,0,44,112,20,56,45,60,81,60,124,108,126,110,28,238,1,222,13,188,29,184,25,192,1,216,24,56,29,128,1,192
        .byte 0,0,11,28,5,14,11,79,20,79,31,27,159,155,135,59,128,119,3,111,7,110,6,112,0,118,6,14,7,96,0,112
        .byte 0,0,2,199,129,67,194,211,197,19,199,198,231,230,225,206,224,29,192,219,129,219,1,156,128,29,129,131,1,216,0,28
        .byte 0,0,192,177,224,80,240,180,241,68,177,241,185,249,184,115,120,7,240,54,224,118,0,103,96,7,224,96,0,118,0,7
        .byte 44,0,20,0,45,48,81,56,124,60,126,108,2,108,28,238,1,222,13,190,13,190,29,220,24,24,1,128,25,216,28,56
        .byte 11,0,5,0,11,76,20,78,31,15,31,155,0,155,135,59,128,119,131,111,131,111,7,119,6,6,0,96,6,118,7,14
        .byte 2,192,1,64,2,211,133,19,199,195,199,230,192,38,225,206,224,29,224,219,224,219,193,221,129,129,0,24,129,157,129,195
        .byte 0,176,0,80,192,180,225,68,241,240,177,249,176,9,184,115,120,7,248,54,248,54,112,119,96,96,0,6,96,103,224,112
        .byte 0,44,0,20,0,45,0,81,0,124,0,126,0,60,0,0,204,248,206,252,142,28,80,236,141,248,205,176,141,160,69,0
        .byte 0,11,0,5,64,11,64,20,0,31,128,31,0,15,0,0,51,62,51,191,35,135,20,59,35,126,51,108,35,104,17,64
        .byte 192,2,64,1,208,2,16,5,192,7,224,7,192,3,0,0,140,207,204,239,200,225,197,14,136,223,12,219,8,218,4,80
        .byte 176,0,80,0,180,0,68,1,240,1,248,1,240,0,0,0,227,51,243,59,114,56,177,67,226,55,195,54,130,54,1,20
        .byte 0,88,0,40,0,90,0,162,0,248,0,252,0,8,0,112,28,0,204,248,222,60,146,222,77,238,77,124,221,120,141,176
        .byte 0,22,0,10,128,22,128,40,0,62,0,63,0,2,0,28,7,0,51,62,55,143,164,183,147,123,19,95,55,94,35,108
        .byte 128,5,128,2,160,5,32,10,128,15,192,15,128,0,0,7,1,192,140,207,205,227,233,45,228,222,196,215,141,215,8,219
        .byte 96,1,160,0,104,1,136,2,224,3,240,3,32,0,192,1,0,112,227,51,243,120,122,75,185,55,241,53,227,117,194,54
        .byte 204,232,205,212,17,212,73,252,204,232,228,0,194,0,129,252,192,252,225,182,192,22,128,6,192,4,224,0,112,0,0,0
        .byte 51,58,51,117,4,117,18,127,51,58,57,0,48,128,32,127,48,63,184,109,176,5,160,1,48,1,56,0,28,0,0,0
        .byte 140,206,76,221,65,29,196,159,140,206,14,64,12,32,200,31,204,15,110,27,108,1,104,0,76,0,14,0,7,0,0,0
        .byte 163,51,83,55,80,71,241,39,163,51,3,144,3,8,242,7,243,3,219,134,91,0,26,0,19,0,3,128,1,192,0,0
        .byte 0,0,108,232,109,212,1,212,93,252,236,232,220,0,184,0,216,0,232,0,220,0,190,0,222,0,188,0,216,0,0,0
        .byte 0,0,27,58,27,117,0,117,23,127,59,58,55,0,46,0,54,0,58,0,55,0,47,128,55,128,47,0,54,0,0,0
        .byte 0,0,134,206,70,221,64,29,197,223,142,206,13,192,11,128,13,128,14,128,13,192,11,224,13,224,11,192,13,128,0,0
        .byte 0,0,161,179,81,183,80,7,241,119,163,179,3,112,2,224,3,96,3,160,3,112,2,248,3,120,2,240,3,96,0,0
        .byte 16,53,2,8,8,180,60,138,60,62,54,126,118,64,119,56,123,128,125,176,61,176,27,184,3,152,24,0,29,152,3,184
        .byte 68,13,0,130,2,45,143,34,143,15,141,159,29,144,29,206,30,224,31,108,15,108,6,238,0,230,6,0,7,102,0,238
        .byte 81,3,128,32,64,139,163,200,227,195,227,103,7,100,135,115,7,184,7,219,3,219,129,187,128,57,1,128,129,217,128,59
        .byte 212,64,32,8,208,34,40,242,248,240,248,217,1,217,225,220,1,238,193,246,192,246,224,110,96,14,0,96,96,118,224,14
        .byte 0,0,14,52,28,8,60,180,124,138,118,62,118,126,55,56,27,128,3,184,25,152,29,128,3,152,0,56,1,128,3,128
        .byte 0,0,3,141,7,2,15,45,159,34,157,143,157,159,13,206,6,224,0,238,6,102,7,96,0,230,0,14,0,96,0,224
        .byte 0,0,64,227,129,192,67,203,167,200,231,99,231,103,131,115,1,184,128,59,129,153,1,216,128,57,128,3,0,24,0,56
        .byte 0,0,208,56,32,112,208,242,41,242,249,216,249,217,224,220,0,110,224,14,96,102,0,118,96,14,224,0,0,6,0,14
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,3,192,7,224,7,224,3,192,1,128,3,192,3,192,7,224,7,224,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,240,1,248,1,248,0,240,0,96,0,240,0,240,1,248,1,248,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,60,0,126,0,126,0,60,0,24,0,60,0,60,0,126,0,126,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,15,128,31,128,31,0,15,0,6,0,15,0,15,128,31,128,31,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,54,0,109,0,109,0,127,0,54,0,0,0,14,14,223,31,223,255,23,119,10,170,221,221,207,255
        .byte 0,0,0,0,0,0,13,128,27,64,27,64,31,192,13,128,0,0,131,131,247,199,247,255,197,221,130,170,119,119,243,255
        .byte 0,0,0,0,0,0,3,96,6,208,6,208,7,240,3,96,0,0,224,224,253,241,253,255,113,119,160,170,221,221,252,255
        .byte 0,0,0,0,0,0,0,216,1,180,1,180,1,252,0,216,0,0,56,56,127,124,255,127,220,93,168,42,119,119,255,63
        .byte 0,28,0,126,0,242,108,240,218,120,218,96,254,192,108,224,1,128,15,0,30,0,216,0,192,2,4,70,222,238,207,252
        .byte 0,7,128,31,128,60,27,60,54,158,54,152,63,176,27,56,0,96,3,192,7,128,54,0,176,0,129,17,183,187,51,255
        .byte 192,1,224,7,32,15,6,207,141,167,13,166,15,236,6,206,0,24,0,240,1,224,13,128,44,0,96,68,237,238,204,255
        .byte 112,0,248,1,200,3,193,179,227,105,131,105,3,251,129,179,0,6,0,60,0,120,3,96,11,0,24,17,187,123,243,63
        .byte 0,0,0,0,0,0,0,108,0,182,0,182,0,254,0,108,0,0,112,112,248,251,255,251,238,232,85,80,187,187,255,243
        .byte 0,0,0,0,0,0,0,27,128,45,128,45,128,63,0,27,0,0,28,28,254,62,255,254,59,186,21,84,238,238,255,252
        .byte 0,0,0,0,0,0,192,6,96,11,96,11,224,15,192,6,0,0,7,7,191,143,191,255,142,238,5,85,187,187,63,255
        .byte 0,0,0,0,0,0,176,1,216,2,216,2,248,3,176,1,0,0,193,193,239,227,239,255,163,187,65,85,238,238,207,255
        .byte 56,0,126,0,79,0,15,54,30,91,6,91,3,127,7,54,1,128,0,240,0,120,0,27,64,3,98,32,119,123,63,243
        .byte 14,0,31,128,19,192,131,205,199,150,193,150,192,223,129,205,0,96,0,60,0,30,192,6,208,0,24,136,221,222,207,252
        .byte 3,128,7,224,4,240,96,243,177,229,176,101,240,55,96,115,0,24,0,15,128,7,176,1,52,0,6,34,183,119,51,255
        .byte 0,224,1,248,1,60,216,60,108,121,108,25,252,13,216,28,0,6,192,3,224,1,108,0,13,0,129,136,237,221,204,255
        .byte 91,0,114,0,52,0,244,0,127,118,118,235,246,235,55,118,59,8,221,126,62,62,190,12,28,0,0,0,0,0,0,0
        .byte 22,192,28,128,13,0,61,0,159,221,221,186,253,186,141,221,14,194,183,95,143,143,47,131,7,0,0,0,0,0,0,0
        .byte 5,176,7,32,3,64,15,64,103,247,183,110,191,110,99,119,131,176,237,215,227,227,203,224,1,192,0,0,0,0,0,0
        .byte 1,108,1,200,0,208,3,208,217,253,173,219,175,219,216,221,32,236,251,117,248,248,50,248,0,112,0,0,0,0,0,0
        .byte 0,0,4,0,28,0,7,118,6,235,6,235,199,118,67,8,220,126,126,62,126,12,46,0,14,0,14,0,30,0,40,0
        .byte 0,0,1,0,7,0,129,221,193,186,193,186,177,221,16,194,183,31,159,143,31,131,11,128,3,128,3,128,7,128,10,0
        .byte 0,0,0,64,1,192,96,119,176,110,176,110,108,119,132,48,237,199,231,227,199,224,2,224,0,224,0,224,1,224,2,128
        .byte 0,0,0,16,0,112,216,29,172,27,172,27,219,29,33,12,251,113,249,248,49,248,0,184,0,56,0,56,0,120,0,160
        .byte 0,0,0,0,0,0,247,223,54,219,54,219,246,219,198,219,198,219,247,223,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,253,247,205,182,205,182,253,182,241,182,241,182,253,247,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,255,125,179,109,179,109,191,109,188,109,188,109,255,125,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,127,223,108,219,108,219,111,219,111,27,111,27,127,223,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 2,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,2,0,0,0,0,0,7,0,15,128,13,128
        .byte 0,128,0,0,0,128,0,0,0,128,0,0,0,128,0,0,0,128,0,0,0,128,0,0,0,0,1,192,3,224,3,96
        .byte 0,32,0,0,0,32,0,0,0,32,0,0,0,32,0,0,0,32,0,0,0,32,0,0,0,0,0,112,0,248,0,216
        .byte 0,8,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,8,0,0,0,0,0,28,0,62,0,54
        .byte 0,0,6,128,149,96,150,240,96,96,49,0,222,48,192,48,9,0,15,0,127,128,127,128,63,0,206,0,224,0,7,0
        .byte 0,0,1,160,37,88,37,188,24,24,12,64,55,140,48,12,2,64,3,192,31,224,31,224,15,192,51,128,56,0,1,192
        .byte 0,0,0,104,9,86,9,111,6,6,3,16,13,227,12,3,0,144,0,240,7,248,7,248,3,240,12,224,14,0,0,112
        .byte 0,0,0,26,130,85,194,91,129,129,0,196,195,120,195,0,0,36,0,60,1,254,1,254,0,252,3,56,3,128,0,28
        .byte 0,0,3,64,74,176,75,120,48,48,24,128,15,54,0,54,0,0,24,80,31,224,31,192,15,128,0,64,12,224,14,0
        .byte 0,0,0,208,18,172,18,222,12,12,6,32,131,205,128,13,0,0,6,20,7,248,7,240,3,224,0,16,3,56,3,128
        .byte 0,0,0,52,4,171,132,183,3,3,1,136,96,243,96,3,0,0,1,133,1,254,1,252,0,248,0,4,0,206,0,224
        .byte 0,0,0,13,193,42,225,45,192,192,0,98,216,60,216,0,0,0,64,97,128,127,0,127,0,62,0,1,128,51,0,56
        .byte 0,0,3,224,7,240,6,48,6,48,7,240,3,224,0,0,0,128,0,128,0,128,0,128,0,128,0,184,0,176,0,184
        .byte 0,0,0,248,1,252,1,140,1,140,1,252,0,248,0,0,0,32,0,32,0,32,0,32,0,32,0,46,0,44,0,46
        .byte 0,0,0,62,0,127,0,99,0,99,0,127,0,62,0,0,0,8,0,8,0,8,0,8,0,8,128,11,0,11,128,11
        .byte 0,0,128,15,192,31,192,24,192,24,192,31,128,15,0,0,0,2,0,2,0,2,0,2,0,2,224,2,192,2,224,2
        .byte 0,0,1,192,3,224,3,96,3,96,3,224,1,192,0,0,0,128,0,128,0,128,0,128,0,128,0,160,0,128,0,160
        .byte 0,0,0,112,0,248,0,216,0,216,0,248,0,112,0,0,0,32,0,32,0,32,0,32,0,32,0,40,0,32,0,40
        .byte 0,0,0,28,0,62,0,54,0,54,0,62,0,28,0,0,0,8,0,8,0,8,0,8,0,8,0,10,0,8,0,10
        .byte 0,0,0,7,128,15,128,13,128,13,128,15,0,7,0,0,0,2,0,2,0,2,0,2,0,2,128,2,0,2,128,2
        .byte 0,0,0,128,0,128,0,128,0,128,0,128,0,128,0,0,0,128,0,128,0,128,0,128,0,128,0,128,0,128,0,128
        .byte 0,0,0,32,0,32,0,32,0,32,0,32,0,32,0,0,0,32,0,32,0,32,0,32,0,32,0,32,0,32,0,32
        .byte 0,0,0,8,0,8,0,8,0,8,0,8,0,8,0,0,0,8,0,8,0,8,0,8,0,8,0,8,0,8,0,8
        .byte 0,0,0,2,0,2,0,2,0,2,0,2,0,2,0,0,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2
        .byte 0,0,3,128,7,192,6,192,6,192,7,192,3,128,0,0,1,0,1,0,1,0,1,0,1,0,5,0,1,0,5,0
        .byte 0,0,0,224,1,240,1,176,1,176,1,240,0,224,0,0,0,64,0,64,0,64,0,64,0,64,1,64,0,64,1,64
        .byte 0,0,0,56,0,124,0,108,0,108,0,124,0,56,0,0,0,16,0,16,0,16,0,16,0,16,0,80,0,16,0,80
        .byte 0,0,0,14,0,31,0,27,0,27,0,31,0,14,0,0,0,4,0,4,0,4,0,4,0,4,0,20,0,4,0,20
        .byte 0,0,255,255,128,1,170,171,191,255,184,30,167,238,223,54,223,184,191,247,128,5,163,133,168,7,32,8,16,16,15,224
        .byte 0,0,255,255,96,0,234,170,239,255,174,7,169,251,183,205,55,238,239,253,96,1,104,225,234,1,8,2,4,4,3,248
        .byte 0,0,255,255,24,0,186,170,251,255,235,129,234,126,109,243,141,251,123,255,88,0,90,56,122,128,130,0,1,1,0,254
        .byte 0,0,255,255,6,0,174,170,254,255,122,224,186,159,219,124,227,126,222,255,22,0,22,142,30,160,32,128,64,64,128,63
        .byte 255,255,128,1,170,171,191,255,184,62,167,206,223,54,223,182,191,240,128,7,163,133,163,133,160,7,32,72,16,16,15,224
        .byte 255,255,96,0,234,170,239,255,174,15,169,243,183,205,183,237,47,252,224,1,104,225,104,225,232,1,8,18,4,4,3,248
        .byte 255,255,24,0,186,170,251,255,235,131,234,124,109,243,109,251,11,255,120,0,90,56,90,56,122,0,130,4,1,1,0,254
        .byte 255,255,6,0,174,170,254,255,250,224,58,159,219,124,219,126,194,255,30,0,22,142,22,142,30,128,32,129,64,64,128,63
        .byte 255,255,128,1,170,171,188,63,179,222,175,38,223,154,223,208,191,247,128,5,163,149,35,151,32,16,16,32,15,192,0,0
        .byte 255,255,96,0,234,170,239,15,172,247,171,201,183,230,55,244,239,253,96,1,104,229,200,229,8,4,4,8,3,240,0,0
        .byte 255,255,24,0,186,170,251,195,235,61,106,242,173,249,13,253,123,255,88,0,90,57,114,57,2,1,1,2,0,252,0,0
        .byte 255,255,6,0,174,170,254,240,122,207,154,188,107,126,67,127,222,255,22,0,86,142,92,142,64,128,128,64,0,63,0,0
        .byte 7,254,56,0,66,192,77,82,158,210,140,12,129,24,128,240,128,0,126,30,125,238,27,246,232,6,234,22,2,80,1,224
        .byte 129,255,14,0,16,176,147,84,167,180,35,3,32,70,32,60,32,0,159,135,159,123,134,253,186,1,186,133,0,148,0,120
        .byte 224,127,3,128,4,44,36,213,41,237,200,192,136,17,8,15,8,0,231,225,231,222,97,191,110,128,110,161,0,37,0,30
        .byte 248,31,0,224,1,11,73,53,74,123,50,48,98,4,194,3,2,0,121,248,185,247,216,111,27,160,91,168,64,9,128,7
        .byte 60,62,66,192,77,82,158,210,140,12,129,24,128,240,128,0,125,238,123,246,24,6,234,54,234,22,1,224,0,0,0,0
        .byte 143,15,16,176,147,84,167,180,35,3,32,70,32,60,32,0,159,123,158,253,134,1,186,141,186,133,0,120,0,0,0,0
        .byte 227,195,4,44,36,213,41,237,200,192,136,17,8,15,8,0,231,222,103,191,97,128,110,163,110,161,0,30,0,0,0,0
        .byte 248,240,1,11,73,53,74,123,50,48,98,4,194,3,2,0,185,247,217,239,24,96,219,168,91,168,128,7,0,0,0,0
        .byte 7,254,56,0,66,192,77,82,158,210,140,12,129,24,128,240,126,30,125,238,27,246,232,6,235,22,2,16,1,224,0,0
        .byte 129,255,14,0,16,176,147,84,167,180,35,3,32,70,32,60,159,135,159,123,134,253,186,1,186,197,0,132,0,120,0,0
        .byte 224,127,3,128,4,44,36,213,41,237,200,192,136,17,8,15,231,225,231,222,97,191,110,128,110,177,0,33,0,30,0,0
        .byte 248,31,0,224,1,11,73,53,74,123,50,48,98,4,194,3,121,248,185,247,216,111,27,160,91,172,64,8,128,7,0,0
        .byte 0,0,255,255,128,0,170,171,191,255,176,31,207,231,191,59,127,187,127,253,0,3,67,139,72,11,64,8,48,48,15,224
        .byte 0,0,255,255,32,0,234,170,239,255,236,7,243,249,239,206,223,238,95,255,192,0,208,226,210,2,16,2,12,12,3,248
        .byte 0,0,255,255,8,0,186,170,251,255,251,1,124,254,187,243,183,251,215,255,48,0,180,56,180,128,132,0,3,3,0,254
        .byte 0,0,255,255,2,0,174,170,254,255,126,192,159,63,238,252,237,254,245,255,12,0,45,14,45,32,33,0,192,192,128,63
        .byte 255,255,128,0,170,171,191,255,176,63,207,207,191,55,191,183,127,251,0,3,67,139,67,139,64,11,64,72,32,16,31,224
        .byte 255,255,32,0,234,170,239,255,236,15,243,243,239,205,239,237,223,254,192,0,208,226,208,226,208,2,16,18,8,4,7,248
        .byte 255,255,8,0,186,170,251,255,251,3,252,252,123,243,123,251,183,255,48,0,180,56,180,56,180,0,132,4,2,1,1,254
        .byte 255,255,2,0,174,170,254,255,254,192,63,63,222,252,222,254,237,255,12,0,45,14,45,14,45,0,33,1,64,128,128,127
        .byte 255,255,128,0,170,171,184,63,167,223,223,39,191,155,191,219,127,253,0,3,67,139,64,8,48,48,15,192,0,0,0,0
        .byte 255,255,32,0,234,170,238,15,233,247,247,201,239,230,239,246,95,255,192,0,208,226,16,2,12,12,3,240,0,0,0,0
        .byte 255,255,8,0,186,170,251,131,250,125,125,242,187,249,187,253,215,255,48,0,180,56,132,0,3,3,0,252,0,0,0,0
        .byte 255,255,2,0,174,170,254,224,126,159,159,124,110,254,110,255,245,255,12,0,45,14,33,0,192,192,0,63,0,0,0,0
        .byte 0,0,0,0,1,192,3,224,59,246,127,255,127,239,59,239,1,198,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,112,0,248,142,253,223,255,223,251,206,251,128,113,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,28,0,62,99,191,247,255,247,254,243,190,96,28,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,7,128,15,216,239,253,255,189,255,188,239,24,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 63,240,127,248,255,204,255,196,255,230,127,126,62,63,56,15,126,63,252,159,252,158,255,254,127,254,63,252,0,0,0,0
        .byte 15,252,31,254,63,243,63,241,191,249,159,223,207,143,206,3,223,143,255,39,191,39,191,255,159,255,15,255,0,0,0,0
        .byte 3,255,135,255,207,252,79,252,111,254,231,247,243,227,243,128,247,227,255,201,239,201,239,255,231,255,195,255,0,0,0,0
        .byte 192,255,225,255,51,255,19,255,155,255,249,253,252,248,60,224,253,248,127,242,123,242,251,255,249,255,240,255,0,0,0,0
        .byte 0,0,63,224,127,240,255,152,255,140,255,204,127,254,63,126,60,30,62,62,124,158,253,222,255,254,255,254,127,252,63,248
        .byte 0,0,15,248,31,252,63,230,63,227,63,243,159,255,143,223,143,7,143,143,159,39,191,119,191,255,191,255,31,255,15,254
        .byte 0,0,3,254,7,255,143,249,207,248,207,252,231,255,227,247,227,193,227,227,231,201,239,221,239,255,239,255,199,255,131,255
        .byte 0,0,128,255,193,255,99,254,51,254,51,255,249,255,248,253,120,240,248,248,121,242,123,247,251,255,251,255,241,255,224,255
        .byte 120,112,204,248,196,248,228,248,252,240,253,192,126,0,126,0,126,224,126,112,127,120,127,120,255,124,255,124,126,60,60,0
        .byte 30,28,51,62,49,62,57,62,63,60,63,112,31,128,31,128,31,184,31,156,31,222,31,222,63,223,63,223,31,143,15,0
        .byte 7,135,140,207,140,79,142,79,15,207,15,220,7,224,7,224,7,238,7,231,135,247,135,247,207,247,207,247,199,227,3,192
        .byte 193,225,227,51,227,19,227,147,195,243,3,247,1,248,1,248,129,251,193,249,225,253,225,253,243,253,243,253,241,248,0,240
        .byte 0,0,120,112,204,248,196,248,228,248,252,240,253,192,126,0,126,0,126,224,255,120,255,124,127,124,126,60,60,0,0,0
        .byte 0,0,30,28,51,62,49,62,57,62,63,60,63,112,31,128,31,128,31,184,63,222,63,223,31,223,31,143,15,0,0,0
        .byte 0,0,7,135,140,207,140,79,142,79,15,207,15,220,7,224,7,224,7,238,143,247,207,247,199,247,199,227,3,192,0,0
        .byte 0,0,193,225,227,51,227,19,227,147,195,243,3,247,1,248,1,248,129,251,227,253,243,253,241,253,241,248,0,240,0,0
        .byte 15,192,63,252,127,206,127,198,127,230,59,252,1,216,1,224,56,240,124,248,204,124,196,124,230,252,127,248,63,240,3,192
        .byte 3,240,15,255,159,243,159,241,159,249,14,255,0,118,0,120,14,60,31,62,51,31,49,31,57,191,31,254,15,252,0,240
        .byte 0,252,195,255,231,252,103,252,103,254,195,191,128,29,0,30,3,143,135,207,204,199,204,71,206,111,135,255,3,255,0,60
        .byte 0,63,240,255,57,255,25,255,153,255,240,239,96,7,128,7,192,227,225,243,243,49,243,17,243,155,225,255,192,255,0,15
        .byte 1,248,31,206,63,198,127,230,127,252,127,216,59,192,1,224,1,240,1,248,56,252,124,124,78,124,71,252,103,248,63,240
        .byte 0,126,135,243,143,241,159,249,31,255,31,246,14,240,0,120,0,124,0,126,14,63,31,31,19,159,17,255,25,254,15,252
        .byte 128,31,225,252,99,252,103,254,199,255,135,253,3,188,0,30,0,31,128,31,195,143,199,199,196,231,196,127,134,127,3,255
        .byte 224,7,56,127,24,255,153,255,241,255,97,255,0,239,128,7,192,7,224,7,240,227,241,241,241,57,241,31,225,159,192,255
        .byte 0,0,0,0,0,0,0,0,3,192,6,32,7,240,3,228,0,24,3,250,7,205,7,205,7,157,3,253,3,250,0,228
        .byte 0,0,0,0,0,0,0,0,0,240,1,136,1,252,0,249,0,6,128,254,65,243,65,243,65,231,64,255,128,254,0,57
        .byte 0,0,0,0,0,0,0,0,0,60,0,98,0,127,64,62,128,1,160,63,208,124,208,124,208,121,208,63,160,63,64,14
        .byte 0,0,0,0,0,0,0,0,0,15,128,24,192,31,144,15,96,0,232,15,52,31,52,31,116,30,244,15,232,15,144,3
        .byte 0,0,0,48,0,112,56,64,60,0,2,112,12,56,24,24,24,128,0,128,1,128,1,128,0,0,0,0,0,0,0,0
        .byte 0,0,0,12,0,28,14,16,15,0,0,156,3,14,6,6,6,32,0,32,0,96,0,96,0,0,0,0,0,0,0,0
        .byte 0,0,0,3,0,7,3,132,3,192,0,39,128,195,129,129,1,136,0,8,0,24,0,24,0,0,0,0,0,0,0,0
        .byte 0,0,192,0,192,1,0,225,0,240,192,9,224,48,96,96,0,98,0,2,0,6,0,6,0,0,0,0,0,0,0,0
        .byte 0,24,0,24,96,32,96,0,8,0,0,0,0,32,8,24,48,12,49,12,0,0,1,128,1,128,0,0,0,0,0,0
        .byte 0,6,0,6,24,8,24,0,2,0,0,0,0,8,2,6,12,3,12,67,0,0,0,96,0,96,0,0,0,0,0,0
        .byte 128,1,128,1,6,2,6,0,0,128,0,0,0,2,128,129,195,0,195,16,0,0,0,24,0,24,0,0,0,0,0,0
        .byte 96,0,96,0,129,128,1,128,0,32,0,0,128,0,96,32,48,192,48,196,0,0,0,6,0,6,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 123,107,69,180,69,180,70,72,71,188,67,243,65,227,64,1,66,20,64,135,68,15,71,255,71,255,67,255,64,255,64,7
        .byte 222,218,17,109,17,109,17,146,17,239,208,252,208,120,80,0,16,133,208,33,209,3,209,255,209,255,208,255,208,63,208,1
        .byte 183,182,68,91,68,91,132,100,196,123,52,63,52,30,20,0,68,33,116,8,244,64,244,127,244,127,244,63,244,15,116,0
        .byte 173,237,209,22,209,22,33,25,241,30,205,15,141,7,5,0,81,8,29,2,61,16,253,31,253,31,253,15,253,3,29,0
        .byte 214,222,45,162,45,162,18,98,61,226,207,194,199,130,128,2,40,66,225,2,240,34,255,226,255,226,255,194,255,2,224,2
        .byte 181,183,139,104,139,104,132,152,143,120,179,240,177,224,160,0,138,16,184,64,188,8,191,248,191,248,191,240,191,192,184,0
        .byte 237,109,34,218,34,218,33,38,35,222,44,252,44,120,40,0,34,132,46,16,47,2,47,254,47,254,47,252,47,240,46,0
        .byte 123,91,136,182,136,182,136,73,136,247,11,63,11,30,10,0,8,161,11,132,139,192,139,255,139,255,11,255,11,252,11,128
        .byte 255,255,0,0,142,49,159,121,189,221,191,253,158,57,159,249,255,255,248,15,183,247,191,253,159,253,135,241,184,13,191,125
        .byte 255,255,0,0,99,140,103,222,111,119,111,255,103,142,103,254,255,255,254,3,237,253,111,255,103,255,97,252,110,3,111,223
        .byte 255,255,0,0,24,227,153,247,219,221,219,255,153,227,153,255,255,255,255,128,123,127,219,255,217,255,24,127,219,128,219,247
        .byte 255,255,0,0,198,56,230,125,118,247,246,255,230,120,230,127,255,255,63,224,222,223,246,255,246,127,198,31,54,224,246,253
        .byte 255,255,0,0,140,17,158,57,185,205,190,61,159,249,154,41,240,15,240,7,224,7,179,229,191,61,158,253,175,241,176,13
        .byte 255,255,0,0,99,4,103,142,110,115,111,143,103,254,102,138,252,3,252,1,248,1,108,249,111,207,103,191,107,252,108,3
        .byte 255,255,0,0,24,193,153,227,219,156,219,227,153,255,153,162,255,0,127,0,126,0,91,62,219,243,217,239,26,255,219,0
        .byte 255,255,0,0,70,48,230,120,54,231,246,248,230,127,166,104,63,192,31,192,31,128,150,207,246,252,246,123,198,191,54,192
        .byte 64,15,64,127,65,255,67,255,67,255,67,255,67,255,65,255,65,254,64,124,65,0,64,72,67,0,79,248,74,240,127,255
        .byte 208,3,208,31,208,127,208,255,208,255,208,255,208,255,208,127,144,127,16,31,16,64,16,18,16,192,19,254,18,188,223,255
        .byte 244,0,244,7,244,31,244,63,244,63,244,63,244,63,244,31,228,31,196,7,4,16,132,4,4,48,132,255,4,175,247,255
        .byte 61,0,253,1,253,7,253,15,253,15,253,15,253,15,253,7,249,7,241,1,1,4,33,1,1,12,225,63,193,43,253,255
        .byte 175,245,158,121,161,133,191,253,191,253,191,253,170,169,149,81,128,1,128,1,128,1,128,1,128,1,128,1,0,0,255,255
        .byte 107,253,103,158,104,97,111,255,111,255,111,255,106,170,101,84,96,0,96,0,96,0,96,0,96,0,96,0,0,0,255,255
        .byte 90,255,153,231,90,24,219,255,219,255,219,255,154,170,25,85,24,0,24,0,24,0,24,0,24,0,24,0,0,0,255,255
        .byte 214,191,230,121,22,134,246,255,246,255,246,255,166,170,70,85,6,0,6,0,6,0,6,0,6,0,6,0,0,0,255,255
        .byte 240,2,254,2,255,130,255,194,255,194,255,194,255,194,255,130,127,130,62,2,0,130,18,2,0,194,31,242,15,82,255,254
        .byte 188,0,191,128,191,224,191,240,191,240,191,240,191,240,191,224,159,224,143,128,128,32,132,128,128,48,135,252,131,212,191,255
        .byte 47,0,47,224,47,248,47,252,47,252,47,252,47,252,47,248,39,248,35,224,32,8,33,32,32,12,33,255,32,245,239,255
        .byte 11,192,11,248,11,254,11,255,11,255,11,255,11,255,11,254,9,254,8,248,8,2,8,72,8,3,200,127,72,61,251,255
        .byte 0,3,6,131,149,96,150,240,96,102,49,6,30,0,0,0,9,0,15,0,15,128,127,128,127,0,62,224,0,0,28,0
        .byte 192,0,193,160,37,88,37,188,152,25,140,65,7,128,0,0,2,64,3,192,3,224,31,224,31,192,15,184,0,0,7,0
        .byte 48,0,48,104,9,86,9,111,102,6,99,16,1,224,0,0,0,144,0,240,0,248,7,248,7,240,3,238,0,0,1,192
        .byte 12,0,12,26,130,85,194,91,153,129,24,196,0,120,0,0,0,36,0,60,0,62,1,254,1,252,128,251,0,0,0,112
        .byte 0,0,3,64,74,176,75,123,48,51,24,128,15,6,0,6,0,0,0,0,17,192,63,192,63,128,31,0,0,0,14,192
        .byte 0,0,0,208,18,172,210,222,204,12,6,32,131,193,128,1,0,0,0,0,4,112,15,240,15,224,7,192,0,0,3,176
        .byte 0,0,0,52,4,171,180,183,51,3,1,136,96,240,96,0,0,0,0,0,1,28,3,252,3,248,1,240,0,0,0,236
        .byte 0,0,0,13,193,42,237,45,204,192,0,98,24,60,24,0,0,0,0,0,0,71,0,255,0,254,0,124,0,0,0,59
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,111,190,109,182,109,182,109,182,109,182,109,182,111,190,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,155,239,155,109,155,109,155,109,155,109,155,109,155,239,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,230,251,102,219,102,219,102,219,102,219,102,219,230,251,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,249,190,217,182,217,182,217,182,217,182,217,182,249,190,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,218,0,78,0,44,0,47,110,254,215,110,215,111,110,236,16,220,126,187,124,124,48,125,0,56,0,0,0,0,0,0
        .byte 128,54,128,19,0,11,192,11,155,191,181,219,245,219,27,187,4,55,223,174,31,31,76,31,0,14,0,0,0,0,0,0
        .byte 160,13,224,4,192,2,240,2,230,239,237,118,253,118,198,238,193,13,183,235,199,199,211,7,128,3,0,0,0,0,0,0
        .byte 104,3,56,1,176,0,188,0,249,187,187,93,191,93,177,187,112,67,237,250,241,241,244,193,224,0,0,0,0,0,0,0
        .byte 0,0,0,32,0,56,110,224,215,96,215,96,110,227,16,194,126,59,124,126,48,126,0,116,0,112,0,112,0,120,0,20
        .byte 0,0,0,8,0,14,27,184,53,216,53,216,219,184,132,48,223,142,159,31,140,31,0,29,0,28,0,28,0,30,0,5
        .byte 0,0,0,2,128,3,6,238,13,118,13,118,54,238,33,12,183,227,231,199,227,7,64,7,0,7,0,7,128,7,64,1
        .byte 0,0,128,0,224,0,129,187,131,93,131,93,141,187,8,67,237,248,249,241,248,193,208,1,192,1,192,1,224,1,80,0
        .byte 122,238,247,119,231,119,67,162,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 158,187,253,221,249,221,144,232,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 231,174,127,119,126,119,36,58,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 185,235,223,221,223,157,137,14,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 192,0,248,165,119,187,7,154,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 48,0,126,41,221,238,129,230,0,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 12,0,95,138,183,123,160,121,0,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 3,0,151,226,237,222,104,30,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,40,0,168,0,0,0,122,0,56,0,0,0,48,108,56,84,56,109,28,17,28,248,14,252,7,2
        .byte 0,0,0,0,0,0,10,0,42,0,0,0,30,128,14,0,0,0,12,27,14,21,78,27,71,4,7,62,3,191,129,192
        .byte 0,0,0,0,0,0,2,128,10,128,0,0,7,160,3,128,0,0,195,6,67,133,211,134,17,193,129,207,192,239,32,112
        .byte 0,0,0,0,0,0,0,160,2,160,0,0,1,232,0,224,0,0,176,193,80,225,180,225,68,112,224,115,240,59,8,28
        .byte 0,0,0,0,0,0,5,0,21,0,0,0,15,64,7,0,0,0,3,0,3,108,3,84,7,109,7,17,6,252,2,254
        .byte 0,0,0,0,0,0,1,64,5,64,0,0,3,208,1,192,0,0,0,192,0,219,0,213,65,219,65,196,1,191,128,191
        .byte 0,0,0,0,0,0,0,80,1,80,0,0,0,244,0,112,0,0,0,48,192,54,64,53,208,118,16,113,192,111,224,47
        .byte 0,0,0,0,0,0,0,20,0,84,0,0,0,61,0,28,0,0,0,12,176,13,80,13,180,29,68,28,240,27,248,11
        .byte 10,72,2,123,13,254,31,255,31,17,30,252,13,243,3,145,0,96,2,240,6,240,6,144,15,96,14,144,6,240,0,96
        .byte 2,146,192,158,131,127,199,255,71,196,7,191,195,124,64,228,0,24,0,188,1,188,1,164,3,216,3,164,1,188,0,24
        .byte 128,164,176,39,224,223,241,255,17,241,193,239,48,223,16,57,0,6,0,47,0,111,0,105,0,246,0,233,0,111,0,6
        .byte 32,41,236,9,248,55,252,127,68,124,240,123,204,55,68,14,128,1,192,11,192,27,64,26,128,61,64,58,192,27,128,1
        .byte 0,160,3,224,134,0,67,4,255,4,127,126,126,248,0,200,0,48,0,120,1,120,3,72,7,176,7,72,3,120,0,48
        .byte 0,40,0,248,33,128,16,193,63,193,159,223,31,190,0,50,0,12,0,30,0,94,0,210,1,236,1,210,0,222,0,12
        .byte 0,10,0,62,8,96,68,48,79,240,231,247,135,239,128,12,0,3,128,7,128,23,128,52,0,123,128,116,128,55,0,3
        .byte 128,2,128,15,2,24,17,12,19,252,249,253,225,251,32,3,192,0,224,1,224,5,32,13,192,30,32,29,224,13,192,0
        .byte 6,0,15,96,9,112,6,240,9,96,15,96,15,64,6,0,9,48,63,184,23,124,128,252,255,252,127,184,222,64,16,80
        .byte 1,128,3,216,2,92,1,188,2,88,3,216,3,208,1,128,2,76,15,238,5,223,32,63,63,255,31,238,55,144,4,20
        .byte 0,96,0,246,0,151,0,111,0,150,0,246,0,244,0,96,0,147,131,251,193,119,200,15,207,255,135,251,13,228,1,5
        .byte 0,24,128,61,192,37,192,27,128,37,128,61,0,61,0,24,192,36,224,254,240,93,242,3,243,255,225,254,3,121,64,65
        .byte 12,0,30,192,18,224,13,224,18,192,30,128,30,0,12,0,18,0,31,126,126,255,44,254,32,255,0,2,3,192,2,128
        .byte 3,0,7,176,4,184,3,120,4,176,7,160,7,128,3,0,4,128,135,223,223,191,139,63,200,63,128,0,0,240,0,160
        .byte 0,192,1,236,1,46,0,222,1,44,1,232,1,224,0,192,1,32,225,247,247,239,226,207,242,15,32,0,0,60,0,40
        .byte 0,48,0,123,128,75,128,55,0,75,0,122,0,120,0,48,0,72,248,125,253,251,248,179,252,131,8,0,0,15,0,10
        .byte 0,0,1,224,3,240,3,240,7,248,7,248,7,248,15,236,15,228,15,228,15,204,7,200,7,248,1,224,0,0,0,0
        .byte 0,0,0,120,0,252,0,252,1,254,1,254,1,254,3,251,3,249,3,249,3,243,1,242,1,254,0,120,0,0,0,0
        .byte 0,0,0,30,0,63,0,63,128,127,128,127,128,127,192,254,64,254,64,254,192,252,128,124,128,127,0,30,0,0,0,0
        .byte 0,0,128,7,192,15,192,15,224,31,224,31,224,31,176,63,144,63,144,63,48,63,32,31,224,31,128,7,0,0,0,0
        .byte 29,224,56,24,12,4,2,2,128,128,128,60,64,14,63,220,0,0,0,0,0,128,1,64,1,64,3,160,3,160,0,0
        .byte 7,120,14,6,3,1,128,128,32,32,32,15,144,3,15,247,0,0,0,0,0,32,0,80,0,80,0,232,0,232,0,0
        .byte 1,222,131,129,64,192,32,32,8,8,200,3,228,0,195,253,0,0,0,0,0,8,0,20,0,20,0,58,0,58,0,0
        .byte 128,119,96,224,16,48,8,8,2,2,242,0,57,0,112,255,0,0,0,0,0,2,0,5,0,5,128,14,128,14,0,0
        .byte 30,0,32,28,64,52,64,64,0,4,93,4,120,8,49,112,1,0,0,0,1,0,2,128,2,128,7,64,7,64,0,0
        .byte 7,128,8,7,16,13,16,16,0,1,23,65,30,2,12,92,0,64,0,0,0,64,0,160,0,160,1,208,1,208,0,0
        .byte 1,224,194,1,68,3,4,4,64,0,69,208,135,128,3,23,0,16,0,0,0,16,0,40,0,40,0,116,0,116,0,0
        .byte 0,120,112,128,209,0,1,1,16,0,17,116,33,224,192,197,0,4,0,0,0,4,0,10,0,10,0,29,0,29,0,0
        .byte 65,176,131,8,3,4,129,2,128,2,225,2,127,0,31,0,0,0,0,0,0,0,2,128,2,128,7,64,7,64,0,0
        .byte 16,108,32,194,0,193,160,64,160,0,184,64,31,192,7,192,0,0,0,0,0,0,0,160,0,160,1,208,1,208,0,0
        .byte 4,27,136,48,64,48,40,16,40,0,46,16,7,240,1,240,0,0,0,0,0,0,0,40,0,40,0,116,0,116,0,0
        .byte 193,6,34,12,16,12,10,4,10,0,11,132,1,252,0,124,0,0,0,0,0,0,0,10,0,10,0,29,0,29,0,0
        .byte 0,0,0,0,0,0,0,0,3,240,4,8,9,228,10,212,10,212,11,244,9,228,4,8,3,240,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,252,1,2,2,121,2,181,2,181,2,253,2,121,1,2,0,252,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,63,128,64,64,158,64,173,64,173,64,191,64,158,128,64,0,63,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,192,15,32,16,144,39,80,43,80,43,208,47,144,39,32,16,192,15,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,224,2,208,2,208,3,240,1,224,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,120,0,180,0,180,0,252,0,120,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,45,0,45,0,63,0,30,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,7,64,11,64,11,192,15,128,7,0,0,0,0,0,0
        .byte 4,8,64,128,0,129,0,0,1,192,130,160,27,236,3,96,1,192,0,1,64,128,0,128,8,8,0,0,0,0,0,0
        .byte 1,2,16,32,64,32,0,0,0,112,32,168,6,251,0,216,0,112,64,0,16,32,0,32,2,2,0,0,0,0,0,0
        .byte 128,64,4,8,16,8,0,0,0,28,8,42,193,190,0,54,0,28,16,0,4,8,0,8,128,128,0,0,0,0,0,0
        .byte 32,16,1,2,4,2,0,0,0,7,130,10,176,111,128,13,0,7,4,0,1,2,0,2,32,32,0,0,0,0,0,0
        .byte 0,0,0,0,8,8,5,208,3,224,6,176,6,176,7,240,3,96,5,208,8,8,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,2,2,1,116,0,248,1,172,1,172,1,252,0,216,1,116,2,2,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,128,128,0,93,0,62,0,107,0,107,0,127,0,54,0,93,128,128,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,32,32,64,23,128,15,192,26,192,26,192,31,128,13,64,23,32,32,0,0,0,0,0,0,0,0,0,0
        .byte 0,0,0,0,115,94,115,90,51,94,51,88,123,216,0,0,44,0,20,96,45,56,81,60,124,60,126,62,2,126,28,126
        .byte 0,0,0,0,156,215,156,214,140,215,12,214,30,246,0,0,11,0,5,24,11,78,20,79,31,15,159,143,128,159,135,31
        .byte 0,0,0,0,231,53,167,53,227,53,131,53,135,189,0,0,2,192,1,70,130,211,197,19,199,195,231,227,224,39,225,199
        .byte 0,0,0,0,121,205,105,205,120,205,96,205,97,239,0,0,0,176,128,81,224,180,241,68,241,240,249,248,248,9,248,113
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,44,0,20,96,45,56,81,60,124,60,126,62,60,126,0,126
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,11,0,5,24,11,78,20,79,31,15,159,143,143,31,128,31
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,192,1,70,130,211,197,19,199,195,231,227,227,199,224,7
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,176,128,81,224,180,241,68,241,240,249,248,248,241,248,1
        .byte 16,0,0,44,16,20,0,45,16,81,0,124,16,126,0,60,204,0,206,252,142,30,80,238,141,252,205,184,141,176,69,0
        .byte 4,0,0,11,4,5,64,11,68,20,0,31,132,31,0,15,51,0,51,191,163,135,148,59,35,127,51,110,35,108,17,64
        .byte 1,0,192,2,65,1,208,2,17,5,192,7,225,7,192,3,12,192,204,239,232,225,229,14,200,223,140,219,8,219,4,80
        .byte 0,64,176,0,80,64,180,0,68,65,240,1,248,65,240,0,3,48,243,59,122,56,185,67,242,55,227,54,194,54,1,20
        .byte 16,0,0,88,16,40,0,90,16,162,0,248,16,252,0,120,28,0,204,248,222,60,146,222,77,238,77,124,221,120,141,176
        .byte 4,0,0,22,4,10,128,22,132,40,0,62,4,63,0,30,7,0,51,62,55,143,164,183,147,123,19,95,55,94,35,108
        .byte 1,0,128,5,129,2,160,5,33,10,128,15,193,15,128,7,1,192,140,207,205,227,233,45,228,222,196,215,141,215,8,219
        .byte 0,64,96,1,160,64,104,1,136,66,224,3,240,67,224,1,0,112,227,51,243,120,122,75,185,55,241,53,227,117,194,54
        .byte 3,32,4,144,11,40,23,180,47,186,47,186,47,186,47,250,46,250,44,26,44,90,46,186,47,58,47,122,16,196,15,120
        .byte 0,200,1,36,2,202,5,237,139,238,139,238,139,238,139,254,139,190,139,6,139,22,139,174,139,206,139,222,4,49,3,222
        .byte 0,50,0,73,128,178,65,123,162,251,162,251,162,251,162,255,162,239,162,193,162,197,162,235,162,243,162,247,65,12,128,247
        .byte 128,12,64,18,160,44,208,94,232,190,232,190,232,190,232,191,232,187,104,176,104,177,232,186,232,188,232,189,16,67,224,61
        .byte 0,0,238,219,170,146,170,219,236,137,218,219,218,219,0,0,0,0,3,64,3,64,3,64,0,0,7,32,31,152,31,152
        .byte 0,0,251,182,170,164,234,182,123,34,246,182,246,182,0,0,0,0,0,208,0,208,0,208,0,0,1,200,7,230,7,230
        .byte 0,0,190,237,42,169,186,173,158,200,189,173,189,173,0,0,0,0,0,52,0,52,0,52,0,0,0,114,129,249,129,249
        .byte 0,0,111,187,74,170,110,171,39,178,111,107,111,107,0,0,0,0,0,13,0,13,0,13,0,0,128,28,96,126,96,126
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,64,3,64,3,64,0,0,7,32,31,152,31,152
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,208,0,208,0,208,0,0,1,200,7,230,7,230
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,52,0,52,0,52,0,0,0,114,129,249,129,249
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,13,0,13,0,0,128,28,96,126,96,126
frmlst:
        .byte 0,2
        .byte 2,2
        .byte 4,2
        .byte 6,2
        .byte 8,2
        .byte 10,2
        .byte 12,2
        .byte 14,2
        .byte 16,2
        .byte 18,2
        .byte 20,2
        .byte 22,1
        .byte 23,1
        .byte 24,2
        .byte 26,4
        .byte 30,3
        .byte 33,3
        .byte 36,3
        .byte 39,1
        .byte 40,2
        .byte 42,2
        .byte 44,2
        .byte 46,1
        .byte 47,3
        .byte 50,1
        .byte 51,1
        .byte 52,2
        .byte 54,1
        .byte 55,1
        .byte 56,1
        .byte 57,2
        .byte 59,1
        .byte 60,2
        .byte 62,2
        .byte 64,2
        .byte 66,2
        .byte 68,2
        .byte 70,2
        .byte 72,1
        .byte 73,3
        .byte 76,2
        .byte 78,2
        .byte 80,2
        .byte 82,2
        .byte 84,1
        .byte 85,2,87,0
scdat:
        .word 121,460,522,465,373,110,382,334,300,493
        .byte 255,0,67,55,55,255,0,26,58,0,255,55,4,255,0,24,58,0,255,55,4,0,0,0,55,55,55,255,0,20,55,0,55,55,55
        .byte 0,0,55,55,55,255,0,82,52,28,27,0,0,30,29,51,255,0,22,53,31,32,0,0,36,23,50,255,0,21,22,56,71
        .byte 72,72,72,58,72,41,255,0,4,56,55,55,255,0,14,72,72,72,71,255,72,4,55,255,0,4,55,55,55,255,0,6,255,79,30
        .byte 255,49,30,76,76,76,255,61,24,255,76,63,255,34,60,255,0,90
        .byte 12,6,13,12,6,12,12,13,6,12,6,12,13,12,6,12,13,6,13,12,13,6,12,13,12,6,13,12,13,12,0,14,4
        .byte 0,4,0,4,0,73,0,0,69,255,0,4,70,0,14,4,0,14,0,14,0,4,0,0,15,0,0,4,15,0,14,0,14
        .byte 0,73,0,0,69,255,0,4,70,0,0,4,255,0,5,4,0,0,4,0,0,4,15,255,0,5,73,0,0,69,255,0,4
        .byte 70,0,0,15,255,0,5,15,0,0,4,0,0,14,255,0,4,22,22,73,0,0,69,255,0,4,70,255,0,8,4,0,0
        .byte 15,255,0,7,55,55,73,0,0,69,255,0,4,70,255,0,8,4,255,0,4,255,5,7,0,75,0,0,69,255,0,4,70
        .byte 255,0,6,255,5,7,13,13,6,12,6,12,6,0,75,0,0,69,255,0,4,70,255,0,6,12,13,6,13,6,12,13,20
        .byte 4,0,4,0,0,4,0,75,0,0,69,255,0,4,70,255,0,6,4,0,15,4,0,0,14,20,15,0,15,0,0,4,0
        .byte 75,0,0,69,255,0,4,70,255,0,6,4,0,0,4,0,0,14,20,14,0,4,0,0,14,0,75,0,0,69,255,0,4
        .byte 70,255,0,6,14,0,0,14,0,0,0,20,0,0,15,0,0,14,0,75,0,0,69,255,0,4,70,255,0,6,4,0,0
        .byte 14,0,0,0,20,255,0,7,75,0,0,69,255,0,4,70,255,0,6,4,255,0,6,20,255,0,7,75,0,0,69,255,0,4
        .byte 70,255,0,6,14,0,0,0,12,13,6,20,255,0,7,75,0,0,69,255,0,4,70,255,0,10,4,2,3,12,12,13,6
        .byte 255,0,4,75,0,0,69,255,0,4,70,255,0,10,14,2,3,1,3,2,4,255,0,7,69,255,0,4,70,255,0,10,4
        .byte 2,3,1,3,2,14,255,0,4,75,255,0,10,22,55,255,0,6,14,2,3,1,3,2,255,0,5,75,255,0,10,55,55
        .byte 255,0,7,2,3,1,3,2,255,5,7,255,33,8,255,5,10,2,3,1,3,2,255,74,25,2,3,255,74,30
        .byte 52,28,27,0,0,30,29,51,0,0,65,66,0,0,0,62,63,64,255,0,7,255,21,4,0,53,31,32,0,0,36,23,50
        .byte 0,0,67,68,255,0,13,4,8,8,4,8,53,25,26,37,40,45,47,50,0,22,22,48,48,48,255,0,11,4,8,9,4
        .byte 8,53,90,42,38,43,44,101,50,78,78,255,0,15,4,7,9,4,7,0,39,255,49,4,54,0,78,78,255,0,15,17,7
        .byte 9,15,7,255,5,18,255,0,7,15,7,9,18,7,0,17,8,9,0,0,82,255,0,4,9,16,8,9,9,17,8,255,5,6
        .byte 9,18,7,9,4,7,20,15,7,9,0,0,82,255,0,4,9,18,7,9,9,14,7,9,4,8,9,4,8,9,4,7,9
        .byte 4,7,20,18,7,9,0,0,17,255,0,4,9,4,7,9,9,18,7,9,4,7,9,4,7,0,4,7,0,4,7,20,18
        .byte 7,9,0,11,11,11,0,0,0,9,4,7,9,9,18,7,9,4,7,9,4,7,0,4,0,0,17,0,20,4,7,9,0
        .byte 9,17,7,9,0,0,9,4,7,9,9,4,7,9,15,7,9,4,7,255,5,6,20,4,7,9,0,9,15,7,9,0,0
        .byte 9,14,7,9,9,4,7,9,4,7,9,4,7,0,16,8,9,4,8,20,15,7,9,0,0,18,7,0,0,0,9,18,7
        .byte 9,9,15,7,9,4,7,9,14,7,0,4,7,9,4,7,20,4,7,0,0,0,4,7,0,0,0,9,4,7,9,0,0,0
        .byte 9,4,7,9,4,7,9,4,7,9,4,7,20,4,7,9,0,11,11,11,0,0,0,9,4,7,9,0,0,0,9,4,7
        .byte 9,17,7,9,17,7,9,17,7,20,4,7,9,0,0,14,255,0,4,9,17,7,9,0,0,0,9,4,7,0,17,7,9
        .byte 15,7,9,15,7,20,17,7,9,0,9,4,7,9,0,0,0,15,7,255,0,4,9,15,7,0,15,7,255,0,6,20,15
        .byte 7,0,0,0,16,7,9,255,0,21,20,255,0,25,12,6,6,13,20,255,0,9,12,6,6,13,255,0,6,12,6,6,13
        .byte 0,0,0,1,3,0,12,255,6,4,13,255,0,5,1,3,9,9,12,6,13,0,0,0,1,3,0,0,9,9,1,3,0
        .byte 33,1,3,2,3,255,33,6,1,3,33,33,33,1,255,33,4,1,3,255,33,4,1,3,33
        .byte 255,0,9,77,22,55,255,0,13,4,75,75,4,255,0,10,55,55,55,255,0,13,4,8,7,4,8,255,0,9,22,22,48,48,48
        .byte 255,0,11,4,8,9,4,8,255,0,25,4,7,9,4,7,255,0,25,17,7,9,15,7,255,5,18,255,0,7,15,7,9
        .byte 18,7,0,17,8,9,255,0,7,9,16,8,9,9,17,8,255,5,6,9,18,7,9,4,7,20,15,7,9,255,0,7,9
        .byte 18,7,9,9,14,7,9,4,8,9,4,8,9,4,7,9,4,7,20,18,7,9,255,0,7,9,4,7,9,9,18,7,9
        .byte 4,7,9,4,7,0,4,7,0,4,7,20,18,7,9,0,11,11,11,0,0,0,9,4,7,9,9,18,7,9,4,7,9
        .byte 4,7,0,4,0,0,17,0,20,4,7,9,0,9,17,7,9,0,0,9,4,7,9,9,4,7,9,15,7,9,4,7,255,5,6
        .byte 20,4,7,9,0,9,15,7,9,0,0,9,14,7,9,9,4,7,9,4,7,9,4,7,0,16,8,9,4,8,20,15,7
        .byte 9,0,0,18,7,0,0,0,9,18,7,9,9,15,7,9,4,7,9,14,7,0,4,7,9,4,7,20,4,7,0,0,0
        .byte 4,7,0,0,0,9,4,7,9,0,0,0,9,4,7,9,4,7,9,4,7,9,4,7,20,4,7,9,0,11,11,11,0,0,0
        .byte 9,4,7,9,0,0,0,9,4,7,9,14,7,9,14,7,9,15,7,20,4,7,9,0,0,14,255,0,4,9,17,7,9
        .byte 0,0,0,9,4,7,0,0,0,9,0,0,9,0,0,20,16,7,0,0,9,4,7,9,0,0,0,15,7,255,0,4,9
        .byte 15,7,255,0,9,20,255,0,5,16,0,9,255,0,21,20,255,0,25,12,6,6,13,20,255,0,9,12,6,6,13,255,0,6
        .byte 12,6,6,13,0,0,0,1,3,0,12,255,6,4,13,255,0,5,1,3,9,9,12,6,13,0,0,0,1,3,0,0,9,9
        .byte 1,3,0,33,1,3,2,3,255,33,6,1,3,33,33,33,1,255,33,4,1,3,255,33,4,1,3,33
        .byte 52,28,27,55,0,30,29,51,0,0,65,66,0,0,0,62,63,64,255,0,7,21,21,75,4,0,53,31,32,0,0,36,23
        .byte 50,0,55,67,68,255,0,13,4,8,7,4,8,53,25,26,37,40,45,47,50,0,22,22,48,48,48,255,0,11,4,8,9
        .byte 4,8,53,90,42,38,43,44,101,50,255,0,17,4,7,9,4,7,0,39,255,49,4,54,255,0,18,17,7,9,4,7,255,5,18
        .byte 83,83,83,5,5,11,0,15,7,9,15,7,0,17,8,9,8,4,8,8,4,8,8,4,8,8,16,8,4,8,8,4,8,8
        .byte 4,8,9,16,7,9,4,7,20,15,0,9,9,4,7,9,4,7,9,4,7,9,4,7,16,7,9,16,7,9,16,7,9
        .byte 4,7,9,4,7,20,17,0,0,0,4,7,9,4,7,9,16,7,9,4,7,4,7,9,16,7,9,4,7,9,4,7,9
        .byte 4,7,20,17,0,0,0,4,7,9,4,7,9,4,7,9,4,7,4,7,9,4,7,9,16,7,9,4,7,9,4,7,20
        .byte 11,11,11,0,4,7,9,16,7,9,16,7,9,16,7,16,7,9,15,7,9,15,7,9,16,7,9,4,7,20,4,7,7
        .byte 0,4,7,9,255,0,12,7,9,255,0,6,16,0,20,4,7,9,0,15,7,9,0,255,59,7,255,0,14,20,15,255,0,28
        .byte 20,255,0,29,11,11,11,255,0,20,11,11,11,255,0,8,11,11,11,255,0,11,11,11,11,0,0,9,4,7,9,255,0,27
        .byte 14,7,255,0,28,4,7,255,0,28,15,7,255,0,29,12,12,6,6,6,1,3,2,255,33,8,12,6,12,6,6,255,33,10
        .byte 2,2,3,3
        .byte 255,0,71,87,87,255,0,28,58,58,255,0,27,22,22,85,255,0,27,72,72,84,255,0,28,73,255,0,29,73,255,0,29
        .byte 73,255,0,26,52,28,27,0,0,30,29,51,255,0,22,53,31,32,0,0,36,23,50,255,0,22,53,25,26,37,40,45,47
        .byte 50,255,0,22,53,90,42,38,43,44,101,50,255,0,23,39,255,49,4,54,255,0,7,86,86,255,0,27,58,86,86,255,0,28
        .byte 4,22,255,0,28,55,55,255,0,156
        .byte 52,28,27,0,0,30,29,51,0,0,65,66,0,0,0,62,63,64,255,0,7,21,21,0,0,0,53,31,32,0,0,36,23
        .byte 50,0,0,67,68,255,0,14,102,0,0,102,53,25,26,37,40,45,47,50,255,0,18,102,8,8,102,88,35,42,38,43,44
        .byte 46,89,255,0,14,8,0,0,0,102,7,9,0,255,95,4,8,8,78,8,255,95,14,8,78,0,0,102,7,9,0,102,8,8
        .byte 102,9,9,0,99,255,0,14,99,0,0,0,102,7,9,0,102,7,8,102,7,0,0,99,255,0,14,99,0,0,0,102,0,0,0
        .byte 102,7,9,102,7,0,0,99,255,0,14,99,0,0,0,102,0,0,0,102,7,9,0,0,0,78,99,255,95,14,99,78,8,8
        .byte 255,95,4,102,7,9,255,0,4,99,255,0,14,99,0,9,9,102,8,8,102,102,255,0,6,99,255,0,14,99,0,0,0
        .byte 102,8,9,102,102,255,0,6,99,255,0,14,99,0,0,0,102,7,9,0,255,95,4,8,8,78,99,255,95,14,99,78,0,0
        .byte 102,7,9,0,102,8,8,102,9,9,0,99,255,0,14,99,0,0,0,102,7,0,0,102,7,8,102,7,0,0,99,255,0,14
        .byte 99,255,0,7,102,7,9,102,7,0,0,99,255,0,14,99,255,0,7,102,7,9,0,0,0,78,99,255,95,15,78,8,8
        .byte 255,95,4,102,7,9,255,0,4,99,255,0,14,99,0,9,9,0,0,0,102,102,7,9,255,0,4,99,255,0,14,99,255,0,7
        .byte 102,7,255,0,5,99,255,0,14,99,255,0,7,102,7,255,0,4,78,255,95,16,78,255,0,6,92,100,100,100,91,78,78
        .byte 255,0,15,8,78,78,92,93,93,93,91
        .byte 255,0,12,22,22,22,62,63,64,22,22,255,0,5,21,21,255,0,15,52,28,27,0,0,30,29,51,255,0,22,53,31,32
        .byte 22,22,36,23,50,255,0,22,53,25,26,37,40,45,47,50,255,0,10,22,22,22,255,0,9,88,35,42,38,43,44,46,89
        .byte 255,0,10,22,22,22,0,255,103,21,255,0,5,22,22,22,0,255,104,21,255,0,12,105,8,9,0,105,8,9,0,105,8
        .byte 9,0,105,8,9,0,105,8,9,255,0,11,105,8,9,0,105,8,9,0,105,8,9,0,105,8,9,0,105,8,9,255,0,11
        .byte 105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,105,7,9
        .byte 0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,255,0,11
        .byte 105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,105,7,9
        .byte 0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,255,0,11
        .byte 105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,105,7,9
        .byte 0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,0,105,7,9,255,0,94
        .byte 255,103,30
        .byte 255,0,12,22,22,22,255,0,105,22,22,22,255,0,27,22,22,22,0,255,103,8,255,0,7,255,103,6,255,0,5,22,22,22
        .byte 0,255,104,8,255,0,7,255,104,6,255,0,12,105,8,9,0,105,8,9,255,0,5,105,8,9,0,105,8,9,255,0,11
        .byte 105,8,9,0,105,8,9,0,0,8,9,0,105,8,9,0,105,8,9,255,0,11,105,7,9,0,105,7,9,0,0,7,9
        .byte 0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,0,7,9,0,105,7,9,0,105,7,9,255,0,11
        .byte 105,7,9,0,105,7,9,0,0,7,9,0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,0,7,9
        .byte 0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,0,7,9,0,105,7,9,0,105,7,9,255,0,11
        .byte 105,7,9,0,105,7,9,0,0,7,9,0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,0,7,9
        .byte 0,105,7,9,0,105,7,9,255,0,11,105,7,9,0,105,7,9,0,0,7,9,0,105,7,9,0,105,7,9,255,0,11
        .byte 105,7,9,0,105,7,9,0,0,7,9,0,105,7,9,0,105,7,9,255,0,48,55,22,255,0,28,55,55,255,0,14,255,103,30
        .byte 255,0,31,12,6,7,0,0,6,7,0,0,12,7,0,12,13,7,0,0,0,12,13,6,0,0,12,0,0,12,6,0,12
        .byte 7,0,0,0,12,7,6,7,12,7,6,7,13,7,13,7,0,0,0,13,0,0,12,0,13,0,6,0,13,6,7,0,6
        .byte 7,12,7,13,7,12,7,13,7,12,7,6,7,0,0,0,6,0,0,13,0,6,0,6,6,0,13,7,0,13,7,6,7
        .byte 6,7,13,7,6,7,13,7,6,7,0,0,0,13,0,0,6,0,6,0,13,0,6,0,12,6,7,0,0,6,7,0,0
        .byte 13,7,0,12,13,7,99,0,99,6,255,0,4,12,0,0,6,6,255,0,16,255,99,5,255,0,12,100,0,100,255,0,11
        .byte 99,99,99,255,0,6,79,79,255,0,4,255,100,5,255,0,11,99,255,0,6,80,80,79,79,79,0,0,0,100,100,100,255,0,11
        .byte 106,0,106,106,255,0,6,80,80,79,79,0,0,0,100,255,0,8,106,0,106,0,0,106,0,106,106,255,0,6,80,80,79
        .byte 255,0,11,106,76,106,76,106,0,255,106,4,0,0,95,95,0,0,80,80,79,255,0,6,95,95,0,0,0,255,106,5,0,0
        .byte 106,106,0,0,97,95,95,0,0,80,80,80,0,0,0,79,79,0,95,95,0,0,0,75,106,0,106,0,0,95,0,0,0
        .byte 97,0,95,0,0,0,80,80,80,0,0,0,79,80,0,0,95,255,0,5,95,0,95,0,255,95,5,255,0,4,80,80,0
        .byte 79,0,0,79,80,255,0,5,255,95,7,0,255,95,4,106,95,0,0,80,80,80,0,79,0,0,79,80,80,0,0,0,95
        .byte 106,255,95,6,0,0,106,106,106,95,0,0,255,80,4,0,79,0,79,255,80,4,0,0,0,95,255,106,4,255,0,4,255,95,4
        .byte 0,0,255,80,4,0,79,0,255,80,5,255,0,4,95,95,95,255,0,10,255,80,5,0,79,0,0,255,80,6,255,0,5
        .byte 95,95,0,95,95,95,0,255,80,6,0,0,79,79,0,79,0,255,80,7,0,95,95,255,0,4,95,95,95,255,80,6,0
        .byte 79,255,80,4,79,0,255,80,6,79,255,95,5,0,95,95,95,79,255,80,4,0,79,80,80,80
numsc:
        .byte 10
nmedat:
        .byte 8,2,88,176,8,15,88,120,8,16,88,72,8,17,88,104,8,26,72,104,8,17,88,88,8,19,24,64,8,20,40,80,8,21,40,128,8,18,8,152,8,18,56,16,5,33,40,32,255
        .byte 5,3,144,160,5,30,40,56,5,29,128,128,5,27,128,96,5,26,112,112,5,24,112,96,5,25,112,128,5,28,128,112,5,31,40,72,255
        .byte 0,0,136,32,2,5,32,128,1,8,80,16,3,26,8,32,3,22,80,88,3,13,32,80,4,14,64,56,4,14,104,216,4,14,24,216,1,9,120,176,255
        .byte 5,3,8,88,5,15,32,64,5,16,32,16,5,17,32,32,5,24,16,32,5,25,16,64,5,26,16,48,5,17,32,48,255
        .byte 0,0,144,8,6,34,168,184,6,12,112,128,3,26,8,32,6,34,168,72,4,14,80,104,3,13,32,80,3,42,104,216,7,33,96,192,7,10,32,120,255
        .byte 3,4,128,184,3,16,40,88,3,26,80,96,3,33,112,176,3,39,24,96,8,18,8,24,8,18,144,112,8,18,64,152,255
        .byte 0,2,144,224,3,26,8,32,3,13,24,72,4,14,40,224,2,40,152,88,2,40,24,144,2,41,56,112,2,41,112,72,2,40,88,96,255
        .byte 4,44,136,56,0,0,136,16,7,33,96,192,3,13,32,80,4,44,136,88,4,44,136,120,4,44,136,152,4,44,136,184,1,8,64,16,7,33,96,64,3,26,16,128,255
        .byte 4,0,160,120,4,24,16,112,4,25,16,144,4,26,16,128,4,27,32,112,4,28,32,128,4,29,32,144,4,13,32,80,255
        .byte 7,31,8,32,255
NUMOBJ = 2
objdta:
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,102,102,102,102,102,102
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,8,141,8,8,141
font:
        .byte 0,0,0,0,0,0,0,0
        .byte 12,30,30,60,24,96,96,0
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
        .byte 124,238,238,238,254,254,124,0
        .byte 56,120,120,56,56,56,124,0
        .byte 124,198,62,124,240,254,254,0
        .byte 124,206,30,236,254,254,124,0
        .byte 56,88,216,252,252,252,24,0
        .byte 254,224,28,238,254,254,124,0
        .byte 124,224,252,238,254,254,124,0
        .byte 126,6,28,60,60,60,60,0
        .byte 124,238,124,238,254,254,124,0
        .byte 124,198,126,6,126,126,60,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,48,0,0,48,48,96
        .byte 0,0,12,24,48,24,12,0
        .byte 0,0,0,126,0,126,0,0
        .byte 0,0,48,24,12,24,48,0
        .byte 0,124,198,12,24,0,24,0
        .byte 0,124,222,254,254,192,124,0
        .byte 0,124,246,246,254,246,246,0
        .byte 0,252,246,252,246,246,252,0
        .byte 0,124,246,240,246,246,124,0
        .byte 0,248,236,230,230,236,248,0
        .byte 0,254,240,252,240,240,254,0
        .byte 0,254,240,252,240,240,240,0
        .byte 0,124,246,224,222,230,124,0
        .byte 0,246,246,254,246,246,246,0
        .byte 0,126,28,28,28,28,126,0
        .byte 0,6,6,6,246,246,124,0
        .byte 0,236,220,240,216,238,230,0
        .byte 0,120,120,120,120,120,126,0
        .byte 0,198,238,254,214,238,238,0
        .byte 0,206,238,246,222,238,230,0
        .byte 0,124,246,246,254,254,124,0
        .byte 0,252,246,246,252,240,240,0
        .byte 0,124,246,198,246,222,124,0
        .byte 0,252,246,246,252,244,246,0
        .byte 0,124,240,124,14,238,124,0
        .byte 0,254,56,56,56,56,56,0
        .byte 0,238,238,238,238,254,124,0
        .byte 0,238,238,238,238,108,56,0
        .byte 0,238,238,238,214,254,108,0
        .byte 0,238,238,56,56,238,238,0
        .byte 0,222,222,124,60,216,248,112
        .byte 0,254,12,24,48,96,254,0
        .byte 0,30,24,24,24,24,30,0
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 0,48,120,252,48,48,48,0
        .byte 0,0,0,0,0,0,0,255
        .byte 0,60,102,248,96,96,254,0
        .byte 96,241,159,14,96,241,159,14
        .byte 52,75,181,109,238,110,213,81
        .byte 54,218,219,237,109,109,114,38
        .byte 182,91,77,236,237,213,203,82
        .byte 7,12,12,12,12,12,12,12
        .byte 255,68,222,65,88,0,33,0
        .byte 224,48,48,48,48,48,48,48
        .byte 12,12,12,12,12,12,12,7
        .byte 48,48,48,48,48,48,48,224
        .byte 0,0,0,0,0,0,0,255
        .byte 0,0,0,0,0,0,0,0
        .byte 26,122,250,250,26,194,122,26
        .byte 255,0,255,255,0,68,170,17
        .byte 255,0,255,199,108,56,56,56
        .byte 56,56,56,124,255,255,0,255
        .byte 0,56,124,199,187,68,131,0
        .byte 255,0,255,255,102,126,102,0
        .byte 0,120,220,220,192,192,192,0
        .byte 88,94,95,95,92,65,94,88
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
        .byte 248,248,248,0,0,0,0,0,0,0,8,8,9,7,8,8,8,8,8,8,8,99
keys:   .byte 10,8,63,47,33,35,21,31,30,26,24