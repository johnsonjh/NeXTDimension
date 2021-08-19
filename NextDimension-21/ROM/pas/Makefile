# Makefile for the p-code assembler.  The Makefile
# controls the construction of the lex and yacc based
# parser and scanner as well as the usual compilation stuff.
CFLAGS= -g

YACC=	yacc
LEX=	lex

OBJS=	gram.o scan.o main.o util.o pass1.o pass2.o output.o list.o

LIBS=	-ll

TARGET= pas

default:	$(TARGET)

install:

installh:


$(TARGET):	$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)


gram.o:	gram.y
	$(YACC) -d gram.y
	-mv y.tab.c gram.tab.c
	-mv y.tab.h gram.tab.h
	$(CC) $(CFLAGS) -c gram.tab.c
	mv gram.tab.o gram.o
	rm -f gram.tab.c

	
scan.o:	scan.l gram.tab.h
	$(LEX) scan.l
	$(CC) $(CFLAGS) -c lex.yy.c
	mv lex.yy.o scan.o
	rm -f lex.yy.c
	
clean:
	rm -f *.o gram.tab.c gram.tab.h lex.yy.c gram.output $(TARGET)
	
