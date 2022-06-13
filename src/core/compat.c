
#include "compat.h"

#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif


#ifdef __MINGW32__
// windows doesn't have symbolic links of this type
size_t readlink(const char *path, char *buf, size_t bufsiz) { return -1; }
int symlink(const char *target, const char *linkpath) { return -1; }
// windows doesn't have hard links of this type
int link(const char *oldpath, const char *newpath) { return -1; }

// Code ported from MIT licensed library: https://github.com/win32ports/dirent_h/blob/master/dirent.h
int alphasort(const struct dirent **a, const struct dirent **b)
{
    struct dirent **dira = (struct dirent **)a, **dirb = (struct dirent **)b;
    if (!dira || !dirb)
        return 0;
    return strcoll((*dira)->d_name, (*dirb)->d_name);
}

int __strverscmp(const char *s1, const char *s2)
{
    return strcoll(s1, s2);
}

int versionsort(const struct dirent **a, const struct dirent **b)
{
    struct dirent **dira = (struct dirent **)a, **dirb = (struct dirent **)b;
    if (!dira || !dirb)
        return 0;
    return __strverscmp((*dira)->d_name, (*dirb)->d_name);
}

int scandir(const char *dirp, struct dirent ***namelist,
                   int (*filter)(const struct dirent *),
                   int (*compar)(const struct dirent **, const struct dirent **))
{
    struct dirent **entries = NULL, **tmp_entries = NULL;
    long int i = 0, index = 0, count = 16;
    DIR *d = opendir(dirp);
    struct dirent *data = (struct dirent *)d;
    if (!data)
    {
        closedir(d);
        _set_errno(ENOENT);
        return -1;
    }
    entries = (struct dirent **)malloc(sizeof(struct dirent *) * count);
    if (!entries)
    {
        closedir(d);
        _set_errno(ENOMEM);
        return -1;
    }
    while ((data = readdir(d)) != NULL)
    {
        if (!filter || filter(data))
        {
            entries[index] = (struct dirent *)malloc(sizeof(struct dirent));
            if (!entries[index])
            {
                closedir(d);
                for (i = 0; i < index; ++i)
                    free(entries[index]);
                free(entries);
                _set_errno(ENOMEM);
                return -1;
            }
            memcpy(entries[index], &data, sizeof(struct dirent));
            if (++index == count)
            {
                tmp_entries = (struct dirent **)realloc(entries, sizeof(struct dirent *) * count * 2);
                if (!tmp_entries)
                {
                    closedir(d);
                    for (i = 0; i < index; ++i)
                        free(entries[index - 1]);
                    free(entries);
                    _set_errno(ENOMEM);
                    return -1;
                }
                entries = tmp_entries;
                count *= 2;
            }
        }
    }
    if (compar)
        qsort(entries, index, sizeof(struct dirent *), (int (*)(const void *, const void *))compar);
    entries[index] = NULL;
    if (namelist)
        *namelist = entries;
    closedir(d);
    return index;
}

#endif

#ifndef __MINGW32__
#define aligned_free_(p) free(p);
#define mkdir_(path, mode) mkdir(path, mode)
#else
#define aligned_free_(p) _aligned_free(p)
#define mkdir_(path, mode) mkdir(path)
#endif

int is_dir(const char *filename)
{
    struct stat statbuf = {0};
    stat(filename, &statbuf);
    return (S_ISDIR(statbuf.st_mode));
}

int is_link(const char *filename)
{
#ifndef __MINGW32__
    struct stat statbuf = {0};
    stat(filename, &statbuf);
    return (S_ISLNK(statbuf.st_mode));
#else
    // no links on windows
    return 0;
#endif
}

#ifndef __MINGW32__
char *realpath_(const char *path, char *resolved_path, size_t maxLength)
{
    return realpath(path, resolved_path);
}
#else
char *realpath_(const char *path, char *resolved_path, size_t maxLength)
{
    return _fullpath(resolved_path, path, maxLength);
}
#endif

int getNumberOfCores(void)
{
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1)
    {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if (count < 1)
        {
            count = 1;
        }
    }
    return count;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}
