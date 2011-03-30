/*
  (c) Mathias Brossard <mathias@brossard.org>
*/

#include "has.c"
#include "has_json.c"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void has_dump_string(const char *str, size_t l)
{
    char *s = malloc(l + 1); 
    int i;
    if(!s) {
        return;
    }
    memcpy(s, str, l);
    s[l] = '\0';
    for(i = 0; i < l ; i++) {
        if(s[i] == '\n') s[i] = ' ';
    }
    printf("%s", s);
    free(s);
 }

static char *wspaces(int s)
{
    static char spcs[64];
    if(s > 31) {
        s = 31;
    }
    memset(spcs, ' ', 2 * s);
    spcs[2 * s] = '\0';
    return spcs;
}

int has_walk_dump(has_t *cur, has_walk_t type, int index,
                  const char *string, size_t size, has_t *element,
                  void *pointer)
{
    if(cur == NULL) {
        return -1;
    }

    if(type == has_walk_hash_begin) {
        *((int*)pointer) += 1;
        printf("{\n");
    } else if(type == has_walk_hash_key) {
        printf("%skey[%d] = '", wspaces(*((int *)pointer)), index);
        has_dump_string(string, size);
        printf("'\n");
    } else if(type == has_walk_hash_value_begin) {
        printf("%svalue[%d] = ", wspaces(*((int *)pointer)), index);
    } else if(type == has_walk_hash_end) {
        *((int*)pointer) -= 1;
        printf("%s}\n", wspaces(*((int *)pointer)));
    } else if(type == has_walk_array_begin) {
        *((int*)pointer) += 1;
        printf("[\n");
    } else if(type == has_walk_array_entry_begin) {
        printf("%sentry[%d] = ", wspaces(*((int *)pointer)), index);
    } else if(type == has_walk_array_end) {
        *((int*)pointer) -= 1;
        printf("%s]\n", wspaces(*((int *)pointer)));
    } else if(type == has_walk_string) {
        printf("\"");
        has_dump_string(string, size);
        printf("\"\n");
    } else if(type == has_walk_other) {
        if(cur->type == has_null) {
            printf("null\n");
        }
        /* Nothing yet */
    }

    return 0;
}

void has_dump(has_t *e) {
    int *i = 0;
    has_walk(e, has_walk_dump, &i);
}

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
