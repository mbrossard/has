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
 */
has_t *has_json_parse(const char *buffer);

#ifdef __cplusplus
};
#endif

#endif
