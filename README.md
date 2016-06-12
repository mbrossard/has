has
===

[![Build Status](https://travis-ci.org/mbrossard/has.svg?branch=master)](https://travis-ci.org/mbrossard/has)

has (hash, array, scalar) is a C library for people used to dynamic
languages.

Currently it's in an unfinished, not completely documented and buggy
state. Don't use it in production.

Features:

  * No dependencies
  * Works with arbitrary keys and strings
  * Support for zero-copy of string data

has_json
========

Additional module to make conversion between has and JSON encoded
strings.

Features:

  * Only depends on jsmn (included), easily embedded.
  * Support for UTF-8 string handling.
  * Support for zero-copy of strings data when parsing JSON (except
    when decoding is requested and necessary).
