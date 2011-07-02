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
#include <stdio.h>

#define ESTIMATOR_TRESHOLD 2048

static const char * hexchar = "0123456789ABCDEF";

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

    return i;
}

int decode_utf8(const char *input, size_t l, int32_t *codepoint)
{
    int i = 0, j  = 0;
    unsigned char c;
    int32_t cp = 0;

    if(codepoint == NULL || l < 1) {
        return -1;
    }

    c = *((const unsigned char *)input);
    if(c < 0x80) {                  /* 1 byte */
        *codepoint = c;
        return 1;
    } else if((c & 0xE0) == 0xC0) { /* 2 bytes */
        cp = c & 0x1F;
        j = 2;
    } else if((c & 0xF0) == 0xE0) { /* 3 bytes */
        cp = c & 0x0F;
        j = 3;
    } else if((c & 0xF8) == 0xF0) { /* 4 bytes */
        cp = c & 0x07;
        j = 4;
    }

    if(l < j) {
        return -1;
    }

    for(i = 1; i < j; i++) {
        c = *((const unsigned char *)input + i);
        if((c & 0xC0) != 0x80) {
            return -1;
        }
        cp = (cp << 6) | (c & 0x7F);
    }
    *codepoint = cp;
    return j;
}

unsigned int decode_4hex(const char * hex)
{
    unsigned int i, r = 0;

    for (i = 0 ; i < 4 ; i++) {
        unsigned char c = hex[i];

        if ((c >= '0') && (c <= '9')) c -= '0';
        else if ((c >= 'A') && (c <= 'F')) c -= 'A' - 10;
        else if ((c >= 'a') && (c <= 'a')) c -= 'a' - 10;
        if(c & 0xF0) return 0xFFFFFFFF;
        r = (r << 4) | c;
    }
    return r;
}

