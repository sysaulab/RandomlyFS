/*
 FUSE: Filesystem in Userspace
 Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

 This program can be distributed under the terms of the GNU GPL.
 See the file LICENSE.txt.

 */

/*
 * Loopback macFUSE file system in C. Uses the high-level FUSE API.
 * Based on the fusexmp_fh.c example from the Linux FUSE distribution.
 * Amit Singh <http://osxbook.com>
 */

#include <AvailabilityMacros.h>
#include <stdio.h>
#include <time.h>

#define HAVE_EXCHANGE 0
#define HAVE_RENAMEX 1
#define HAVE_ACCESS 0

#define FUSE_USE_VERSION 26

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse/fuse.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/attr.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/xattr.h>
#include <unistd.h>

#if defined(_POSIX_C_SOURCE)
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;
#endif

#define G_PREFIX                       "org"
#define G_KAUTH_FILESEC_XATTR G_PREFIX ".apple.system.Security"
#define A_PREFIX                       "com"
#define A_KAUTH_FILESEC_XATTR A_PREFIX ".apple.system.Security"
#define XATTR_APPLE_PREFIX             "com.apple."

/* MY CODE V V V V V V V V V V V V V V V V V V V V V V V V V V */
#include "libseedy/libseedy64.h"
#include "cc2032.h"
#include "da128.h"

/*
 *   Intended to be the seeding RNG that seeds other RNGs.
 *   It is initialised randomnly with libseedy at each start.
 */
CC2032 RNG_ROOT;
DA128 RNG_FS;
char BASEPATH[1025];
char RNGPATH[1025];
char MNTPATH[1025];

#if defined(__GNUC__)
typedef unsigned __int128 uint128_t;
typedef __int128 int128_t;
#else
#   error "Unsupported compiler"
#endif /* defined(__GNUC__) */

//#define RANDFS_OPT_DEBUG_ON

#if defined(RANDFS_OPT_DEBUG_ON)
void debug(char* key, char* val)
{
    static FILE* debug_file = NULL;
    if(debug_file == NULL)
    {
        char actualpath [PATH_MAX+1];
        sprintf(actualpath, "%s/debug.log", BASEPATH);
        debug_file = fopen(actualpath, "a");
        if(debug_file == NULL)
        {
            fprintf(stderr, "PANICKING. CANNOT OPEN DEBUG FILE!!!\n%s\n", actualpath);
            exit(0);
        }
    }
    fprintf(debug_file, "%s:%s\n", key, val);
    fflush(debug_file);
}
char VALUE[100];
#define debugU128x(label,value) sprintf(VALUE, "%016llx%016llx", (uint64_t)(value>>64),(uint64_t)value);debug(label, VALUE)
#define debugU64x(label,value) sprintf(VALUE, "%016llx", (uint64_t)value);debug(label, VALUE)
#define debugU64(label,value) sprintf(VALUE, "%llu", (uint64_t)value);debug(label, VALUE)
#else
#define debug(a,b) ;
#define debugU128x(a,b) ;
#define debugU64x(a,b) ;
#define debugU64(a,b) ;
#endif

/* To handle the opened directories. */

#define GET_BANK(fi) (((FILE*)fi->fh)->_cookie)
#define GET_OFFSET(fi) (((FILE*)fi->fh)->_extra)
#define GETU128(a,b) (uint128_t)((uint128_t)(a)<<64|(uint128_t)(b))
#define GETINDEX(fi) GETU128(GET_BANK(fi),GET_OFFSET(fi))

/* MY CODE A A A A A A A A A A A A A A A A A A A A A A A A A A A A A */

struct loopback {
    uint32_t blocksize;
    bool case_insensitive;
};

static struct loopback loopback;

int ishex(char c)
{
    if
    (
        ( c >= 'a' && c <= 'f' ) ||
        ( c >= 'A' && c <= 'F' ) ||
        ( c >= '0' && c <= '9' )
    )
    {
        return 1;
    }

    return 0;
}

