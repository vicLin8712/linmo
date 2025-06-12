#pragma once

#include <types.h>

/* Memory allocation constants */
#define MALLOC_MIN_SIZE 4          /* Minimum allocation size in bytes */
#define MALLOC_MAX_SIZE 0x7FFFFFFF /* Maximum safe allocation size */

/* Memory allocation functions */
void free(void *ptr);
void *malloc(uint32_t size);
void *calloc(uint32_t nmemb, uint32_t size);
void *realloc(void *ptr, uint32_t size);

/* Heap management */
void mo_heap_init(size_t *zone, uint32_t len);
