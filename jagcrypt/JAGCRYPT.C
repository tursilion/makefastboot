/*
 * Jaguar Cartridge Encryption Software
 * Portable C version
 *
 * Copyright 1993-1995 Atari Corporation
 * All Rights Reserved
 *
 * Written by Eric R. Smith
 * Based on the 680x0 assembly language
 * version by Dave Staugas.
 */

// This version modified by Tursi to NOT inject MD5 sum into
// the binary or patch any MOVEIs, so that the boot code can
// truly do anything without restriction (from this tool. The JagCD
// still imposes a few restrictions.)
// Tursi also inserted the private key so it didn't need the disk file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rsa.h"
#include "proto.h"

/* possible output formats */
#define HILO 0
#define ROM4 1
#define SINGLE 2

#undef BUFSIZE
#define BUFSIZE 4096

/* Global variables */
/* private key binary image */
byte privateK[128] = {
  0x1f,0xd8,0xb4,0xfb,0xcf,0xb9,0x67,0x60,
  0x6c,0x9c,0x2f,0x1d,0x16,0xa0,0x13,0xca,
  0x83,0xda,0x63,0x2a,0xb0,0x7b,0xab,0xaa,
  0xe9,0x92,0x62,0xa3,0x57,0x56,0xd6,0xa9,
  0x9a,0x96,0x6b,0x14,0x75,0x39,0xa3,0x98,
  0xeb,0x5b,0xa7,0x42,0x1c,0x54,0xe0,0x1c,
  0xd7,0xee,0x2e,0xe7,0xa5,0xf9,0x7e,0xdf,
  0xce,0xe8,0xed,0xca,0xdc,0x3d,0x2f,0xe8,
  0xab
};

/* public key binary image */
byte publicK[128] = {
  0x2f,0xc5,0x0f,0x79,0xb7,0x96,0x1b,0x10,
  0xa2,0xea,0x46,0xab,0xa1,0xf0,0x1d,0xaf,
  0xc5,0xc7,0x94,0xc0,0x08,0xb9,0x81,0x80,
  0x5e,0x5b,0x93,0xf5,0x03,0x02,0x41,0xfe,
  0x75,0xb7,0x1c,0xe8,0xe7,0x22,0x79,0xa3,
  0xd5,0xbe,0x30,0x45,0xf9,0xea,0x35,0xd9,
  0x8a,0x0a,0x15,0x40,0xb4,0xb4,0xe8,0x4e,
  0xa6,0xdd,0x17,0xee,0x42,0x33,0x10,0x0d,
  0xf9
};

byte inbuf[0x2000];			/* input buffer */
byte romimg[0x1c02];		/* image for first part of ROM */

/* magic GPU boot code, compiled */
byte boot_orig[] = {
#include "md5.dat"
};

/* tursi's boot code loads here */
byte boot_tursi[8192+8];	// encryption code always skips the first 8 bytes, so whatever
int boot_tursi_size = 0;

/* pointer to abstract */
byte *boot1 = boot_orig;

long filesize=0;		/* length of file (+ 0x2000, since ROM starts at 0x802000 */
long romsize=0;			/* size of ROM needed to contain the file */
long MD5state[4];		/* MD5 hash for file+fill */
int TursiMode = 0; /* operates normally unless you set -tursi */

/*
 * flushinp: eat the rest of an input line, up to and including a carriage return
 */
void
flushinp(void)
{
	int c;

	do {
		c = getchar();
	} while (c != '\n');
}

/*
 * boot flags and vectors (updated if necessary)
 */
byte Romconfig[12] = {
	0x04, 0x04, 0x04, 0x04,
	0x00, 0x80, 0x20, 0x00,
	0x00, 0x00, 0x00, 0x00
};