uint128_t parse_path (char* path)
{
    uint128_t result = 0;
    int done = 0;
    for(int pos = 3; done == 0; pos++)
    {
        switch (path[pos])
        {
            case '0':
                result = (result << 4) | 0;
                break;
            case '1':
                result = (result << 4) | 1;
                break;
            case '2':
                result = (result << 4) | 2;
                break;
            case '3':
                result = (result << 4) | 3;
                break;
            case '4':
                result = (result << 4) | 4;
                break;
            case '5':
                result = (result << 4) | 5;
                break;
            case '6':
                result = (result << 4) | 6;
                break;
            case '7':
                result = (result << 4) | 7;
                break;
            case '8':
                result = (result << 4) | 8;
                break;
            case '9':
                result = (result << 4) | 9;
                break;
            case 'a':
            case 'A':
                result = (result << 4) | 10;
                break;
            case 'b':
            case 'B':
                result = (result << 4) | 11;
                break;
            case 'c':
            case 'C':
                result = (result << 4) | 12;
                break;
            case 'd':
            case 'D':
                result = (result << 4) | 13;
                break;
            case 'e':
            case 'E':
                result = (result << 4) | 14;
                break;
            case 'f':
            case 'F':
                result = (result << 4) | 15;
                break;
            default:
                done = 1;
        }
    }
//    debugU64x("r_low",(result>>64));
//    debugU64x("r_high",(result));
    return result;
}

#define isData(path) if( path[1] == '0' && path[2] == 'x' && ishex(path[3]) )

static int
loopback_getattr( const char *path, struct stat *stbuf)
{
    /* https://www.man7.org/linux/man-pages/man7/inode.7.html */
    
    int res;
debug("loopback_getattr()", (char*)path);
    if (0 == strcmp("/", path) )
    {
        res = lstat(BASEPATH, stbuf);
        return 0;
    }
    else if (0 == strcmp("/.metadata_never_index", path) )
    {
        res = lstat(RNGPATH, stbuf);
        stbuf->st_size = 0;
        return 0;
    }    
    else isData(path)
    {
        res = lstat(RNGPATH, stbuf);
        uint128_t base_offset = parse_path((char*)path);
        uint128_t avail;
        uint128_t max = GETU128(0xffffFFFFffffFFFF, 0xffffFFFFffffFFFF);
        uint128_t two63m1 = 0x7FFFFFFFFFFFFFFF;

/*        if(base_offset >= max)
        {
            return -ENOENT;
        }*/

        avail = max - base_offset;
        if(base_offset)
            avail++;
        if(avail > two63m1)
        {
            avail = two63m1;
            debug("SIZE", "2^31-1");
        }
        else
        {
            debugU64("SIZE", avail);
        }

        stbuf->st_size = avail;
            debugU64("SIZE", stbuf->st_size);
        return 0;
        }
    else
    {
debug("NONE", "NONE");
        return -ENOENT;
    }

}

static int
loopback_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
debug("loopback_fgetattr()", (char*)path);
    int res;
    (void)path;

    res = fstat(fi->fh, stbuf);

    // Fall back to global I/O size. See loopback_getattr().
    stbuf->st_blksize = 0;

    if (res == -1) {
        return -errno;
    }

    return 0;
}

#if HAVE_ACCESS

static int
loopback_access(
    const char *path, 
    int mask)
{
    debug("access()", "");
    int res;

    /*
     * Standard access permission flags:
     * F_OK            test for existence of file
     * X_OK            test for execute or search permission
     * W_OK            test for write permission
     * R_OK            test for read permission
     *
     * Extended access permission flags that can be enabled by setting
     * FUSE_CAP_ACCESS_EXTENDED (See loopback_init()):
     * _READ_OK        read file data / read directory
     * _WRITE_OK       write file data / add file to directory
     * _EXECUTE_OK     execute file / search in directory
     * _DELETE_OK      delete file / delete directory
     * _APPEND_OK      append to file / add subdirectory to directory
     * _RMFILE_OK      remove file from directory
     * _RATTR_OK       read basic attributes
     * _WATTR_OK       write basic attributes
     * _REXT_OK        read extended attributes
     * _WEXT_OK        write extended attributes
     * _RPERM_OK       read permissions
     * _WPERM_OK       write permissions
     * _CHOWN_OK       change ownership
     */

    res = access(path, mask & (F_OK | X_OK | W_OK | R_OK));
    if (res == -1)
        return -errno;

    return 0;
}

