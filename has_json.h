/*
 * Copyright 2011 Mathias Brossard <mathias@brossard.org>
 */
/**
 * @file has_json.h
 */

#ifndef _HAS_JSON_H
#define	_HAS_JSON_H

#include "has.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parses JSON-encoded text into a has_t structure
 * @param buffer <tt>NULL</tt>-terminated text
 * @param decode Decode strings
 * @return A pointer to a has_t structure or @c NULL in case of failure.
 *
 * The input string must be a valid UTF-8 encoded string. The strings
 * in the resulting has_t structure will be decoded (unescape).
 */
has_t *has_json_parse(const char *buffer, bool decode);

/**
 * @brief Serializes a has_t structure into JSON text
 * @param input  has_t structure to serialize
 * @param output Pointer to serialization buffer
 * @param size   Pointer to size
 * @param encode Encoding options
 * @return 0 if success, -1 in case of failure.
 *
 */
int has_json_serialize(has_t *input, char **output, size_t *size, int encode);

#ifdef __cplusplus
};
#endif

#endif
