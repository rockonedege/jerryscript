/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Jerry libc platform-specific functions darwin implementation
 */

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "jerry-libc-defs.h"

extern long int syscall_0 (long int syscall_no);
extern long int syscall_1 (long int syscall_no, long int arg1);
extern long int syscall_2 (long int syscall_no, long int arg1, long int arg2);
extern long int syscall_3 (long int syscall_no, long int arg1, long int arg2, long int arg3);

/** Output of character. Writes the character c, cast to an unsigned char, to stdout.  */
int
putchar (int c)
{
  fwrite (&c, 1, sizeof (char), stdout);

  return c;
} /* putchar */

/**
 * Output specified string
 */
int
puts (const char *s) /**< string to print */
{
  while (*s)
  {
    putchar (*s);

    s++;
  }

  return 0;
} /* puts */

/**
 * Exit - cause normal process termination with specified status code
 */
void __attr_noreturn___ __attr_used___
exit (int status) /**< status code */
{
  syscall_1 (SYS_close, (long int) stdin);
  syscall_1 (SYS_close, (long int) stdout);
  syscall_1 (SYS_close, (long int) stderr);

  syscall_1 (SYS_exit, status);

  while (true)
  {
    /* unreachable */
  }
} /* exit */

/**
 * Abort current process, producing an abnormal program termination.
 * The function raises the SIGABRT signal.
 */
void __attr_noreturn___ __attr_used___
abort (void)
{
  syscall_1 (SYS_close, (long int) stdin);
  syscall_1 (SYS_close, (long int) stdout);
  syscall_1 (SYS_close, (long int) stderr);

  raise (SIGABRT);

  while (true)
  {
    /* unreachable */
  }
} /* abort */

/**
 * Send a signal to the current process.
 */
int __attr_used___
raise (int sig)
{
  return (int) syscall_2 (SYS_kill, syscall_0 (SYS_getpid), sig);
} /* raise */

/**
 * fopen
 *
 * @return FILE pointer - upon successful completion,
 *         NULL - otherwise
 */
FILE *
fopen (const char *path, /**< file path */
       const char *mode) /**< file open mode */
{
  bool may_read = false;
  bool may_write = false;
  bool truncate = false;
  bool create_if_not_exist = false;
  bool position_at_end = false;

  assert (path != NULL && mode != NULL);
  assert (mode[1] == '+' || mode[1] == '\0');

  switch (mode[0])
  {
    case 'r':
    {
      may_read = true;
      may_write = (mode[1] == '+');
      break;
    }
    case 'w':
    {
      may_write = true;
      truncate = true;
      create_if_not_exist = true;
      may_read = (mode[1] == '+');
      break;
    }
    case 'a':
    {
      may_write = true;
      position_at_end = true;
      create_if_not_exist = true;
      if (mode[1] == '+')
      {
        assert (!"unsupported mode a+");
      }
      break;
    }
    default:
    {
      assert (!"unsupported mode");
    }
  }

  int flags = 0;
  int access = S_IRUSR | S_IWUSR;
  if (may_read && !may_write)
  {
    flags = O_RDONLY;
  }
  else if (!may_read && may_write)
  {
    flags = O_WRONLY;
  }
  else
  {
    assert (may_read && may_write);

    flags = O_RDWR;
  }

  if (truncate)
  {
    flags |= O_TRUNC;
  }

  if (create_if_not_exist)
  {
    flags |= O_CREAT;
  }

  if (position_at_end)
  {
    flags |= O_APPEND;
  }

  long int ret = syscall_3 (SYS_open, (long int) path, flags, access);

  return (void *) (uintptr_t) (ret);
} /* fopen */

/**
 * The rewind () function sets the file position indicator
 * for the stream pointed to by STREAM to the beginning of the file.
 */
void
rewind (FILE *stream) /**< stream pointer */
{
  syscall_3 (SYS_lseek, (long int) stream, 0, SEEK_SET);
} /* rewind */

/**
 * fclose
 *
 * @return 0 - upon successful completion,
 *         non-zero value - otherwise.
 */
int
fclose (FILE *fp) /**< stream pointer */
{
  syscall_2 (SYS_close, (long int) fp, 0);

  return 0;
} /* fclose */

/**
 * fseek
 */
int
fseek (FILE * fp, /**< stream pointer */
       long offset, /**< offset */
       int whence) /**< specifies position type
                    *   to add offset to */
{
  syscall_3 (SYS_lseek, (long int) fp, offset, whence);

  return 0;
} /* fseek */

/**
 * ftell
 */
long
ftell (FILE * fp) /**< stream pointer */
{
  long int ret = syscall_3 (SYS_lseek, (long int) fp, 0, SEEK_CUR);

  return ret;
} /* ftell */

/**
 * fread
 *
 * @return number of elements read
 */
size_t
fread (void *ptr, /**< address of buffer to read to */
       size_t size, /**< size of elements to read */
       size_t nmemb, /**< number of elements to read */
       FILE *stream) /**< stream pointer */
{
  long int ret;
  size_t bytes_read = 0;

  if (size == 0)
  {
    return 0;
  }

  do
  {
    ret = syscall_3 (SYS_read,
                     (long int) stream,
                     (long int) ((uint8_t *) ptr + bytes_read),
                     (long int) (size * nmemb - bytes_read));

    bytes_read += (size_t) ret;
  }
  while (bytes_read != size * nmemb && ret != 0);

  return bytes_read / size;
} /* fread */

/**
 * fwrite
 *
 * @return number of elements written
 */
size_t
fwrite (const void *ptr, /**< data to write */
        size_t size, /**< size of elements to write */
        size_t nmemb, /**< number of elements */
        FILE *stream) /**< stream pointer */
{
  size_t bytes_written = 0;

  if (size == 0)
  {
    return 0;
  }

  do
  {
    long int ret = syscall_3 (SYS_write,
                              (long int) stream,
                              (long int) ((uint8_t *) ptr + bytes_written),
                              (long int) (size * nmemb - bytes_written));

    bytes_written += (size_t) ret;
  }
  while (bytes_written != size * nmemb);

  return bytes_written / size;
} /* fwrite */

/**
 * This function can get the time as well as a timezone.
 *
 * @return 0 if success, -1 otherwise
 */
int
gettimeofday (void *tp,  /**< struct timeval */
              void *tzp) /**< struct timezone */
{
  return (int) syscall_2 (gettimeofday, (long int) tp, (long int) tzp);
} /* gettimeofday */