#endif /* HAVE_ACCESS */

static int
loopback_readlink( const char *path, char *buf, size_t size)
{
debug("loopback_readlink()", (char*)path);

    int res;

    res = readlink(path, buf, size - 1);
    if (res == -1) 
    {
        return -errno;
    }

    buf[res] = '\0';

    return 0;
}

struct loopback_dirp
{
    DIR *dp;
    struct dirent *entry;
    off_t offset;
};
static inline struct loopback_dirp *
get_dirp(struct fuse_file_info *fi)
{
    debug("get_dirp()", "");
    return (struct loopback_dirp *)(uintptr_t)fi->fh;
}


static int
loopback_opendir(const char *path, struct fuse_file_info *fi)
{
    int result;
    struct loopback_dirp* handl = malloc(sizeof(struct loopback_dirp)); 
    debug("opendir()", (char*)path);
    if(handl == NULL){
        debug("FATAL", "Out of memory");
        exit(EXIT_FAILURE);
    }
    handl->offset = 0;

    /* Make a new handle */
    if(path[1] != '\0')
    {
        debug("ERROR", "Path not found");
        return -ENOENT;
    }
    fi->fh = (uint64_t)handl;
    return 0;
}

static int
loopback_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    struct stat st; 
    struct dirent dir;
    struct loopback_dirp* handl = get_dirp(fi);
    if(handl->offset > 0) return 0;

    debugU64("readdir()",handl->offset);
    uint128_t big_step = GETU128(0x0000000000000000,0x8000000000000000);
    uint128_t cur_step = 0;

    for( int i = 0; i < 1; i = i + 1 )
    {
        handl->entry = &dir;
        lstat(RNGPATH, &st);
        handl->offset = i;
        cur_step = big_step * i;
        snprintf(handl->entry->d_name, 1023, "0x%016llX%016llX.bin", (uint64_t)(cur_step >> 64), (uint64_t)cur_step);
        if(filler(buf, handl->entry->d_name, &st, i + 1))
        {
            debug("filler-buffer", "full");
            return 0;
        }
    }

    cc2032_fill(&RNG_ROOT, (uint8_t*)&cur_step, sizeof(cur_step));
    snprintf(handl->entry->d_name, 1023, "0x%016llX%016llX.bin", (uint64_t)(cur_step >> 64), (uint64_t)(cur_step));
    if(filler(buf, handl->entry->d_name, &st, 7000))
    {
        debug("filler-buffer", "full");
        return 0;
    }

    for( int i = 131071; i < 131072; i = i + 1 )
    {
        handl->entry = &dir;
        lstat(RNGPATH, &st);
        handl->offset = i;
        cur_step = big_step * i;
        snprintf(handl->entry->d_name, 1023, "0x%016llX%016llX.bin", 0xffffFFFFffffFFFFLLU, 0x8000000000000000LLU);
        if(filler(buf, handl->entry->d_name, &st, i + 1))
        {
            debug("filler-buffer", "full");
            return 0;
        }
    }

    handl->entry = &dir;
    lstat(RNGPATH, &st);
    snprintf(handl->entry->d_name, 1023, "0x%016llX%016llX.bin", 0xffffFFFFffffFFFFLLU, 0xffffFFFFffffFFFfLLU);
    if(filler(buf, handl->entry->d_name, &st, 9000))
    {
        debug("filler-buffer", "full");
        return 0;
    }

    return 0;
}

static int
loopback_releasedir(const char *path, struct fuse_file_info *fi)
{
    debug("releasedir()", "");
    free((struct loopback_dirp*)fi->fh);
    return 0;
}

static int
loopback_mknod(const char *path, mode_t mode, dev_t rdev)
{
    debug("mknod()", (char*)path);
    return -EROFS;
}

static int
loopback_mkdir(const char *path, mode_t mode)
{
    debug("mkdir()", (char*)path);
    return -EROFS;
}

static int
loopback_unlink(const char *path)
{
    debug("unlink()", (char*)path);
    return -EROFS;
}

