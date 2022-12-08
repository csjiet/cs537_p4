CC     := gcc
CFLAGS := -Wall -Werror 

SRCS   := client.c server.c 

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

LINK	:= export LD_LIBRARY_PATH=.

.PHONY: all
all: ${PROGS} compile
	${LINK}
	ldd main

${PROGS} : % : %.o Makefile.net
	${CC} $< -o $@ udp.c

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<


compile: libmfs.so
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
	rm -f ${PROGS} ${OBJS}
	killall server
	

visualize: compile
	./mkfs -f test.img -v