/* Lightweight stdio hook layer.
 *
 * This header defines the interface for plugging in character-based I/O
 * routines. Kernel code uses these hooks ('_putchar', '_getchar', '_kbhit') to
 * interact with the console, allowing board-specific implementations to be
 * registered at runtime. Default handlers ensure the system can boot even
 * without a console.
 */

#pragma once

/* Hook installers: Allow registration of device-specific I/O functions. */
void _stdout_install(int (*hook)(int));
void _stdin_install(int (*hook)(void));
void _stdpoll_install(int (*hook)(void));

/* Provide a consistent interface to the installed hooks. */
int _putchar(int c); /* Blocking single-byte output. */
int _getchar(void);  /* Blocking single-byte input (busy-wait). */
int _kbhit(void);    /* Non-blocking poll for input readiness */
