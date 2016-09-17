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

/* input/output functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rsa.h"
#include "proto.h"

/* storage for magic signature for cart (0x2000 bytes) */
extern byte inbuf[];

/* total size of the ROM we're creating (always a power of 2 and > 0x2000) */
extern long romsize;

/*
 * read an ASCII assembly language file (with
 * statements like dc.b $00,$1a) and convert it
 * to binary, placing it into the given buffer.
 * At most "bufsize" bytes will be read.
 * Note that we *only* handle dc.b and hexadecimal
 * numbers; this is an extremely small subset of
 * possible inputs, but it's the only way the
 * key files are stored, so it's all we need.
 * Adding support for decimal numbers and words
 * or longs would be a bit of work.
 *
 * Returns: 0 if a read error occurs
 *          the number of bytes written into the
 *	    buffer on success
 */

int
ReadAsmFile(FILE *fh, byte *buffer, int bufsize)
{
	int c;		/* character retrieved */
	int val;	/* current byte value */
	int numbytes;	/* number of bytes written into buffer */

	numbytes = 0;
	while (numbytes < bufsize) {

	/* look for the next '$' character */
		do {
			c = fgetc(fh);
			if (c < 0)		/* end of file */
				return numbytes;
		} while (c != '$');
	/* now read 2 hex digits to make a byte */
	/* first digit */
		c = fgetc(fh);
		if (c < 0) {
			fprintf(stderr, "ERROR: unexpected end-of-file\n");
			return 0;		/* error */
		}
		c = toupper(c);			/* convert lower case to upper */
		if (c >= 'A' && c <= 'F')
			val = 16*(10 + (c-'A'));
		else if (c >= '0' && c <= '9')
			val = 16*(c-'0');
		else {
			fprintf(stderr, "ERROR: illegal hex digit (%c)\n", c);
			return 0;
		}

	/* second digit */
		c = fgetc(fh);
		if (c < 0) {
			fprintf(stderr, "ERROR: unexpected end-of-file\n");
			return 0;		/* error */
		}
		c = toupper(c);			/* convert lower case to upper */
		if (c >= 'A' && c <= 'F')
			val += 10 + (c-'A');
		else if (c >= '0' && c <= '9')
			val += c-'0';
		else {
			fprintf(stderr, "ERROR: illegal hex digit (%c)\n", c);
			return 0;
		}
		
	/* stuff the byte into the buffer */
		*buffer++ = val;
		numbytes++;
	}

	return numbytes;
}

/*
 * open a file whose base name is derived from
 * "filename", but whose extension is "ext".
 */
FILE *
fopen_with_extension(char *filename, char *ext, char *mode)
{
	char tempfilename[256];
	char *lastperiod;
	char *src, *dst, c;
	FILE *f;

	src = filename;
	dst = tempfilename;
	lastperiod = (char *)0;

	while ( (c = *src++) != 0) {
		if (c == '.')
			lastperiod = dst;
		else if (c == '/' || c == '\\')
			lastperiod = (char *)0;
		*dst++ = c;
	}

	if (lastperiod) {
		strcpy(lastperiod, ext);
	} else {
		strcat(tempfilename, ext);
	}

/* Do error reporting and bailout here, so our callers
 * don't have to.
 */
	f= fopen(tempfilename, mode);
	if (!f) {
		perror(tempfilename);
		exit(2);
	}
	return f;
}

/*
 * Write an output file in HI/LO format. If the file is larger than
 * 2 megabytes and the "nosplit" flag is 0, then the file is split up
 * in 1 megabyte pieces into foo.HI0/foo.LO0, foo.HI1/foo.LO1, etc.
 */

/* files bigger than this much are split */
#define SPLITSIZE 0x200000

