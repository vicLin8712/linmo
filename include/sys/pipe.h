#pragma once

/* Pipe descriptor
 *
 * The buffer size is forced to the next power-of-two so we can mask
 * the head/tail indices instead of performing expensive modulus.
 */
typedef struct {
    char *buf;              /* data buffer */
    uint16_t mask;          /* capacity-1  (size is power of 2) */
    volatile uint16_t head; /* read index */
    volatile uint16_t tail; /* write index */
    volatile uint16_t used; /* bytes currently stored */
} pipe_t;

/* Constructor / destructor */
pipe_t *mo_pipe_create(uint16_t size);
int32_t mo_pipe_destroy(pipe_t *pipe);

void mo_pipe_flush(pipe_t *pipe);
/* bytes currently in pipe */
int32_t mo_pipe_size(pipe_t *pipe);

/* Blocking I/O (runs in task context) */
int32_t mo_pipe_read(pipe_t *pipe, char *data, uint16_t size);
int32_t mo_pipe_write(pipe_t *pipe, const char *data, uint16_t size);

/* Non-blocking I/O (returns partial count on short read/write) */
int32_t mo_pipe_nbread(pipe_t *pipe, char *data, uint16_t size);
int32_t mo_pipe_nbwrite(pipe_t *pipe, const char *data, uint16_t size);
