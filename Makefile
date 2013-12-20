.PHONY: all clean

all: libslower.so

libslower.so: slower.c
	gcc -O2 -fPIC -D_GNU_SOURCE -shared -o $@ $<

clean:
	rm -f libslower.so
