
CFLAGS=-Wall -DUSER_SPACE
CC=cc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TESTS=testdict testrand testhash testdh testgc testmap
TOOLS=mkfs


all: tests tools

objs: $(OBJS)
tests: objs $(TESTS)
tools: objs $(TOOLS)


mkfs: kfs_mkfs.o krand64.o dict.o hash.o dumphex.o gc.o map.o
	$(CC) -o mkfs kfs_mkfs.o krand64.o dict.o hash.o dumphex.o gc.o map.o

testdict: testdict.o hash.o dict.o 
	$(CC) -o testdict testdict.o hash.o dict.o 

testrand: testrand.o krand64.o
	$(CC) -o testrand testrand.o krand64.o

testhash: testhash.o hash.o
	$(CC) -o testhash testhash.o hash.o 

testdh: testdh.o dumphex.o
	$(CC) -o testdh testdh.o dumphex.o

testgc: testgc.o gc.o dumphex.o 
	$(CC) -o testgc testgc.o gc.o dumphex.o 

testmap: testmap.o map.o 
	$(CC) -o testmap testmap.o map.o

dumphex.o:
	$(CC) -c dumphex.c $(CFLAGS)

hash.o:
	$(CC) -c hash.c $(CFLAGS)

gc.o:
	$(CC) -c gc.c $(CFLAGS)

clean:
	@UNAME=`uname|egrep -i "cygwin|windows"`; \
     if [ "$$UNAME" != "" ]; then             \
	   rm *.o *.exe;                          \
	 else                                     \
	   rm *.o $(TESTS) $(TOOLS);              \
	 fi




