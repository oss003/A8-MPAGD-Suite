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
WINDOWLFT = 2
WINDOWHGT = 20
WINDOWWID = 28 ;a
MAPWID = 10
        .byte 255,255,255,255,255,255,255,255,255,255
mapdat:
        .byte 255,27,255,20,255,23,255,255,255,255,255,255,255,19,21,22,24,25,26,255,255,255,14,13,12,11,10,18,17,255,255,255,255,15,3,0,4,6,8,255,255,255,255,16,2,1,5,7,9,255
        .byte 255,255,255,255,255,255,255,255,255,255
stmap:  .byte 35

evnt00:
        jsr cangd	; CANGODOWN
        beq :+
        jmp a00026
:
        jsr ifall	; TABLEFALL
a00026: jsr skobj	; DETECTOBJECT
        sta varobj
        lda #255
        cmp varobj
        bne *+5
        jmp a00490
        lda varobj	; GET
        jsr getob
        lda #50		; BEEP
        asl a
        sta sndtyp
        lda #1
        cmp scno
        beq *+5
        jmp a00143
        lda vark
        clc
        adc #1
        sta vark
        lda #20		; AT
        sta chary
        lda #29
        sta charx
        lda vark	; DISPLAY
        jsr disply
a00143: lda #16
        cmp scno
        beq *+5
        jmp a00206
        lda vark
        clc
        adc #1
        sta vark
        lda #20		; AT
        sta chary
        lda #29
        sta charx
        lda vark	; DISPLAY
        jsr disply
a00206: lda #7
        cmp scno
        beq *+5
        jmp a00291
        lda #3		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp a00291
:
        lda vark
        clc
        adc #1
        sta vark
        lda #20		; AT
        sta chary
        lda #29
        sta charx
        lda vark	; DISPLAY
        jsr disply
a00291: lda #13
        cmp scno
        beq *+5
        jmp a00346
        lda #9		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp a00346
:
        lda varc
        clc
        adc #1
        sta varc
a00346: lda #17
        cmp scno
        beq *+5
        jmp a00418
        lda numlif
        clc
        adc #1
        sta numlif
        lda #21		; AT
        sta chary
        lda #29
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        lda #1
        sta varh
a00418: lda #19
        cmp scno
        beq *+5
        jmp a00490
        lda numlif
        clc
        adc #1
        sta numlif
        lda #21		; AT
        sta chary
        lda #29
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        lda #1
        sta vari
a00490: lda #223
        ldy #9
        cmp (z80_ix),y
        bcc *+5
        jmp a00561
        jsr scrr	; SCREENRIGHT
        lda #32
        ldy #9
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sta varo
        ldy #9
        lda (z80_ix),y
        sta varp
a00561: lda #16
        ldy #9
        cmp (z80_ix),y
        bcs *+5
        jmp a00632
        jsr scrl	; SCREENLEFT
        lda #208
        ldy #9
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sta varo
        ldy #9
        lda (z80_ix),y
        sta varp
a00632: lda #2
        ldy #8
        cmp (z80_ix),y
        bcs *+5
        jmp a00702
        jsr scru	; SCREENUP
        lda #140
        ldy #8
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sta varo
        ldy #9
        lda (z80_ix),y
        sta varp
a00702: lda #140
        ldy #8
        cmp (z80_ix),y
        bcc *+5
        jmp a00773
        jsr scrd	; SCREENDOWN
        lda #8
        ldy #8
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        sta varo
        ldy #9
        lda (z80_ix),y
        sta varp
a00773: jsr laddd	; LADDERBELOW
        beq :+
        jmp a00827
:
        lda #2
        sta varl
        jsr laddu	; CANGOUP
        beq :+
        jmp a00822
:
        lda #1
        sta varl
a00822: jmp a00835
a00827: lda #0
        sta varl
a00835: lda joyval	; KEY
        and #1
        beq :+
        jmp a01207
:
        jsr cangr	; CANGORIGHT
        beq :+
        jmp a01207
:
        lda joyval	; KEY
        and #4
        beq :+
        jmp a00983
:
        jsr laddd	; LADDERBELOW
        beq :+
        jmp a00911
:
        jmp a00979
a00911: lda #0
        cmp vara
        beq *+5
        jmp a00939
        lda #0		; ANIMATE
        jsr animsp
a00939: lda #0
        ldy #6
        sta (z80_ix),y
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a00979: jmp a01207
a00983: lda joyval	; KEY
        and #8
        beq :+
        jmp a01095
:
        jsr laddu	; CANGOUP
        beq :+
        jmp a01023
:
        jmp a01090
a01023: lda #0
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01064
        lda #0		; ANIMATE
        jsr animsp
a01064: ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01090: jmp a01207
a01095: lda #1
        sta vard
        lda #0
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01181
        lda #0		; ANIMATE
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01181
        lda #20		; BEEP
        asl a
        sta sndtyp
a01181: ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01207: lda joyval	; KEY
        and #2
        beq :+
        jmp a01577
:
        jsr cangl	; CANGOLEFT
        beq :+
        jmp a01577
:
        lda joyval	; KEY
        and #4
        beq :+
        jmp a01355
:
        jsr laddd	; LADDERBELOW
        beq :+
        jmp a01283
:
        jmp a01350
a01283: lda #0
        cmp vara
        beq *+5
        jmp a01311
        lda #0		; ANIMATE
        jsr animsp
a01311: lda #1
        ldy #6
        sta (z80_ix),y
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01350: jmp a01577
a01355: lda joyval	; KEY
        and #8
        beq :+
        jmp a01466
:
        jsr laddu	; CANGOUP
        beq :+
        jmp a01394
:
        jmp a01461
a01394: lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01436
        lda #0		; ANIMATE
        jsr animsp
a01436: ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01461: jmp a01577
a01466: lda #2
        sta vard
        lda #1
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01552
        lda #0		; ANIMATE
        jsr animsp
        lda #0
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp a01552
        lda #20		; BEEP
        asl a
        sta sndtyp
a01552: ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01577: lda joyval	; KEY
        and #8
        beq :+
        jmp a01687
:
        lda #3
        sta vard
        jsr laddu	; CANGOUP
        beq :+
        jmp a01687
:
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01662
        lda #0		; ANIMATE
        jsr animsp
a01662: ldy #8 		; SPRITEUP
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
a01687: lda joyval	; KEY
        and #4
        beq :+
        jmp a01798
:
        lda #3
        sta vard
        jsr laddd	; LADDERBELOW
        beq :+
        jmp a01798
:
        lda #2
        ldy #6
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp a01772
        lda #0		; ANIMATE
        jsr animsp
a01772: ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
a01798: lda joyval	; KEY
        and #16
        beq :+
        jmp a01881
:
        lda #1
        cmp varl
        beq *+5
        jmp a01839
        jmp a01881
a01839: jsr cangd	; CANGODOWN
        beq :+
        jmp a01860
:
        jmp a01881
a01860: lda #40		; BEEP
        asl a
        sta sndtyp
        jsr jump	; TABLEJUMP
a01881: lda #DEADLY	; DEADLY
        sta z80_b
        jsr tded
        cmp z80_b
        beq :+
        jmp a01949
:
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        ldy #9
        sta (z80_ix),y
        lda #1 		; KILL
        sta deadf
a01949: jmp grav
evnt01:
        lda #0
        cmp vara
        beq *+5
        jmp b00030
        lda #0		; ANIMATE
        jsr animsp
b00030: rts
evnt02:
        lda #0
        cmp vara
        beq *+5
        jmp c00030
        lda #0		; ANIMATE
        jsr animsp
c00030: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp c02457
:
        lda joyval	; KEY
        and #64
        beq :+
        jmp c02457
:
        lda #9		; INVENTORY
        tax
        jsr minve
        lda #0
        cmp scno
        beq *+5
        jmp c00237
        lda #0
        cmp varopt
        beq *+5
        jmp c00237
        lda #8		; AT
        sta chary
        lda #28
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #9		; AT
        sta chary
        lda #28
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #80		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
c00237: lda #11
        cmp scno
        beq *+5
        jmp c00386
        lda #5
        cmp varopt
        beq *+5
        jmp c00386
        lda #2		; AT
        sta chary
        lda #20
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #3		; AT
        sta chary
        lda #20
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
c00386: lda #4
        cmp scno
        beq *+5
        jmp c00601
        lda #6
        cmp varopt
        beq *+5
        jmp c00601
        lda #2		; AT
        sta chary
        lda #11
        sta charx
        lda #21 		; PUTBLOCK
        jsr pbpattr
        lda #22 		; PUTBLOCK
        jsr pbpattr
        lda #3		; AT
        sta chary
        lda #11
        sta charx
        lda #21 		; PUTBLOCK
        jsr pbpattr
        lda #22 		; PUTBLOCK
        jsr pbpattr
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #24
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #6
        jsr drpob
c00601: lda #9
        cmp scno
        beq *+5
        jmp c00814
        lda #8
        cmp varopt
        beq *+5
        jmp c00814
        lda #4		; AT
        sta chary
        lda #2
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #5		; AT
        sta chary
        lda #2
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #45		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #24
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #8
        jsr drpob
c00814: lda #20
        cmp scno
        beq *+5
        jmp c01005
        lda #15
        cmp varopt
        beq *+5
        jmp c01005
        lda #4		; AT
        sta chary
        lda #8
        sta charx
        lda #60 		; PUTBLOCK
        jsr pbpattr
        lda #5		; AT
        sta chary
        lda #8
        sta charx
        lda #61 		; PUTBLOCK
        jsr pbpattr
        lda #80		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #216
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #15
        jsr drpob
c01005: lda #13
        cmp scno
        beq *+5
        jmp c01177
        lda #7
        cmp varopt
        beq *+5
        jmp c01177
        lda #17		; AT
        sta chary
        lda #14
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #45		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #216
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #7
        jsr drpob
c01177: lda #14
        cmp scno
        beq *+5
        jmp c01338
        lda #11
        cmp varopt
        beq *+5
        jmp c01338
        lda #15		; AT
        sta chary
        lda #7
        sta charx
        lda #56 		; PUTBLOCK
        jsr pbpattr
        lda #80		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #24
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #11
        jsr drpob
c01338: lda #5
        cmp scno
        beq *+5
        jmp c01634
        lda #10
        cmp varopt
        beq *+5
        jmp c01634
        ldy #8
        lda (z80_ix),y
        sta vare
        ldy #9
        lda (z80_ix),y
        sta varf
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #176
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #10
        jsr drpob
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda varf
        ldy #9
        sta (z80_ix),y
        lda vare
        ldy #8
        sta (z80_ix),y
        lda #75 	; DELAY
        jsr delay
        lda #24
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #14
        jsr drpob
        lda varf
        ldy #9
        sta (z80_ix),y
        lda #70		; BEEP
        asl a
        sta sndtyp
c01634: lda #14
        cmp scno
        beq *+5
        jmp c01819
        lda #14
        cmp varopt
        beq *+5
        jmp c01819
        lda #9		; AT
        sta chary
        lda #11
        sta charx
        lda #55 		; PUTBLOCK
        jsr pbpattr
        lda #72 		; PUTBLOCK
        jsr pbpattr
        lda #73 		; PUTBLOCK
        jsr pbpattr
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #24
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #14
        jsr drpob
c01819: lda #16
        cmp scno
        beq *+5
        jmp c02039
        lda #9
        cmp varopt
        beq *+5
        jmp c02039
        lda #72
        ldy #8
        sta (z80_ix),y
        lda #120
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #9
        jsr drpob
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #50 	; DELAY
        jsr delay
        lda #80		; BEEP
        asl a
        sta sndtyp
        lda #128
        ldy #8
        sta (z80_ix),y
        lda #168
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #2
        jsr drpob
c02039: lda #15
        cmp scno
        beq *+5
        jmp c02212
        lda #17
        cmp varopt
        beq *+5
        jmp c02212
        lda #15		; AT
        sta chary
        lda #20
        sta charx
        lda #52 		; PUTBLOCK
        jsr pbpattr
        lda #52 		; PUTBLOCK
        jsr pbpattr
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #24
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #17
        jsr drpob
c02212: lda #23
        cmp scno
        beq *+5
        jmp c02362
        lda #18
        cmp varopt
        beq *+5
        jmp c02362
        lda #5		; AT
        sta chary
        lda #24
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #45		; BEEP
        asl a
        sta sndtyp
        lda #50 	; DELAY
        jsr delay
        lda #6		; AT
        sta chary
        lda #24
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #45		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
c02362: lda #10
        cmp scno
        beq *+5
        jmp c02457
        lda #16
        cmp varopt
        beq *+5
        jmp c02457
        lda #4		; AT
        sta chary
        lda #17
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #45		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
c02457: rts
evnt03:
        lda #0
        cmp varc
        beq *+5
        jmp d00367
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp d00206
        jsr cangl	; CANGOLEFT
        beq :+
        jmp d00188
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
        jsr cangd	; CANGODOWN
        beq :+
        jmp d00148
:
        lda #1
        ldy #11
        sta (z80_ix),y
d00148: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        clc
        adc z80_c
        ldy #9
        sta (z80_ix),y
        jmp d00201
d00188: lda #1
        ldy #11
        sta (z80_ix),y
d00201: jmp d00367
d00206: jsr cangr	; CANGORIGHT
        beq :+
        jmp d00353
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
        jsr cangd	; CANGODOWN
        beq :+
        jmp d00314
:
        lda #0
        ldy #11
        sta (z80_ix),y
d00314: lda #16
        sta z80_c
        ldy #9
        lda (z80_ix),y
        sec
        sbc z80_c
        ldy #9
        sta (z80_ix),y
        jmp d00367
d00353: lda #0
        ldy #11
        sta (z80_ix),y
d00367: lda #0		; ANIMATE
        jsr animsp
        lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp d00414
:
        lda #1 		; KILL
        sta deadf
d00414: rts
evnt04:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp e00131
        jsr cangd	; CANGODOWN
        beq :+
        jmp e00099
:
        ldy #8 		; SPRITEDOWN
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp e00094
        lda #0		; ANIMATE
        jsr animsp
