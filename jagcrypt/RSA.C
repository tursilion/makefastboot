/*
 * Copyright 1995 Atari Corporation
 * All Rights Reserved
 */

#include <stdio.h>
#include <stdlib.h>
#define EXTERN		/* trigger definition of global variables */
#include "rsa.h"
#include "bytemath.h"
#include "proto.h"

#if 0
#define DEBUGMD5	/* define this to get debugging output from the MD5 function */
#endif

const long initstate[4] = {
	0x67452301,
	0xEFCDAB89,
	0x98BADCFE,
	0x10325476
};

static const long
hashtable[] = {
/* futz */
	0xD76AA478,
	0xE8C7B756,
	0x242070DB,
	0xC1BDCEEE,
	0xF57C0FAF,
	0x4787C62A,
	0xA8304613,
	0xFD469501,
	0x698098D8,
	0x8B44F7AF,
	0xFFFF5BB1,
	0x895CD7BE,
	0x6B901122,
	0xFD987193,
	0xA679438E,
	0x49B40821,
/* grind */
	0xF61E2562,
	0xC040B340,
	0x265E5A51,
	0xE9B6C7AA,
	0xD62F105D,
	0x02441453,
	0xD8A1E681,
	0xE7D3FBC8,
	0x21E1CDE6,
	0xC33707D6,
	0xF4D50D87,
	0x455A14ED,
	0xA9E3E905,
	0xFCEFA3F8,
	0x676F02D9,
	0x8D2A4C8A,
/* hack */
	0xFFFA3942,
	0x8771F681,
	0x6D9D6122,
	0xFDE5380C,
	0xA4BEEA44,
	0x4BDECFA9,
	0xF6BB4B60,
	0xBEBFBC70,
	0x289B7EC6,
	0xEAA127FA,
	0xD4EF3085,
	0x04881D05,
	0xD9D4D039,
	0xE6DB99E5,
	0x1FA27CF8,
	0xC4AC5665,
/* ickify */
	0xF4292244,
	0x432AFF97,
	0xAB9423A7,
	0xFC93A039,
	0x655B59C3,
	0x8F0CCC92,
	0xFFEFF47D,
	0x85845DD1,
	0x6FA87E4F,
	0xFE2CE6E0,
	0xA3014314,
	0x4E0811A1,
	0xF7537E82,
	0xBD3AF235,
	0x2AD7D2BB,
	0xEB86D391,
};


/*
; ROL counts for each op
 */

static const int
rotatetable[] = {
	7,
	12,
	17,
	22,

	5,
	9,
	14,
	20,

	4,
	11,
	16,
	23,

	6,
	10,
	15,
	21,
};

static const int initx[] = {		/* initial index */
	0, 4, 20, 0
};
static const int xinc[] = {		/* index increment */
	4, 20, 12, 28
};


/* rotate left circular by "n" (0 < n < 32) */

static long
rol(long a, int n)
{
	unsigned long ua;

	ua = a;
	return (long)(ua << n) | (ua >> (32-n));
}

#ifdef DEBUGMD5
FILE *debugfile;
#endif

void
MD5init(long *state)
{
	int i;
	for (i = 0; i < 4; i++)
		state[i] = initstate[i];
#ifdef DEBUGMD5
	debugfile = fopen("debug.md5", "wb");
#endif
}

void
MD5trans(long *state, byte *inpdata)
{
	long a, b, cc, d;
	const long *hashtab;		/* hash table ptr */
	const int *rotater;		/* rotate count table */
	int rotateindex;	/* rotate count index */
	int index;		/* index (long aligned) in input buffer, mod 64 */
	int incr;		/* index increment */
	int species;		/* type of mangling to do:
				   0 == futz, 1 == grind, 2 == hack, 3 == ickify */
	/* scratch variables */
	long accum;
	int i;

	a = state[0];
	b = state[1];
	cc = state[2];
	d = state[3];

	hashtab = hashtable;
	rotater = rotatetable;
	rotateindex = 0;

	for (species = 0; species < 4; species++) {
		index = initx[species];		/* get initial index into 64 byte input */
		incr = xinc[species];		/* and get the increment */
		for (i = 0; i < 16; i++) {
#ifdef DEBUGMD5
			byte debugoutput[16];

			Putlong(debugoutput,a);
			Putlong(debugoutput+4,b);
			Putlong(debugoutput+8,cc);
			Putlong(debugoutput+12,d);

			fwrite(debugoutput, 1, 12, debugfile);
#endif
			switch(species) {
			case 0:	/* futz */
				accum = (b&cc) | ((~b) & d);
				break;
			case 1:	/* grind */
				accum = (b & d) | (cc & (~d));
				break;
			case 2: /* hack */
				accum = (b ^ d) ^ cc;
				break;
			case 3: /* ickify */
				accum = cc ^ (b | (~d));
				break;
			}
			accum += Getlong(inpdata+index);	/* add next input data entry */
			index = (index + incr) & 0x3f;		/* mod 64 */
			accum += *hashtab++;			/* add next hash table entry */
			accum += a;				/* add a */

			accum = rol(accum, rotater[rotateindex]);
			rotateindex = (rotateindex+1) & 3;	/* next index, wrap */
			accum += b;				/* add b */

			a = d;
			d = cc;
			cc = b;
			b = accum;
		}
		rotater += 4;					/* move to next set of rotation indicies */
	}

	/* update state */
	state[0] += a;
	state[1] += b;
	state[2] += cc;
	state[3] += d;
}

/*
 * do an RSA encryption
 */

/*
 * Dave stored numbers with most significant byte first,
 * the opposite of the way the BSAFE library does. If
 * you're using Dave's high-speed assembly language
 * version, do *not* define REVERSE
 */

#define REVERSE		/* define for BSAFE C */


#ifdef REVERSE
void
Reverse(byte *src)
{
	byte temp[KEYSIZE];
	int i,j;

	memcpy(temp, src, KEYSIZE);

	j = KEYSIZE-1;
	for(i = 0; i < KEYSIZE; i++) {
		src[j] = temp[i];
		--j;
	}
}
#endif

void
MultRSA(byte *src, byte *dest, int blockcnt)
{
	int blocknr;
	int i;

	/* stuff neg. count for this group */
	*dest++ = 0x100 - blockcnt;

	printf("RSA on "); fflush(stdout);

#ifdef REVERSE
	Reverse(C_num);
	Reverse(N_num);
#endif

	for (blocknr = 0; blocknr < blockcnt; blocknr++) {
		byte *a2;

#ifdef REVERSE
		a2 = B_num;
		for (i = 0; i < KEYSIZE-2; i++) {
			*a2++ = *src++;
		}
		*a2++ = 0x15;
		*a2++ = 0;
#else
		a2 = B_num + KEYSIZE;
		for (i = 0; i < KEYSIZE-2; i++) {
			*--a2 = *src++;
		}
		*--a2 = 0x15;
		*--a2 = 0;
#endif
		printf("%d..", blocknr); fflush(stdout);

		ModExp(A_num, B_num, C_num, N_num, KEYSIZE);

#ifdef REVERSE
		a2 = A_num;
		for (i = 0; i < KEYSIZE-1; i++) {
			*dest++ = *a2++;
		}
#else
		a2 = A_num + KEYSIZE;
		for (i = 0; i < KEYSIZE-1; i++) {
			*dest++ = *--a2;
		}
#endif
	}
	printf("\n");
}
