CFLAGS = -O2 -g

test: test.c has.c has.h

clean:
	rm -f *~ test
