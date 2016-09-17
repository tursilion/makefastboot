This is the Atari Jaguar encryption tool for the Atari ST. It requires the private key to be inserted on a floppy disk, that exists on encrypt.st (as a tiny little file, included here as a text file.)

Below I discuss how I hacked it up, but then I found the PC code so this is mostly just history now. ;)

To use the tool, run it on an ST emulator. You need the disk image and the ROM to be encrypted on the hard drive. It will prompt you step by step. If all you want is the header you can rip it from the cart after encryption. (Note that with my third version of the code, it won't work unless you add the checksum bytes to the header.) Note that whatever ROM type you ask for, it writes four 1MB files and an 'xxx'. The xxx is the actual encrypted header.

The encrypted header is buried in the program and starts at offset 0x125C. Presumably it runs for 650 bytes, the size of the encrypted header (as I believe size remains the same). My own boot is only two blocks (130 bytes), the rest is garbage. The first byte indicates how many blocks to load, so for mine it can be changed from F6 to FE.

The very first pass didn't work with JagCD and shipped with the first Skunkboard:

----------------------------------------------

00F035AC: MOVEI   $00F03566,R00    (9800)	; address to pass ROM check
00F035B2: MOVEI   $03D0DEAD,R04    (9804)	; magic value to unlock 68k
00F035B8: JUMP    (R00)            (D000)	; go do it!
00F035BA: NOP                      (E400)	; delay slot
	
----------------------------------------------


The second worked with JagCD and looks like this:

----------------------------------------------

/* .gpu
 .org $00F035AC

 MOVEI #$00FFF000,R1		; AND mask for address
 MOVEI #$00000EEC,R2		; Offset to chip control register
 MOVEI #$03D0DEAD,R4		; magic value for proceeding

 MOVE PC,R0					; get the PC to determine DSP or GPU
 AND R1,R0					; Mask out the relevant bits
 STORE R4,(R0)				; write the code
 SUB R2,R0					; Get control register (G_CTRL or D_CTRL)
 MOVEQ #0,R3				; Clear R3 for code below

GAMEOVR:
 JR GAMEOVR 				; wait for it to take effect
 STORE R3,(R0)				; stop the GPU/DSP

; Need an offset of $48 - this data is overwritten by the encrypt tool
; with the MD5 sum.
 NOP
 NOP 
 MOVEI #$0,R0
 MOVEI #$0,R0
 MOVEI #$0,R0
 MOVEI #$0,R0
 MOVEI #$0,R0
 MOVEI #$0,R0

; JagCD entry point (same for now)

Main: 
 ; There is a relocation at $4A that we can't touch
 MOVEI #$0,R0				; dummy value

 ; real boot starts here 
 MOVEI #$00FFF000,R1		; AND mask for address

 MOVEI #$0,R0				; This movei is hacked by the encryption tool
 MOVEI #$0,R0				; This movei is hacked by the encryption tool

 MOVEI #$00000EEC,R2		; Offset to chip control register
 MOVEI #$03D0DEAD,R4		; magic value for proceeding

 MOVE PC,R0					; get the PC to determine DSP or GPU
 AND R1,R0					; Mask out the relevant bits
 STORE R4,(R0)				; write the code
 SUB R2,R0					; Get control register (G_CTRL or D_CTRL)
 MOVEQ #0,R3				; Clear R3 for code below

GAMEOVR2:
 JR GAMEOVR2				; wait for it to take effect
 STORE R3,(R0)				; stop the GPU/DSP
 
 END
// plus some junk bytes ;)

---------------------------------

The third is meant to work with my own 'signing' tool and so
checksum the cart, to ensure it's intact before booting. It looks
like so:

---------------------------------
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

----------------------------------