e00094: jmp e00127
e00099: lda #1
        ldy #11
        sta (z80_ix),y
        lda #35		; BEEP
        asl a
        sta sndtyp
e00127: jmp e00233
e00131: jsr cangu	; CANGOUP
        beq :+
        jmp e00205
:
        ldy #8 		; SPRITEUP
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        lda #0
        cmp vara
        beq *+5
        jmp e00201
        lda #0		; ANIMATE
        jsr animsp
e00201: jmp e00233
e00205: lda #0
        ldy #11
        sta (z80_ix),y
        lda #35		; BEEP
        asl a
        sta sndtyp
e00233: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp e00269
:
        lda #1 		; KILL
        sta deadf
e00269: rts
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
        jmp f00451
:
        lda joyval	; KEY
        and #64
        beq :+
        jmp f00451
:
        lda #9		; INVENTORY
        tax
        jsr minve
        lda #25
        cmp scno
        beq *+5
        jmp f00238
        lda #4
        cmp varopt
        beq *+5
        jmp f00238
        lda #9		; AT
        sta chary
        lda #14
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #10		; AT
        sta chary
        lda #14
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #50		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
f00238: lda #1
        cmp scno
        beq *+5
        jmp f00387
        lda #4
        cmp varopt
        beq *+5
        jmp f00387
        lda #15		; AT
        sta chary
        lda #22
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #16		; AT
        sta chary
        lda #22
        sta charx
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #0 		; PUTBLOCK
        jsr pbpattr
        lda #50		; BEEP
        asl a
        sta sndtyp
        lda #255	; REMOVE
        ldy #5
        sta (z80_ix),y
f00387: lda #5
        cmp scno
        beq *+5
        jmp f00451
        lda #3
        cmp vark
        beq *+5
        jmp f00451
        lda #19
        cmp varopt
        beq *+5
        jmp f00451
        lda #27
        sta scno
        jsr nwscr
f00451: rts
evnt06:
        lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00089
        jsr cangl	; CANGOLEFT
        beq :+
        jmp g00071
:
        ldy #9 		; SPRITELEFT
        lda (z80_ix),y
        sec
        sbc #2
        sta (z80_ix),y
        jmp g00084
g00071: lda #1
        ldy #11
        sta (z80_ix),y
g00084: jmp g00150
g00089: jsr cangr	; CANGORIGHT
        beq :+
        jmp g00137
:
        ldy #9 		; SPRITERIGHT
        lda (z80_ix),y
        clc
        adc #2
        sta (z80_ix),y
        jmp g00150
g00137: lda #0
        ldy #11
        sta (z80_ix),y
g00150: lda #1
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00237
        lda #0
        cmp vara
        beq *+5
        jmp g00237
        lda #0		; ANIMATE
        jsr animsp
        lda #3
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00237
        lda #35		; BEEP
        asl a
        sta sndtyp
g00237: lda #0
        ldy #11
        cmp (z80_ix),y
        beq *+5
        jmp g00324
        lda #0
        cmp vara
        beq *+5
        jmp g00324
        lda #0		; ANIMATEBACK
        jsr animbk
        lda #3
        ldy #7
        cmp (z80_ix),y
        beq *+5
        jmp g00324
        lda #35		; BEEP
        asl a
        sta sndtyp
g00324: lda #0 	; COLLISION
        sta z80_b
        jsr sktyp
        bcs :+
        jmp g00360
:
        lda #1 		; KILL
        sta deadf
g00360: rts
evnt07:
        rts
evnt08:
        rts
evnt09:
        lda #4
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00037
        lda #1
        ldy #11
        sta (z80_ix),y
j00037: lda #0
        ldy #5
        cmp (z80_ix),y
        beq *+5
        jmp j00112
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        ldy #9
        sta (z80_ix),y
        ldy #8
        lda (z80_ix),y
        clc
        adc #1
        ldy #8
        sta (z80_ix),y
j00112: rts
evnt10:
        lda vara
        clc
        adc #1
        sta vara
        lda #2
        cmp vara
        bcc *+5
        jmp k00042
        lda #0
        sta vara
k00042: lda #1
        cmp varh
        beq *+5
        jmp k00162
        lda #12		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp k00162
:
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #88
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #12
        jsr drpob
        lda varh
        clc
        adc #1
        sta varh
k00162: lda #1
        cmp vari
        beq *+5
        jmp k00283
        lda #13		; GOT
        jsr gotob
        cmp #255
        beq :+
        jmp k00283
:
        lda #200
        ldy #8
        sta (z80_ix),y
        lda #24
        ldy #9
        sta (z80_ix),y
        ldy #9
        lda (z80_ix),y	; PUT
        sta dispx
        ldy #8
        lda (z80_ix),y
        sta dispy
        lda #13
        jsr drpob
        lda vari
        clc
        adc #1
        sta vari
k00283: lda #27
        cmp scno
        beq *+5
        jmp k00402
        lda #0
        cmp varg
        beq *+5
        jmp k00402
        lda #2		; AT
        sta chary
        lda #3
        sta charx
        lda #11  	; MESSAGE
        jsr dmsg
        lda #18		; AT
        sta chary
        lda #12
        sta charx
        lda #20  	; MESSAGE
        jsr dmsg
        lda varg
        clc
        adc #1
        sta varg
        lda #25 	; DELAY
        jsr delay
k00402: lda #27
        cmp scno
        beq *+5
        jmp k00437
        jsr prskey	; WAITKEY
        lda #1		; ENDGAME
        sta gamwon
k00437: rts
evnt11:
        rts
evnt12:
        jsr cls		; CLS
        lda #2		; AT
        sta chary
        lda #10
        sta charx
        lda #0  	; MESSAGE
        jsr dmsg
        lda #1
        sta chary
        lda #18  	; MESSAGE
        jsr dmsg
        lda #3		; AT
        sta chary
        lda #0
        sta charx
        lda #12  	; MESSAGE
        jsr dmsg
        lda #17		; AT
        sta chary
        lda #0
        sta charx
        lda #13  	; MESSAGE
        jsr dmsg
        lda #1		; AT
        sta chary
        lda #15
        sta charx
        lda #14  	; MESSAGE
        jsr dmsg
        lda #7		; AT
        sta chary
        lda #2
        sta charx
        lda #15  	; MESSAGE
        jsr dmsg
        lda #3		; AT
        sta chary
        lda #29
        sta charx
        lda #16  	; MESSAGE
        jsr dmsg
        lda #5		; AT
        sta chary
        lda #8
        sta charx
        lda #10  	; MESSAGE
        jsr dmsg
        lda #9		; AT
        sta chary
        lda #8
        sta charx
        lda #10  	; MESSAGE
        jsr dmsg
        lda #7		; AT
        sta chary
        lda #23
        sta charx
        lda #10  	; MESSAGE
        jsr dmsg
        lda #8		; AT
        sta chary
        lda #23
        sta charx
        lda #10  	; MESSAGE
        jsr dmsg
        lda #3		; AT
        sta chary
        lda #20
        sta charx
        lda #8  	; MESSAGE
        jsr dmsg
        lda #11		; AT
        sta chary
        lda #10
        sta charx
        lda #8  	; MESSAGE
        jsr dmsg
        lda #11		; AT
        sta chary
        lda #12
        sta charx
        lda #8  	; MESSAGE
        jsr dmsg
        lda #11		; AT
        sta chary
        lda #11
        sta charx
        lda #8  	; MESSAGE
        jsr dmsg
        lda #19		; AT
        sta chary
        lda #0
        sta charx
        lda #1		; PRINTMODE
        sta prtmod
        lda #17  	; MESSAGE
        jsr dmsg
        lda #0		; PRINTMODE
        sta prtmod
        lda #21		; AT
        sta chary
        lda #0
        sta charx
        lda #22  	; MESSAGE
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
        jsr cls		; CLS
        lda #20		; AT
        sta chary
        lda #2
        sta charx
        lda #0  	; MESSAGE
        jsr dmsg
        lda #0
        sta varc
        lda #0
        sta varg
        lda #128
        sta varo
        lda #32
        sta varp
        lda #3
        sta numlif
        lda #0
        sta vark
        lda #0
        sta vare
        lda #0
        sta varf
        lda #0		; AT
        sta chary
        lda #1
        sta charx
        lda #3  	; MESSAGE
        jsr dmsg
        lda #21 	; REPEAT
        sta loopa
n00142: lda #1
        sta charx
        lda #3  	; MESSAGE
        jsr dmsg
        dec loopa	; ENDREPEAT
        beq :+
        jmp n00142
:
        lda #1
        sta charx
        lda #4  	; MESSAGE
        jsr dmsg
        lda #0		; AT
        sta chary
        lda #30
        sta charx
        lda #3  	; MESSAGE
        jsr dmsg
        lda #21 	; REPEAT
        sta loopa
n00237: lda #30
        sta charx
        lda #3  	; MESSAGE
        jsr dmsg
        dec loopa	; ENDREPEAT
        beq :+
        jmp n00237
:
        lda #22		; AT
        sta chary
        lda #2
        sta charx
        lda #5  	; MESSAGE
        jsr dmsg
        lda #22		; AT
        sta chary
        lda #30
        sta charx
        lda #6  	; MESSAGE
        jsr dmsg
        lda #21		; AT
        sta chary
        lda #14
        sta charx
        lda #1  	; MESSAGE
        jsr dmsg
        lda #21		; AT
        sta chary
        lda #29
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        lda #21		; AT
        sta chary
        lda #2
        sta charx
        lda #7  	; MESSAGE
        jsr dmsg
        lda #20		; AT
        sta chary
        lda #14
        sta charx
        lda #2  	; MESSAGE
        jsr dmsg
        lda #20		; AT
        sta chary
        lda #29
        sta charx
        lda vark	; DISPLAY
        jsr disply
        rts
evnt14:
        rts
evnt15:
        rts
evnt16:
        lda #60		; BEEP
        asl a
        sta sndtyp
        lda #20 	; DELAY
        jsr delay
        lda #50		; BEEP
        asl a
        sta sndtyp
        lda numlif
        sec
        sbc #1
        sta numlif
        lda varo
        ldy #8
        sta (z80_ix),y
        lda varp
        ldy #9
        sta (z80_ix),y
        lda #21		; AT
        sta chary
        lda #29
        sta charx
        lda numlif	; DISPLAY
        jsr disply
        rts
evnt17:
        lda #50 	; DELAY
        jsr delay
        jsr cls		; CLS
        lda #10		; AT
        sta chary
        lda #10
        sta charx
        lda #0  	; MESSAGE
        jsr dmsg
        lda #9
        sta chary
        lda #18  	; MESSAGE
        jsr dmsg
        lda #12		; AT
        sta chary
        lda #12
        sta charx
        lda #21  	; MESSAGE
        jsr dmsg
        lda #14		; AT
        sta chary
        lda #15
        sta charx
        lda #14  	; MESSAGE
        jsr dmsg
        lda #200 	; DELAY
        jsr delay
        rts
evnt18:
        jsr cls		; CLS
        lda #50 	; DELAY
        jsr delay
        lda #1		; AT
        sta chary
        lda #0
        sta charx
        lda #19  	; MESSAGE
        jsr dmsg
        lda #21		; AT
        sta chary
        lda #11
        sta charx
        lda #20  	; MESSAGE
        jsr dmsg
        jsr prskey	; WAITKEY
        rts
evnt19:
        rts
evnt20:
        rts
ptcusr: rts
msgdat:
        .byte "FOGGYS;QUEST",141
        .byte "        LIVES:",141
        .byte "     CRYSTALS:",141
        .byte "@",141
        .byte "`",141
        .byte "____________________________",141
        .byte "$",141
        .byte "BY:;J.BLYTHE",141
        .byte "x",141
        .byte ";KEY;BLUE;;",13
        .byte "CRYSTAL;BLU",13
        .byte "CRYSTAL;RED",13
        .byte "CRYSTAL;GRN",13
        .byte ";;KEY;RED;;",13
        .byte ";KEY;PINK;;",13
        .byte ";;LADDER;;;",13
        .byte ";;;BOMB;;;;",13
        .byte ";THE;CUBE;;",13
        .byte ";THE;SKULL;",13
        .byte ";PIT;PLANT;",13
        .byte ";;;ROCK;;;;",13
        .byte "EXTRA;LIFE;",13
        .byte "EXTRA;LIFE;",13
        .byte ";;A;SEED;;;",13
        .byte ";;BALLOON;;",13
        .byte ";PICK;AXE;;",13
        .byte ";;BRICKS;;;",13
        .byte ";ICE;DRILL;",13
        .byte ";;SPLICER;;",141
        .byte "z",141
        .byte "WELL;DONE;FOGGY!!",141
        .byte ";;;;;;;;w___________x__y;;;.",13
        .byte ";.;;;;;;@;;;;;;;;;;;;;;@",13
        .byte ";;;;;;;;z;;;CONTROLS;;;@;.",13
        .byte ";;;;;.;;@;;;;;;;;;;;;;;@;;;;;;.",13
        .byte ";;;;.;;;@;;1;KEYBOARD;;z",13
        .byte ";;;;;;;;@;;2;JOYKEY;;;;@",13
        .byte ";;;;;;;;z;;3;JOYMMC;;;;@;;;;.",13
        .byte ";;;;;;.;@;;;;;;;;;;;;;;@;;;;;.",13
        .byte ";;.;;;;;`_xx___________$;;.",13
        .byte 13
        .byte ";;;;;;;;;;KEYS:O,P,Q,A",13
        .byte ";;;;;;;;;;;;;;;I-INV.",13
        .byte ";;;;;;;;;;;;;;;SPC-JUMP",141
        .byte ";;;;;;;;;RUCKSACK;GAMES",13
        .byte ";;;;;;;c;2017;JOHN;BLYTHE",141
        .byte "/+",141
        .byte ">",141
        .byte "?",141
        .byte ";;;;;;;;;POWERED;BY;AGD",141
        .byte ";;;;;;;;;;v",13
        .byte ";;;;;;;;;;u",141
        .byte ";;;HOME;AT;LAST.;FOGGY;TAKES;A",13
        .byte ";",13
        .byte ";WELL;EARNED;REST;FROM;HOLIDAYS.",13
        .byte ";",13
        .byte ";;;;TIME;FOR;BIT;OF;TREASURE",13
        .byte ";",13
        .byte ";;;;;;;;HUNTING;INSTEAD!!!",13
        .byte ";",13
        .byte ";",13
        .byte ";;;;;;;THANKS;FOR;PLAYING!",13
        .byte ";",13
        .byte ";CREDITS..",13
        .byte ";",13
        .byte ";CODING;AND;GFX;-;JOHN;BLYTHE",13
        .byte ";ADDITIONAL;CODE;-;ANDY;JOHNS",13
        .byte ";",13
        .byte ";MANY;THANKS;TO..",13
        .byte ";JONATHAN CAULDWELL;FOR;AGD",141
        .byte "PRESS;A;KEY",141
        .byte "IS;OVER..",141
        .byte ";;;;;6502;CODE;KEES;VAN;OSS",141
