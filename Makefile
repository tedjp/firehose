all: mfwd
.PHONY: mfwd clean all

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
	g++ -Wall -Og -g -o $@ $<
