#====================================================================
#       Macro & Assembler flags
#====================================================================

STADDR = 100000
MACFLAGS = -fb -l -v
ALNFLAGS = -v -v -e -l -rd -a $(STADDR) x 4000

# this one makes a mapfile
ALNFLAGS = -v -v -e -g -l -m -rd -a $(STADDR) x 4000

#====================================================================
#       Default Rules
#====================================================================
.SUFFIXES:      .o .s

.s.o:
	smac $(MACFLAGS) $*
 
#====================================================================
#       EXECUTABLES
#====================================================================

OBJ = bootcode.o
 
mou.cof: $(OBJ)
#	aln $(ALNFLAGS) -o mou.cof $(OBJ) -i ..\\data\\jaguar.bin jagbits
	sln $(ALNFLAGS) -o bootcode.cof $(OBJ)

bootcode.o: bootcode.s
