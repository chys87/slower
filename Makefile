.PHONY: all clean install

INSTDIR := ~/.local/lib64

all: libslower.so

libslower.so: slower.c
	gcc -O2 -march=native -fPIC -D_GNU_SOURCE -shared -Wall -o $@ $< -ldl -lpthread -lm -lrt

clean:
	rm -f libslower.so

install: $(INSTDIR)/libslower.so

$(INSTDIR)/libslower.so: libslower.so
	install -D -T -v -s -m644 $< $@
