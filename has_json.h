/*
  (c) Mathias Brossard <mathias@brossard.org>
*/

#ifndef _HAS_JSON_H
#define	_HAS_JSON_H

#include "has.h"

#ifdef __cplusplus
extern "C" {
#endif

has_t *has_json_parse(const char *buffer);

#ifdef __cplusplus
};
#endif

#endif
