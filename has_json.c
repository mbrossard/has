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

int encode_utf8(int32_t codepoint, char *output)
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

unsigned int decode_4hex(const char * hex)
{
    unsigned int i, r = 0;

    for (i = 0 ; i < 4 ; i++) {
        unsigned char c = hex[i];

        if ((c >= '0') && (c <= '9')) c -= '0';
        else if ((c >= 'A') && (c <= 'F')) c -= 'A';
        else if ((c >= 'a') && (c <= 'a')) c -= 'a';
        if(c & 0xF0) return 0xFFFFFFFF;
        r = (r << 4) | c;
    }
    return r;
}

int has_json_decode_unicode(char *input, unsigned int length,
                            unsigned int *codepoint)
{
    unsigned int point, read = 4;
    if(input == NULL || codepoint == NULL ||
       (length < 4) || (point = decode_4hex(input)) == 0xFFFFFFFF) {
        return -1;
    }

    if ((point & 0xFC00) == 0xD800) {
        /* It's a surrogate pair */
        if(length < 10) { /* Do we have enough bytes remaining */
            return -1;
        }
        if (input[5] == '\\' && input[6] == 'u') {
            /* Low surrogate */
            unsigned int surrogate;
            if((surrogate = decode_4hex(input + 7)) == 0xFFFFFFFF) {
                return -1;
            }
            read += 6;
            if((point & 0xFC00) != 0xDC00) {
                /* Fail ? */
            }
            point = 0x10000 +
                (((point & 0x3FF) << 10) | (surrogate & 0x3FF));
        } else {
            return -1;
        }
    }
    *codepoint = point;
    return read;
}

int has_json_string_decode(char *input, size_t length,
                           char **output, size_t *newlen)
{
    size_t processed = 0, written = 0;
    char *n = NULL;
    int i, j;

    if(output == NULL || newlen == NULL) {
        return -1;
    }

    /* Find first reverse solidus */
    for(i = 0; input[i] != '\\' && i < length; i++) /* Nothing */ ;

    /* String doesn't require decoding */
    if(i == length) {
        *output = NULL;
        *newlen = length;
        return 0;
    }

    n = malloc(length);
    while(processed < length) {
        for(i = processed; input[i] != '\\' && i < length; i++) /* Nothing */ ;
        if(i == length) {
            j = length - processed;
        } else {
            j = i - processed - 1;
        }
        memcpy(n + written, input + processed, j);
        processed += j;
        written += j;
        if(processed < length && input[processed] == '\\') {
            char *add = NULL, c = input[processed + 1];
            if(c == '"') add = "\"";
            else if(c == '\\') add = "\\";
            else if(c == 'b') add = "\b";
            else if(c == 'f') add = "\f";
            else if(c == 'n') add = "\n";
            else if(c == 'r') add = "\r";
            else if(c == 't') add = "\t";
            else if(c == 'u') {
                unsigned int point;
                processed += 2;
                if((j = has_json_decode_unicode
                    (input + processed, length - processed, &point)) - 1) {
                    /* FIXME: Fail */
                } else {
                    processed += j;
                }

                if((j = encode_utf8(point, n + written)) == -1) {
                    /* FIXME: Fail */
                } else {
                    written += j;
                }
            } else {
                /* FIXME: Fail */
            }
            if(add) {
                processed += 2;
                n[written++] = add[0];
            }
        }
    }

    *output = n;
    *newlen = written;

    return 0;
}

has_t *has_json_build_string(char *ptr, size_t len, bool decode)
{
    char *s;
    size_t l;

    if(!decode) {
        return has_string_new(ptr, len);
    }

    if(has_json_string_decode(ptr, len, &s, &l) < 0) {
        return NULL;
    }
    return (s == NULL) ? has_string_new(ptr, len) :
        has_string_new_o(s, l, true);
}

has_t *has_json_decode_primitive(char *s, size_t l)
{
    has_t *r = NULL;
    char c = s[0];

    if(c == 'n') {
        return (l == 4 && (memcmp(s, "null", 4) == 0)) ?
            (has_new(1)) : NULL;
    } else if(c == '-' || (c >= '0' && c <= '9')) {
        int i;
        /* We have a number */
        for(i = 0; (s[i] != '.' && s[i] != 'e') && i < l; i++) /* Nothing */;
        if(i <= l) {
            /* double fp; */
            /* Floating point */
        } else {
            /* int32_t integer; */
            /* Integer */
        }
    } else if(c == 't') {
        return (l == 4 && (memcmp(s, "true", 4) == 0)) ?
            (has_bool_new(true)) : NULL;
    } else if(c == 'f') {
        return (l == 5 && (memcmp(s, "false", 5) == 0)) ?
            (has_bool_new(false)) : NULL;
    }

    return r;
}