nummsg:
        .byte 23
chgfx:
        .byte 0,0,0,0,0,0,0,0
        .byte 12,108,97,128,30,222,222,32
        .byte 70,6,102,97,0,51,179,48
        .byte 0,8,65,0,16,0,130,32
        .byte 126,129,141,141,129,161,129,126
        .byte 255,126,189,90,165,66,129,0
        .byte 255,126,189,90,165,66,129,0
        .byte 255,126,189,90,165,66,129,0
        .byte 255,126,189,90,165,66,129,0
        .byte 239,239,170,69,170,0,239,0
        .byte 248,254,174,87,171,23,139,87
        .byte 75,87,75,87,0,87,75,87
        .byte 75,87,74,68,34,48,0,0
        .byte 126,255,170,85,3,187,56,66
        .byte 183,183,0,248,240,0,192,128
        .byte 215,215,0,29,13,0,3,1
        .byte 12,25,30,76,56,28,14,24
        .byte 12,88,48,38,72,68,36,24
        .byte 239,239,170,69,170,0,239,0
        .byte 0,0,0,0,0,0,0,0
        .byte 181,0,96,6,0,170,0,85
        .byte 96,96,96,255,170,0,64,96
        .byte 6,6,6,255,170,0,4,6
        .byte 132,41,16,90,32,148,1,40
        .byte 79,159,0,79,0,159,149,79
        .byte 250,253,0,250,0,253,85,250
        .byte 126,129,141,141,129,161,129,126
        .byte 126,129,141,141,129,161,129,126
        .byte 168,130,42,162,136,42,162,42
        .byte 168,130,42,162,136,42,162,42
        .byte 168,130,42,162,136,42,162,42
        .byte 0,0,0,0,66,16,36,255
        .byte 219,255,85,170,255,0,30,12
        .byte 109,255,85,170,255,0,120,48
        .byte 63,64,180,160,128,133,255,0
        .byte 252,2,1,5,13,161,255,0
        .byte 240,252,87,172,240,44,5,14
        .byte 13,63,245,58,15,26,160,112
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,0,0,0,0
        .byte 254,124,184,80,160,64,128,0
        .byte 127,62,29,10,5,2,1,0
        .byte 0,0,0,0,0,0,0,0
        .byte 0,0,0,0,32,0,0,0
        .byte 0,0,0,8,0,0,0,0
        .byte 255,255,255,255,255,255,255,255
        .byte 255,255,255,255,255,255,255,255
        .byte 128,64,160,208,232,244,250,253
        .byte 247,255,247,0,127,255,127,255
        .byte 1,2,5,11,23,47,95,191
        .byte 0,106,64,0,64,0,66,0
        .byte 24,24,36,36,102,126,231,255
        .byte 0,0,0,0,0,0,0,0
        .byte 54,111,221,221,122,50,1,0
        .byte 124,214,191,255,235,213,170,124
        .byte 56,254,255,221,243,127,157,98
        .byte 0,112,236,222,254,251,223,62
        .byte 0,15,63,111,254,249,239,112
        .byte 60,78,191,255,245,106,52,24
        .byte 8,8,4,8,16,32,16,8
        .byte 255,255,255,255,255,255,255,255
        .byte 85,171,87,175,95,191,127,255
        .byte 1,2,5,10,21,42,85,170
        .byte 61,104,208,168,208,128,192,128
        .byte 188,22,11,21,11,1,3,1
        .byte 255,117,170,16,66,0,0,0
        .byte 0,170,0,85,170,85,170,85
        .byte 139,197,227,241,185,221,175,151
        .byte 255,250,210,82,64,64,0,0
        .byte 0,0,0,0,0,0,76,255
        .byte 90,165,219,102,189,102,60,90
        .byte 76,246,251,187,95,70,128,0
        .byte 14,23,45,44,44,88,224,0
        .byte 102,91,164,90,219,25,100,154
        .byte 1,130,102,90,38,28,0,0
        .byte 167,167,167,167,167,167,167,167
        .byte 242,234,242,234,242,234,242,234
bprop:
        .byte 0
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
        .byte 2
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
        .byte 5
        .byte 3
        .byte 3
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
        .byte 2
        .byte 0
        .byte 2
        .byte 0
        .byte 3
        .byte 0
        .byte 0
        .byte 1
        .byte 2
        .byte 1
        .byte 1
        .byte 1
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
        .byte 1
        .byte 1
        .byte 0
        .byte 0
        .byte 0
        .byte 0
        .byte 0
