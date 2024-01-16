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
#include <misc_lib.h>
#include <logging.h>
#include <generic_at.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#ifndef __MINGW32__

typedef struct
{
    const char *pathname;
    int flags;
    mode_t mode;
    int fd;
} openat_data;

static int openat_inner(void *generic_data)
{
    openat_data *data = generic_data;
    data->fd = open(data->pathname, data->flags, data->mode);
    return data->fd;
}

static void cleanup(void *generic_data)
{
    openat_data *data = generic_data;
    if (data->fd >= 0)
    {
        close(data->fd);
    }
}

int openat(int dirfd, const char *pathname, int flags, ...)
{
    openat_data data;

    data.pathname = pathname;
    data.flags = flags;
    if (flags & O_CREAT)
    {
        va_list ap;
        va_start(ap, flags);
        data.mode = va_arg(ap, int);
        va_end(ap);
    }
    else
    {
        data.mode = 0;
    }
    data.fd = -1;

    return generic_at_function(dirfd, &openat_inner, &cleanup, &data);
}

#endif // !__MINGW32__
