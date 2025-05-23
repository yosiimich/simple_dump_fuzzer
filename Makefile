
ifeq "$(origin CC)" "default"
	CC = clang
	CXX = clang++
endif

PROGS = .forkserver.o .compiler

all: $(PROGS)

.forkserver.o:
	$(CC) -c forkserver.c

.compiler:
	$(CC) compiler.c -o compiler

clean:
	rm forkserver.o compiler