sprgfx:
        .byte 0,0,12,0,23,0,7,192,15,224,30,112,61,168,61,36,125,36,126,32,255,252,198,252,187,4,125,250,125,247,60,15
        .byte 0,0,3,0,5,192,1,240,3,248,7,156,15,106,15,73,31,73,31,136,63,255,49,191,46,193,159,126,223,125,207,3
        .byte 0,0,0,192,1,112,0,124,0,254,1,231,131,218,67,210,71,210,7,226,207,255,204,111,75,176,167,223,119,223,243,192
        .byte 0,0,0,48,0,92,0,31,128,63,192,121,160,246,144,244,145,244,129,248,243,255,243,27,18,236,233,247,221,247,60,240
        .byte 0,0,30,0,7,128,7,192,15,224,31,48,62,212,62,144,126,144,127,16,241,252,238,252,95,4,31,120,15,116,0,12
        .byte 0,0,7,128,1,224,1,240,3,248,7,204,15,181,15,164,31,164,31,196,60,127,59,191,23,193,7,222,3,221,0,3
        .byte 0,0,1,224,0,120,0,124,0,254,1,243,67,237,3,233,7,233,7,241,207,31,206,239,69,240,129,247,64,247,192,0
        .byte 0,0,0,120,0,30,0,31,128,63,192,124,80,251,64,250,65,250,65,252,243,199,243,187,17,124,224,125,208,61,48,0
        .byte 16,0,14,0,7,128,7,192,15,224,31,144,63,104,63,72,127,72,127,136,255,252,252,124,123,132,55,216,7,208,27,192
        .byte 4,0,3,128,1,224,1,240,3,248,7,228,15,218,15,210,31,210,31,226,63,255,63,31,30,225,13,246,1,244,6,240
        .byte 1,0,0,224,0,120,0,124,0,254,1,249,131,246,131,244,135,244,135,248,207,255,207,199,71,184,131,125,0,125,1,188
        .byte 0,64,0,56,0,30,0,31,128,63,64,126,160,253,32,253,33,253,33,254,243,255,243,241,17,238,96,223,64,31,0,111
        .byte 0,0,30,0,7,128,7,192,15,224,31,48,62,212,62,144,126,144,127,16,255,252,241,252,110,4,95,120,31,116,15,12
        .byte 0,0,7,128,1,224,1,240,3,248,7,204,15,181,15,164,31,164,31,196,63,255,60,127,27,129,23,222,7,221,3,195
        .byte 0,0,1,224,0,120,0,124,0,254,1,243,67,237,3,233,7,233,7,241,207,255,207,31,70,224,133,247,65,247,192,240
        .byte 0,0,0,120,0,30,0,31,128,63,192,124,80,251,64,250,65,250,65,252,243,255,243,199,17,184,225,125,208,125,48,60
        .byte 0,0,0,48,0,232,3,224,7,240,14,120,21,188,36,188,36,190,4,126,63,255,63,99,32,221,95,190,239,190,240,60
        .byte 0,0,0,12,0,58,0,248,1,252,3,158,5,111,9,47,137,47,129,31,207,255,207,216,72,55,151,239,187,239,60,15
        .byte 0,0,0,3,128,14,0,62,0,127,128,231,193,91,194,75,226,75,224,71,243,255,51,246,210,13,229,251,238,251,207,3
        .byte 0,0,192,0,160,3,128,15,192,31,224,57,240,86,240,146,248,146,248,17,252,255,140,253,116,131,249,126,251,190,243,192
        .byte 0,0,0,120,1,224,3,224,7,240,12,248,43,124,9,124,9,126,8,254,63,143,63,119,32,250,30,248,46,240,48,0
        .byte 0,0,0,30,0,120,0,248,1,252,3,62,10,223,2,95,130,95,130,63,207,227,207,221,136,62,7,190,11,188,12,0
        .byte 0,0,128,7,0,30,0,62,0,127,128,207,194,183,192,151,224,151,224,143,243,248,115,247,162,15,129,239,2,239,3,0
        .byte 0,0,224,1,128,7,128,15,192,31,224,51,240,173,240,37,248,37,248,35,60,254,220,253,232,131,224,123,192,187,0,192
        .byte 0,8,0,112,1,224,3,224,7,240,9,248,22,252,18,252,18,254,17,254,63,255,62,63,33,222,27,236,11,224,3,216
        .byte 0,2,0,28,0,120,0,248,1,252,2,126,5,191,4,191,132,191,132,127,207,255,207,143,136,119,6,251,2,248,0,246
        .byte 128,0,0,7,0,30,0,62,0,127,128,159,193,111,193,47,225,47,225,31,243,255,243,227,226,29,193,190,0,190,128,61
        .byte 32,0,192,1,128,7,128,15,192,31,224,39,240,91,240,75,248,75,248,71,252,255,252,248,120,135,176,111,128,47,96,15
        .byte 0,0,0,120,1,224,3,224,7,240,12,248,43,124,9,124,9,126,8,254,63,255,63,143,32,118,30,250,46,248,48,240
        .byte 0,0,0,30,0,120,0,248,1,252,3,62,10,223,2,95,130,95,130,63,207,255,207,227,136,29,135,190,11,190,12,60
        .byte 0,0,128,7,0,30,0,62,0,127,128,207,194,183,192,151,224,151,224,143,243,255,243,248,98,7,161,239,130,239,3,15
        .byte 0,0,224,1,128,7,128,15,192,31,224,51,240,173,240,37,248,37,248,35,252,255,60,254,216,129,232,123,224,187,192,195
        .byte 3,192,5,160,14,112,14,240,31,248,31,248,63,252,63,252,63,252,127,254,127,235,23,87,106,173,245,91,247,224,240,0
        .byte 0,240,1,104,3,156,3,188,7,254,7,254,15,255,15,255,15,255,159,255,223,250,197,213,90,171,253,86,61,248,60,0
        .byte 0,60,0,90,0,231,0,239,129,255,129,255,195,255,195,255,195,255,231,255,183,254,113,117,214,170,191,85,15,126,15,0
        .byte 0,15,128,22,192,57,192,59,224,127,224,127,240,255,240,255,240,255,249,255,173,255,92,93,181,170,111,213,131,223,3,192
        .byte 3,192,5,160,13,112,12,240,31,248,31,248,63,252,63,252,63,252,127,254,111,238,151,215,234,173,245,91,103,231,0,0
        .byte 0,240,1,104,3,92,3,60,7,254,7,254,15,255,15,255,15,255,159,255,155,251,229,245,122,171,253,86,217,249,0,0
        .byte 0,60,0,90,0,215,0,207,129,255,129,255,195,255,195,255,195,255,231,255,230,254,121,125,222,170,191,85,118,126,0,0
        .byte 0,15,128,22,192,53,192,51,224,127,224,127,240,255,240,255,240,255,249,255,185,191,94,95,183,170,111,213,157,159,0,0
        .byte 3,192,5,160,14,112,14,240,31,248,31,248,63,252,63,252,63,252,127,254,239,254,213,216,170,166,213,79,7,239,0,15
        .byte 0,240,1,104,3,156,3,188,7,254,7,254,15,255,15,255,15,255,159,255,187,255,53,118,170,169,245,83,193,251,192,3
        .byte 0,60,0,90,0,231,0,239,129,255,129,255,195,255,195,255,195,255,231,255,238,255,141,93,106,170,253,84,240,126,240,0
        .byte 0,15,128,22,192,57,192,59,224,127,224,127,240,255,240,255,240,255,249,255,251,191,99,87,154,170,63,85,188,31,60,0
        .byte 3,192,5,160,14,176,15,48,31,248,31,248,63,252,63,252,63,252,127,254,111,238,151,215,234,173,245,91,103,231,0,0
        .byte 0,240,1,104,3,172,3,204,7,254,7,254,15,255,15,255,15,255,159,255,155,251,229,245,122,171,253,86,217,249,0,0
        .byte 0,60,0,90,0,235,0,243,129,255,129,255,195,255,195,255,195,255,231,255,230,254,121,125,222,170,191,85,118,126,0,0
        .byte 0,15,128,22,192,58,192,60,224,127,224,127,240,255,240,255,240,255,249,255,185,191,94,95,183,170,111,213,157,159,0,0
        .byte 102,102,255,255,128,1,179,205,189,189,191,253,183,237,179,205,179,205,179,205,191,253,191,253,166,101,128,1,255,255,102,102
        .byte 153,153,255,255,96,0,108,243,111,111,111,255,109,251,108,243,108,243,108,243,111,255,111,255,105,153,96,0,255,255,153,153
        .byte 102,102,255,255,24,0,219,60,219,219,219,255,219,126,219,60,219,60,219,60,219,255,219,255,90,102,24,0,255,255,102,102
        .byte 153,153,255,255,6,0,54,207,246,246,246,255,182,223,54,207,54,207,54,207,246,255,246,255,150,153,6,0,255,255,153,153
        .byte 102,102,255,255,128,1,191,253,179,205,189,189,191,253,183,237,179,205,179,205,191,253,166,101,160,5,128,1,255,255,102,102
        .byte 153,153,255,255,96,0,111,255,108,243,111,111,111,255,109,251,108,243,108,243,111,255,105,153,104,1,96,0,255,255,153,153
        .byte 102,102,255,255,24,0,219,255,219,60,219,219,219,255,219,126,219,60,219,60,219,255,90,102,90,0,24,0,255,255,102,102
        .byte 153,153,255,255,6,0,246,255,54,207,246,246,246,255,182,223,54,207,54,207,246,255,150,153,22,128,6,0,255,255,153,153
        .byte 102,102,255,255,128,1,191,253,183,237,187,221,189,189,167,229,187,221,191,253,182,109,160,5,160,5,128,1,255,255,102,102
        .byte 153,153,255,255,96,0,111,255,109,251,110,247,111,111,105,249,110,247,111,255,109,155,104,1,104,1,96,0,255,255,153,153
        .byte 102,102,255,255,24,0,219,255,219,126,219,189,219,219,90,126,219,189,219,255,219,102,90,0,90,0,24,0,255,255,102,102
        .byte 153,153,255,255,6,0,246,255,182,223,118,239,246,246,150,159,118,239,246,255,182,217,22,128,22,128,6,0,255,255,153,153
        .byte 48,0,48,0,31,251,56,31,55,236,47,244,47,244,47,52,46,84,46,20,47,52,47,244,240,12,223,248,0,12,0,12
        .byte 12,0,12,0,199,254,206,7,13,251,11,253,11,253,11,205,11,149,11,133,11,205,11,253,60,3,55,254,0,3,0,3
        .byte 3,0,3,0,177,255,243,129,195,126,66,255,66,255,66,243,66,229,66,225,66,243,66,255,207,0,141,255,192,0,192,0
        .byte 0,192,0,192,236,127,124,224,176,223,208,191,208,191,208,188,80,185,80,184,208,188,208,191,51,192,227,127,48,0,48,0
        .byte 3,0,3,0,31,248,56,28,55,236,47,244,47,247,46,119,236,180,236,52,46,116,47,244,48,12,31,248,0,192,0,192
        .byte 0,192,0,192,7,254,14,7,13,251,11,253,203,253,203,157,59,45,59,13,11,157,11,253,12,3,7,254,0,48,0,48
        .byte 0,48,0,48,129,255,195,129,195,126,66,255,114,255,114,231,78,203,78,195,66,231,66,255,195,0,129,255,0,12,0,12
        .byte 0,12,0,12,224,127,112,224,176,223,208,191,220,191,220,185,211,178,211,176,208,185,208,191,48,192,224,127,0,3,0,3
        .byte 0,48,0,48,31,248,56,28,247,236,239,244,47,244,44,244,41,116,40,116,44,247,47,247,48,12,31,248,12,0,12,0
        .byte 0,12,0,12,7,254,14,7,61,251,59,253,11,253,11,61,10,93,10,29,203,61,203,253,12,3,7,254,3,0,3,0
        .byte 0,3,0,3,129,255,195,129,207,126,78,255,66,255,66,207,66,151,66,135,114,207,114,255,195,0,129,255,0,192,0,192
        .byte 192,0,192,0,224,127,112,224,179,223,211,191,208,191,208,179,208,165,208,161,220,179,220,191,48,192,224,127,0,48,0,48
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,170,170,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,170,170,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,170,170,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,170,170,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,0,0,0,0,0,0,0,0,0,0,85,85,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,85,85,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,170,255,255
        .byte 0,0,56,28,103,230,92,58,91,218,58,92,58,92,60,60,63,252,56,28,59,220,91,218,92,58,103,230,56,28,0,0
        .byte 0,0,14,7,153,249,151,14,150,246,14,151,14,151,15,15,15,255,14,7,14,247,150,246,151,14,153,249,14,7,0,0
        .byte 0,0,195,129,102,126,165,195,165,189,195,165,195,165,195,195,195,255,195,129,195,189,165,189,165,195,102,126,195,129,0,0
        .byte 0,0,112,224,153,159,233,112,105,111,112,233,112,233,240,240,240,255,112,224,112,239,105,111,233,112,153,159,112,224,0,0
        .byte 6,0,24,0,7,224,28,58,27,218,59,29,59,29,60,60,63,252,191,252,184,28,91,216,92,56,7,224,0,24,0,96
        .byte 1,128,6,0,1,248,135,14,134,246,78,199,78,199,15,15,15,255,47,255,46,7,22,246,23,14,1,248,0,6,0,24
        .byte 0,96,1,128,0,126,161,195,161,189,211,177,211,177,195,195,195,255,203,255,203,129,133,189,133,195,0,126,128,1,0,6
        .byte 0,24,0,96,128,31,232,112,104,111,116,236,116,236,240,240,240,255,242,255,114,224,97,111,225,112,128,31,96,0,128,1
        .byte 1,224,0,0,7,224,28,56,27,216,184,220,184,220,188,61,191,253,63,253,59,221,28,56,31,248,7,224,0,0,7,128
        .byte 0,120,0,0,1,248,7,14,6,246,46,55,46,55,111,15,111,255,79,255,78,247,7,14,7,254,1,248,0,0,1,224
        .byte 0,30,0,0,0,126,129,195,129,189,203,141,203,141,219,195,219,255,211,255,211,189,129,195,129,255,0,126,0,0,0,120
        .byte 128,7,0,0,128,31,224,112,96,111,114,227,114,227,246,240,246,255,244,255,116,239,224,112,224,127,128,31,0,0,0,30
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,127,222,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,159,247,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,231,253,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,121,255,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,128,95,222,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,224,151,247,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,248,229,253,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,121,127,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,63,128,79,204,127,222,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,224,19,243,159,247,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,248,196,252,231,253,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,49,63,121,255,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,63,128,79,204,79,214,63,158,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,128,15,224,19,243,147,245,143,231,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,224,3,248,196,252,100,253,227,249,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,56,0,254,49,63,89,63,120,254,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,128,17,128,145,32,72,64,72,204,109,214,127,222,63,140,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,32,4,96,36,72,18,16,18,51,155,117,159,247,15,227,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,8,0,0,8,1,24,9,18,4,132,196,140,102,221,231,253,195,248,255,255
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,70,130,68,1,33,49,35,89,183,121,255,48,254,255,255
        .byte 0,0,0,0,128,0,0,0,0,64,0,64,0,0,0,12,0,4,0,33,0,17,0,19,128,90,113,158,63,140,255,255
        .byte 0,0,0,0,32,0,0,0,0,16,0,16,0,0,0,3,0,1,64,8,64,4,192,4,160,22,156,103,15,227,255,255
        .byte 0,0,0,0,8,0,0,0,0,4,0,4,0,0,192,0,64,0,16,2,16,1,48,1,168,5,231,25,195,248,255,255
        .byte 0,0,0,0,2,0,0,0,0,1,0,1,0,0,48,0,16,0,132,0,68,0,76,0,106,1,121,198,48,254,255,255
        .byte 0,0,0,0,0,0,0,0,0,4,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,19,127,222,255,255
        .byte 0,0,0,0,0,0,0,0,0,1,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,8,192,4,159,247,255,255
        .byte 0,0,0,0,0,0,0,0,64,0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,48,1,231,253,255,255
        .byte 0,0,0,0,0,0,0,0,16,0,32,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,76,0,121,255,255,255
        .byte 127,254,192,3,160,153,176,77,152,37,172,1,150,65,171,33,181,145,186,201,189,101,158,177,175,89,183,173,192,3,127,254
        .byte 159,255,240,0,104,38,108,19,102,9,107,0,101,144,106,200,109,100,110,178,111,89,103,172,107,214,109,235,240,0,159,255
        .byte 231,255,60,0,154,9,219,4,89,130,26,192,25,100,26,178,27,89,155,172,91,214,25,235,154,245,219,122,60,0,231,255
        .byte 249,255,15,0,102,130,54,193,150,96,6,176,6,89,134,172,70,214,38,235,150,245,198,122,102,189,182,222,15,0,249,255
        .byte 0,0,0,0,0,8,0,0,0,8,0,4,0,8,0,4,0,2,0,5,0,2,0,33,0,66,0,36,0,66,0,36
        .byte 0,0,0,0,0,2,0,0,0,2,0,1,0,2,0,1,128,0,64,1,128,0,64,8,128,16,0,9,128,16,0,9
        .byte 0,0,0,0,128,0,0,0,128,0,64,0,128,0,64,0,32,0,80,0,32,0,16,2,32,4,64,2,32,4,64,2
        .byte 0,0,0,0,32,0,0,0,32,0,16,0,32,0,16,0,8,0,20,0,8,0,132,0,8,1,144,0,8,1,144,0
        .byte 0,0,0,0,0,2,0,0,0,2,0,4,0,2,0,4,0,10,0,84,0,8,0,84,0,136,0,68,0,34,0,68
        .byte 0,0,0,0,128,0,0,0,128,0,0,1,128,0,0,1,128,2,0,21,0,2,0,21,0,34,0,17,128,8,0,17
        .byte 0,0,0,0,32,0,0,0,32,0,64,0,32,0,64,0,160,0,64,5,128,0,64,5,128,8,64,4,32,2,64,4
        .byte 0,0,0,0,8,0,0,0,8,0,16,0,8,0,16,0,40,0,80,1,32,0,80,1,32,2,16,1,136,0,16,1
        .byte 3,128,3,128,1,128,3,128,3,128,3,128,3,0,3,0,3,128,3,128,3,128,3,128,1,128,3,128,3,128,3,128
        .byte 0,224,0,224,0,96,0,224,0,224,0,224,0,192,0,192,0,224,0,224,0,224,0,224,0,96,0,224,0,224,0,224
        .byte 0,56,0,56,0,24,0,56,0,56,0,56,0,48,0,48,0,56,0,56,0,56,0,56,0,24,0,56,0,56,0,56
        .byte 0,14,0,14,0,6,0,14,0,14,0,14,0,12,0,12,0,14,0,14,0,14,0,14,0,6,0,14,0,14,0,14
        .byte 7,224,7,224,7,224,3,224,3,224,7,224,7,224,7,224,7,224,7,224,7,224,7,224,7,224,7,192,7,224,7,224
        .byte 1,248,1,248,1,248,0,248,0,248,1,248,1,248,1,248,1,248,1,248,1,248,1,248,1,248,1,240,1,248,1,248
        .byte 0,126,0,126,0,126,0,62,0,62,0,126,0,126,0,126,0,126,0,126,0,126,0,126,0,126,0,124,0,126,0,126
        .byte 128,31,128,31,128,31,128,15,128,15,128,31,128,31,128,31,128,31,128,31,128,31,128,31,128,31,0,31,128,31,128,31
        .byte 31,248,31,240,31,240,31,248,31,248,15,248,31,248,31,248,31,248,31,240,31,248,31,248,15,248,15,248,31,248,31,248
        .byte 7,254,7,252,7,252,7,254,7,254,3,254,7,254,7,254,7,254,7,252,7,254,7,254,3,254,3,254,7,254,7,254
        .byte 129,255,1,255,1,255,129,255,129,255,128,255,129,255,129,255,129,255,1,255,129,255,129,255,128,255,128,255,129,255,129,255
        .byte 224,127,192,127,192,127,224,127,224,127,224,63,224,127,224,127,224,127,192,127,224,127,224,127,224,63,224,63,224,127,224,127
frmlst:
        .byte 0,4
        .byte 4,4
        .byte 8,4
        .byte 12,3
        .byte 15,3
        .byte 18,4
        .byte 22,3
        .byte 25,8
        .byte 33,1
        .byte 34,2
        .byte 36,3,39,0
