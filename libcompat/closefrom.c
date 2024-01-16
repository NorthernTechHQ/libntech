/*
  Copyright 2024 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <platform.h>
#include <dir.h>
#include <misc_lib.h>

#ifdef HAVE_SYS_PSTAT_H
# include <sys/pstat.h>
#endif

/*
 * Does the same as FreeBSD's closefrom: Close all file descriptors higher than
 * the given one.
 */
int closefrom(int fd)
{
#ifdef F_CLOSEM
    return (fcntl(fd, F_CLOSEM, 0) == -1) ? -1 : 0;

#elif defined(HAVE_PSTAT_GETFILE2)
    const int block_size = 100;
    struct pst_fileinfo2 info[block_size];
    pid_t our_pid = getpid();

    while (true)
    {
        int count = pstat_getfile2(info, sizeof(info[0]), block_size, 0, our_pid);
        if (count < 0)
        {
            return -1;
        }
        else if (count == 0)
        {
            break;
        }

        for (int info_index = 0; info_index < count; info_index++)
        {
            close(info[info_index].psf_fd);
        }
    }

#else
# ifndef _WIN32
    char proc_dir[50];

    xsnprintf(proc_dir, sizeof(proc_dir), "/proc/%i/fd", getpid());

    DIR *iter = opendir(proc_dir);
    if (iter)
    {
        int iter_fd = dirfd(iter);
        const struct dirent *entry;

        while ((entry = readdir(iter)))
        {
            int curr_fd;

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            if (sscanf(entry->d_name, "%i", &curr_fd) != 1)
            {
                closedir(iter);
                errno = EBADF;
                return -1;
            }

            if (curr_fd != iter_fd && curr_fd >= fd)
            {
                close(curr_fd);
            }
        }

        closedir(iter);

        return 0;
    }
# endif // !_WIN32

    // Fall back to closing every file descriptor. Very inefficient, but will
    // work.
# ifdef _SC_OPEN_MAX
    long max_fds = sysconf(_SC_OPEN_MAX);
# else
    long max_fds = 1024;
# endif

    for (long curr_fd = fd; curr_fd < max_fds; curr_fd++)
    {
        close(curr_fd);
    }
#endif

    return 0;
}
