/* Subset of <sys/stat.h> for file status information */

#pragma once

/* File type bit masks */
#define S_IFMT 0170000   /* Mask for all file-type bits */
#define S_IFSOCK 0140000 /* Socket */
#define S_IFLNK 0120000  /* Symbolic link */
#define S_IFREG 0100000  /* Regular file */
#define S_IFBLK 0060000  /* Block device */
#define S_IFDIR 0040000  /* Directory */
#define S_IFCHR 0020000  /* Character device */
#define S_IFIFO 0010000  /* FIFO / pipe */

/* POSIX test macros for file types */
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)   /* Is symbolic link */
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)   /* Is regular file */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)   /* Is directory */
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)   /* Is character device */
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)   /* Is block device */
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)  /* Is FIFO/pipe */
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK) /* Is socket */

/* Permission bits */

/* Special permissions */
#define S_ISUID 0004000 /* Set-user-ID on execution */
#define S_ISGID 0002000 /* Set-group-ID on execution */
#define S_ISVTX 0001000 /* Sticky bit */

/* Owner permissions */
#define S_IRWXU 0000700 /* Read, write, execute by owner */
#define S_IRUSR 0000400 /* Read by owner */
#define S_IWUSR 0000200 /* Write by owner */
#define S_IXUSR 0000100 /* Execute by owner */

/* Group permissions */
#define S_IRWXG 0000070 /* Read, write, execute by group */
#define S_IRGRP 0000040 /* Read by group */
#define S_IWGRP 0000020 /* Write by group */
#define S_IXGRP 0000010 /* Execute by group */

/* Other permissions */
#define S_IRWXO 0000007 /* Read, write, execute by others */
#define S_IROTH 0000004 /* Read by others */
#define S_IWOTH 0000002 /* Write by others */
#define S_IXOTH 0000001 /* Execute by others */

/* File status structure */
struct stat {
    unsigned long st_dev;   /* ID of device containing file */
    unsigned long st_ino;   /* Inode number */
    unsigned long st_nlink; /* Number of hard links */
    unsigned int st_mode;   /* Protection + file type */
    unsigned int st_uid;    /* User ID of owner */
    unsigned int st_gid;    /* Group ID of owner */
    unsigned int __pad0;    /* Padding for alignment */
    unsigned long st_rdev;  /* ID of device (if special file) */
    long st_size;           /* Total size, bytes */
    long st_blksize;        /* Blocksize for filesystem I/O */
    long st_blocks;         /* Number of 512-B blocks allocated */

    /* Timestamps (seconds + nanoseconds, UTC) */
    unsigned long st_atime;      /* Time of last access */
    unsigned long st_atime_nsec; /* Nanoseconds component of atime */
    unsigned long st_mtime;      /* Time of last modification */
    unsigned long st_mtime_nsec; /* Nanoseconds component of mtime */
    unsigned long st_ctime;      /* Time of last status change */
    unsigned long st_ctime_nsec; /* Nanoseconds component of ctime */
    long __unused[3];            /* Reserved for future use */
};
