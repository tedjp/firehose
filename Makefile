all: mfwd
.PHONY: mfwd clean all dist

mfwd: mfwd.cpp
	g++ -std=gnu++17 -Wall -Og -g -o $@ $<

dist: mfwd.tar.gz

mfwd.tar.gz: mfwd.cpp safefd.h Makefile
	tar cvvzf $@ $^

clean:
	rm -f mfwd.tar.gz mfwd

sender: sender.c
	gcc -Wall -o $@ $^