scdat:
        .word 425,471,436,372,323,381,312,298,276,343,255,305,230,299,292,268,350,320,349,203,222,352,366,303,202,225
        .word 236,382
        .byte 2,2,1,1,1,3,1,3,3,2,255,1,4,2,2,1,1,6,2,1,1,1,2,1,3,3,2,1,14,16,16,15,1
        .byte 255,3,4,1,14,16,16,0,16,0,16,255,0,5,15,1,2,8,3,2,0,17,16,0,15,2,3,1,14,16,0,17,16
        .byte 74,16,0,75,255,0,6,16,15,1,3,0,0,0,17,0,0,15,1,14,0,75,0,0,16,0,17,255,0,7,74,16,76
        .byte 2,2,255,0,7,16,255,0,5,17,255,0,10,17,0,1,3,255,13,4,0,0,0,16,76,255,0,17,1,1,1,14,255,0,4
        .byte 74,16,255,0,18,2,1,1,255,0,6,17,255,0,8,13,13,255,0,7,4,24,25,1,255,0,8,13,13,13,21,22,0,0
        .byte 2,1,16,255,0,7,28,28,1,255,0,8,17,0,0,21,22,0,0,1,3,2,0,0,0,19,19,0,0,28,28,1,13,13,13
        .byte 255,0,8,21,22,0,0,1,3,1,0,0,0,13,13,13,4,24,25,3,8,1,1,13,13,255,0,6,21,22,0,0,2
        .byte 3,1,255,0,5,16,15,7,3,2,14,17,16,255,0,8,21,22,0,13,1,3,1,255,0,5,17,0,15,1,1,0,0
        .byte 17,255,0,5,255,13,6,2,1,3,2,255,0,8,1,1,255,0,8,15,1,2,5,1,2,1,3,1,14,255,0,8,1
        .byte 2,255,0,10,6,1,24,25,18,6,18,9,10,255,0,7,2,1,255,0,5,13,13,13,6,0,15,2,77,78,1,14,0,0
        .byte 11,255,0,4,19,19,0,1,1,255,0,4,13,7,2,1,13,54,54,2,77,78,2,18,10,0,11,255,0,4,19,19,13
        .byte 1,1,0,13,13,13,1,2,3,3,2,54,54,2,24,25,1,0,11,0,11,255,0,4,13,13,5,3,1,13,1,2,1
        .byte 3,3,3,2,2,0,0,1,77,78,1,0,11,0,11,0,0,13,13,1,3,3,1
        .byte 255,3,5,1,3,1,1,17,0,0,1,77,78,2,0,11,0,11,0,0,6,1,1,8,255,3,4,1,23,23,3,3,3
        .byte 2,16,76,17,2,24,25,1,0,12,18,11,0,0,0,2,1,3,23,23,1,3,3,3,23,23,23,3,1,16,75,1,3
        .byte 12,10,1,0,0,0,11,0,0,0,2,3,23,23,0,1,2,1,1,23,0,23,1,3,1,16,2,3,3,11,14,0,0,0
        .byte 11,0,0,0,1,3,23,255,0,4,15,1,2,23,23,23,3,1,2,2,3,1,11,255,0,4,11,13,13,13,2,3,23,23
        .byte 255,0,4,17,1,3,3,23,2,3,3,1,3,1,12,9,9,9,18,13,2,1,1,3,23,23,255,0,6,15,1,255,3,6
        .byte 7,2,255,0,5,1,1,3,3,23,0,0,23,255,0,6,19,19,3,3,3,1,1,1,14,255,0,5,2,3,3,23,0
        .byte 54,54,255,0,9,1,1,1,14,17,16,76,255,0,5,1,3,23,0,23,54,54,23,0,255,13,5,8,1,1,0,16,0,0
        .byte 16,255,0,6,2,3,23,0,0,23,23,3,23,1,2,2,2,1,1,2,0,0,16,0,0,17,255,0,5,17,1,1,23,23,23
        .byte 3,3,1,3,3,2,1,1,14,16,16,76,0,17,255,0,6,13,13,13,1,3,3,23,3,1,255,3,4,1,1,14,74
        .byte 16,75,255,0,9,15,1,2,1,3,1,3,1,1,2,1,2,3,6,1,0,0,17,255,0,4,13,13,13,255,0,4,11
        .byte 7,1,2,1,1,0,17,11,0,0,1,2,14,255,0,7,2,1,1,255,0,4,12,9,26,24,25,26,9,18,11,18,9
        .byte 255,0,8,13,13,1,5,2,13,17,255,0,5,29,29,0,0,0,11,255,0,9,17,2,1,2,3,3,1,13,13,19,19
        .byte 0,0,29,29,0,0,0,11,255,0,8,13,13,1,255,3,5,2,1,13,13,13,26,24,25,26,255,13,11,2,1,1,255,3,6
        .byte 7,255,1,4,2,2,255,1,5,2,1,2,2,1,2,1,1,3,3,6,255,3,6,2,255,3,4,1,3,3,3,2,1
        .byte 3
        .byte 255,0,5,23,3,3,2,1,1,21,22,1,1,2,1,1,21,22,1,1,1,2,3,23,5,23,255,0,4,23,23,3,1
        .byte 14,0,0,21,22,15,14,16,15,1,21,22,17,16,15,1,1,3,23,3,255,0,4,23,3,3,1,0,0,0,21,22,0
        .byte 74,16,0,2,21,22,0,16,76,15,2,1,3,1,255,0,5,23,3,2,0,0,0,21,22,0,0,17,0,1,21,22,74
        .byte 16,0,0,16,15,1,2,255,0,4,23,23,1,14,0,0,0,21,22,255,0,4,8,21,22,0,17,0,0,17,0,16,255,0,5
        .byte 23,3,1,75,0,0,0,21,22,255,0,4,2,21,22,255,0,6,75,255,0,4,23,3,3,2,1,0,0,0,21,22,255,0,4
        .byte 1,21,22,255,0,12,23,3,1,14,0,0,0,21,22,255,0,4,1,13,13,17,255,0,11,23,3,1,17,0,0,0,21
        .byte 22,255,0,5,2,1,13,13,255,0,9,23,23,3,8,1,13,21,22,13,13,255,0,5,16,1,1,2,255,13,6,0,0,0
        .byte 23,3,3,3,2,1,21,22,16,76,255,0,5,16,15,1,7,1,2,1,1,2,1,255,0,4,23,3,1,1,14,21,22
        .byte 17,255,0,6,17,0,0,1,3,3,1,3,3,3,255,0,4,23,3,1,14,0,21,22,255,0,10,15,1,1,23,23,23
        .byte 3,0,0,0,23,3,3,2,0,0,21,22,255,0,12,15,5,3,1,3,255,0,4,23,3,1,17,0,21,22,255,0,13
        .byte 15,1,1,1,255,0,4,23,3,1,16,0,21,22,255,0,21,23,3,1,2,0,21,22,0,38,38,255,0,4,38,38,255,0,11
        .byte 23,23,3,7,1,17,21,22,31,38,38,31,0,0,0,38,38,31,31,255,0,9,23,3,23,3,1,13,13,13,255,20,4
        .byte 13,13,13,255,20,4,255,13,6,255,0,4,23,23,3,3,1,2,255,1,5,2,1,1,8,255,1,4,2,7,1,2,1
        .byte 23,3,1,1,3,1,3,3,1,2,2,1,1,2,1,1,7,1,1,255,3,4,1,2,1,1,3,23,1,1,16,1,3,3
        .byte 1,1,14,0,16,0,0,16,76,0,0,0,1,2,1,8,1,14,15,2,1,3,2,14,16,15,2,1,14,0,0,74,16
        .byte 0,0,17,255,0,4,16,15,1,17,16,76,0,15,1,1,1,0,16,76,16,255,0,5,75,255,0,7,17,0,1,0,17
        .byte 255,0,4,1,14,74,16,0,17,255,0,15,1,255,0,6,1,0,0,17,255,0,17,2,1,21,22,13,13,13,1,255,0,13
        .byte 13,13,13,255,0,6,21,22,15,2,3,2,255,0,13,7,1,1,255,0,6,21,22,0,1,7,1,255,0,6,13,13,13
        .byte 255,0,4,1,3,1,255,0,6,21,22,0,15,1,1,255,0,13,1,3,8,255,0,6,21,22,0,0,1,1,13,13,255,0,11
        .byte 2,3,1,255,0,6,21,22,0,0,1,1,14,255,0,12,1,3,1,255,0,8,13,13,1,2,255,0,13,1,1,1,255,0,9
        .byte 15,2,7,255,13,6,255,0,7,1,2,1,13,13,255,0,8,1,1,17,6,0,0,6,255,0,8,1,3,2,14,255,0,9
        .byte 1,1,13,255,0,9,13,13,13,1,3,1,255,0,7,13,13,13,2,3,8,13,255,0,10,15,8,1,1,255,0,9,15
        .byte 1,23,3,1,13,17,255,0,9,1,3,2,17,255,0,8,17,1,0,23,3,1,255,13,4,255,0,5,17,1,3,1,16
        .byte 0,0,0,17,255,13,5,1,0,23,23,3,1,5,1,2,13,13,13,21,22,13,1,23,3,13,21,22,13,13,255,1,4
        .byte 2,1
        .byte 255,0,7,3,1,21,22,1,3,23,255,0,21,3,1,21,22,1,3,23,255,0,20,23,3,1,21,0,2,3,3,23,255,0,18
        .byte 23,3,1,14,0,0,15,1,2,1,3,23,23,255,0,14,23,3,1,1,255,0,4,74,16,15,2,1,7,3,3,23,255,0,8
        .byte 23,23,3,2,1,2,14,255,0,5,16,76,0,15,1,2,1,1,3,23,23,0,23,0,0,3,1,1,2,14,16,0,0,0
        .byte 19,19,0,0,17,255,0,4,16,15,1,2,1,1,3,3,23,23,1,14,0,17,0,17,0,0,0,13,13,255,0,7,17
        .byte 0,0,0,15,1,1,2,2,1,255,0,42,13,13,255,0,12,255,13,8,17,255,0,19,3,5,1,1,2,1,1,2,13,13
        .byte 255,0,18,23,255,3,7,5,2,13,13,13,0,0,0,21,22,255,13,10,255,0,4,23,0,0,23,23,3,3,3,2,0,0,0
        .byte 21,22,255,1,5,2,1,1,2,1,255,0,9,23,23,3,1,0,0,0,21,22,2,3,3,3,23,3,3,23,23,3,255,0,10
        .byte 23,3,1,0,0,0,21,22,1,3,23,23,0,0,23,0,0,23,255,0,11,3,2,13,13,0,21,22,1,8,23,255,0,8
        .byte 54,54,255,0,7,23,3,1,14,0,0,21,22,1,3,255,0,9,54,54,255,0,8,3,1,0,0,0,21,22,2,3,23
        .byte 255,0,18,3,1,21,22,13,13,13,1,3,255,0,8
        .byte 255,0,10,23,3,1,21,22,1,1,1,3,3,23,23,0,0,3,3,23,3,255,0,11,3,1,21,22,0,15,255,1,5
        .byte 8,255,1,5,18,9,9,10,255,0,7,23,1,21,22,0,0,0,16,17,255,0,4,16,76,255,0,5,11,255,0,7,23
        .byte 1,21,22,0,0,0,17,255,0,5,17,255,0,6,11,255,0,8,1,21,22,255,0,16,11,255,0,7,23,1,13,13,19,19
        .byte 17,255,0,7,17,255,0,5,11,255,0,7,23,3,1,1,255,13,6,0,255,13,6,0,0,0,11,255,0,9,23,3,1,1
        .byte 7,1,1,16,0,15,1,1,3,3,1,0,0,0,12,255,9,5,10,255,0,6,23,23,3,1,17,0,75,1,3,255,23,4
        .byte 255,0,8,11,0,0,23,255,0,5,1,17,40,39,16,17,1,23,0,0,23,23,0,23,0,0,23,23,3,11,24,25,1
        .byte 3,0,23,0,23,1,16,40,40,16,16,1,0,0,0,255,3,4,2,1,1,3,1,12,77,78,15,1,3,0,0,0,3
        .byte 1,13,13,1,1,3,0,0,0,1,2,1,1,14,16,16,1,14,17,24,25,0,16,1,255,0,4,23,3,3,23,23,255,0,5
        .byte 16,0,0,74,16,17,0,0,0,38,38,0,17,1,23,255,0,12,18,17,18,10,0,17,255,0,4,38,38,0,0,2,23
        .byte 255,0,15,11,255,0,6,34,35,0,0,1,23,255,0,15,11,0,0,0,19,19,37,32,33,36,0,7,255,0,13,255,13,14
        .byte 23,23,255,0,9,54,54,0,2,1,1,3,3,1,5,2,1,3,1,1,3,0,23,255,0,10,54,54,0,23,23,3,3
        .byte 23,0,23,23,3,23,23,0,23,255,0,15
        .byte 0,0,0,23,3,1,0,0,45,255,0,7,8,255,5,5,8,255,0,8,3,1,14,255,0,12,43,6,42,255,0,6,45
        .byte 0,0,0,3,1,75,255,0,4,46,255,0,8,5,0,46,255,0,10,8,1,255,0,25,23,3,1,19,19,255,0,11,5
        .byte 255,0,8,23,23,0,23,3,1,255,5,4,8,0,46,255,0,15,3,3,23,3,1,14,0,0,0,43,5,255,0,8,7
        .byte 255,0,8,255,1,4,14,0,0,0,45,0,5,255,0,18,16,15,14,0,0,0,46,0,0,5,255,0,18,17,255,0,8
        .byte 5,5,7,255,0,13,46,255,0,11,5,42,255,0,26,5,45,255,0,7,7,5,5,5,255,0,5,255,13,4,5,5,7
        .byte 0,0,0,7,255,0,5,7,5,5,42,0,43,7,255,0,5,1,2,1,255,0,17,41,19,8,8,255,0,4,3,7,1
        .byte 255,0,17,41,41,7,42,255,0,4,23,3,3,13,0,0,7,5,0,5,0,5,0,5,0,5,0,5,0,5,0,5,5
        .byte 0,0,0,17,0,0,23,3,1,0,0,38,38,255,0,12,38,38,0,0,17,1,1,2,0,0,3,2,31,31,38,38,255,31,12
        .byte 38,38,31,31,2,2,7,3,0,3,3,1,255,20,20,1,3,23,23,0,23,3,1,1,1,2,1,2,1,1,2,255,1,4
        .byte 2,1,1,5,1,1,1,2,1,3,23,0
        .byte 3,23,3,23,23,3,3,23,23,3,23,255,0,8,23,3,3,23,23,3,8,3,23,1,1,2,2,1,8,1,1,1,2
        .byte 1,3,23,255,0,4,23,3,255,1,4,2,1,1,2,1,0,16,0,17,0,0,0,16,76,0,15,1,3,255,0,4,3
        .byte 2,14,16,0,0,74,16,17,15,1,0,17,255,0,5,17,0,0,0,15,1,23,0,0,0,1,14,0,75,0,0,0,17
        .byte 0,0,1,255,0,11,17,1,3,23,0,23,1,0,40,40,255,0,7,17,255,0,8,38,0,13,1,23,0,0,0,1,17
        .byte 255,0,7,17,0,255,13,5,0,0,255,13,4,1,3,255,0,4,23,1,13,13,0,0,255,13,5,1,1,2,1,1,0,0
        .byte 2,1,3,1,3,23,255,0,5,3,7,2,0,0,1,1,2,3,1,3,23,3,3,2,0,0,1,3,23,23,255,0,8
        .byte 23,1,0,0,2,3,3,23,3,0,0,23,3,1,0,0,1,3,255,0,11,1,0,0,1,3,23,255,0,5,1,2,0,0
        .byte 1,23,255,0,11,2,0,0,1,23,255,0,6,23,1,0,0,1,3,255,0,10,23,3,1,1,3,23,255,0,6,5,1
        .byte 0,0,1,23,255,0,11,23,3,0,23,255,0,6,23,3,1,1,2,3,0,23,255,0,21,23,3,3,23,255,0,161
        .byte 0,0,0,45,255,0,10,44,255,0,8,7,5,21,22,5,255,0,25,21,22,255,0,12,46,255,0,10,45,0,0,21,22
        .byte 8,255,0,25,21,22,255,0,28,5,255,0,16,46,255,0,38,7,255,0,5,46,255,0,13,7,5,5,7,255,0,14,8
        .byte 5,5,8,255,0,6,12,10,255,0,6,5,255,0,10,11,255,0,9,11,255,0,4,46,255,0,12,11,255,0,9,11,255,0,6
        .byte 255,5,5,7,255,0,5,11,255,0,9,11,0,0,53,255,0,7,12,255,9,5,18,12,10,255,0,5,45,0,0,11,255,0,6
        .byte 8,255,0,9,45,0,11,255,0,6,46,0,11,255,0,10,17,255,0,7,11,255,0,8,11,0,0,53,0,0,0,7,0,0,0
        .byte 16,255,0,7,11,255,0,8,11,255,0,7,255,13,5,17,0,0,7,255,5,5,7,255,0,5,11,255,0,6,5,1,1
        .byte 2,1,1,13,21,22,5,42,0,11,0,43,5,255,0,5,11,0,0,53,255,0,4,3,23,23,3,1,2,21,22,5,0,0
        .byte 12,9,9,5,18,9,9,10,0,11,255,0,6,6,0,0,0,23,3,1,21,22,8,255,0,5,8,21,22,7,5,5,5
        .byte 7,255,0,5,5
        .byte 23,23,3,23,3,1,21,22,8,5,0,5,0,5,7,21,22,7,255,0,9,8,3,2,1,1,2,14,21,22,255,0,5
        .byte 43,5,21,22,255,0,11,2,1,14,16,0,0,21,22,255,0,6,5,21,22,255,0,5,45,255,0,4,5,1,2,0,17
        .byte 0,43,7,5,0,5,0,5,5,5,8,21,22,255,0,7,46,0,0,0,7,6,255,0,10,46,43,5,21,22,255,0,10
        .byte 5,8,5,0,0,0,19,19,255,0,7,5,21,22,0,45,255,0,9,255,13,7,255,0,8,21,22,255,0,9,45,5,255,1,5
        .byte 2,5,5,5,7,255,0,4,5,0,22,255,0,11,3,23,3,23,3,1,14,255,0,20,5,0,0,0,23,3,1,0,0
        .byte 45,255,0,5,5,255,0,5,7,5,0,0,0,21,22,255,0,5,3,2,0,46,255,0,5,5,7,255,0,10,21,22,5
        .byte 0,0,0,23,1,14,255,0,6,5,42,255,0,7,46,0,0,0,21,22,255,0,4,3,1,255,0,20,21,22,5,0,0
        .byte 23,1,14,255,0,10,45,255,0,9,21,0,0,0,23,3,2,17,255,0,4,8,255,0,17,5,0,0,3,1,16,76,255,0,24
        .byte 23,1,13,0,0,0,38,38,0,0,7,0,5,0,5,0,5,0,5,7,38,38,0,0,0,5,0,54,54,1,1,31,31,31
        .byte 38,38,255,31,12,38,38,31,31,31,5,0,54,54,3,2,255,20,22,13,0,0,0,23,3,2,255,1,7,2,255,1,6
        .byte 2,2,1,1,1,2,1,1
        .byte 255,44,28,255,0,33,46,255,0,26,45,255,0,7,19,19,255,0,10,46,0,0,0,8,7,5,5,0,8,0,5,8,0
        .byte 5,0,7,5,0,6,0,8,5,0,5,0,7,5,0,5,0,8,43,54,54,42,255,0,11,39,38,255,0,10,44,0,54,54
        .byte 255,0,11,45,40,40,255,0,10,17,255,0,24,45,0,0,13,255,0,13,11,255,0,13,1,255,0,8,46,255,0,4,11
        .byte 0,0,0,45,255,0,5,46,0,0,0,1,255,0,13,11,255,0,13,1,255,0,13,11,255,0,13,1,255,0,12,11,11
        .byte 255,0,13,8,255,0,12,11,11,255,0,13,1,255,0,12,11,12,10,255,0,11,75,2,0,0,0,17,255,0,8,12,18
        .byte 11,10,255,0,9,74,16,1,0,0,0,16,76,255,0,9,11,11,0,0,0,17,255,0,5,17,1,3,255,13,9,21,22
        .byte 13,13,13,24,25,255,13,10,1,3,1,2,1,1,1,5,1,1,1,21,22,2,1,1,77,78,1,2,1,1,7,1,1
        .byte 2,2,1,3,3,255,0,4,23,0,23,3,2,21,22,2,3,23,32,33,255,0,12
        .byte 14,255,0,9,6,21,22,5,44,44,44,24,77,78,25,255,44,7,255,0,5,46,0,54,54,54,5,21,22,255,0,4,27
        .byte 24,25,27,0,0,0,45,255,0,5,45,255,0,4,54,54,54,5,21,22,255,0,5,30,30,255,0,15,54,54,54,7,21
        .byte 22,255,0,5,30,30,0,0,19,19,255,0,14,43,5,5,7,0,0,7,27,24,25,27,7,5,5,7,5,8,5,255,0,7
        .byte 45,0,0,0,43,0,42,0,0,43,0,77,78,0,42,43,0,42,43,0,42,255,0,6,46,255,0,11,77,78,255,0,26
        .byte 24,25,0,0,45,255,0,18,46,0,0,0,37,32,33,36,255,0,4,46,255,0,17,44,0,0,38,38,255,0,16,75,255,0,9
        .byte 38,38,255,0,16,16,76,255,0,8,38,38,255,0,15,74,16,255,0,9,38,38,255,0,15,17,16,255,0,9,38,38,255,0,15
        .byte 16,16,76,255,0,8,38,38,0,0,17,255,0,12,16,16,0,17,255,0,7,38,38,0,0,16,76,255,0,10,17,13,13
        .byte 17,16,255,0,7,38,38,0,17,16,255,0,5,255,13,7,1,1,255,13,8,66,67,67,65,255,13,7,1,1,1,3,1
        .byte 7,1,14,15,1,3,1,1,1,2,1,2,1,66,65,255,1,7,2,0,0,23,255,0,6,23,23,255,0,6,23,0,23
        .byte 255,0,8
        .byte 255,44,4,15,255,1,4,2,1,14,0,44,0,15,2,1,3,23,0,0,0,23,23,3,3,1,255,0,6,15,14,0,16
        .byte 17,255,0,6,16,1,3,3,23,3,3,1,1,2,14,255,0,9,75,0,46,255,0,5,17,15,1,2,1,1,2,14,16,16
        .byte 255,0,4,46,255,0,16,17,16,45,0,0,17,16,76,255,0,5,46,255,0,15,17,255,0,4,75,255,0,25,46,255,0,12
        .byte 45,255,0,12,45,255,0,26,45,255,0,23,46,255,0,11,45,255,0,89,75,255,0,27,17,255,0,26,74,16,17,255,0,4
        .byte 13,255,0,21,16,16,76,0,38,38,5,38,38,255,0,16,17,0,0,13,13,31,31,38,38,5,38,38,31,31,13,13,255,0,4
        .byte 17,0,0,0,255,13,7,1,1,255,20,4,5,255,20,4,1,2,255,13,8,1,1,1,3,1,7,1,14,15,1,3,1,1
        .byte 6,2,1,2,1,3,8,255,1,7,2,0,0,23,255,0,6,23,23,255,0,8,23,255,0,8
        .byte 44,44,21,22,0,255,44,4,255,0,4,44,255,0,13,44,0,0,21,22,255,0,26,21,22,255,0,7,46,255,0,18,21
        .byte 22,255,0,18,45,0,40,19,255,0,4,21,22,0,46,255,0,18,40,19,255,0,4,21,22,255,0,19,59,57,57,57,58
        .byte 59,57,58,59,58,255,0,4,45,255,0,12,45,0,54,54,54,0,0,59,57,58,255,0,17,45,0,0,54,54,54,255,0,12
        .byte 51,24,25,49,0,0,46,255,0,11,45,255,0,8,51,48,48,48,50,49,255,0,21,51,50,50,255,48,4,49,255,0,19
        .byte 51,48,48,50,255,48,5,49,255,0,17,51,255,47,9,48,49,255,0,15,51,48,255,47,9,50,48,49,255,0,7,17,255,0,5
        .byte 51,48,48,47,47,47,48,48,48,50,255,47,5,49,255,0,5,74,16,0,17,0,0,51,50,50,48,48,47,47,47,42,43
        .byte 48,255,47,6,49,255,0,5,16,0,16,0,51,48,48,50,48,48,47,47,47,0,0,50,48,48,48,50,48,48,48,49,0
        .byte 17,0,0,16,255,13,10,50,50,5,5,50,50,255,13,12,1,1,1,3,1,7,1,2,1,1,3,50,52,52,50,1,2
        .byte 1,3,2,1,7,1,1,8,1,1,2,0,0,23,255,0,6,23,1,50,52,52,50,2,23,0,0,23,23,255,0,7
        .byte 13,255,44,27,1,17,255,0,7,59,57,57,58,255,0,15,1,16,75,255,0,25,1,16,16,76,255,0,18,45,255,0,5
        .byte 2,13,13,17,255,0,24,1,2,1,13,61,255,0,19,46,0,0,0,3,23,3,1,13,255,0,4,45,255,0,10,59,57
        .byte 58,0,0,59,57,58,23,0,23,3,1,255,0,11,59,58,0,0,0,45,255,0,8,3,1,255,0,6,40,39,0,0,59
        .byte 57,58,46,255,0,12,23,1,255,0,6,40,40,255,0,19,2,255,0,6,17,255,0,20,1,17,255,0,4,74,16,255,0,20
        .byte 3,13,255,0,5,16,76,255,0,5,75,255,0,13,3,1,255,0,4,17,16,0,0,17,0,0,74,16,255,0,5,17,255,0,7
        .byte 23,2,0,0,0,74,16,16,76,0,16,76,0,0,16,76,17,0,0,0,16,0,75,255,0,5,23,1,0,0,17,0,16,16
        .byte 0,17,16,0,0,17,16,0,16,0,0,74,16,17,16,255,0,5,3,2,13,61,16,0,16,16,0,16,16,19,61,16,16
        .byte 0,16,0,0,61,16,16,16,76,0,0,54,54,23,3,1,255,13,22,0,54,54,0,23,7,255,1,4,3,1,1,1,2
        .byte 1,2,1,3,1,1,1,8,255,1,4,2,255,0,9,23,23,255,0,8,23,255,0,8
        .byte 255,0,11,50,52,52,50,255,0,24,50,52,52,50,23,255,0,22,23,50,52,52,48,23,23,255,0,19,50,50,50,48,52,52
        .byte 255,50,4,255,0,17,23,50,255,0,8,50,255,0,18,50,255,0,8,50,23,255,0,17,48,50,50,50,52,52,50,50,50
        .byte 48,255,0,19,23,23,50,52,52,50,23,255,0,6,23,255,0,16,48,52,52,50,255,0,4,23,0,23,23,23,255,0,14
        .byte 23,50,52,52,48,0,0,255,50,4,48,255,50,4,255,0,13,50,52,52,50,23,0,50,255,0,4,39,39,17,50,255,0,13
        .byte 50,52,52,50,0,0,50,255,0,4,39,39,0,48,23,255,0,9,255,50,4,52,52,50,23,23,48,0,0,255,50,4,48,48
        .byte 255,0,9,23,50,0,0,0,52,52,50,48,50,50,0,0,50,23,23,255,0,12,23,48,0,0,0,52,52,255,0,6,50
        .byte 23,255,0,14,50,19,19,0,52,52,255,0,6,48,255,0,15,255,50,4,52,52,0,0,17,255,50,4,255,0,8,54,54
        .byte 255,0,8,48,52,52,255,50,4,255,0,11,54,54,255,0,7,23,50,52,52,50,255,0,23,50,50,52,52,50,48,255,0,12
        .byte 255,0,11,50,52,52,50,23,255,0,21,23,48,50,52,52,50,48,255,0,20,23,48,50,0,52,52,0,50,48,23,255,0,14
        .byte 23,0,23,23,48,50,0,0,52,52,0,0,50,48,23,0,0,0,23,255,0,8,255,50,4,48,50,0,0,0,52,52,0,0,0
        .byte 255,50,4,48,50,50,50,255,0,6,50,255,0,20,50,255,0,6,48,255,0,20,50,255,0,5,23,50,52,52,50,48,255,50,7
        .byte 48,50,48,255,50,4,52,52,48,23,255,0,5,50,52,52,50,50,50,48,50,42,255,0,4,43,50,50,48,48,50,52,52
        .byte 50,255,0,6,50,255,0,8,44,39,38,44,255,0,8,48,255,0,6,50,255,0,8,51,39,38,49,255,0,8,50,23,255,0,5
        .byte 50,52,52,50,48,50,50,42,0,255,50,4,0,43,48,50,50,50,52,52,50,23,255,0,5,50,52,52,50,42,255,0,12
        .byte 43,50,52,52,50,255,0,5,23,48,52,52,255,0,16,52,52,50,255,0,6,50,52,52,255,0,5,51,0,38,38,0,49
        .byte 255,0,5,52,52,50,255,0,5,23,50,52,52,255,0,5,50,31,38,38,31,50,255,0,5,52,52,48,255,0,6,50,52,52
        .byte 255,0,5,50,255,20,4,50,17,0,38,38,0,52,52,50,255,0,4,54,54,48,52,52,0,0,19,19,0,50,255,68,4
        .byte 50,16,76,0,0,0,52,52,50,23,0,0,0,54,54,255,50,4,48,255,50,7,48,50,50,50,48,48,255,50,4,255,0,13
        .byte 23,0,23,255,0,6,23,23,255,0,7
        .byte 255,44,28,0,0,0,45,255,0,5,46,255,0,15,45,0,46,255,0,27,44,255,0,18,7,5,5,5,57,58,0,0,46
        .byte 44,7,5,5,7,0,6,0,0,8,5,5,7,0,0,5,7,255,0,5,8,255,0,5,44,5,0,0,45,255,0,5,54,54
        .byte 255,0,6,45,255,0,7,59,57,57,5,38,38,0,0,5,0,0,5,54,54,5,0,0,5,255,0,12,44,5,38,38,0
        .byte 59,58,255,0,14,59,7,255,0,5,44,8,57,58,0,0,5,0,0,5,45,0,5,0,0,5,255,0,6,5,57,58,0,0
        .byte 46,44,5,255,0,4,8,5,5,8,0,0,8,5,5,7,255,0,6,5,45,255,0,4,44,7,59,58,255,0,8,45,0,0,0
        .byte 59,57,58,0,0,0,7,0,0,0,59,57,57,255,0,27,44,255,0,27,44,7,57,58,255,0,4,59,57,57,58,0,0,0
        .byte 59,57,57,58,59,57,57,7,57,58,0,0,0,44,5,0,45,255,0,7,46,255,0,11,46,255,0,4,44,5,255,0,5
        .byte 59,57,58,255,0,8,59,58,0,0,5,0,0,59,57,57,58,6,255,0,14,46,255,0,11,44,5,17,0,59,58,255,0,16
        .byte 8,255,0,5,44,5,16,17,255,0,13,19,19,255,0,4,17,0,0,21,22,44,5,5,8,0,8,0,5,0,5,0,8
        .byte 0,8,0,8,0,5,0,7,0,7,5,5,5,7,21,22,8
        .byte 255,44,11,24,25,24,25,24,25,255,44,11,0,0,45,255,0,4,46,0,0,0,255,47,6,0,0,45,46,0,0,0,45
        .byte 255,0,14,255,47,6,255,0,22,255,47,6,255,0,11,8,5,5,7,255,0,4,45,38,38,24,25,24,25,24,25,38,38
        .byte 255,0,5,7,5,8,5,7,42,0,43,5,7,0,0,31,38,38,43,42,43,42,43,42,38,38,31,0,0,7,5,42,0
        .byte 46,5,75,0,0,45,0,43,8,5,255,20,12,5,5,42,0,0,45,0,5,13,0,0,0,46,0,43,255,5,14,42,255,0,5
        .byte 8,1,13,255,0,25,5,3,2,255,0,25,5,23,1,17,5,5,21,22,5,7,0,0,0,8,5,5,7,0,0,0,7
        .byte 5,5,7,21,22,7,5,8,23,1,16,76,0,21,22,43,5,0,0,0,5,42,43,5,0,0,0,5,42,43,5,21,22
        .byte 0,0,0,3,1,13,0,0,21,22,0,43,5,0,5,42,0,0,43,5,0,5,42,0,0,5,21,22,0,0,0,1,14
        .byte 0,0,0,21,22,255,0,15,255,5,5,7,1,17,0,0,0,21,22,255,0,20,5,1,255,13,4,5,0,6,0,5,255,0,17
        .byte 7,3,1,1,1,2,17,0,0,0,43,5,0,5,0,7,0,5,0,5,0,7,0,5,0,8,21,22,5,0,23,3,3
        .byte 1,13,255,0,19,21,22,5,0,0,0,23,3,2,255,0,19,21,22,6,0,0,23,23,3,1,0,5,0,53,0,7,0
        .byte 5,0,8,0,7,0,5,0,5,0,8,0,5,0,5
        .byte 255,44,16,0,0,0,255,44,10,54,54,255,0,9,46,255,0,14,44,44,54,54,45,255,0,20,46,0,0,44,44,255,0,15
        .byte 59,57,58,255,0,6,45,0,44,44,255,0,18,46,255,0,7,44,44,255,0,20,59,58,255,0,4,44,44,255,0,6,59
        .byte 57,57,58,255,0,16,44,44,38,38,255,0,24,44,44,38,38,255,0,21,59,57,58,44,44,255,0,6,45,255,0,6,59
        .byte 58,255,0,11,44,44,255,0,26,44,44,255,0,17,45,255,0,5,59,58,0,44,44,255,0,8,59,57,58,255,0,15,44,44
        .byte 46,255,0,25,44,44,255,0,21,59,58,0,0,0,44,44,255,0,12,59,57,57,58,255,0,10,44,44,255,0,5,59,57
        .byte 58,0,46,255,0,14,45,0,44,44,255,0,26,44,59,58,21,22,255,0,8,53,255,0,14,44,44,59,21,22,57,58,255,0,21
        .byte 44
        .byte 255,44,15,255,0,12,44,44,255,0,6,46,255,0,16,54,54,54,44,44,0,0,40,39,255,0,11,45,255,0,7,54,54,54
        .byte 44,44,0,0,38,41,255,0,22,44,44,59,57,57,57,58,255,0,21,44,44,255,0,24,46,0,44,44,255,0,6,45,0,0,0
        .byte 59,58,255,0,7,45,255,0,6,44,44,255,0,26,44,44,255,0,20,19,19,255,0,4,44,44,0,0,45,255,0,10,59
        .byte 58,255,0,4,59,57,57,58,0,0,0,44,44,255,0,26,44,44,255,0,26,44,44,255,0,5,46,255,0,4,59,58,255,0,10
        .byte 45,0,0,0,44,44,255,0,26,44,44,0,0,0,59,57,57,58,255,0,19,44,44,255,0,26,44,44,0,0,46,255,0,9
        .byte 59,57,57,58,255,0,4,45,255,0,5,44,44,255,0,26,44,44,255,0,8,53,0,0,53,255,0,9,53,0,0,53,0
        .byte 44,59,57,57,58,59,57,58,255,0,9,59,57,58,255,0,8,44
        .byte 255,44,11,23,3,2,68,62,2,3,23,255,44,10,54,54,41,41,44,255,41,6,64,63,62,42,255,41,9,54,54,44,44
        .byte 54,54,255,41,8,64,63,62,42,255,41,10,54,54,44,44,44,255,41,8,64,63,1,3,23,255,44,6,41,41,41,255,44,5
        .byte 255,41,8,64,3,1,44,0,0,44,255,41,8,44,0,0,44,44,255,41,4,44,44,44,23,23,44,255,0,4,44,255,41,8
        .byte 44,0,0,44,44,255,41,4,44,255,0,9,44,41,41,41,255,44,6,0,0,44,44,41,41,41,44,44,255,0,9,44,41,41,41
        .byte 44,255,0,7,44,44,255,41,4,44,0,255,44,9,41,41,41,44,23,44,23,23,3,1,1,2,44,255,41,4,44,0,66
        .byte 41,38,38,255,41,12,64,63,62,62,62,44,44,41,41,41,44,0,66,38,38,38,31,38,255,41,9,64,63,255,62,4,44
        .byte 255,41,4,44,0,44,66,47,47,47,65,41,41,41,255,44,4,23,44,23,23,3,1,1,2,44,255,41,4,44,0,0,44
        .byte 67,67,67,44,44,41,41,44,255,0,11,44,41,41,41,44,44,0,255,44,6,41,41,41,255,44,10,0,0,44,255,41,4
        .byte 44,0,44,255,41,15,54,54,44,0,0,44,255,41,4,44,0,44,255,41,15,54,54,44,0,0,44,66,38,38,38,44,44,44
        .byte 38,38,38,65,255,44,6,255,41,5,44,44,44,0,0,66,31,31,38,38,255,31,5,38,65,255,0,5,255,44,7,255,0,4
        .byte 66,255,47,9,65,44,255,0,16,44,255,67,9,255,0,18
        .byte 3,1,13,0,71,0,255,71,4,6,21,22,6,71,71,255,0,12,23,3,1,13,255,9,6,18,21,22,18,9,10,255,0,13
        .byte 23,3,1,0,6,0,0,6,0,7,21,22,5,0,11,71,71,46,71,71,71,255,0,8,3,2,255,0,4,70,0,0,21
        .byte 22,0,0,12,255,9,6,10,255,0,7,1,1,0,0,0,45,0,0,0,21,22,7,255,0,5,44,44,0,11,255,0,6
        .byte 23,1,14,255,0,7,21,22,255,0,9,11,255,0,5,23,3,2,0,45,255,0,6,21,22,5,0,8,0,8,0,0,0
        .byte 5,11,8,0,5,0,26,3,3,1,255,0,8,21,22,0,45,255,0,7,11,255,0,4,53,2,1,14,255,0,8,21,22
        .byte 255,0,8,46,11,255,0,27,11,255,0,10,17,255,0,16,11,255,0,4,53,255,13,4,5,16,8,0,5,0,7,21,22
        .byte 7,0,8,0,7,0,8,0,8,11,7,0,5,0,26,1,2,1,1,13,16,255,0,4,5,21,22,255,0,9,12,255,9,5
        .byte 3,3,3,2,1,16,76,0,0,0,7,21,22,5,255,0,14,23,0,23,3,1,13,255,0,5,21,22,255,0,6,46,0,0,0
        .byte 45,255,0,7,23,3,1,255,0,4,7,21,22,8,255,0,15,23,3,2,1,14,255,0,4,5,21,22,255,0,4,45,255,0,10
        .byte 23,3,1,14,0,0,0,46,0,0,0,21,22,5,255,0,4,45,34,35,255,0,7,3,2,14,0,0,0,45,0,0,0
        .byte 8,21,22,255,0,5,24,77,78,25,255,0,6,1,14,255,0,8,5,21,22,5,255,0,5,77,78,255,0,7
        .byte 255,0,4,45,255,0,37,46,255,0,7,255,6,5,255,0,21,46,0,6,70,70,19,6,255,0,7,46,0,0,0,71,0,0,0
        .byte 71,0,0,0,71,0,71,71,6,19,19,0,6,255,0,11,6,0,6,0,6,0,6,0,6,0,6,24,25,19,19,19,6
        .byte 255,0,11,70,255,0,11,69,38,38,19,6,255,0,11,6,0,0,71,71,0,71,0,19,19,71,0,69,38,38,71,6,0,0,0
        .byte 46,255,0,8,21,22,6,6,0,6,0,6,6,6,24,25,255,6,4,255,0,11,6,21,22,6,0,0,70,0,0,0,70
        .byte 11,11,0,0,70,255,0,9,45,0,0,0,21,22,255,0,7,45,12,11,18,9,10,255,0,8,71,0,71,0,6,21,22
        .byte 6,0,71,255,0,6,12,255,9,5,255,0,6,255,6,5,21,22,6,6,6,255,0,6,46,0,0,12,9,9,255,0,6
        .byte 6,0,0,70,255,0,4,70,6,255,0,13,45,255,0,4,6,255,71,4,0,0,71,71,6,255,0,18,255,6,5,21,22
        .byte 6,6,6,255,0,19,70,0,0,6,21,22,6,0,70,255,0,10,45,255,0,5,46,255,0,6,21,22,70,0,0,46,255,0,12
        .byte 54,54,255,0,7,6,21,22,6,255,0,6,46,255,0,8,54,54,255,0,8,21,22,255,0,25,6,21,22,6,255,0,14
        .byte 255,0,23,11,255,0,9,46,255,0,17,11,255,0,21,45,255,0,5,11,255,0,9,44,0,44,0,44,0,44,0,44,0
        .byte 44,0,44,0,44,0,0,0,11,255,0,27,11,255,0,27,11,255,0,4,5,0,7,0,5,255,0,15,7,0,5,11,5
        .byte 0,5,255,0,24,12,10,255,0,27,11,255,0,27,11,255,0,27,12,10,0,0,5,0,59,57,58,0,5,0,5,0,5
        .byte 0,8,0,6,0,7,0,5,0,5,0,0,0,7,11,5,0,18,255,9,12,10,255,0,11,12,9,9,7,0,59,58,255,0,9
        .byte 12,255,9,9,18,5,18,9,9,255,0,4,45,255,0,18,6,45,0,0,0,5,0,59,58,255,0,20,5,255,0,13,45
        .byte 255,0,17,8,0,5,0,7,0,5,0,5,0,8,0,5,0,7,0,5,0,7,0,5,0,8,0,7,255,0,17,46,255,0,41
        .byte 255,0,25,11,255,0,4,45,255,0,19,46,0,0,11,255,0,27,11,255,0,27,11,255,0,8,44,255,0,8,45,255,0,4
        .byte 44,255,0,4,11,255,0,14,34,35,255,0,11,11,0,0,5,0,7,0,5,0,0,0,5,0,5,8,24,25,5,0,5
        .byte 0,5,0,0,0,5,0,5,11,5,255,0,5,53,0,0,0,53,0,0,5,24,25,8,0,0,0,53,0,0,0,53,0,0
        .byte 11,255,0,13,26,24,25,26,255,0,10,11,255,0,14,29,29,255,0,11,11,255,0,10,19,19,0,0,29,29,255,0,11
        .byte 11,0,0,5,0,5,0,7,0,5,0,5,7,5,26,24,25,26,0,7,0,5,0,5,0,8,0,7,11,5,0,255,9,12
        .byte 18,11,255,0,11,12,9,9,9,10,255,0,11,12,255,9,14,0,11,255,0,13,45,255,0,13,11,0,0,45,255,0,24
        .byte 11,255,0,11,46,255,0,15,11,255,0,22,45,255,0,4,11,255,0,27,11,255,0,26
        .byte 255,0,21,24,25,255,0,14,45,255,0,11,77,78,255,0,15,46,0,0,0,44,255,0,6,24,25,255,0,7,45,255,0,18
        .byte 39,38,0,0,0,45,255,0,22,39,38,255,0,24,7,7,24,25,7,7,0,0,0,48,0,48,0,48,0,68,0,48,0
        .byte 68,0,68,0,0,8,255,7,4,42,77,78,0,7,255,0,24,43,42,0,26,255,0,24,38,0,0,4,255,0,27,27,255,0,24
        .byte 34,35,0,7,0,0,0,48,0,48,0,48,0,68,0,48,0,68,0,68,0,0,8,255,7,5,24,25,7,42,0,0,0
        .byte 255,9,12,10,0,0,0,12,9,9,9,18,11,11,255,0,5,9,9,10,255,0,9,11,255,0,8,11,11,0,0,0,45
        .byte 0,0,0,11,255,0,14,46,0,0,0,12,11,10,0,45,255,0,4,11,255,0,11,44,255,0,7,11,11,255,0,6,11
        .byte 255,0,19,11,11,255,0,20,45,255,0,5,11,11,255,0,10,46,255,0,15,11,12,9,9,10,255,0,23,11,0,0,0
        .byte 11,0
        .byte 255,0,5,46,255,0,6,45,0,45,255,0,5,39,255,0,27,39,0,0,46,255,0,24,11,255,0,6,45,57,58,255,0,15
        .byte 46,0,0,11,0,71,71,71,34,35,71,0,0,0,59,57,57,58,0,45,0,0,55,75,73,255,0,6,11,71,1,1,1
        .byte 24,25,1,255,0,12,17,0,0,0,46,0,0,0,11,1,14,0,0,77,78,255,0,4,46,255,0,7,55,16,0,0,59
        .byte 58,0,0,71,1,14,23,23,0,77,78,0,72,73,255,0,5,45,55,72,73,0,16,255,0,5,71,1,12,9,9,10,0
        .byte 24,25,23,17,255,0,8,17,0,74,16,73,255,0,4,1,14,0,23,0,11,0,15,14,0,16,73,255,0,6,74,16,0,0
        .byte 16,0,45,75,0,64,1,0,23,0,0,11,255,0,4,16,255,0,6,61,0,16,73,0,16,76,0,61,0,15,1,255,0,4
        .byte 11,255,0,4,16,61,0,34,35,0,0,61,13,16,0,0,16,0,0,17,0,0,69,255,0,4,11,255,0,4,16,61,37
        .byte 32,33,36,13,13,1,13,17,61,16,0,0,16,61,0,69,255,0,4,11,0,0,21,22,13,13,24,25,24,25,1,1,3
        .byte 1,255,13,16,21,22,1,1,1,2,1,1,3,3,23,3,1,1,2,1,1,1,2,2,255,1,6,2,1,21,22,3,3
        .byte 1,1,3,255,54,4,255,3,14,11,3,1,21,22,23,23,3,3,44,255,54,4,44,23,23,23,0,0,23,0,23,23,0
        .byte 23,0,23,11,3,2,21,22,0,0,23,23,255,44,6,255,0,4,23,255,0,8,11,3,2,21,22,255,0,6,23,255,0,14
        .byte 23,0,11,3,2,21,22,255,0,23,11,3,1,21,22
