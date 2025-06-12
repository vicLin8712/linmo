#include <hal.h>
#include <sys/errno.h>
#include <sys/syscall.h>
#include <sys/task.h>

#include "private/stdio.h"

/* syscall wrappers */
#define _(name, num, rettype, arglist) static rettype _##name arglist;
SYSCALL_TABLE
#undef _

/* syscall table */
#define _(name, num, ret, args) [num] = (void *) _##name,
static const void *syscall_table[SYS_COUNT] = {SYSCALL_TABLE};
#undef _

/* Weak, generic dispatcher */
int syscall(int num, void *a1, void *a2, void *a3)
{
    if (num <= 0 || num >= SYS_COUNT)
        return -ENOSYS;
    return ((int (*)(void *, void *, void *)) syscall_table[num])(a1, a2, a3);
}

static char *_env[1] = {0};
char **environ = _env;
int errno = 0;

/* UNIX syscalls (dummy implementation) */

static int _fork(void)
{
    errno = EAGAIN;
    return -1;
}

static int _exit(int status)
{
    _kill(status, -1);
    while (1)
        ;

    return -1;
}

static int _wait(int *status)
{
    errno = ECHILD;
    return -1;
}

static int _pipe(int fildes[2])
{
    errno = EFAULT;
    return -1;
}

static int _kill(int pid, int sig)
{
    errno = EINVAL;
    return -1;
}

static int _execve(char *name, char **argv, char **env)
{
    errno = ENOMEM;
    return -1;
}

static int _dup(int oldfd)
{
    errno = EBADF;
    return -1;
}

static int _getpid(void)
{
    return 1;
}

static int _sbrk(int incr)
{
    extern uint32_t _end, _stack;
    static char *brk = (char *) &_end;
    char *prev = brk;
    if (brk + incr >= (char *) &_stack) {
        errno = ENOMEM;
        return -1;
    }
    brk += incr;
    return (int) prev;
}

static int _usleep(int usec)
{
    errno = EINTR;
    return 0;
}

static int _stat(char *file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

static int _open(char *path, int flags)
{
    return -1;
}

static int _close(int file)
{
    return -1;
}

static int _read(int file, char *ptr, int len)
{
    int idx;

    for (idx = 0; idx < len; idx++)
        *ptr++ = _getchar();

    return len;
}

static int _write(int file, char *ptr, int len)
{
    int idx;

    for (idx = 0; idx < len; idx++)
        _putchar(*ptr++);

    return len;
}

static int _lseek(int file, int ptr, int dir)
{
    return 0;
}

static int _chdir(const char *path)
{
    errno = EFAULT;
    return -1;
}

static int _mknod(const char *path, int mode, int dev)
{
    errno = EFAULT;
    return -1;
}

static int _unlink(char *name)
{
    errno = ENOENT;
    return -1;
}

static int _link(char *old, char *new)
{
    errno = EMLINK;
    return -1;
}

/* Linmo syscalls (wrapper implementation) */

int _tadd(void *task, int stack_size)
{
    return mo_task_spawn(task, stack_size);
}

int sys_task_add(void *task, int stack_size)
{
    return syscall(SYS_tadd, task, (void *) stack_size, 0);
}

int _tcancel(int id)
{
    return mo_task_cancel(id);
}

int sys_task_cancel(int id)
{
    return syscall(SYS_tcancel, (void *) id, 0, 0);
}

int _tyield(void)
{
    mo_task_yield();

    return 0;
}

int sys_task_yield(void)
{
    syscall(SYS_tyield, 0, 0, 0);

    return 0;
}

int _tdelay(int ticks)
{
    mo_task_delay(ticks);

    return 0;
}

int sys_task_delay(int ticks)
{
    syscall(SYS_tdelay, (void *) ticks, 0, 0);

    return 0;
}

int _tsuspend(int id)
{
    return mo_task_suspend(id);
}

int sys_task_suspend(int id)
{
    return syscall(SYS_tsuspend, (void *) id, 0, 0);
}

int _tresume(int id)
{
    return mo_task_resume(id);
}

int sys_task_resume(int id)
{
    return syscall(SYS_tresume, (void *) id, 0, 0);
}


int _tpriority(int id, int priority)
{
    return mo_task_priority(id, priority);
}

int sys_task_priority(int id, int priority)
{
    return syscall(SYS_tpriority, (void *) id, (void *) priority, 0);
}

int _tid(void)
{
    return mo_task_id();
}

int sys_task_id(void)
{
    return syscall(SYS_tid, 0, 0, 0);
}

int _twfi(void)
{
    mo_task_wfi();

    return 0;
}

int sys_task_wfi(void)
{
    syscall(SYS_twfi, 0, 0, 0);

    return 0;
}

int _tcount(void)
{
    return mo_task_count();
}

int sys_task_count(void)
{
    return syscall(SYS_tcount, 0, 0, 0);
}

int _ticks(void)
{
    return mo_ticks();
}

int sys_ticks(void)
{
    return syscall(SYS_ticks, 0, 0, 0);
}

int _uptime(void)
{
    return mo_uptime();
}

int sys_uptime(void)
{
    return syscall(SYS_uptime, 0, 0, 0);
}
