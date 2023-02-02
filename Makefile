CFLAGS=`pkg-config --cflags Qt6Widgets`
LIBS=`pkg-config --libs Qt6Widgets` -lbtparse

DEBUG?=0
ifeq ($(DEBUG),1)
  CFLAGS+=-g
else
  CFLAGS+=-O3
endif

LiteratureReview: Source/main.cpp
	g++ $(CFLAGS) -o LiteratureReview $^ $(LIBS)