numsc:
        .byte 28
nmedat:
        .byte 0,0,112,72,4,3,72,176,6,4,40,208,2,5,64,192,255
        .byte 6,4,72,152,0,0,16,176,5,5,120,144,255
        .byte 6,4,32,136,4,6,120,144,0,0,56,80,1,7,128,112,1,7,128,160,255
        .byte 0,0,136,88,4,3,72,152,4,6,96,112,255
        .byte 0,0,64,24,6,4,80,152,2,5,40,88,255
        .byte 0,0,32,168,2,5,32,136,5,5,120,72,1,10,104,96,255
        .byte 0,0,80,32,6,6,104,168,4,3,8,192,4,6,72,144,1,7,128,64,1,7,128,176,255
        .byte 0,0,32,32,4,3,88,56,4,3,24,184,255
        .byte 0,0,72,32,6,4,32,104,4,6,16,128,255
        .byte 0,0,8,80,2,5,32,56,1,7,128,80,1,7,128,192,6,4,56,136,255
        .byte 0,0,120,64,2,5,16,112,1,9,80,104,1,9,48,112,255
        .byte 0,0,120,216,4,3,24,128,2,5,16,192,1,10,72,160,1,10,88,160,1,10,104,160,1,10,120,160,255
        .byte 0,0,120,216,1,7,120,104,1,7,120,128,4,6,16,120,255
        .byte 0,0,120,216,4,3,48,112,2,5,120,96,255
        .byte 0,0,120,176,2,5,120,128,255
        .byte 0,0,8,112,3,3,32,88,3,3,32,136,2,5,112,88,255
        .byte 0,0,16,112,6,3,40,48,1,7,112,120,2,5,128,80,6,8,72,80,6,8,72,160,255
        .byte 0,0,128,216,2,5,136,144,6,4,88,112,4,3,8,64,4,6,40,112,255
        .byte 0,0,16,216,1,7,32,88,1,7,32,152,6,6,8,80,6,8,64,120,6,4,104,168,255
        .byte 0,0,80,96,6,4,56,184,6,6,104,160,255
        .byte 0,0,136,144,2,5,56,184,6,3,80,128,255
        .byte 0,0,72,208,1,7,72,88,1,7,128,40,255
        .byte 0,0,72,112,4,3,72,160,255
        .byte 0,0,40,104,6,3,96,104,2,5,40,160,255
        .byte 0,0,72,32,4,8,40,56,4,8,56,72,4,8,72,88,4,8,40,128,4,8,56,144,4,8,72,160,255
        .byte 0,0,72,32,5,5,72,80,4,8,40,56,4,8,40,168,255
        .byte 0,0,72,32,4,8,24,120,4,8,104,120,1,10,24,184,255
        .byte 0,0,120,64,1,9,0,168,1,1,88,176,255