void
usage( void )
{
	fprintf(stderr, "Usage: jagcrypt -[h|n|4] [-p] filename.ext\n");
	fprintf(stderr, "Output format is specified by the first flag:\n");
	fprintf(stderr, "   -h: Use HI/LO format, split in pieces if > 2 megs total\n");
	fprintf(stderr, "   -n: Use HI/LO format, don't split large files\n");
	fprintf(stderr, "   -4: Use 4xROM format, output in .U1,.U2,.U3,.U4\n");
	fprintf(stderr, "   -x: Use single output file, no splitting, big endian\n");
	fprintf(stderr, "Exactly one of the flags above must appear\n");
	fprintf(stderr, "Optional flags:\n");
	fprintf(stderr, "   -p: Use previous encryption data from a .XXX file\n");
	fprintf(stderr, "   -tursi <bin>: load and encrypt the compiled GPU BIN as a header, no patching. Only XXX exported.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Examples:\n");
	fprintf(stderr, "To encrypt the 4 meg file FOO.ROM into FOO.HI0, FOO.HI1,\n");
	fprintf(stderr, "FOO.LO0 and FOO.LO1, use:\n");
	fprintf(stderr, "   jagcrypt -h foo.rom\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "To quickly encrypt BAR.ABS into BAR.U1, BAR.U2, BAR.U3, and\n");
	fprintf(stderr, "BAR.U4 (re-using the RSA data from BAR.XXX), use:\n");
	fprintf(stderr, "   jagcrypt -4 -p bar.abs\n");
	exit(1);
}
	
int
main(int argc, char **argv)
{
	int useprevdata;	/* 1 if we should use an existing .XXX file for RSA data */
	int i;		/* scratch variables */
	FILE *fhandle;
	byte *nnum, *cnum;
	int outfmt;		/* output format */
	int nosplit;		/* split HI/LO parts of output (if 0) or not (if -1) */
	char *filename;		/* name of file to encrypt */
	long xxxsize;		/* size of .XXX file */
	long powof2;		/* smallest power of 2 that the ROM can fit into */
	int ateof;		/* flag: set to 1 at end of file */
	
	xxxsize = 1036;		/* default size of a .XXX file */

	printf("\n\n");
	printf("JAGUAR Cartridge (quik) Encryption Code\n");
	printf("      version 0.5c\n");
	printf("Authorized Users Only, Please\n\n\n");

	if (argc < 3) {
		usage();
	}

	/* figure out output format from first parameter */
	argv++;
	if (argv[0][0] != '-') {
		fprintf(stderr, "ERROR: you must specify an output format\n");
		usage();
	}
	switch(argv[0][1]) {
		case 'h':	/* HI/LO output format, split big files */
			outfmt = HILO;
			nosplit = 0;
			break;
		case 'n':	/* HI/LO output format, do not split big files */
			outfmt = HILO;
			nosplit = 1;
			break;
		case '4':	/* 4xROM output format */
			outfmt = ROM4;
			nosplit = 1;
			break;
		case 'x':	/* single file output format */
			outfmt = SINGLE;
			nosplit = 1;
			break;
		default:
			fprintf(stderr, "ERROR: unknown output format `%c'\n", argv[0][1]);
			usage();
			break;
	}
	argv++;
	if (!*argv) {
		usage();
	}
	/* check for '-p' flag for "Use previous encryption data */
	useprevdata = 0;
	while (argv[0][0] == '-') {
		if (argv[0][1] == 'p') {
			useprevdata = 1;
		} else if (0 == strcmp(argv[0], "-tursi")) {
			printf("Switching to Tursi's encryption mode.\n");
			TursiMode=1;
			boot1 = boot_tursi;
		} else {
			fprintf(stderr, "ERROR: unknown option `%s'\n", argv[0]);
			usage();
		}
		argv++;
	}

	if (!*argv) {
		fprintf(stderr, "ERROR: no file name specified\n");
		usage();
	}
	if (argv[1]) {
		fprintf(stderr, "ERROR: only 1 file may be specified at a time\n");
		usage();
	}
	filename = argv[0];
	 
	if (!useprevdata) {
		/* calculate new encryption data */

#if 0
// no longer need to read keys from disk - no longer secret

		printf("Insert Private Key Disk in drive A: (then hit Enter)...\n");
		/* flush input buffer */
		flushinp();

	/* get private key */
		fhandle = fopen(PRIVKEYPATH, "r");
		if (!fhandle) {
			perror(PRIVKEYPATH);
			exit(1);
		}
		if (!ReadAsmFile(fhandle, privateK, 128)) {
			fprintf(stderr, "ERROR reading key from %s\n", PRIVKEYPATH);
			exit(1);
		}
		fclose(fhandle);
	
	/* get public key (this could be built into the program) */
		fhandle = fopen(PUBKEYPATH, "r");
		if (!fhandle) {
			perror(PUBKEYPATH);
			exit(1);
		}
		if (!ReadAsmFile(fhandle, publicK, 128)) {
			fprintf(stderr, "ERROR reading key from %s\n", PUBKEYPATH);
			exit(1);
		}
		fclose(fhandle);
#endif

	/* copy keys to RSA numbers */
		nnum = N_num;
		cnum = C_num;
		*nnum++ = 0;
		*cnum++ = 0;
		for (i = 0; i < KEYSIZE-1; i++) {
			*nnum++ = publicK[i];
			*cnum++ = privateK[i];
		}
	}

	/* try to open the file to encrypt */ 
	fhandle = fopen(filename, "rb");
	if (!fhandle) {
	/* report error and exit */
		perror(filename);
		exit(1);
	}

	if (TursiMode) {
		// the filename is the boot file to encrypt, so drag that in
		// note: the encryption code skips the first 8 bytes, so we do here too
		boot_tursi_size = fread(&boot_tursi[8], 1, sizeof(boot_tursi)-8, fhandle);
		boot_tursi_size -= 8;
		fclose(fhandle);
	} else {
		int c = fgetc(fhandle);
		ungetc(c, fhandle);
		if (c >= 0xf6) {
			printf("Skipping suspected header on %s\n", filename);
			fseek(fhandle, 8192, SEEK_SET);
		}
		printf("Calculating MD5 on: %s\n", filename);
	}

	filesize = 0x2000;	/* file starts at offset +$2000 */
	romsize = 0x2000;

	if (!TursiMode) { 
		MD5init(MD5state);
	}

/*
 *
 *  Cartridge ROM memory map:
 *
 *	$800000-$80028A		10x65+1 byte block RSA signature 
 *	$80028B-$8002BF		Unprotected, unused ROM (53 bytes)
 *
 *start of MD5 protection
 *	$8002C0-$8003FF		start of MD5 protected space, unused (320 bytes)
 *	$800400-$800403		start-up vector (usually $802000)
 *	$800404-$800407		start-up vector (usually $802000)
 *	$800408-$80040B		Flags [31:01] undefined use 0's
 *				  bit0:  0-normal boot, 1-skip boot (diag cart) 
 *
 *	$80040B-$801FFF		usually unused, protected (7156 bytes)
 *
 *	$802000-$9FFFFF		game cartridge ROM (may be smaller or larger)
 *end of MD5 protection
 *
 *
 * calculate MD5 on the non-game portion of protected ROM space
 */

	/* fill buffer with all FF's */
	memset(inbuf, 0xff, 0x2000);

	/* initialize start up vectors, etc. */
	memcpy(inbuf+0x400, Romconfig, 12);

	if (!TursiMode) {
		/* do MD5 on first part of ROM */
		cnum = inbuf+0x2c0;
		do {
			MD5trans(MD5state, cnum);
			cnum += 64;
		} while (cnum < (inbuf+0x2000));

		/* now read in blocks of game cart and do MD5 for each */
		ateof = 0;
		
		do {
			long numbytes;

			numbytes = fread(inbuf, 1, BUFSIZE, fhandle);
			if (numbytes < 0) {
				fprintf(stderr, "ERROR: read error on input\n");
				exit(1);
			}
			if (numbytes == 0) {
				/* end of file reached */
				ateof = 1;
				break;
			}
			filesize += numbytes;
			romsize += BUFSIZE;
			if (numbytes < BUFSIZE) {
			/* only a partial buffer */
			/* fill the rest of the buffer with FFs */
				memset(inbuf+numbytes, 0xff, BUFSIZE-numbytes);
			/* indicate end of file */
				ateof = 1;
			}

			cnum = inbuf;
			do {
				MD5trans(MD5state, cnum);
				cnum += 64;
			} while (cnum < inbuf+BUFSIZE);

		} while (!ateof);

		fclose(fhandle);		/* we're done with the input file */
	}

/* We need MD5 to cover all of ROM, which often needs to be larger than
 * the file
 */

	/* find smallest power of 2 which can enclose the file */
	powof2 = 2;
	while (powof2 < romsize)
		powof2 *= 2;

	/* fill the rest of ROM with ff's */
	memset(inbuf, 0xff, BUFSIZE);

	if (!TursiMode) {
		while (romsize < powof2) {
			MD5trans(MD5state, inbuf);
			romsize += 64;
		}
	}
	romsize = powof2;

/* are we using the old encrypted data in foo.XXX */
	if (useprevdata) {
	/* read in previous encryption data */
		fhandle = fopen_with_extension(filename, ".XXX", "rb");
		if (!fhandle) {
			perror("Unable to open .XXX file");
			exit(1);
		}
		xxxsize = fread(romimg, 1, 0x1c02, fhandle);
		if (xxxsize < 0) {
			fprintf(stderr, "Read error on .XXX file\n");
			exit(1);
		}
		if (xxxsize >= 0x1c01) {
			fprintf(stderr, ".XXX file is too large ( > 7168 bytes).\n");
			exit(1);
		}
		fclose(fhandle);
	} else {
	/* start the RSA up from scratch */
	#define HASHBASE 0x24
	#define RANGE 0x54
		byte *a0, *a1, *a2, *a3, *a4;
		long d0, d1, d2, d3;

		printf("Calculating RSA... (don't blink! Can you believe this used to take MINUTES?)\n");

		/* stuff appropriate values into the GPU code */
		/* (Sorry about the mess, this was a straight conversion of Dave's
		 *  assembly code)
		 */
		a0 = boot1+8;
		a4 = (byte *)MD5state;
		d1 = 0xbc;
		a3 = a0+HASHBASE;

		d0 = *(long *)a4;		/* don't use getlong, MD5state is in native format */
		if (!TursiMode) {
			Putlong(a3-4, ~d0);		/* writes MD5 startup state to offset 0x20 */
		}
		d0 = *(long *)(a4+4);
		if (!TursiMode) {
			Putlong(a3+32, ~d0);	/* writes second part of MD5 state to offset 0x44 */
		}

		d3 = 0;
		for (i = 0; i < 8; i++) {	/* writes the MD5 hash to offset 0x24 */
			d0 = Getlong(a0+d1);	/* need Getlong because it's in 68000 format */
			if (!TursiMode) {
				Putlong(a3, d0);
			}
			a3 += 4;
			d0 = *(long *)(a4+d3);
			if (!TursiMode) {
				Putlong(a0+d1,d0);
			}
			d3 = (d3+4) & 0x0f;
			d1 += 0x40;
			
		}

		a1 = a0 + (RANGE+2);		
		d1 = 0x02c00080;	/* 0x8002c0, word swapped */
		if (!TursiMode) {
			Putlong(a1, d1);			/* overwrites first MOVEI in the CD boot part (hash start adr) */
		}
		a1 += 6;
		d1 = romsize + 0x800000;
		d1 = ((unsigned long)d1 >> 16) | ((d1 & 0x0000ffff) << 16);	/* word swap */
		if (!TursiMode) {
			Putlong(a1, d1);			/* overwrites the second MOVEI in the CD boot part (hash end adr) */
		}

		/* do 1 other tricky transform */
		a2 = boot1+8;
		a1 = a0;
		d1 = 0;
		for (i = 0; i < 0x280; i++) {
			d3 = d2 = *a2++;
			d2 -= d1;
			d1 = d3;
			*a1++ = (d2 & 0x00ff);
		}

		/* patching is complete, now let's RSA */

		/* prepare keysize-1 byte chunks for RSA */
		/* For TursiMode, we might not need 10, but we'll do them anyway */
		MultRSA(a0, romimg, 10);		/* parameters are source, destination, numblocks */
	}

	/* now write out the data */

	/* build the first 0x2000 byte block, corresponding to the ROM config
	 * longword, the RSA signature, the startup vector, and the flags
	 */

	memset(inbuf, 0xff, sizeof(inbuf));

	/* copy over the RSA signature */
	for (i = 0; i < 651; i++) {
		inbuf[i] = romimg[i];
	}

	memcpy(inbuf+0x400, Romconfig, 12);

	// patch number of blocks
	if (TursiMode) {
		int b = (boot_tursi_size+64)/65;
		printf("New boot uses %d blocks\n", b);
		b = 0x100 - b;
		inbuf[0] = b;
	} else {
		printf("ROM size %d bytes...\n", romsize);
	}

	/* create signature file */
	fhandle = fopen_with_extension(filename, ".XXX", "wb");
	if (!fhandle) {
		perror(".XXX file");
		exit(1);
	}
	if (fwrite(inbuf, 1, xxxsize, fhandle) != xxxsize) {
		fprintf(stderr, "Write error on .XXX file!\n");
		exit(1);
	}
	fclose(fhandle);

	if (!TursiMode) {
		printf("Writing data...\n");
		if (outfmt == HILO) {	/* Hi/Lo split */
			WriteHILO(filename, nosplit);
		} else if (outfmt == SINGLE) {
			WriteSINGLE(filename);
		} else {
			Write4xROM(filename);
		}
	}

	return 0;
}
