; Jaguar cart encrypted boot.
; Want to try and keep this under 130 bytes total, so it fits in two blocks
; We need to deal with two entry points, one for Console and one for CD.
; This uses my custom version of the encryption tool that doesn't overwrite
; huge blocks, so the only patches we need to watch out for are the CD's.
; this frees up a lot of space for code.
; However, the CD overwrites a lot of data, particularly the MD5 hash,
; meaning you can't use that space after all if you want to work with the CD.
; This code works, but I don't necessarily have ALL the black-out areas marked.
; (Note, too, the CD unit runs this code on the DSP, not the GPU.)

.gpu
 .org $00F035AC

; entry point for console

; before we unlock the system, do a quick checksum of the cart
; unlike the official code, we take the data for the checksome
; from unencrypted memory after the boot vector:
;
; 0x400 - boot cart width (original)
; 0x404 - boot cart address (original)
; 0x408 - flags (original)
; 0x410 - start address to sum
; 0x414 - number of 32-bit words to sum
; 0x418 - 32-bit checksum + 0x03D0DEAD (this is important! that's the wakeup value!)
;
; By adding the key to the checksum, and subtracting as we go through
; the cart, we can just write the result to GPU RAM and not spend any code
; on comparing it - the console will do that for us. That really helped this fit.

; Be careful.. SMAC does NOT warn when a JR is out of range. Only got 15 words!

; from here we have 32 bytes until the DSP program blocks out
ENTRY:
 JR gpumode							; skip over
 nop
 
SHARED:
 movei #$800410,r14			; location of data
 load (r14),r15					; get start
 load (r14+1),r2				; get count - offset is in longs
 load (r14+2),r8				; get desired checksum + key

chklp:
 load (r15),r4					; get long
 subq #1,r2							; count down
 addqt #4,r15						; next address
 jr NZ,chklp						; loop if not done on r2
 sub r4,r8							; (delay) subtract value from checksum, result should be 0x3D0DEAD at end
 jump (r6)							; back to caller
 nop
; 30/32

; have to break it up, cause the CD unit wipes the hash memory... GPU is okay though
gpumode:
 move pc,r6							; set up for return
 addq #14,r6
 movei #SHARED,r0
 jump (r0)
 nop
 
 MOVEI #$00FFF000,R1		; AND mask for address
 AND R1,R6							; mask out the relevant bits of PC
 
 MOVEQ #0,R3						; Clear R3 for code below
 
 MOVEI #$00000EEC,R2		; Offset to chip control register
 STORE R8,(R6)					; write the code (checksum result, hopefully 3d0dead)
 SUB R2,R6							; Get control register (G_CTRL or D_CTRL)

GAMEOVR:
 JR GAMEOVR 						; wait for it to take effect
 STORE R3,(R6)					; stop the GPU/DSP
; 68

;	.org $f035f4	<-- doesn't work, we'll just pad then
 dc.w $5475,$7273

; JagCD entry point 
; should be at f035f4 (72/$48)
; we should have 50 bytes here 
dspmode: 
 ; There is a CD relocation at $4A that we can't touch, the MOVEI covers it
 MOVEI #$12345678,R9				; this movei is hacked by the CD boot
 
 MOVE PC,R6									; prepare for subroutine
 ADDQ #14,R6
 MOVEI #SHARED+$180B8,R0		; prepare for long jump (DSP offset included)
 JUMP (R0)									; go do it
 NOP												; delay slot
 
 MOVEI #$00FFF000,R1				; AND mask for address
 AND R1,R6									; mask out the relevant bits of PC
 
 MOVEQ #0,R3								; Clear R3 for code below
 
 MOVEI #$00000EEC,R2				; Offset to chip control register
 STORE R8,(R6)							; write the code (checksum result, hopefully 3d0dead)
 SUB R2,R6									; Get control register (G_CTRL or D_CTRL)

DGAMEOVR:
 JR DGAMEOVR 								; wait for it to take effect
 STORE R3,(R6)							; stop the GPU/DSP
; 44/50
; 116 total

 ; there's lots of empty space here... Tursi wuz here.
 ; pad up to 130 bytes (even though the JagCD will hack some of them, see below)
 dc.w $5475,$7273,$6920,$7775,$7a20
 dc.w $6865,$7265
; 130
 
; MOVEI #$0,R9							; this movei is hacked by the CD boot ($f03624) (120/$78)
; MOVEI #$0,R10							; this movei is hacked by the CD boot ($f0362A) - this insn is split by the 130 byte mark
 
 END

