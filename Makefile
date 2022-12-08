CC     := gcc
CFLAGS := -Wall -Werror 

SRCS   := client.c server.c 

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS} run

${PROGS} : % : %.o Makefile.net
	${CC} $< -o $@ udp.c

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<

run: compile
	
	export LD_LIBRARY_PATH=.
	ldd main

compile: libmfs.so libmfs.o
	gcc -o mkfs mkfs.c -Wall 
	gcc -o main main.c -Wall -L. -lmfs 
	ldd main

libmfs.so: libmfs.o udp.c
	${CC} ${CFLAGS} -fpic -c udp.c
	gcc -shared -Wl,-soname,libmfs.so -o libmfs.so libmfs.o udp.o -lc	

libmfs.o: libmfs.c
	gcc -fPIC -g -c -Wall libmfs.c

clean:
	rm -r *.o
	rm ./main
	rm ./libmfs.so
	rm ./mkfs
	killall server
	rm -f ${PROGS} ${OBJS}

visualize: run
	./mkfs -f test.img -v