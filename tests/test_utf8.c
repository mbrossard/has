/*
  (c) Mathias Brossard <mathias@brossard.org>
*/

#include "has.c"
#include "has_json.c"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char **argv)
{
    char buffer[256];
    int32_t cp;
    memset(buffer, 0, 256);

    /* Examples from http://en.wikipedia.org/wiki/UTF-16 */
    assert(has_json_encode_unicode(0x7A, buffer) == 6);
    assert(memcmp(buffer, "\\u007A", 6) == 0);
    assert(has_json_encode_unicode(0x6C34, buffer) == 6);
    assert(memcmp(buffer, "\\u6C34", 6) == 0);
    assert(has_json_encode_unicode(0x10000, buffer) == 12);
    assert(memcmp(buffer, "\\uD800\\uDC00", 12) == 0);
    assert(has_json_encode_unicode(0x64321, buffer) == 12);
    assert(memcmp(buffer, "\\uD950\\uDF21", 12) == 0);
    assert(has_json_encode_unicode(0x1D11E, buffer) == 12);
    assert(memcmp(buffer, "\\uD834\\uDD1E", 12) == 0);
    assert(has_json_encode_unicode(0x10FFFD, buffer) == 12);
    assert(memcmp(buffer, "\\uDBFF\\uDFFD", 12) == 0);
    /* printf("%s\n", buffer); */

    /* Gathered from Description http://en.wikipedia.org/wiki/UTF-8 */
    buffer[0] = '\xC2'; buffer[1] = '\xA2';
    assert(decode_utf8(buffer, 2, &cp) == 2);
    assert(cp == 0xA2);
    buffer[0] = '\xE2'; buffer[1] = '\x82'; buffer[2] = '\xAC';
    assert(decode_utf8(buffer, 3, &cp) == 3);
    assert(cp == 0x20AC);
    buffer[0] = '\xF0'; buffer[1] = '\xA4'; buffer[2] = '\xAD'; buffer[3] = '\xA2';
    assert(decode_utf8(buffer, 4, &cp) == 4);
    assert(cp == 0x024B62);

    return 0;
}
