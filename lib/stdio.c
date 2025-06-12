/* Minimal stdio hooks.
 *
 * Default handlers do nothing (or return error codes) so the kernel can run
 * even if the board code forgets to install real console hooks. These hooks
 * allow a consistent I/O interface regardless of the underlying hardware.
 */

#include "private/stdio.h"

/* Ignores output character, returns 0 (success). */
static int stdout_null(int c)
{
    (void) c;
    return 0;
}

/* Returns -1 to indicate no input is available. */
static int stdin_null(void)
{
    return -1;
}

/* Returns 0 to indicate no input is ready. */
static int poll_null(void)
{
    return 0;
}

/* Active hooks, initialized to default no-op handlers.
 * These pointers will be updated by board-specific initialization code.
 */
static int (*stdout_hook)(int) = stdout_null;
static int (*stdin_hook)(void) = stdin_null;
static int (*poll_hook)(void) = poll_null;

/* Hook installers: Register the provided I/O functions. */
void _stdout_install(int (*hook)(int))
{
    stdout_hook = (hook) ? hook : stdout_null;
}

void _stdin_install(int (*hook)(void))
{
    stdin_hook = (hook) ? hook : stdin_null;
}

void _stdpoll_install(int (*hook)(void))
{
    poll_hook = (hook) ? hook : poll_null;
}

/* I/O helpers: Dispatch to the currently installed hooks. */

/* Calls the registered stdout hook to output a character. */
int _putchar(int c)
{
    return stdout_hook(c);
}

/* Calls the registered stdin hook to get a character.
 * This function blocks (busy-waits) until input is available.
 */
int _getchar(void)
{
    int ch;
    while ((ch = stdin_hook()) < 0)
        ; /* Spin loop, effectively waiting for input. */
    return ch;
}

/* Calls the registered poll hook to check for input readiness. */
int _kbhit(void)
{
    return poll_hook();
}
