//Compatability header for mingw and other systems
#pragma once

#ifdef __MINGW32__
#include <corecrt.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

// windows doesn't have symbolic links of this type
size_t readlink(const char *path, char *buf, size_t bufsiz);
int symlink(const char *target, const char *linkpath);

// windows doesn't have hard links of this type
int link(const char *oldpath, const char *newpath);

#define qsort_r qsort_s
#define aligned_alloc(alignment, nsize) _aligned_malloc(nsize, alignment)

//no symlinks on windows
#define lstat stat

// Code ported from MIT licensed library: https://github.com/win32ports/dirent_h/blob/master/dirent.h
int alphasort(const struct dirent **a, const struct dirent **b);
int __strverscmp(const char *s1, const char *s2);
int versionsort(const struct dirent **a, const struct dirent **b);
int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));

#endif

#ifndef __MINGW32__
    #define aligned_free_(p) free(p);
    #define mkdir_(path, mode) mkdir(path,mode)
#else
    #define aligned_free_(p) _aligned_free(p)
    #define mkdir_(path, mode) mkdir(path)
#endif

int is_dir(const char *filename);

int is_link(const char *filename);


#ifndef __MINGW32__
char *realpath_(const char *path, char *resolved_path, size_t maxLength);
#else
char *realpath_(const char *path, char *resolved_path, size_t maxLength);
#endif