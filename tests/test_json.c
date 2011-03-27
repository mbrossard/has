/*
  (c) Mathias Brossard <mathias@brossard.org>
*/

#include "has.c"
#include "has_json.c"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
        "{ \"a\": 1, \"b\": null,"
        "\"x\": [0, 1, 2, 3], "
        "\"d\": { "
        "\"e\": 1.0,"
        "\"f\": 3.1415"
        "}}";
    has_t *json;

    if((json = has_json_parse(buffer, false))) {
        has_dump(json);
        printf("Success\n");
    } else {
        printf("Failure\n");
    }
    has_free(json);
    return 0;
}
