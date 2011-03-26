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

/*
 * @brief Parses JSON-encoded text into a has_t structure
 * @param buffer NULL-terminated text
 * @return A pointer to a has_t structure or NULL in case of failure.
 *
 * The input string must be a valid UTF-8 encoded string. The strings
 * in the resulting has_t structure will not be decoded.
 */
has_t *has_json_parse(const char *buffer);

int has_json_serialize(has_t *input, char **output, size_t *size);

#ifdef __cplusplus
};
#endif

#endif
