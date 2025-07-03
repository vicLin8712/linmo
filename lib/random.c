/* libc: Random number generation. */

#include <lib/libc.h>

/* Global state for the legacy random() function. */
static struct random_data _g_rand_data = {0xBAADF00Du};

/* xorshift32 PRNG step function: updates state and returns next value. */
static inline uint32_t prng_step(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

/* Seeds the global random number generator. Seed 0 is remapped to 1. */
void srand(uint32_t seed)
{
    _g_rand_data.state = seed ? seed : 1U;
}

/* Legacy interface:
 * returns a pseudo-random value in [0, RAND_MAX] using the
 * global state.
 */
int32_t random(void)
{
    return (int32_t) ((prng_step(&_g_rand_data.state) >> 17) & RAND_MAX);
}

/* Re-entrant random number generator.
 * @buf    – pointer to caller-supplied state (must have been seeded).
 * @result – where to store the next value in [0, RAND_MAX].
 * Returns 0 on success, -1 on bad pointer.
 */
int random_r(struct random_data *buf, int32_t *result)
{
    if (!buf || !result)
        return -1;

    if (buf->state == 0)
        buf->state = 1u;

    /* Compute and store the next random value. */
    *result = (int32_t) ((prng_step(&buf->state) >> 17) & RAND_MAX);
    return 0; /* Success. */
}
