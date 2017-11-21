// makefastboot.cpp : Defines the entry point for the console application.
// write a fast jaguar boot sector to the beginning of a game 
// note: OVERWRITES the header, make sure there is one!
// The new header only loads two encrypted blocks and does
// a quick checksum. It will boot anything like TYPEAB,
// but in 1.5s instead of 5s. Code by Tursi harmlesslion.com
// with assistance from KSkunk. Inserts new checksum info at $410.

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>

unsigned char bigbuf[6*1024*8192];

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *fp;
	char buf[128];

	unsigned char fastboot[0x84] = {
#if 1
	// newer version that checksums the cart - requires added header info at 0x410:
	// first address to checksum, number of 32-bit words to checksum, and checksum + 0x03d0dead.
	// Both addresses must be multiples of 4. Should work on GPU and DSP.
	 0xFE,0x82,0xD9,0x87,0x55,0xE1,0xDE,0x2A,0x08,0xA6,0x64,0xB9,0x2D,0x9D,0x29,0xEF,	// 00000000 ....U..*..d.-.).
	 0xC9,0x3A,0xE8,0xEC,0xAE,0xAE,0x73,0x0F,0x75,0x8E,0x96,0xD3,0x5C,0x90,0x82,0x0C,	// 00000010 .:....s.u...\...
	 0xE4,0x92,0x99,0x19,0xCA,0xDD,0xD5,0xE0,0x22,0xD3,0x81,0x00,0x4C,0xEB,0xC4,0xA9,	// 00000020 ........"...L...
	 0x25,0xCD,0xFC,0x21,0xDB,0x91,0xC3,0x94,0x14,0x8D,0x61,0xB1,0xCC,0x5D,0x49,0x85,	// 00000030 %..!......a..]I.
	 0x70,0x12,0x10,0xBC,0x9A,0x30,0x53,0x00,0x56,0xAC,0x80,0x86,0xB1,0x1C,0xD3,0x3F,	// 00000040 p....0S.V......?
	 0x29,0x20,0xD1,0x66,0xCA,0x53,0xEA,0x9A,0x8A,0xD8,0xEC,0xEB,0x82,0xBC,0x34,0x89,	// 00000050 ) .f.S........4.
	 0xF4,0xC1,0x9C,0x77,0x92,0x95,0x43,0x19,0x7F,0x76,0x52,0x46,0x39,0x2E,0x74,0xAC,	// 00000060 ...w..C..vRF9.t.
	 0xBA,0xC7,0x3C,0xC4,0x5A,0x29,0x6D,0xE6,0x2F,0x59,0xCB,0x4D,0xFE,0xA7,0x49,0xA3,	// 00000070 ..<.Z)m./Y.M..I.
	 0x53,0x0C,0x2E,0x1C                                                            	// 00000080 S...
	 // See bootcode.s for the source
#elif 0
	// newer version that works with JagCD (DSP) and normal (GPU)
0xFE, 0x58, 0x00, 0x94, 0xE7, 0x8D, 0xBA, 0xB9, 0x54, 0x15, 0x5D, 0x1C, 0x0A, 0x3B, 0x4B, 0x73, 
0xDD, 0xB5, 0x91, 0xD0, 0x3E, 0xF9, 0x0A, 0xC9, 0x63, 0x60, 0xCE, 0x46, 0x0C, 0x26, 0xC0, 0xBA, 
0x6C, 0x88, 0xD9, 0xED, 0xF1, 0xD2, 0xE6, 0x61, 0xF9, 0xF4, 0xC6, 0xDB, 0xAA, 0xB7, 0x2E, 0x69, 
0xA6, 0x28, 0xE6, 0xA3, 0x48, 0x3F, 0xEC, 0x10, 0x31, 0xF5, 0xC3, 0xD2, 0xE1, 0xC1, 0xB7, 0xA1, 
0x9A, 0x1D, 0xD8, 0x8E, 0xB3, 0xE4, 0x22, 0xFE, 0xBF, 0xC4, 0xF5, 0xCD, 0x0D, 0xB4, 0x03, 0x1A, 
0x9B, 0x7E, 0xDC, 0x8C, 0x5B, 0x46, 0x2F, 0x3A, 0xFF, 0x0D, 0x1A, 0x97, 0xCB, 0xAC, 0x17, 0xFC, 
0xD7, 0xDC, 0x68, 0xBD, 0x11, 0x52, 0x3C, 0x3F, 0x21, 0x22, 0x8F, 0xC3, 0xD6, 0x71, 0x2B, 0x81, 
0xB7, 0x43, 0x8F, 0xA1, 0x8F, 0xEF, 0xB0, 0x27, 0xB0, 0x93, 0x0A, 0x23, 0x0A, 0x4A, 0xBE, 0xA1, 
0xC8, 0x84, 0x0F, 0x09 

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
  */

#else
	// older version that's just normal GPU boot
	0xFF,0xAB,0xAC,0x0D,0x51,0xC6,0x08,0x86,0xA0,0x17,0xE9,0x7A,0x2C,0x7E,0x97,0x26,
	0xA5,0x9B,0xB7,0x82,0x8D,0x14,0x37,0x5B,0x6E,0x24,0xE5,0x69,0x8E,0x86,0x71,0xE2,
	0xA9,0xDD,0x90,0x05,0xDB,0xF9,0xB8,0x82,0x64,0x8E,0xCF,0x40,0xB0,0xA2,0x1A,0xAF,
	0x98,0x12,0x09,0x9E,0xDE,0x80,0x8A,0xEA,0x24,0x82,0x80,0x0D,0xF3,0xC7,0xF8,0xC1,
	0x1B,0x03

	// decrypts to:
  	// 00F035AC: MOVEI   $00F03566,R00    (9800)	; address to pass ROM check
	// 00F035B2: MOVEI   $03D0DEAD,R04    (9804)	; magic value to unlock 68k
	// 00F035B8: JUMP    (R00)            (D000)	; go do it!
	// 00F035BA: NOP                      (E400)	; delay slot
	// plus some junk bytes ;)

#endif
	};
	
	if (argc < 2) {
		printf("Pass the name of a Jaguar game with header to patch\n");
		printf("A fastboot header will be applied to it.\n");
		return 0;
	}

	fp=fopen(argv[1], "rb+");
	if (NULL == fp) {
		printf("Can't open file for modify: %s\n", argv[1]);
		return -1;
	}
	int sz = fread(bigbuf, 1, sizeof(bigbuf), fp);
	fclose(fp);

	// make a backup - Win specific code here
	char szBackup[MAX_PATH];
	memset(szBackup, 0, MAX_PATH);
	strncpy(szBackup, argv[1], MAX_PATH-5);
	strcat(szBackup, ".BAK");
	CopyFile(argv[1], szBackup, FALSE);

	if ((bigbuf[0] != 0xf6) && (bigbuf[0] != 0xff) && (bigbuf[0] != 0xfe)) {
		printf("File does not seem to have a header on it - add new header? (Don't do this if not a ROM)\n");
		gets(buf);
		if (tolower(buf[0]) != 'y') return -1;

		if (sz>=4*1024*1024-8192) sz=4*1024*1024-8192;
		memmove(&bigbuf[0x2000], bigbuf, sz);

		// write 0x2000 ff bytes
		memset(bigbuf, 0xff, 0x2000);

		// write boot information
		memset(&bigbuf[0x0400], 0x04, 4);	// cart width
		bigbuf[0x0404]=0x00;				// boot address (assume 0x802000)
		bigbuf[0x0405]=0x80;
		bigbuf[0x0406]=0x20;
		bigbuf[0x0407]=0x00;
		memset(&bigbuf[0x0408], 0, 4);		// boot flags - must be zeroed...
	}

	// now do the patch
	memcpy(bigbuf, fastboot, 132);

	printf("Set cartridge width:\n  1: 8-bit\n  2:16-bit\n  3:32-bit (Default)\n> ");
	gets(buf);
	int width = 0;
	switch (buf[0]) {
		case '1':	width = 0; break;
		case '2':	width = 2; break;
		default:	width = 4; break;
	}
	printf("\n");
	
        // double check these times... I'm assuming 25MHz system clock
	printf("Set cartridge speed:\n  1:10 clocks - 400nS (Default)\n  2: 8 clocks - 320 nS\n  3: 6 clocks - 240nS\n  4: 5 clocks - 200nS\n> ");
	gets(buf);
	switch(buf[0]) {
		default:	width |= 0; break;
		case '2':	width |= 8; break;
		case '3':	width |= 16; break;
		case '4':	width |= 24; break;
	}
	memset(&bigbuf[0x0400], width, 4);

	printf("\nGenerating checksum... ");
	unsigned int checksum = 0x03d0dead;	// jag's magic number
	unsigned int start = 0x802000;
	unsigned int end = (sz+0x800000) & 0xfffffc;
	unsigned int cnt = (end-start)/4;
	for (int idx=0; idx<cnt; idx++) {
		// big endian data, so do it all by hand
		int pos = idx*4+start-0x800000;
		checksum+=bigbuf[pos]<<24;
		checksum+=bigbuf[pos+1]<<16;
		checksum+=bigbuf[pos+2]<<8;
		checksum+=bigbuf[pos+3];
	}
	printf("0x%08X\n", checksum);
	bigbuf[0x410]=(start>>24)&0xff;
	bigbuf[0x411]=(start>>16)&0xff;
	bigbuf[0x412]=(start>>8)&0xff;
	bigbuf[0x413]=(start)&0xff;
	bigbuf[0x414]=(cnt>>24)&0xff;
	bigbuf[0x415]=(cnt>>16)&0xff;
	bigbuf[0x416]=(cnt>>8)&0xff;
	bigbuf[0x417]=(cnt)&0xff;
	bigbuf[0x418]=(checksum>>24)&0xff;
	bigbuf[0x419]=(checksum>>16)&0xff;
	bigbuf[0x41a]=(checksum>>8)&0xff;
	bigbuf[0x41b]=(checksum)&0xff;

	fp=fopen(argv[1], "wb");
	if (NULL == fp) {
		printf("Can't reopen file for writing.\n");
		return -1;
	}
	fwrite(bigbuf, 1, sz, fp);
	fclose(fp);

	printf("JagCD compatible - Successfully patched %s\n", argv[1]);

	// todo: I only know for sure for 16-bit output, so this
	// split question doesn't come up for the others right now
	if ((width&0x06)==2) {
		printf("Output swapped version for 16-bit EPROM? (Y/N) > ");
		gets(buf);
		if ((buf[0]=='y')||(buf[0]=='Y')) {
			char fn[512];
			strcpy(fn, argv[1]);
			char *p=strrchr(fn, '.');
			if (p) *p='\0';
			strcat(fn,"_swapped.bin");

			for (int idx=0; idx<sz; idx+=2) {
				int c=bigbuf[idx];
				bigbuf[idx]=bigbuf[idx+1];
				bigbuf[idx+1]=c;
			}

			fp=fopen(fn, "wb");
			if (NULL == fp) {
				printf("Can't open file for writing.\n");
				return -1;
			}
			fwrite(bigbuf, 1, sz, fp);
			fclose(fp);
		}
	}

	return 0;
}

