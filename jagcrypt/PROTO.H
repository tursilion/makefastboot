typedef unsigned char byte;

void MD5init(long *state);
void MD5trans(long *state, byte *inpdata);
void MultRSA(byte *src, byte *dst, int numblocks);

int ReadAsmFile(FILE *, byte *, int);
FILE *fopen_with_extension(char *name, char *ext, char *mode);
void WriteHILO(char *name, int nosplitflag);
void Write4xROM(char *name);
void WriteSINGLE(char *name);