void
WriteHILO(char *filename, int nosplit)
{
	FILE *hi, *lo;			/* file pointers for high and low data, respectively */
	FILE *infile;			/* input file */
	long byteswritten;		/* bytes of ROM written */
	long bytesleftinfile;		/* bytes left to write in this 2 megabyte piece */
	char hiext[6];			/* file extensions for hi/lo parts of file */
	char loext[6];
	byte *outbuf;			/* pointer to output data */
	byte tmpbuf[4];			/* input buffer */

	if ( romsize <= SPLITSIZE || nosplit) {
		strcpy(hiext, ".HI");
		strcpy(loext, ".LO");
		nosplit = 1;
	} else {
		strcpy(hiext, ".HI0");
		strcpy(loext, ".LO0");
	}

	hi = fopen_with_extension(filename, hiext, "wb");
	lo = fopen_with_extension(filename, loext, "wb");

	/* write the first 0x2000 bytes, including the RSA'd stuff */
	outbuf = inbuf;
	bytesleftinfile = SPLITSIZE;
	for (byteswritten = 0; byteswritten < 0x2000; byteswritten +=4 ) {
		fputc(outbuf[1],hi);
		fputc(outbuf[0], hi);
		fputc(outbuf[3], lo);
		fputc(outbuf[2], lo);
		outbuf += 4;
		bytesleftinfile -= 4;
	}

	/* open the file to be ROMed */
	infile = fopen(filename, "rb");
	if (!infile) {
		perror(filename);
		exit(1);
	}
	tmpbuf[0] = fgetc(infile);
	ungetc(tmpbuf[0], infile);
	if (tmpbuf[0] < 0xf6) {
		printf("Skipping suspected existing header...\n");
		fseek(infile, 8192, SEEK_SET);
	}

	outbuf = tmpbuf;	/* use small buffer for temporary I/O */
	while (byteswritten < romsize) {
		if (bytesleftinfile <= 0 && !nosplit) {
		/* we've filled up 2 megs of data, open a new set of files */
		/* (!nosplit means that large files *are* split) */
			fclose(lo);
			fclose(hi);

		/* change the file extension from .HI0 to .HI1, etc. */
			hiext[3]++; loext[3]++;
	
		/* re-open files */
			hi = fopen_with_extension(filename, hiext, "wb");
			lo = fopen_with_extension(filename, loext, "wb");

			bytesleftinfile = SPLITSIZE;
		}

	/*
	 * HACK ALERT: fgetc() returns -1 (0xffff) on end of file.
	 * This is perfect for us, since we want to stuff FFs into the
	 * end of the buffer... so the lack of error checking below
	 * is *deliberate*
	 */
		outbuf[0] = fgetc(infile);
		outbuf[1] = fgetc(infile);
		outbuf[2] = fgetc(infile);
		outbuf[3] = fgetc(infile);

		fputc(outbuf[1],hi);
		fputc(outbuf[0], hi);
		fputc(outbuf[3], lo);
		fputc(outbuf[2], lo);
		byteswritten += 4;
		bytesleftinfile -= 4;
	}

	fclose(infile);
	fclose(hi);
	fclose(lo);
}

// Write a single file, no splitting at all, big endian format
void
WriteSINGLE(char *filename, int nosplit)
{
	FILE *outfile;			/* file pointers for output file */
	FILE *infile;			/* input file */
	long byteswritten=0;	/* bytes of ROM written */
	byte *outbuf;			/* pointer to output data */
	byte tmpbuf[4];			/* input buffer */

	outfile = fopen_with_extension(filename, ".jag", "wb");

	/* write the first 0x2000 bytes, including the RSA'd stuff */
	fwrite(inbuf, 512, 16, outfile);
	byteswritten = 8192;

	/* open the file to be ROMed */
	infile = fopen(filename, "rb");
	if (!infile) {
		perror(filename);
		exit(1);
	}
	tmpbuf[0] = fgetc(infile);
	ungetc(tmpbuf[0], infile);
	if (tmpbuf[0] >= 0xf6) {
		printf("Skipping suspected existing header...\n");
		fseek(infile, 8192, SEEK_SET);
	}

	outbuf = tmpbuf;	/* use small buffer for temporary I/O */
	while (byteswritten < romsize) {
		memset(outbuf, 0xff, 4);
		fread(outbuf, 4, 1, infile);
		fwrite(outbuf, 4, 1, outfile);
		byteswritten += 4;
	}
	printf("Wrote %d bytes of %d to %s\n", byteswritten, romsize, filename);

	fclose(infile);
	fclose(outfile);
}

/*
 * Write an output file in 4xROM format; foo.U1 contains
 * the first byte of each longword, foo.U2 contains the
 * second byte, etc.
 */

void
Write4xROM(char *filename)
{
	FILE *u1, *u2, *u3, *u4;	/* file pointers for the four pieces respectively */
	FILE *infile;			/* input file */
	long byteswritten;		/* bytes of ROM written */
	int c;
	byte *outbuf;

/*
 * create the output file buffers
 */
	u1 = fopen_with_extension(filename, ".U1", "wb");
	u2 = fopen_with_extension(filename, ".U2", "wb");
	u3 = fopen_with_extension(filename, ".U3", "wb");
	u4 = fopen_with_extension(filename, ".U4", "wb");
/*
 * spit out the 0x2000 size header, with RSA signature etc.
 */
	outbuf = inbuf;
	for (byteswritten = 0; byteswritten < 0x2000; byteswritten += 4) {
		fputc(outbuf[0], u4);
		fputc(outbuf[1], u3);
		fputc(outbuf[2], u2);
		fputc(outbuf[3], u1);
		outbuf += 4;
	}

/*
 * now send out the rest of the file
 */
	infile = fopen(filename, "rb");
	if (!infile) {
		perror(filename);
		exit(1);
	}
	c = fgetc(infile);
	ungetc(c, infile);
	if (c < 0xf6) {
		printf("Skipping suspected existing header...\n");
		fseek(infile, 8192, SEEK_SET);
	}

	while (byteswritten < romsize) {
		c = fgetc(infile);
		fputc(c, u4);
		c = fgetc(infile);
		fputc(c, u3);
		c = fgetc(infile);
		fputc(c, u2);
		c = fgetc(infile);
		fputc(c, u1);
		byteswritten += 4;
	}
	fclose(infile);
	fclose(u1);
	fclose(u2);
	fclose(u3);
	fclose(u4);
}


