.PHONY: all clean

all: libslower.so

libslower.so: slower.c
	gcc -O2 -s -fPIC -D_GNU_SOURCE -shared -Wall -o $@ $< -ldl -lpthread -lm

clean:
	rm -f libslower.so