static has_t *has_json_build(jsmntok_t *tokens, size_t cur, size_t max,
                             const char *buffer, size_t *processed, bool decode)
{
    has_t *r = NULL;
    size_t count = 0;
    int i, error = 0;

    if (tokens[cur].end < 0 || tokens[cur].start < 0) {
        return NULL;
    }

    switch (tokens[cur].type) {
        case JSMN_PRIMITIVE:
            r = has_json_decode_primitive((char *) buffer + tokens[cur].start,
                                          tokens[cur].end - tokens[cur].start);
            count++;
            break;
        case JSMN_STRING:
            has_json_build_string((char *) buffer + tokens[cur].start,
                                  tokens[cur].end - tokens[cur].start, decode);
            count++;
            break;
        case JSMN_ARRAY:
            r = has_array_new(tokens[cur].size);
            count++;
            for(i = 0; i < tokens[cur].size && error == 0; i++) {
                has_t *e = has_json_build(tokens, cur + count, max,
                                          buffer, &count, decode);
                if(e == NULL || has_array_push(r, e) == NULL) {
                    error = 1;
                }
            }
            break;
        case JSMN_OBJECT:
            r = has_hash_new(tokens[cur].size);
            count++;
            for(i = 0; i < tokens[cur].size && error == 0; i+= 2) {
                has_t *k = has_json_build(tokens, cur + count, max,
                                          buffer, &count, decode);
                if(k) {
                    has_t *v = has_json_build(tokens, cur + count, max,
                                              buffer, &count, decode);
                    if(v == NULL || has_hash_add(r, k, v) == NULL) {
                        error = 1;
                    }
                }
            }
            break;
        default:
            /* Unknown type */
            break;
    }
    if(processed && count > 0) {
        *processed += count;
    }

    if(error && r) {
        has_free(r);
    }

    return r;
}

has_t *has_json_parse(const char *buffer, bool decode)
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

    r = has_json_build(tokens, 0, max_tokens, buffer, NULL, decode);
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
    has_json_serializer_buffer_t *b;

    if(s == NULL) {
        return -1;
    }
    b = s->pointer;
    if(size > b->size - b->current) {
        /* Handle resizing */
        return -1;
    }

    memcpy(b->buffer + b->current, value, size);
    b->current += size;

    return 0;
}

int has_json_string_encode(has_json_serializer_t *s,
                           const char *input, size_t length)
{
    unsigned int start = 0, stop = 0;

    while(stop < length) {
        unsigned char c = input[stop];
        char buffer[6] = { '\\', 'u', '0', '0', 0, 0 };
        char *add = NULL;
        size_t l = 2;

        if(c == '"') add = "\"";
        else if(c == '\\') add = "\\\\";
        else if(c == '\b') add = "\\b";
        else if(c == '\f') add = "\\f";
        else if(c == '\n') add = "\\n";
        else if(c == '\r') add = "\\r";
        else if(c == '\t') add = "\\t";
        else if(c < 32) {
            const char * hexchar = "0123456789ABCDEF";
            buffer[4] = hexchar[c >> 4];
            buffer[5] = hexchar[c & 0x0F];
            add = buffer;
            l = 6;
        }
        if(add) {
            if((s->outputter(s->pointer, input + start, stop - start)) < 0 ||
               (s->outputter(s->pointer, add, l)) < 0) {
                return -1;
            }
            start = stop + 1;
        }
        stop++;
    }

    if((start < stop) &&
       ((s->outputter(s->pointer, input + start, stop - start)) < 0)) {
        return -1;
    }

    return 0;
}

int has_json_serialize_string(has_json_serializer_t *s,
                              const char *value, size_t size)
{
    if(s == NULL || s->outputter == NULL) {
        return -1;
    }

    if(s->encode) {
        return (((s->outputter)(s->pointer, "\"", 1) == 0) &&
                (has_json_string_encode(s, value, size) == 0) &&
                ((s->outputter)(s->pointer, "\"", 1) == 0)) ? 0 : -1;
    } else {
        return (((s->outputter)(s->pointer, "\"", 1) == 0) &&
                ((s->outputter)(s->pointer, value, size) == 0) &&
                ((s->outputter)(s->pointer, "\"", 1) == 0)) ? 0 : -1;
    }
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

int has_json_serialize(has_t *input, char **output, size_t *size, int encode)
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
