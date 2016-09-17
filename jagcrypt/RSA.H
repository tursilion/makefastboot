#ifndef EXTERN
#define EXTERN extern
#endif

#define KEYSIZE 66

/*
 * big numbers for RSA
 */
EXTERN unsigned char A_num[KEYSIZE], B_num[KEYSIZE], C_num[KEYSIZE], N_num[KEYSIZE];


/*
 * function to get a long from a stream of bytes
 */
#define Getlong(x) ( ((long)(x)[0] << 24) | ((long)(x)[1] << 16) | ((long)(x)[2] << 8) | ((long)(x)[3]) )

/*
 * function to put a long into a stream of bytes
 */
static void
Putlong(unsigned char *dest, long x)
{
	dest[0] = (x >> 24) & 0x00ff;
	dest[1] = (x >> 16) & 0x00ff;
	dest[2] = (x >> 8) & 0x00ff;
	dest[3] = x & 0x00ff;
}
