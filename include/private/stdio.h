/* Lightweight stdio hook layer
 *
 * This header defines the interface for plugging in character-based I/O
 * routines. Kernel code uses these hooks ('_putchar', '_getchar', '_kbhit') to
 * interact with the console, allowing board-specific implementations to be
 * registered at runtime. Default handlers ensure the system can boot even
 * without a console.
 */

#pragma once

/* Hook Installation Functions
 *
 * These functions allow registration of device-specific I/O functions at
 * runtime. Typically called during hardware initialization to connect the
 * kernel I/O system to actual hardware drivers.
 */

/* Install stdout hook for character output */
void _stdout_install(int (*hook)(int));

/* Install stdin hook for character input */
void _stdin_install(int (*hook)(void));

/* Install polling hook for input readiness check */
void _stdpoll_install(int (*hook)(void));

/* Hook Interface Functions
 *
 * These functions provide a consistent interface to the installed hooks,
 * dispatching to registered implementations or default stubs if no hooks
 * have been installed.
 */

/* Blocking single-byte output */
int _putchar(int c);

/* Blocking single-byte input (busy-wait) */
int _getchar(void);

/* Non-blocking poll for input readiness */
int _kbhit(void);