static int
loopback_rmdir(const char *path)
{
    debug("rmdir()", (char*)path);
    return -EROFS;
}

static int
loopback_symlink(const char *from, const char *to)
{
    debug("symlink()", (char*)from);
    return -EROFS;
}

static int
loopback_rename(const char *from, const char *to)
{
    debug("symlink()", (char*)from);
    return -EROFS;
}

#if HAVE_EXCHANGE

static int
loopback_exchange(const char *path1, const char *path2, unsigned long options)
{
debug("loopback_exchange()", (char*)path1);
    int res;

    res = exchangedata(path1, path2, options);
    if (res == -1) {
        return -errno;
    }

    return 0;
}

#endif /* HAVE_EXCHANGE */

static int
loopback_link(const char *from, const char *to)
{
    debug("link()", (char*)from);
    return -EROFS;
    return 0;
}

static int
loopback_fsetattr_x(const char *path, struct setattr_x *attr, struct fuse_file_info *fi)
{
    debug("fsetattr_x()", (char*)path);
    return -EROFS;
    return 0;
}

static int
loopback_setattr_x(const char *path, struct setattr_x *attr)
{
    debug("setattr_x()", (char*)path);
    return -EROFS;
    return 0;
}

static int
loopback_getxtimes(const char *path, struct timespec *bkuptime,
                   struct timespec *crtime)
{
    debug("getxtimes()", (char*)path);
    return -ENOATTR;
    return 0;
}

static int
loopback_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    debug("create()", (char*)path);
    return -EROFS;
}

static int
loopback_open(const char *path, struct fuse_file_info *fi)
{
    debug("open()", (char*)path);
    isData(path)
    {
        FILE* fd = calloc(1, sizeof(FILE));
        if( fd == NULL )
        {
            debug("FATAL", "Out of memory");
            exit(EXIT_FAILURE);
        }
        uint128_t basepos = parse_path((char*)path);
        *(uint64_t*)&fd->_cookie = basepos>>64; /* base bank */
        *(uint64_t*)&fd->_extra  = basepos; /* base offset */
        fi->fh = (uint64_t)fd;
        return 0;
    }
    return -ENODATA;
}

static int
loopback_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    uint128_t base_index = GETINDEX( fi );
    uint128_t full_index = GETINDEX( fi ) + offset;
    debugU128x("read()", full_index);
    da128_fill(RNG_FS, (uint8_t*)buf, size, full_index);
    return size;
}

static int
loopback_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    return -EROFS;
}

static int
loopback_flush(const char *path, struct fuse_file_info *fi)
{
    debug("flush()", (char*)path);
    return 0;
}

static int
loopback_release(const char *path, struct fuse_file_info *fi)
{
    debug("release()", (char*)path);
    free((FILE*)fi->fh);
    return 0;
}

static int
loopback_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
    return 0;
}

static int
loopback_setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position)
{
    return -EROFS;
}

static int
loopback_getxattr(const char *path, const char *name, char *value, size_t size,
                  uint32_t position)
{
    return -ENOATTR;
}

static int
loopback_listxattr(const char *path, char *list, size_t size)
{
    return -ENOATTR;
}

static int
loopback_removexattr(const char *path, const char *name)
{
    return -EROFS;
}

static int
loopback_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi)
{
    return -EROFS;
}

static int
loopback_setvolname(const char *name)
{
    return -EROFS;
}

static int
loopback_statfs_x(const char *path, struct statfs *stbuf)
{
    stbuf->f_bsize  = 64;   /* optimal transfer block size */
    stbuf->f_blocks = 0x1;  /* total data blocks in file system */
    stbuf->f_bavail = 0;    /* free blocks available to unprivileged user */
    stbuf->f_bfree = 0;     /* free blocks in fs */
    stbuf->f_files = 65536;     /* total file nodes in file system */
    stbuf->f_ffree = 0;     /* free file nodes in fs */
    return 0;
}

#if HAVE_RENAMEX

static int
loopback_renamex(const char *path1, const char *path2, unsigned int flags)
{
    return -EROFS;
}

#endif /* HAVE_RENAMEX */

