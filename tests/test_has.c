/*
  (c) Mathias Brossard <mathias@brossard.org>
*/

#include "has.c"

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

double epoch_double()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + (t.tv_usec * 1.0) / 1000000.0;
}

int main(int argc, char **argv)
{
    int i, j = 1024 * 1024 * 1;
    char *buffer = malloc(8 * j + 4);
#ifndef BENCH
    has_t* h = has_hash_new(j / 4);
    has_t* vals = has_new(j);
#else
    has_t* h = has_hash_new(j * 2);
#endif
    double t1, t2;
    
    t1 = epoch_double();
    for(i = 0; i < j; i++) {
        sprintf(buffer + i * 8, "%08x", i);
#ifndef BENCH
        has_string_init(&(vals[i]), buffer + i * 8, 8, false);
#endif
    }
    t2 = epoch_double();
    printf("Init: %f\n", t2 - t1);
    
    t1 = epoch_double();
    for(i = 0; i < j; i++) {
#ifndef BENCH
        has_hash_set(h, buffer + i * 8, 8, &(vals[i]));
#else
        has_hash_set(h, buffer + i * 8, 8, NULL);
#endif
    }
    t2 = epoch_double();
    printf("Adding: %f\n", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < j; i++) {
        has_hash_get(h, buffer + i * 8, 8);
    }
    t2 = epoch_double();
    printf("Looking up: %f\n", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < j; i++) {
        has_hash_remove(h, buffer + i * 8, 8);
    }
    t2 = epoch_double();
    printf("Individual removal: %f\n", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < j; i++) {
#ifndef BENCH
        has_hash_set(h, buffer + i * 8, 8, &(vals[i]));
#else
        has_hash_set(h, buffer + i * 8, 8, NULL);
#endif
    }
    t2 = epoch_double();
    printf("Adding: %f\n", t2 - t1);

    t1 = epoch_double();
    for(i = 0; i < j; i++) {
#ifndef BENCH
        has_hash_set(h, buffer + i * 8, 8, &(vals[i]));
#else
        has_hash_set(h, buffer + i * 8, 8, NULL);
#endif
    }
    t2 = epoch_double();
    printf("Adding again: %f\n", t2 - t1);


    t1 = epoch_double();
    has_free(h);
    t2 = epoch_double();
    printf("Deleting: %f\n", t2 - t1);

#ifndef BENCH
    free(vals);
#endif
    free(buffer);
    return 0;
}
