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
      test_page_cache test_kfs_mount

TOOLS=kfs_mkfs kfs_info kfs_server kfs_set_sb_meta

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


$(LIBKFS): krand64.o dict.o hash.o dumphex.o gc.o map.o kfs_io.o \
	      kfs_io.o kfs_page_cache.o kfs_super.o
	$(AR) -r $(LIBKFS) kfs_info.o krand64.o dict.o hash.o dumphex.o gc.o \
		     map.o kfs_io.o kfs_page_cache.o kfs_super.o

kfs_info: kfs_info.o $(LIBKFS)
	$(CC) -o kfs_info kfs_info.o $(LDFLAGS)


kfs_mkfs: kfs_mkfs.o $(LIBKFS)
	$(CC) -o kfs_mkfs kfs_mkfs.o $(LDFLAGS)


kfs_server: kfs_server.o $(LIBKFS)
	$(CC) -o kfs_server kfs_server.o $(LDFLAGS)

kfs_set_sb_meta: kfs_set_sb_meta.o $(LIBKFS)
	$(CC) -o kfs_set_sb_meta kfs_set_sb_meta.o $(LDFLAGS)





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

test_page_cache: test_page_cache.o krand64.o dict.o hash.o dumphex.o gc.o \
	        kfs_io.o kfs_page_cache.o map.o
	$(CC) -o test_page_cache test_page_cache.o krand64.o dict.o hash.o \
		dumphex.o map.o kfs_io.o kfs_page_cache.o gc.o -lpthread

clean:
	@UNAME=`uname|egrep -i "cygwin|windows"`; \
     if [ "$$UNAME" != "" ]; then             \
	   rm *.o *.exe $(LIBKFS);                \
	 else                                     \
	   rm *.o $(TESTS) $(TOOLS) $(LIBKFS);    \
	 fi

