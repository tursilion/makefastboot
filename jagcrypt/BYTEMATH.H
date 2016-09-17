/* Copyright (C) RSA Data Security, Inc. created 1991.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* does this compiler support prototypes ? */
/* set to 1 for DOS and 0 for UNIX */
#define PROTOTYPES 1

/* maximum number of bytes in an argument */
#define MAX_m 100

#if PROTOTYPES
void ModExp (unsigned char *, unsigned char *, unsigned char *, 
             unsigned char *, int);
#else
void ModExp ();
#endif

