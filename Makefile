.PHONY: clean all dist

all: firehose

firehose: firehose.cpp safefd.h
	g++ -std=gnu++11 -Wall -Og -g -o $@ $<

dist: firehose.tar.gz

firehose.tar.gz: firehose.cpp safefd.h Makefile
	tar cvvzf $@ $^

clean:
	rm -f firehose.tar.gz firehose sender

sender: sender.c
	gcc -std=gnu11 -Wall -o $@ $^