NUMOBJ = 20
objdta:
        .byte 63,192,64,32,95,208,95,232,95,244,81,244,84,20,81,148,95,244,95,244,64,4,127,252,42,168,127,252,127,252,0,0,0,128,208,0,128,208
        .byte 1,128,2,192,7,96,14,208,31,248,30,200,31,72,30,200,39,252,79,254,79,234,37,84,0,0,79,254,155,109,159,255,1,56,56,1,56,56
        .byte 1,128,2,192,7,96,14,208,31,248,30,200,31,72,30,200,39,252,79,254,79,234,37,84,0,0,79,254,155,109,159,255,254,0,16,254,0,16
        .byte 1,128,2,192,7,96,14,208,31,248,30,200,31,72,30,200,39,252,79,254,79,234,37,84,0,0,79,254,155,109,159,255,7,32,168,7,32,168
        .byte 63,192,64,32,95,208,95,232,95,244,81,244,84,20,81,148,95,244,95,244,64,4,127,252,42,168,127,252,127,252,0,0,23,40,200,23,40,200
        .byte 63,192,64,32,95,208,95,232,95,244,81,244,84,20,81,148,95,244,95,244,64,4,127,252,42,168,127,252,127,252,0,0,15,80,192,15,80,192
        .byte 0,4,0,12,0,0,1,254,0,170,0,0,64,12,96,12,96,12,96,12,0,0,255,254,170,170,0,0,96,12,96,12,9,8,104,9,8,104
        .byte 15,0,17,128,0,0,2,192,2,192,7,224,27,248,39,252,88,126,121,62,120,122,121,54,121,42,56,84,30,184,7,224,6,104,176,6,104,176
        .byte 1,0,3,128,7,64,11,160,23,80,47,168,127,84,255,250,85,84,43,168,21,80,11,160,5,64,2,128,1,0,0,0,20,16,40,20,16,40
        .byte 7,192,27,176,61,120,102,204,99,140,97,12,83,148,46,232,20,80,77,100,111,236,53,88,112,28,5,64,123,188,252,126,13,24,208,13,24,208
        .byte 7,0,52,0,119,4,244,8,174,12,232,26,238,112,245,192,214,133,187,85,221,223,174,112,87,142,74,250,53,84,15,248,14,64,96,14,64,96
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,28,0,46,0,126,0,93,0,235,0,209,0,170,0,124,0,7,32,88,7,32,88
        .byte 0,0,0,0,3,192,7,224,1,128,63,252,76,50,93,186,113,142,119,238,119,238,113,142,93,186,76,50,63,252,0,0,17,48,24,17,48,24
        .byte 0,0,0,0,3,192,7,224,1,128,63,252,76,50,93,186,113,142,119,238,119,238,113,142,93,186,76,50,63,252,0,0,19,56,24,19,56,24
        .byte 0,0,1,128,7,224,9,112,19,248,19,152,63,188,57,244,121,190,79,254,94,226,127,194,95,194,43,196,149,91,98,166,254,0,16,254,0,16
        .byte 7,192,9,224,25,240,23,240,31,240,31,176,15,32,7,192,1,0,1,0,0,128,0,128,0,64,2,32,5,32,0,192,10,40,136,10,40,136
        .byte 0,0,120,0,30,0,7,0,3,160,1,80,0,224,1,80,3,184,7,28,14,12,20,6,40,6,80,2,160,2,64,0,21,8,216,21,8,216
        .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,254,254,234,234,212,212,170,170,212,212,170,170,212,212,14,16,24,14,16,24
        .byte 0,0,0,0,0,0,0,8,55,228,85,122,79,245,86,122,53,164,3,200,4,32,13,176,16,8,53,172,49,140,31,248,6,24,64,6,24,64
        .byte 120,0,132,0,165,80,133,88,132,0,120,26,128,2,192,127,199,201,207,201,204,127,205,89,204,111,237,217,109,239,12,127,26,64,184,26,64,184
