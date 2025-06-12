/* syscall numbers and user-space wrappers */

#pragma once

#include <sys/stat.h>

/* syscall table: name, number, return-type, argument-list */
#define SYSCALL_TABLE                                        \
    /* POSIX-style stubs */                                  \
    _(fork, 1, int, (void) )                                 \
    _(exit, 2, int, (int status))                            \
    _(wait, 3, int, (int *status))                           \
    _(pipe, 4, int, (int fildes[2]))                         \
    _(kill, 5, int, (int pid, int sig))                      \
    _(execve, 6, int, (char *name, char **argv, char **env)) \
    _(dup, 7, int, (int oldfd))                              \
    _(getpid, 8, int, (void) )                               \
    _(sbrk, 9, int, (int incr))                              \
    _(usleep, 10, int, (int usec))                           \
    _(stat, 11, int, (char *file, struct stat *st))          \
    _(open, 12, int, (char *path, int flags))                \
    _(close, 13, int, (int fd))                              \
    _(read, 14, int, (int fd, char *buf, int len))           \
    _(write, 15, int, (int fd, char *buf, int len))          \
    _(lseek, 16, int, (int fd, int off, int whence))         \
    _(chdir, 17, int, (const char *path))                    \
    _(mknod, 18, int, (const char *path, int mode, int dev)) \
    _(unlink, 19, int, (char *name))                         \
    _(link, 20, int, (char *old, char *new))                 \
    /* 21-31 reserved */                                     \
    /* Linmo syscalls */                                     \
    _(tadd, 32, int, (void *task, int stack_sz))             \
    _(tcancel, 33, int, (int id))                            \
    _(tyield, 34, int, (void) )                              \
    _(tdelay, 35, int, (int ticks))                          \
    _(tsuspend, 36, int, (int id))                           \
    _(tresume, 37, int, (int id))                            \
    _(tpriority, 38, int, (int id, int prio))                \
    _(tid, 39, int, (void) )                                 \
    _(twfi, 40, int, (void) )                                \
    _(tcount, 41, int, (void) )                              \
    _(ticks, 42, int, (void) )                               \
    _(uptime, 43, int, (void) )

#define _(name, num, rettype, arglist) SYS_##name = num,
typedef enum { SYSCALL_TABLE SYS_COUNT } mo_syscall_t;
#undef _

/* Generic trap (weak – arch may override) */
int syscall(int num, void *arg1, void *arg2, void *arg3);

/* Forward declarations of user wrappers */
#define _(name, num, rettype, arglist) rettype sys_##name arglist;
SYSCALL_TABLE
#undef _
