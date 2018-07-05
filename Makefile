.PHONY: clean all dist

all: firehose

firehose: firehose.cpp safefd.h
	g++ -std=gnu++17 -Wall -Og -g -o $@ $<

dist: firehose.tar.gz

firehose.tar.gz: firehose.cpp safefd.h Makefile
	tar cvvzf $@ $^

clean:
	rm -f firehose.tar.gz firehose

sender: sender.c
	gcc -Wall -o $@ $^
