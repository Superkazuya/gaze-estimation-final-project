CFLAGS = `pkg-config --cflags opencv`
LIBS = `pkg-config --libs opencv x11` -lm

gazetracking : $(wildcard *.c)
	gcc $^ $(CFLAGS) $(LIBS) -o $@ -U DEBUG -DCAMSHIFT=0

debug : $(wildcard *.c) 
	gcc $^ $(CFLAGS) $(LIBS) -o $@ -D DEBUG -DCAMSHIFT=0
