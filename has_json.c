/*
 * Copyright 2011 Mathias Brossard <mathias@brossard.org>
 */
/**
 * @file has_json.c
 */

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

int32_t encode_utf8(int32_t codepoint, char *output)
{
    int i = 0;

    if(codepoint < 0 || codepoint > 0x10FFFF || output == NULL) {
        return -1;
    }

    if(codepoint < 0x80) {
        output[i++] = (char) codepoint;
    } else if(codepoint < 0x800) {
        output[i++] = 0xC0 | (char) ((codepoint & 0x0007C0) >> 6);
        output[i++] = 0x80 | (char) ((codepoint & 0x00003F));
    } else if(codepoint < 0x10000) {
        output[i++] = 0xE0 | (char) ((codepoint & 0x00F000) >> 12);
        output[i++] = 0x80 | (char) ((codepoint & 0x000FC0) >> 6);
        output[i++] = 0x80 | (char) ((codepoint & 0x00003F));
    } else {
        output[i++] = 0xF0 | (char) ((codepoint & 0x1C0000) >> 18);
        output[i++] = 0x80 | (char) ((codepoint & 0x03F000) >> 12);
        output[i++] = 0x80 | (char) ((codepoint & 0x000FC0) >> 6);
        output[i++] = 0x80 | (char) ((codepoint & 0x00003F));
    }

    /* This version should be smaller but it's less readable. */
    /*
    if(codepoint < 0x80) {
        output[i++] = (char) codepoint;
    } else {
        if(codepoint < 0x800) {
            output[i++] = 0xC0 | (char) ((codepoint & 0x0007C0) >> 6);
        } else {
            if(codepoint < 0x10000) {
                output[i++] = 0xE0 | (char) ((codepoint & 0x00F000) >> 12);
            } else {
                output[i++] = 0xF0 | (char) ((codepoint & 0x1C0000) >> 18);
                output[i++] = 0x80 | (char) ((codepoint & 0x03F000) >> 12);
            }
            output[i++] = 0x80 | (char) ((codepoint & 0x000FC0) >> 6);
        }
        output[i++] = 0x80 | (char) ((codepoint & 0x00003F));
    }
    */

    return i;
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

typedef struct {
    size_t size;
    bool encode;
} has_json_estimate_t;

int has_json_estimate_walker(has_t *cur, has_walk_t type, int index,
                  const char *string, size_t size, has_t *element,
                  void *pointer)
{
    has_json_estimate_t *e = pointer;

    if(cur == NULL) {
        return -1;
    }

    if(type == has_walk_hash_begin) {
        e->size += 2 +                      /* '{' and '}'             */
            + ( cur->value.hash.count ) * 4 /* key quotes, ':' and ',' */
            - 1;                            /* no ',' after last entry */
    } else if(type == has_walk_hash_key) {
        e->size += size;
        /* FIXME: handle string encoding case */
    } else if(type == has_walk_array_begin) {
        e->size += 2 +                       /* '[' and ']'         */
            + ( cur->value.hash.count ) - 1; /* ',' between entries */
    } else if(type == has_walk_string) {
        e->size += 2 + size;
        /* FIXME: handle string encoding case */
    } else if(type == has_walk_other) {
        /* FIXME: Nothing yet */
    }

    return 0;
}

size_t has_json_estimate(has_t *input, bool encode)
{
    has_json_estimate_t e;
    e.size = 0;
    e.encode = false;
    return (has_walk(input, has_json_estimate_walker, &e) == 0) ? e.size : 0;
}

typedef struct has_json_serializer_t has_json_serializer_t;

typedef int (*has_json_outputter) (has_json_serializer_t *s,
                                   const char *value, size_t size);

struct has_json_serializer_t {
    has_json_outputter outputter;
    void *pointer;
    bool encode;
};

typedef struct {
    char *buffer;
    size_t size;
    size_t current;
    bool owner;
} has_json_serializer_buffer_t;

int has_json_serializer_buffer_outputter(has_json_serializer_t *s,
                                         const char *value, size_t size)
{
    return 0;
}

int has_json_serialize_string(has_json_serializer_t *s,
                              const char *value, size_t size)
{
    if(s == NULL || s->outputter == NULL) {
        return -1;
    }
    
    return (((s->outputter)(s->pointer, "\"", 1) == 0) &&
            ((s->outputter)(s->pointer, value, size) == 0) &&
            ((s->outputter)(s->pointer, "\"", 1) == 0)) ? 0 : -1;
}

int has_json_serializer_walker(has_t *cur, has_walk_t type, int index,
                               const char *string, size_t size, has_t *element,
                               void *pointer)
{
    has_json_serializer_t *s = pointer;
    int r = 0;
    if(s == NULL || s->outputter == NULL) {
        return -1;
    }

    if(cur == NULL) {
        r = (s->outputter)(s->pointer, "null", 4);
    } else if(type == has_walk_hash_begin) {
        r = (s->outputter)(s->pointer, "{", 1);
    } else if(type == has_walk_hash_key) {
        r = ((has_json_serialize_string(s, string, size) == 0) &&
             ((s->outputter)(s->pointer, ":", 1) == 0)) ? 0 : -1;
    } else if(type == has_walk_hash_value_end) {
        if(index < cur->value.hash.count - 1) {
            r = (s->outputter)(s->pointer, ",", 1);
        }
    } else if(type == has_walk_hash_end) {
        r = (s->outputter)(s->pointer, "}", 1);
    } else if(type == has_walk_array_begin) {
        r = (s->outputter)(s->pointer, "[", 1);
    } else if(type == has_walk_array_entry_end) {
        if(index != cur->value.hash.count - 1) {
            r = (s->outputter)(s->pointer, ",", 1);
        }
    } else if(type == has_walk_array_end) {
        r = (s->outputter)(s->pointer, "]", 1);
    } else if(type == has_walk_string) {
        r = has_json_serialize_string(s, string, size);
    } else if(type == has_walk_other) {
        /* Nothing yet */
    }

    return r;
}

int has_json_serialize(has_t *input, char **output, size_t *size)
{
    has_json_serializer_buffer_t sb;
    has_json_serializer_t s;
    if(output == NULL) {
        return -1;
    } else if(output && *output) {
        sb.buffer = *output;
        if(size && *size > 0) {
            sb.size = *size;
        } else {
            return -1;
        }
    } else {
        if((sb.buffer = malloc(4096)) == NULL) {
            return -1;
        }
        sb.size = 4096;
    }
    sb.current = 0;

    s.outputter = has_json_serializer_buffer_outputter;
    s.pointer = &sb;

    if(has_walk(input, has_json_serializer_walker, &s) == 0) {
        *output = sb.buffer;
        *size = sb.current;
    } else {
        return -1;
    }
    return 0;
}