void *
loopback_init(struct fuse_conn_info *conn)
{
    conn->want |= FUSE_CAP_VOL_RENAME | FUSE_CAP_XTIMES ;

#if HAVE_ACCESS
    conn->want |= FUSE_CAP_ACCESS_EXTENDED;
#endif

#ifdef FUSE_ENABLE_CASE_INSENSITIVE
    if (loopback.case_insensitive) {
        conn->want |= FUSE_CAP_CASE_INSENSITIVE;
    }
#endif
    return NULL;
}

void
loopback_destroy(void *userdata)
{
    /* nothing */
}

static struct fuse_operations loopback_oper = {
    .init        = loopback_init,
    .destroy     = loopback_destroy,
    .getattr     = loopback_getattr,
    .fgetattr    = loopback_fgetattr,
#if HAVE_ACCESS
    .access      = loopback_access,
#endif
    .readlink    = loopback_readlink,
    .opendir     = loopback_opendir,
    .readdir     = loopback_readdir,
    .releasedir  = loopback_releasedir,
    .mknod       = loopback_mknod,
    .mkdir       = loopback_mkdir,
    .symlink     = loopback_symlink,
    .unlink      = loopback_unlink,
    .rmdir       = loopback_rmdir,
    .rename      = loopback_rename,
    .link        = loopback_link,
    .create      = loopback_create,
    .open        = loopback_open,
    .read        = loopback_read,
    .write       = loopback_write,
    .flush       = loopback_flush,
    .release     = loopback_release,
    .fsync       = loopback_fsync,
    .setxattr    = loopback_setxattr,
    .getxattr    = loopback_getxattr,
    .listxattr   = loopback_listxattr,
    .removexattr = loopback_removexattr,
#if HAVE_EXCHANGE
    .exchange    = loopback_exchange,
#endif
    .getxtimes   = loopback_getxtimes,
    .setattr_x   = loopback_setattr_x,
    .fsetattr_x  = loopback_fsetattr_x,
    .fallocate   = loopback_fallocate,
    .setvolname  = loopback_setvolname,
    .statfs_x    = loopback_statfs_x,
#if HAVE_RENAMEX
    .renamex     = loopback_renamex,
#endif

    .flag_nullpath_ok = 1,
    .flag_nopath = 1,
};

static const struct fuse_opt loopback_opts[] = {
    { "blocksize=%u", offsetof(struct loopback, blocksize), 0 },
    { "case_insensitive", offsetof(struct loopback, case_insensitive), true },
    FUSE_OPT_END
};

int
main(int argc, char **argv)
{
    int new = 0;
    int res = 0;
    char* home = getenv("HOME");
    snprintf(BASEPATH, 1024, "%s/.rrfs", home);
    snprintf(MNTPATH, 1024, "%s/Randomly", BASEPATH);
    snprintf(RNGPATH, 1024, "%s/seed.bin", BASEPATH);
    debug("START", "##### ##### ##### ##### ##### ##### ##### #####");

    char cmd[1025];
    snprintf(cmd, 1024, "umount -f %s", MNTPATH);
    system(cmd);
    mkdir(BASEPATH, 0740);
    mkdir(MNTPATH, 0740);

    int forced_argc = 2;
    char* forced_argv[2] = {"rrfs", MNTPATH};
    struct fuse_args args = FUSE_ARGS_INIT(forced_argc, forced_argv);

    cc2032_init(&RNG_ROOT,seedy64);

    FILE* store = fopen(RNGPATH, "r");
    if (!(store != NULL && ( 1 == fread(&RNG_FS, sizeof(RNG_FS), 1, store))))
    {
        da128_init(RNG_FS);
        store = fopen(RNGPATH, "w");
        if( store != NULL )
            fwrite(&RNG_FS, sizeof(RNG_FS), 1, store);
    }
 
    loopback.blocksize = 4096;
    loopback.case_insensitive = 0;
    if (fuse_opt_parse(&args, &loopback, loopback_opts, NULL) == -1)
    {
        exit(1);
    }

    umask(0);
    res = fuse_main(args.argc, args.argv, &loopback_oper, NULL);
    
    fuse_opt_free_args(&args);
    return res;
}

