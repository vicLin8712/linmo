#include <lib/libc.h>
#include <sys/pipe.h>
#include <sys/task.h>

#include "private/error.h"
#include "private/utils.h"

static inline bool pipe_is_empty(const pipe_t *p)
{
    return p->used == 0;
}

static inline bool pipe_is_full(const pipe_t *p)
{
    return p->used == (p->mask + 1);
}

static inline char pipe_get_byte(pipe_t *p)
{
    char val = p->buf[p->head];
    p->head = (p->head + 1) & p->mask;
    p->used--;
    return val;
}

static inline void pipe_put_byte(pipe_t *p, char c)
{
    p->buf[p->tail] = c;
    p->tail = (p->tail + 1) & p->mask;
    p->used++;
}

pipe_t *mo_pipe_create(uint16_t size)
{
    if (size < 2)
        size = 2;
    if (!ispowerof2(size))
        size = nextpowerof2(size);

    pipe_t *p = malloc(sizeof *p);
    if (!p)
        return NULL;

    p->buf = malloc(size);
    if (!p->buf) {
        free(p);
        return NULL;
    }

    p->mask = size - 1;
    p->head = p->tail = p->used = 0;
    return p;
}

int32_t mo_pipe_destroy(pipe_t *p)
{
    if (!p || !p->buf)
        return ERR_FAIL;

    free(p->buf);
    free(p);
    return ERR_OK;
}

void mo_pipe_flush(pipe_t *p)
{
    CRITICAL_ENTER();
    p->head = p->tail = p->used = 0;
    CRITICAL_LEAVE();
}

int32_t mo_pipe_size(pipe_t *p)
{
    return p ? p->used : -1;
}

/* Blocking read/write */
static void pipe_wait_until_readable(pipe_t *p)
{
    while (1) {
        CRITICAL_ENTER();
        if (!pipe_is_empty(p)) {
            CRITICAL_LEAVE();
            return;
        }
        /* nothing to read – drop critical section and yield CPU */
        CRITICAL_LEAVE();
        mo_task_wfi();
    }
}

static void pipe_wait_until_writable(pipe_t *p)
{
    while (1) {
        CRITICAL_ENTER();
        if (!pipe_is_full(p)) {
            CRITICAL_LEAVE();
            return;
        }
        /* buffer full – yield until space is available */
        CRITICAL_LEAVE();
        mo_task_wfi();
    }
}

/* Blocking read/write */
int32_t mo_pipe_read(pipe_t *p, char *dst, uint16_t len)
{
    if (!p || !dst || len == 0)
        return ERR_FAIL;

    uint16_t i = 0;
    while (i < len) {
        pipe_wait_until_readable(p);

        CRITICAL_ENTER();
        dst[i++] = pipe_get_byte(p);
        CRITICAL_LEAVE();
    }
    return i;
}

int32_t mo_pipe_write(pipe_t *p, const char *src, uint16_t len)
{
    if (!p || !src || len == 0)
        return ERR_FAIL;

    uint16_t i = 0;
    while (i < len) {
        pipe_wait_until_writable(p);

        CRITICAL_ENTER();
        pipe_put_byte(p, src[i++]);
        CRITICAL_LEAVE();
    }
    return i;
}

/* Non-blocking */
int32_t mo_pipe_nbread(pipe_t *p, char *dst, uint16_t len)
{
    if (!p || !dst || len == 0)
        return ERR_FAIL;

    uint16_t i = 0;
    CRITICAL_ENTER();
    while (i < len && !pipe_is_empty(p))
        dst[i++] = pipe_get_byte(p);
    CRITICAL_LEAVE();

    return i;
}

int32_t mo_pipe_nbwrite(pipe_t *p, const char *src, uint16_t len)
{
    if (!p || !src || len == 0)
        return ERR_FAIL;

    uint16_t i = 0;
    CRITICAL_ENTER();
    while (i < len && !pipe_is_full(p))
        pipe_put_byte(p, src[i++]);
    CRITICAL_LEAVE();

    return i;
}
