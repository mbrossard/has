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
    char *buffer = 
        "{ \"alpha\": 1, \"bravo\": null,"
        "\"x-ray\": [0, 1, 2, 3], "
        "\"delta\": { "
        "\"echo\": 1.0,"
        "\"foxtrot\": 3.1415"
        "}}";
    has_t *json1, *json2;
    char *out1 = NULL, *out2 = NULL;
    size_t l1, l2;

    assert((json1 = has_json_parse(buffer, false)) != NULL);
    assert((has_json_serialize(json1, &out1, &l1, 1) == 0));
    assert((json2 = has_json_parse(out1, false)) != NULL);
    assert((has_json_serialize(json2, &out2, &l2, 1) == 0));
    assert(l1 == l2);
    /* printf("%s\n\n%s", out1, out2); */
    assert(memcmp(out1, out2, l1) == 0);
    has_free(json1);
    has_free(json2);
    free(out1);
    free(out2);
    return 0;
}
