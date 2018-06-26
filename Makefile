all: mfwd
.PHONY: mfwd clean all dist

#HEADERS = source.h sink.h
#SOURCES = \
#		  mfwd.cpp \
#		  source.cpp \
#		  sink.cpp \
#		  ucastsource.cpp \
#		  ucastsink.cpp \
#		  mcastsource.cpp \
#		  mcastsink.cpp \
#		  #

mfwd: mf2.cpp
	g++ -std=gnu++17 -Wall -Og -g -o $@ $<

dist:
	tar cvvf mfwd.tar.gz mf2.cpp Makefile safefd.h
