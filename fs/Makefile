# add this flags to CFLAGS to debug threads -fsanitize=address
# add this to link to debug threads -fsanitize=address -static-libasan


CFLAGS=-Wall -DUSER_SPACE -g -O0 
LDFLAGS=-fsanitize=address -static-libasan
CC=cc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
TESTS=testdict testrand testhash testdh testgc testmap testsizes kfs_test \
      testbcache

TOOLS=kfs_mkfs kfs_info kfs_server

all: tests tools

objs: $(OBJS)
tests: objs $(TESTS)
tools: objs $(TOOLS)


kfs_info: kfs_info.o krand64.o dict.o hash.o dumphex.o gc.o map.o kfs_io.o
	$(CC) -o kfs_info kfs_info.o krand64.o dict.o hash.o dumphex.o gc.o \
		     map.o kfs_io.o

kfs_mkfs: kfs_mkfs.o krand64.o dict.o hash.o dumphex.o gc.o map.o kfs_io.o
	$(CC) -o kfs_mkfs kfs_mkfs.o krand64.o dict.o hash.o dumphex.o gc.o \
		     map.o kfs_io.o

kfs_server: kfs_server.o krand64.o dict.o hash.o dumphex.o gc.o map.o kfs_io.o
	$(CC) -o kfs_server kfs_server.o krand64.o dict.o hash.o dumphex.o gc.o \
		     map.o kfs_io.o

kfs_test: kfs_test.o krand64.o dict.o hash.o dumphex.o gc.o map.o kfs_io.o
	$(CC) -o kfs_test kfs_test.o krand64.o dict.o hash.o dumphex.o gc.o \
		     map.o kfs_io.o



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

testsizes: sizes.o
	$(CC) -o testsizes sizes.o

testbcache: testbcache.o krand64.o dict.o hash.o dumphex.o gc.o map.o \
	        kfs_io.o kfs_cache.o
	$(CC) -o testbcache testbcache.o krand64.o dict.o hash.o dumphex.o gc.o \
		map.o kfs_io.o kfs_cache.o -lpthread 


dumphex.o:
	$(CC) -c dumphex.c $(CFLAGS)

hash.o:
	$(CC) -c hash.c $(CFLAGS)

gc.o:
	$(CC) -c gc.c $(CFLAGS)

sizes.o:
	$(CC) -c sizes.c $(CFLAGS)

clean:
	@UNAME=`uname|egrep -i "cygwin|windows"`; \
     if [ "$$UNAME" != "" ]; then             \
	   rm *.o *.exe;                          \
	 else                                     \
	   rm *.o $(TESTS) $(TOOLS);              \
	 fi




