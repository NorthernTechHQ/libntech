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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#if !HAVE_DECL_DIRFD
int dirfd(DIR *dirp);
#endif

/* Under Solaris, HP-UX and AIX dirfd(3) is just looking inside DIR structure */

#if defined(__sun)

int dirfd(DIR *dirp)
{
    return dirp->d_fd != -1 ? dirp->d_fd : ENOTSUP;
}

#elif defined(__hpux) || defined(_AIX)

int dirfd(DIR *dirp)
{
    return dirp->dd_fd != -1 ? dirp->dd_fd : ENOTSUP;
}

#endif
