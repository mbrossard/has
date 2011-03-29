/*
  (c) Mathias Brossard <mathias@brossard.org>
*/

#include "has.c"
#include "has_json.c"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void test_recode_unicode(int32_t code, char *utf16, size_t u16len,
                         char *utf8, size_t u8len)
{
    char buffer[256];
    int32_t cp = 0;
    memset(buffer, 0, sizeof(buffer));

    assert(has_json_encode_unicode(code, buffer) == u16len);
    assert(memcmp(buffer, utf16, u16len) == 0);
    assert(has_json_decode_unicode(buffer + 2, u16len - 2, &cp) == u16len - 2);
    assert(encode_utf8(cp, buffer) == u8len);
    assert(memcmp(buffer, utf8, u8len) == 0);
}

void test_decode_utf8(char *utf8, size_t u8len, int32_t code)
{
    int32_t cp = 0;
    assert(decode_utf8(utf8, u8len, &cp) == u8len);
    assert(cp == code);
}

int main(int argc, char **argv)
{
    /* Examples from http://en.wikipedia.org/wiki/UTF-16 */
    test_recode_unicode(0x7A,            "\\u007A",  6,             "\x7A", 1);
    test_recode_unicode(0x6C34,          "\\u6C34",  6,     "\xe6\xb0\xb4", 3);
    test_recode_unicode(0x10000,  "\\uD800\\uDC00", 12, "\xf0\x90\x80\x80", 4);
    test_recode_unicode(0x64321,  "\\uD950\\uDF21", 12, "\xf1\xa4\x8c\xa1", 4);
    test_recode_unicode(0x1D11E,  "\\uD834\\uDD1E", 12, "\xf0\x9d\x84\x9e", 4);
    test_recode_unicode(0x10FFFD, "\\uDBFF\\uDFFD", 12, "\xf4\x8f\xbf\xbd", 4);

    /* Gathered from Description http://en.wikipedia.org/wiki/UTF-8 */
    test_decode_utf8("\x24",             1,     0x24);
    test_decode_utf8("\xC2\xA2",         2,     0xA2);
    test_decode_utf8("\xE2\x82\xAC",     3,   0x20AC);
    test_decode_utf8("\xF0\xA4\xAD\xA2", 4, 0x024B62);

    return 0;
}
