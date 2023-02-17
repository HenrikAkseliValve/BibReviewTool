CPPFLAGS=`pkg-config --cflags Qt6Widgets`
CFLAGS=
LIBS=`pkg-config --libs Qt6Widgets` -lbtparse

DEBUG?=0
ifeq ($(DEBUG),1)
  CFLAGS+=-g
else
  CFLAGS+=-O3
endif

LiteratureReview: Source/btparseWrite.o Source/main.o
	g++ $(CFLAGS) -o LiteratureReview $^ $(LIBS)
Source/btparseWrite.o: Source/btparseWrite.c
	gcc -c $(CFLAGS) -o $@ $^
Source/main.o: Source/main.cpp
	g++ -c $(CFLAGS) $(CPPFLAGS) -o $@ $^
