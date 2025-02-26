# add this flags to CFLAGS to debug threads -fsanitize=address
# add this to link to debug threads -fsanitize=address -static-libasan


CFLAGS=-Wall -DUSER_SPACE -g -O0 
LDFLAGS=-lkfs -L.
CC=cc
AR=ar
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
LIBKFS=libkfs.a


TESTS=testdict testrand testhash testdh testgc testmap testsizes \
      test_cache test_page_cache test_kfs_mount

TOOLS=help_build kfs_mkfs kfs_info kfs_server kfs_set_sb_meta

all: tests tools

objs: $(OBJS) $(LIBKFS)
tests: objs $(TESTS)
tools: objs $(TOOLS)

runtests: all 
	@echo "MKFS TESTS ";                 \
	./kfs_mkfs -s 50M  kfs_50M;          \
	./kfs_mkfs -s 200M kfs_200M;         \
	./kfs_mkfs -s 1G   kfs_1G;           \
	./kfs_info ./kfs_50M;                \
	./kfs_info ./kfs_200M;               \
	./kfs_info ./kfs_1G


testclean:
	rm -rf 

$(LIBKFS): krand64.o dict.o hash.o dumphex.o gc.o map.o kfs_io.o \
	      page_cache.o kfs_super.o cache.o
	$(AR) -r $(LIBKFS) krand64.o dict.o hash.o dumphex.o gc.o \
		     map.o kfs_io.o page_cache.o kfs_super.o cache.o

kfs_info: kfs_info.o $(LIBKFS)
	$(CC) -o kfs_info kfs_info.o $(LDFLAGS)


kfs_mkfs: kfs_mkfs.o mkfs_help.o eio.o $(LIBKFS)
	$(CC) -o kfs_mkfs kfs_mkfs.o mkfs_help.o eio.o $(LDFLAGS)


kfs_server: kfs_server.o $(LIBKFS)
	$(CC) -o kfs_server kfs_server.o $(LDFLAGS)

kfs_set_sb_meta: kfs_set_sb_meta.o $(LIBKFS)
	$(CC) -o kfs_set_sb_meta kfs_set_sb_meta.o $(LDFLAGS)

help_build:
	$(CC) -o help_build help_build.c



# tests
test_kfs_mount: test_kfs_mount.o $(LIBKFS)
	$(CC) -o test_kfs_mount test_kfs_mount.o $(LDFLAGS)

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

test_cache: test_cache.o cache.o
	$(CC) -o test_cache test_cache.o cache.o 

test_page_cache: test_page_cache.o dumphex.o page_cache.o utils.o eio.o \
	cache.o
	$(CC) -o test_page_cache test_page_cache.o dumphex.o eio.o page_cache.o \
		cache.o -lpthread

mkfs_help.o: mkfs_help.c
	$(CC) -c mkfs_help.c


mkfs_help.c: help_build
	./help_build mkfs_help.txt mkfs_help.c


clean:
	@UNAME=`uname|egrep -i "cygwin|windows"`; \
     if [ "$$UNAME" != "" ]; then             \
	   rm *.o *.exe $(LIBKFS);                \
	 else                                     \
	   rm *.o $(TESTS) $(TOOLS) $(LIBKFS);    \
	 fi

