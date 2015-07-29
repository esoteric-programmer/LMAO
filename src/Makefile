CC=gcc
CFLAGS=-O3 -Wall -c -I.
LEX=flex
YACC=bison -d -v
OBJ=../bin/bison.o ../bin/lex.o ../bin/gen_init.o ../bin/initialize.o ../bin/main.o ../bin/malbolge.o

all: lmao

../bin:
	mkdir ../bin/

../bin/lmao.tab.c: lmao.y
	$(YACC) -o ../bin/lmao.tab.c lmao.y

../bin/bison.o: ../bin/lmao.tab.c
	$(CC) $(CFLAGS) -o ../bin/bison.o ../bin/lmao.tab.c

../bin/lex.yy.c: lmao.l ../bin/lmao.tab.c
	$(LEX) -o ../bin/lex.yy.c lmao.l

../bin/lex.o: ../bin/lex.yy.c
	$(CC) $(CFLAGS) -o ../bin/lex.o ../bin/lex.yy.c

../bin/%.o: %.c *.h
	$(CC) $(CFLAGS) -o $@ $<

lmao: ../bin $(OBJ)
	gcc $(OBJ) -o ../bin/lmao -lfl

clean:
	rm -rf ../bin/

