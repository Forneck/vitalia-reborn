# Generated automatically from Makefile.in by configure.
# tbaMUD Makefile.in - Makefile template used by 'configure'
# Clean-up provided by seqwith.

# C compiler to use
CC = gcc

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -Wno-char-subscripts -Wno-unused-but-set-variable

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

BINDIR = ../bin

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE)

LIBS =  -lcrypt  -lm -lz

SRCFILES := $(shell ls *.c | sort)
OBJFILES := $(patsubst %.c,%.o,$(SRCFILES))  

default: all

all: .accepted
	$(MAKE) $(BINDIR)/circle
	$(MAKE) utils

.accepted:
	@./licheck less

utils: .accepted
	(cd util; $(MAKE) all)

circle:
	$(MAKE) $(BINDIR)/circle

$(BINDIR)/circle : $(OBJFILES)
	$(CC) -o $(BINDIR)/circle $(PROFILE) $(OBJFILES) $(LIBS)

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@ 

clean:
	rm -f *.o depend

# Dependencies for the object files (automagically generated with
# gcc -MM)

depend:
	$(CC) -MM *.c > depend

-include depend
