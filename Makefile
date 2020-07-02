.PHONY: all clean install

INSTDIR := ~/.local/lib64

all: libslower.so

libslower.so: slower.c
	gcc -O2 -march=native -s -fPIC -D_GNU_SOURCE -shared -Wall -o $@ $< -ldl -lpthread -lm

clean:
	rm -f libslower.so

install: $(INSTDIR)/libslower.so

$(INSTDIR)/libslower.so: libslower.so
	install -D -T -C -v -m644 $< $@