font:
        .byte 0,0,0,0,0,0,0,0
        .byte 0,48,120,120,48,48,0,48
        .byte 0,24,6,6,9,17,32,64
        .byte 24,52,60,94,126,114,114,60
        .byte 0,94,126,126,42,82,4,120
        .byte 2,2,14,66,126,66,126,66
        .byte 126,189,153,90,36,60,0,24
        .byte 124,185,246,232,213,239,189,126
        .byte 0,12,24,24,24,24,12,0
        .byte 0,96,48,48,48,48,96,0
        .byte 24,52,114,255,149,90,52,24
        .byte 28,62,127,127,87,71,71,62
        .byte 0,0,0,0,0,24,24,48
        .byte 0,0,0,0,126,0,0,0
        .byte 0,0,0,0,0,0,16,0
        .byte 56,124,254,254,234,226,226,124
        .byte 124,254,206,214,230,254,124,0
        .byte 24,120,120,24,24,126,126,0
        .byte 252,254,6,124,192,254,254,0
        .byte 252,254,6,62,6,254,252,0
        .byte 216,216,216,216,254,254,24,0
        .byte 254,254,192,252,6,254,252,0
        .byte 124,252,192,252,198,254,124,0
        .byte 254,254,6,14,28,24,24,0
        .byte 124,254,198,124,198,254,124,0
        .byte 124,254,198,254,126,6,6,0
        .byte 0,0,0,48,0,0,48,0
        .byte 0,0,0,0,0,0,0,0
        .byte 120,124,126,126,126,66,66,126
        .byte 0,0,0,0,0,0,0,0
        .byte 0,24,52,36,126,171,126,36
        .byte 60,14,7,7,7,7,14,60
        .byte 0,78,86,78,86,78,86,74
        .byte 60,126,6,126,126,102,102,0
        .byte 120,126,6,124,102,126,124,0
        .byte 62,126,0,96,96,126,62,0
        .byte 124,126,6,102,102,126,124,0
        .byte 62,126,0,124,96,126,62,0
        .byte 62,126,0,124,96,96,96,0
        .byte 62,126,0,110,102,126,62,0
        .byte 102,102,6,126,102,102,102,0
        .byte 126,126,0,24,24,126,126,0
        .byte 6,6,0,6,6,126,124,0
        .byte 102,110,28,120,124,110,102,0
        .byte 96,96,0,96,96,126,62,0
        .byte 99,119,31,107,99,99,99,0
        .byte 124,126,6,102,102,102,102,0
        .byte 60,126,6,102,102,126,60,0
        .byte 124,126,6,126,124,96,96,0
        .byte 60,126,6,102,102,112,54,6
        .byte 124,126,6,126,124,110,102,0
        .byte 62,126,0,60,14,126,124,0
        .byte 126,126,0,24,24,24,24,0
        .byte 102,102,6,102,102,126,62,0
        .byte 102,102,6,102,126,60,24,0
        .byte 99,99,3,107,127,119,99,0
        .byte 99,119,46,28,62,119,99,0
        .byte 102,102,6,126,6,62,60,0
        .byte 126,126,0,12,24,62,126,0
        .byte 30,12,12,12,12,12,12,30
        .byte 0,0,192,96,48,24,12,0
        .byte 0,240,48,48,48,48,240,0
        .byte 60,78,94,126,122,60,8,24
        .byte 0,0,127,127,42,85,0,127
        .byte 0,86,75,85,74,69,32,31
        .byte 0,0,60,6,62,102,62,0
        .byte 0,96,96,124,102,102,124,0
        .byte 60,66,153,161,161,153,66,60
        .byte 0,6,6,62,102,102,62,0
        .byte 0,0,60,102,124,96,60,0
        .byte 0,28,48,56,48,48,48,0
        .byte 0,0,56,108,108,60,12,120
        .byte 0,96,96,124,102,102,102,0
        .byte 0,24,0,56,24,24,60,0
        .byte 0,6,0,6,6,6,102,60
        .byte 0,96,108,120,120,108,102,0
        .byte 0,48,48,48,48,48,28,0
        .byte 0,0,54,127,127,107,99,0
        .byte 0,0,124,102,102,102,102,0
        .byte 0,0,60,102,102,102,60,0
        .byte 0,0,124,102,102,124,96,96
        .byte 0,0,62,102,102,62,6,7
        .byte 0,0,30,51,48,48,48,0
        .byte 0,0,62,96,60,6,124,0
        .byte 0,48,124,48,48,48,28,0
        .byte 212,224,224,224,224,224,224,192
        .byte 127,254,213,224,224,224,252,248
        .byte 0,0,31,39,74,85,72,87
        .byte 126,129,177,177,129,133,129,126
        .byte 0,0,120,124,46,86,14,86
        .byte 0,40,40,92,92,92,0,40
        .byte 0,30,24,112,24,24,30,0
        .byte 0,24,24,24,24,24,24,0
        .byte 0,240,48,28,48,48,240,0
        .byte 0,60,120,0,0,0,0,0
        .byte 60,66,153,161,161,153,66,60
jtab:
        .byte 249,250,251,254,254,255,255,255,0,0,0,1,1,1,2,2,6,7,6,15,16,99
keys:   .byte 10,8,63,47,33,23,13,31,30,26,24