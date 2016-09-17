/* Copyright (C) RSA Data Security, Inc. created 1991.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */
/*
 * modified by Eric R. Smith, Atari Corporation, to produce more
 * efficient code under gcc and similar modern compilers
 */
#include "bytemath.h"
#include <string.h>

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

#define BIT(C, i, m) ((C)[(i)/8] & (1 << ((i) & 7)))

/* A = B */
#define Copy(A, B, m) memcpy((A), (B), (m))

/* A = 0 */
#define Clear(A, m) memset((A), 0, (m))

/* A = 1 */
static INLINE void
One (unsigned char *A, int m)
{
  Clear (A, m);
  A[0] = 1;
}

/* A = B if B < N, (B-N) if B >= N */
static INLINE void
Adjust (unsigned char *A, unsigned char *B, unsigned char *N, int m)
{
  int i, x;
  unsigned char T[MAX_m];
  
  x = 0;
  for (i = 0; i < m; i++) {
    x += B[i] - N[i];
    T[i] = (unsigned char)(x & 0xFF);
    x >>= 8;
  }
  
  if (x >= 0)
    Copy (A, T, m);
  else
    Copy (A, B, m);
}

/* v = -1/N mod 256 */
static INLINE void
MontCoeff (unsigned char *v, unsigned char *N, int m)
{
  int i;
  m = m;

  *v = 0;
  for (i = 0; i < 8; i++)
    if (! ((N[0]*(*v) & (1 << i))))
      *v += (1 << i);
}


/* A = 2*B */
static INLINE void
Double (unsigned char *A, unsigned char *B, int m)
{
  int i, x;
  
  x = 0;
  for (i = 0; i < m; i++) {
    x += 2*B[i];
    B[i] = (unsigned char)(x & 0xFF);
    x >>= 8;
  }
  /* shouldn't carry */
}


/* A = B*(256**m) mod N */
static INLINE void
Mont (unsigned char *A, unsigned char *B, unsigned char *N, int m)
{
  int i;

  Copy (A, B, m);

  for (i = 0; i < 8*m; i++) {
    Double (A, A, m);
    Adjust (A, A, N, m);
  }
}

/* A = B*C/(256**m) mod N where v*N = -1 mod 256 */
static INLINE void
MontMult (unsigned char *A, unsigned char *B, unsigned char *C, unsigned char *N, unsigned char v, int m)
{
  int i, j;
  unsigned char ei, T[2*MAX_m];
  unsigned int x;

  Clear (T, 2*m);
  
  for (i = 0; i < m; i++) {
    x = 0;
    for (j = 0; j < m; j++) {
      x += (unsigned int)T[i+j] + (unsigned int)B[i] * (unsigned int)C[j];
      T[i+j] = (unsigned char)(x & 0xFF);
      x >>= 8;
    }
    T[i+m] = (unsigned char)(x & 0xFF);
  }

  for (i = 0; i < m; i++) {
    x = 0;
    ei = (unsigned char)(((unsigned int)v * (unsigned int)T[i]) & 0xFF);
    for (j = 0; j < m; j++) {
      x += (unsigned int)T[i+j] + (unsigned int)ei * (unsigned int)N[j];
      T[i+j] = (unsigned char)(x & 0xFF);
      x >>= 8;
    }
    A[i] = (unsigned char)(x & 0xFF);
  }
  
  x = 0;
  for (i = 0; i < m; i++) {
    x += (unsigned int)T[i+m] + (unsigned int)A[i];
    A[i] = (unsigned char)(x & 0xFF);
    x >>= 8;
  }
  /* shouldn't carry */
}

/* A = (B**C)/(256**((C-1)*m)) mod N, where v*N = -1 mod 256 */
static INLINE void
MontExp (unsigned char *A, unsigned char *B, unsigned char *C, unsigned char *N, unsigned char v, int m)
{
  int i;
  unsigned char T[MAX_m];
  
  One (T, m);
  Mont (T, T, N, m);
  
  for (i = 8*m-1; i >= 0; i--) {
    MontMult (T, T, T, N, v, m);
    if (BIT (C, i, m))
      MontMult (T, T, B, N, v, m);
  }

  Copy (A, T, m);
}

/* A = B/(256**m) mod N, where v*N = -1 mod 256 */
static INLINE void
UnMont (unsigned char *A, unsigned char *B, unsigned char *N, unsigned char v, int m)
{
  unsigned char T[MAX_m];
  
  One (T, m);
  MontMult (A, B, T, N, v, m);

  Adjust (A, A, N, m);
}

/***************************************************
 * Main Entry Point				   *
 ***************************************************/

/* All operands have least significant byte first. */
/* A = B**C mod N */
void ModExp (A, B, C, N, m)
unsigned char *A, *B, *C, *N;
int m;
{
  unsigned char T[MAX_m], v;

  MontCoeff (&v, N, m);
  Mont (T, B, N, m);
  MontExp (T, T, C, N, v, m);
  UnMont (A, T, N, v, m);
}


