/* minimal <sys/stat.h> subset */

#pragma once

/* File-type bit masks */
#define S_IFMT 0170000   /* mask for all file-type bits */
#define S_IFSOCK 0140000 /* socket */
#define S_IFLNK 0120000  /* symbolic link */
#define S_IFREG 0100000  /* regular file */
#define S_IFBLK 0060000  /* block device */
#define S_IFDIR 0040000  /* directory */
#define S_IFCHR 0020000  /* character device */
#define S_IFIFO 0010000  /* FIFO / pipe */

/* POSIX test macros */
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

/* Permission bits (owner / group / others) */
#define S_ISUID 0004000 /* set-user-ID on execution */
#define S_ISGID 0002000 /* set-group-ID on execution */
#define S_ISVTX 0001000 /* sticky bit */

#define S_IRWXU 0000700 /* rwx for owner */
#define S_IRUSR 0000400 /* read by owner */
#define S_IWUSR 0000200 /* write by owner */
#define S_IXUSR 0000100 /* exec by owner */

#define S_IRWXG 0000070 /* rwx for group */
#define S_IRGRP 0000040 /* read by group */
#define S_IWGRP 0000020 /* write by group */
#define S_IXGRP 0000010 /* exec by group */

#define S_IRWXO 0000007 /* rwx for others */
#define S_IROTH 0000004 /* read by others */
#define S_IWOTH 0000002 /* write by others */
#define S_IXOTH 0000001 /* exec by others */

struct stat {
    unsigned long st_dev;   /* ID of device containing file */
    unsigned long st_ino;   /* inode number */
    unsigned long st_nlink; /* number of hard links */
    unsigned int st_mode;   /* protection + file type */
    unsigned int st_uid;    /* user ID of owner */
    unsigned int st_gid;    /* group ID of owner */
    unsigned int __pad0;
    unsigned long st_rdev; /* ID of device (if special file) */
    long st_size;          /* total size, bytes */
    long st_blksize;       /* blocksize for filesystem I/O */
    long st_blocks;        /* number of 512-B blocks allocated */

    /* Timestamps (seconds + nanoseconds, UTC) */
    unsigned long st_atime;
    unsigned long st_atime_nsec;
    unsigned long st_mtime;
    unsigned long st_mtime_nsec;
    unsigned long st_ctime;
    unsigned long st_ctime_nsec;
    long __unused[3];
};
