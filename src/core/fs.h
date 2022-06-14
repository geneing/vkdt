#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __MINGW32__
typedef off64_t loff_t;
static inline int copy_file_range(int fd0, off64_t *offset0, int fd1, off64_t *offset1, size_t len, unsigned int flags)
{
  off64_t pos0 = lseek(fd0, 0, SEEK_CUR);
  off64_t pos1 = lseek(fd1, 0, SEEK_CUR);
  ssize_t n = 0;
  ssize_t ntotal = 0;
  const int SIZ = 4096;
  char buffer[SIZ];

  if( offset0 )
    lseek(fd0, *offset0, SEEK_SET);
  if(offset1)
    lseek(fd1, *offset1, SEEK_SET);
  
  while ((n = read(fd0, buffer, SIZ)) > 0)
  {
    if (write(fd1, buffer, SIZ) != n){
      return -1;
    }
    ntotal += n;
  }
  lseek(fd0, pos0, SEEK_SET);
  lseek(fd1, pos1, SEEK_SET);
  if( offset0 )
    offset0 += ntotal;
  if( offset1 )
    offset1 += ntotal;

  return ntotal;
}
#endif

static inline int // returns zero on success
dt_file_copy(
    const char *dst,
    const char *src)
{
  ssize_t ret;
  struct stat sb;
  loff_t len = -1;
  int fd0 = open(src, O_RDONLY), fd1 = -1;
  if(fd0 == -1 || fstat(fd0, &sb) == -1) goto copy_error;
  len = sb.st_size;
  if(sb.st_mode & S_IFDIR) { len = 0; goto copy_error; } // don't copy directories
  if(-1 == (fd1 = open(dst, O_CREAT | O_WRONLY | O_TRUNC, 0644))) goto copy_error;
  do {
    ret = copy_file_range(fd0, 0, fd1, 0, len, 0);
  } while((len-=ret) > 0 && ret > 0);
copy_error:
  if(fd0 >= 0) close(fd0);
  if(fd0 >= 0) close(fd1);
  return len;
}

static inline int // returns zero on success
dt_file_delete(
    const char *fn)
{
  return unlink(fn);
}

static inline int // returns zero on success
dt_mkdir(
    const char *pathname,
    mode_t      mode)
{
  return mkdir_(pathname, mode);
}
