#include "has_json.h"

#include "jsmn/jsmn.h"
#include "jsmn/jsmn.c"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define ESTIMATOR_TRESHOLD 2048

static size_t has_json_token_testimator(const char *a, size_t l)
{
    size_t i, s = 0;
    for(i = 0; i < l; i++) {
        char c = a[i];
        if(c == '{' || c == '}' || c == ',' ||
           c == '[' || c == ']' || c == '"') {
            s += 1;
        }
    }
    return s / 2;
}

static has_t *has_json_build(jsmntok_t *tokens, size_t cur, size_t max,
                             const char *buffer, size_t *consumed)
{
    has_t *r = NULL;
    size_t count = 0;
    int i, error = 0;

    if (tokens[cur].end < 0 || tokens[cur].start < 0) {
        return NULL;
    }

    switch (tokens[cur].type) {
        case JSMN_PRIMITIVE:
            r = has_string_new((char *) buffer + tokens[cur].start,
                               tokens[cur].end - tokens[cur].start);
            /* TODO */
            count++;
            break;
        case JSMN_STRING:
            r = has_string_new((char *) buffer + tokens[cur].start,
                               tokens[cur].end - tokens[cur].start);
            count++;
            break;
        case JSMN_ARRAY:
            r = has_array_new(tokens[cur].size);
            count++;
            for(i = 0; i < tokens[cur].size; i++) {
                has_t *e = has_json_build(tokens, cur + count, max, buffer, &count);
                if(e) {
                    has_array_push(r, e);
                } else {
                    error = 1;
                    break;
                }
            }
            break;
        case JSMN_OBJECT:
            r = has_hash_new(tokens[cur].size);
            count++;
            for(i = 0; i < tokens[cur].size; i+= 2) {
                has_t *k = has_json_build(tokens, cur + count, max, buffer, &count);
                if(k) {
                    if(k->type == has_string) {
                        has_t *v = has_json_build(tokens, cur + count, max, buffer, &count);
                        has_hash_set(r, k->value.string.pointer, k->value.string.size, v);
                    }
                    has_free(k);
                }
            }
            break;
        default:
            break;
            /* Unknown type */
    }
    if(consumed && count > 0) {
        *consumed += count;
    }

    if(error && r) {
        has_free(r);
    }

    return r;
}


has_t *has_json_parse(const char *buffer)
{
    jsmn_parser parser;
    jsmntok_t *tokens;
    size_t size = strlen(buffer);
    size_t max_tokens = size < ESTIMATOR_TRESHOLD ? ESTIMATOR_TRESHOLD / 2 :
        has_json_token_testimator(buffer, size);
    has_t *r = NULL;

    if((tokens = malloc(max_tokens * sizeof(jsmntok_t))) == NULL) {
        return NULL;
    }
    jsmn_init_parser(&parser, buffer, tokens, max_tokens);

    if(jsmn_parse(&parser) != 0) {
        free(tokens);
        return NULL;
    }

    r = has_json_build(tokens, 0, max_tokens, buffer, NULL);
    free(tokens);
    return r;
}

