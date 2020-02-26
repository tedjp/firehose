.PHONY: clean all dist

all: firehose

firehose: addr.cpp addr.h firehose.cpp safefd.h
	g++ -std=gnu++11 -fmax-errors=5 -Wall -Og -g -o $@ addr.cpp firehose.cpp

dist: firehose.tar.gz

firehose.tar.gz: addr.cpp addr.h firehose.cpp safefd.h Makefile
	tar cvvzf $@ $^

clean:
	rm -f firehose.tar.gz firehose sender sender6

sender: sender.c
	gcc -std=gnu11 -Wall -o $@ $^

sender6: sender6.c
	gcc -std=gnu11 -Wall -o $@ $^
