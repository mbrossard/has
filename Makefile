
CFLAGS = -O0 -g -I. -Wall -pedantic $(EXTRA)

TESTS = tests/test_has tests/test_json tests/test_utf8 \
	tests/test_x509 tests/test_pkcs10

all: $(TESTS)

tests/test_has: tests/test_has.c has.c has.h
	$(CC) $(CFLAGS) -o $@ $<

tests/test_json: tests/test_json.c has.c has.h has_json.c has_json.h
	$(CC) $(CFLAGS) -o $@ $<

tests/test_utf8: tests/test_utf8.c has.c has.h has_json.c has_json.h
	$(CC) $(CFLAGS) -o $@ $<

tests/test_x509: tests/test_x509.c has.c has.h has_json.c has_json.h has_x509.c
	$(CC) $(CFLAGS) -o $@ $< -lcrypto

tests/test_pkcs10: tests/test_pkcs10.c has.c has.h has_json.c has_json.h has_x509.c
	$(CC) $(CFLAGS) -o $@ $< -lcrypto

clean:
	rm -f $(TESTS)
	find . \( -name \*.o -or -name \*~ \) -delete
	find . \( -name \*.dSYM  -prune \) -exec rm -rf '{}' ';'
