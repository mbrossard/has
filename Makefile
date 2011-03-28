
CFLAGS = -O2 -g -I. -Wall -pedantic

all: tests/test_has tests/test_json tests/test_utf8

tests/test_has: tests/test_has.c has.c has.h
	$(CC) $(CFLAGS) -o $@ $<

tests/test_json: tests/test_json.c has.c has.h has_json.c has_json.h
	$(CC) $(CFLAGS) -o $@ $<

tests/test_utf8: tests/test_utf8.c has.c has.h has_json.c has_json.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f tests/test_has tests/test_json
	find . \( -name \*.o -or -name \*~ \) -delete
	find . \( -name \*.dSYM  -prune \) -exec rm -rf '{}' ';'
