src=./
CC=cc
CFLAGS= -O3  -D_FILE_OFFSET_BITS=64 
prefix=/usr/local

all: $(src)/rrfs

$(src)/rrfs: $(src)/rrfs.c /usr/local/lib/libfuse-t.a

clean:
	rm -fr bin include $(src)/rrfs
