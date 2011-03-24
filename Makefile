CFLAGS = -O2 -g -I.

all: tests/test_has

tests/test_has: tests/test_has.c has.c has.h
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *~ tests/test_has
