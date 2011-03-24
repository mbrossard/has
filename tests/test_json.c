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

void has_dump(has_t *e, int tab) {
    char spcs[64];
    int i;

    if(e == NULL) {
        return;
    }
    memset(spcs, ' ', 2 * tab);
    spcs[2 * tab + 1] = '\0';

    if(e->type == has_hash) {
        int j = 0;
        printf("{\n");
        for(i = 0; i < e->value.hash.size; i++) {
            has_hash_list_t *f;
            for(f = e->value.hash.elements[i]; f;) {
                printf("%s  key[%d] = '", spcs, j);
                has_dump_string(f->key, f->size);
                printf("'\n");
                printf("%s  value[%d] = ", spcs, j);
                has_dump(f->value, tab + 1);
                f = f->next;
                j++;
            }
        }
        printf("%s}\n", spcs);
    } else if(e->type == has_array) {
        printf("[\n");
        for(i = 0; i < e->value.array.count; i++) {
            printf("%s  item[%d] = ", spcs, i);
            has_dump(e->value.array.elements[i], tab + 2);
        }
        printf("%s]\n", spcs);
    } else if(e->type == has_string) {
                printf("\"");
                has_dump_string(e->value.string.pointer,
                                e->value.string.size);
                printf("\"\n");
    }
}

int main(int argc, char **argv)
{
    char *buffer = 
        "{ \"a\": 1, \"b\": null,"
        "\"x\": [0, 1, 2, 3], "
        "\"d\": { "
        "\"e\": 1.0,"
        "\"f\": 3.1415"
        "}";

    has_t *json = has_json_parse(buffer);

    if(json) {
        has_dump(json, 0);
        printf("Success\n");
    } else {
        printf("Failure\n");
    }
    has_free(json);
    return 0;
}
