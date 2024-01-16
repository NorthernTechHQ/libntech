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
#include <errno.h>

#ifndef __MINGW32__

typedef struct
{
    const char *pathname;
    uid_t owner;
    gid_t group;
    int flags;
} fchownat_data;

static int fchownat_inner(void *generic_data)
{
    fchownat_data *data = generic_data;
    if (data->flags & AT_SYMLINK_NOFOLLOW)
    {
        return lchown(data->pathname, data->owner, data->group);
    }
    else
    {
        return chown(data->pathname, data->owner, data->group);
    }
}

static void cleanup(ARG_UNUSED void *generic_data)
{
}

int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags)
{
    fchownat_data data;
    data.pathname = pathname;
    data.owner = owner;
    data.group = group;
    data.flags = flags;

    return generic_at_function(dirfd, &fchownat_inner, &cleanup, &data);
}

#endif // !__MINGW32__