int has_json_decode_unicode(char *input, unsigned int length,
                            int32_t *codepoint)
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

        if (input[4] == '\\' && input[5] == 'u') {
            /* Low surrogate */
            unsigned int surrogate;
            if((surrogate = decode_4hex(input + 6)) == 0xFFFFFFFF) {
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

void escape_utf8_point(int32_t point, char *output)
{
    output[0] = '\\';
    output[1] = 'u';
    output[2] = hexchar[(point >> 12) & 0xF];
    output[3] = hexchar[(point >>  8) & 0xF];
    output[4] = hexchar[(point >>  4) & 0xF];
    output[5] = hexchar[(point)       & 0xF];
}

int has_json_encode_unicode(uint32_t codepoint, char *output)
{

    if(codepoint < 0x10000) {
        escape_utf8_point(codepoint, output);
        return 6;
   } else if(codepoint < 0x10FFFF) {
        uint32_t low, high;
        high = 0xD800 + ((codepoint - 0x10000) >> 10);
        low  = 0xDC00 + ((codepoint - 0x10000) & 0x3FF);
        escape_utf8_point(high, output);
        escape_utf8_point( low, output + 6);
        return 12;
    } else {
        return -1;
    }
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

    n = malloc(length); /* Decoded string will always be smaller */
    while(processed < length) {
        for(i = processed; input[i] != '\\' && i < length; i++) /* Nothing */ ;
        if(i == length) {
            j = length - processed;
        } else {
            j = i - processed;
        }

        memcpy(n + written, input + processed, j);
        processed += j;
        written += j;
        if(processed < length && input[processed] == '\\') {
            char c = input[processed + 1];
            if(c == 'u') {
                int32_t point = 0;
                processed += 2;
                if((j = has_json_decode_unicode
                    (input + processed, length - processed, &point)) == - 1) {
                    return -1;
                }
                processed += j;
                if((j = encode_utf8(point, n + written)) == -1) {
                    return -1;
                }
                written += j;
            } else {
                if(c == '"') c = '\"';
                else if(c == '\\') c = '\\';
                else if(c == 'b') c = '\b';
                else if(c == 'f') c = '\f';
                else if(c == 'n') c = '\n';
                else if(c == 'r') c = '\r';
                else if(c == 't') c = '\t';
                else return -1;
                processed += 2;
                n[written++] = c;
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
        char buffer[32], *tmp;
        int i;
        /* We have a number */
        if(l > (sizeof(buffer) - 1)) {
            return NULL;
        }
        memcpy(buffer, s, l);
        buffer[l] = '\0';
        for(i = 0; (s[i] != '.' && s[i] != 'e') && i < l; i++) /* Nothing */;
        if(i < l) {
            /* Floating point */
            double fp = strtod(buffer, &tmp);
            if(buffer != tmp) {
                r = has_double_new(fp);
            }
        } else {
            /* Integer */
            int32_t integer = strtol(buffer, &tmp, 10);
            if(buffer != tmp) {
                r = has_int_new(integer);
            }
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
            r = has_json_build_string((char *) buffer + tokens[cur].start,
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

typedef struct has_json_serializer_t has_json_serializer_t;

typedef int (*has_json_outputter) (has_json_serializer_t *s,
                                   const char *value, size_t size);

struct has_json_serializer_t {
    has_json_outputter outputter;
    void *pointer;
    int flags;
    int indent;
};

typedef struct {
    char *buffer;
    size_t size;
    size_t current;
    bool owner;
} has_json_serializer_buffer_t;

int has_json_serializer_buffer_outputter(has_json_serializer_buffer_t *b,
                                         const char *value, size_t size)
{
    if(b == NULL) {
        return -1;
    }

    if(size > (b->size - b->current)) {
        char *t;
        size_t s = b->size * 2;
        while((s - b->current) < size) s = s * 2;
        if(!b->owner || (t = realloc(b->buffer, s)) == NULL) {
            return -1;
        }
        b->buffer = t;
        b->size = s;
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
        char buffer[6] = { '\\', 'u', '0', '0', 0, 0 }, unicode[12];
        char *add = NULL;
        int l = 2, m = 1;

        if(c == '"') {
            add = "\\\"";
        } else if(c == '\\') {
            add = "\\\\";
        } else if(s->flags & HAS_JSON_SERIALIZE_ENCODE) {
            if(c == '\b') add = "\\b";
            else if(c == '\f') add = "\\f";
            else if(c == '\n') add = "\\n";
            else if(c == '\r') add = "\\r";
            else if(c == '\t') add = "\\t";
            else if(c < 32) {
                buffer[4] = hexchar[c >> 4];
                buffer[5] = hexchar[c & 0x0F];
                add = buffer;
                l = 6;
            } else if(c > 0x80) {
                int32_t codepoint = 0;
                if(((m = decode_utf8(input + stop, length - stop, &codepoint)) < 0) ||
                   ((l = has_json_encode_unicode(codepoint, unicode)) < 0)) {
                    return -1;
                }
                add = unicode;
            }
        }
        if(add) {
            if((stop - start) > 0) {
                if((s->outputter(s->pointer, input + start, stop - start)) < 0) {
                    return -1;
                }
            }
            if((s->outputter(s->pointer, add, l)) < 0) {
                return -1;
            }
            start = stop + m;
        }
        stop += m;
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

    if(s->flags & HAS_JSON_SERIALIZE_ENCODE) {
        return (((s->outputter)(s->pointer, "\"", 1) == 0) &&
                (has_json_string_encode(s, value, size) == 0) &&
                ((s->outputter)(s->pointer, "\"", 1) == 0)) ? 0 : -1;
    } else {
        return (((s->outputter)(s->pointer, "\"", 1) == 0) &&
                ((s->outputter)(s->pointer, value, size) == 0) &&
                ((s->outputter)(s->pointer, "\"", 1) == 0)) ? 0 : -1;
    }
}


#define INDENT(s) \
    if(s->flags & HAS_JSON_SERIALIZE_PRETTY) { \
        int i; \
        for(i = 0; i < s->indent ; i++) { \
            (s->outputter)(s->pointer, "  ", 2); \
        } \
    }
#define PRETTY(s, c, a)  \
    if(s->flags & HAS_JSON_SERIALIZE_PRETTY) { \
        (s->outputter)(s->pointer, c, 1); \
        s->indent += a; \
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
        INDENT(s);
        r = (s->outputter)(s->pointer, "null", 4);
    } else if(type == has_walk_hash_begin) {
        r = (s->outputter)(s->pointer, "{", 1);
        PRETTY(s, "\n", 1);
    } else if(type == has_walk_hash_key) {
        INDENT(s);
        r = ((has_json_serialize_string(s, string, size) == 0) &&
             ((s->outputter)(s->pointer, ":", 1) == 0)) ? 0 : -1;
        PRETTY(s, " ", 0);
    } else if(type == has_walk_hash_value_end) {
        if(index < cur->value.hash.count - 1) {
            r = (s->outputter)(s->pointer, ",", 1);
            PRETTY(s, "\n", 0);
        }
    } else if(type == has_walk_hash_end) {
        PRETTY(s, "\n", -1);
        INDENT(s);
        r = (s->outputter)(s->pointer, "}", 1);
    } else if(type == has_walk_array_begin) {
        r = (s->outputter)(s->pointer, "[", 1);
        PRETTY(s, "\n", 1);
    } else if(type == has_walk_array_entry_begin) {
        INDENT(s);
    } else if(type == has_walk_array_entry_end) {
        if(index < cur->value.array.count - 1) {
            r = (s->outputter)(s->pointer, ",", 1);
            PRETTY(s, "\n", 0);
        }
    } else if(type == has_walk_array_end) {
        PRETTY(s, "\n", -1);
        INDENT(s);
        r = (s->outputter)(s->pointer, "]", 1);
    } else if(type == has_walk_string) {
        r = has_json_serialize_string(s, string, size);
    } else if(type == has_walk_other) {
        if(cur->type == has_null) {
            r = (s->outputter)(s->pointer, "null", 4);
        } else if(cur->type == has_integer) {
            char buffer[64];
            int l = snprintf(buffer, sizeof(buffer) - 1, "%d", cur->value.integer);
            r = (l > 0) ? (s->outputter)(s->pointer, buffer, l) : -1;
        } else if(cur->type == has_double) {
            char buffer[64];
            int l = snprintf(buffer, sizeof(buffer) - 1, "%f", cur->value.fp);
            r = (l > 0) ? (s->outputter)(s->pointer, buffer, l) : -1;
        } else if(cur->type == has_boolean) {
            r = (cur->value.boolean) ? (s->outputter)(s->pointer, "true", 4) :
                (s->outputter)(s->pointer, "false", 5);
        }
    }

    return r;
}

int has_json_serialize(has_t *input, char **output, size_t *size, int flags)
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
        sb.owner = 1;
    }
    sb.current = 0;

    s.flags = flags;
    s.indent = 0;
    s.outputter = (has_json_outputter)has_json_serializer_buffer_outputter;
    s.pointer = &sb;

    if((has_walk(input, has_json_serializer_walker, &s) == 0) &&
       (has_json_serializer_buffer_outputter(&sb, "\0", 1) == 0)) {
        *output = sb.buffer;
        *size = sb.current - 1;
    } else {
        free(sb.buffer);
        return -1;
    }
    return 0;
}
