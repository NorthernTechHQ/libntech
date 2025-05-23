/*
  Copyright 2025 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#ifndef CFENGINE_FSATTR_H
#define CFENGINE_FSATTR_H

#include <stdbool.h>
#include <assert.h>

typedef enum
{
    FS_ATTRS_SUCCESS = 0,    /* Operation succeeded */
    FS_ATTRS_FAILURE,        /* Unexpected failure */
    FS_ATTRS_DOES_NOT_EXIST, /* File does not exists */
    FS_ATTRS_NOT_SUPPORTED,  /* Operation is not supported */
} FSAttrsResult;

/**
 * @brief Get string representation of error code
 * @param res Error code (see FSAttrsResult)
 * @return String representation of error code
 */
static inline const char *FSAttrsErrorCodeToString(FSAttrsResult res)
{
    switch (res)
    {
    case FS_ATTRS_SUCCESS:
        return "Success";
    case FS_ATTRS_FAILURE:
        return "Unexpected failure";
    case FS_ATTRS_DOES_NOT_EXIST:
        return "File does not exist";
    case FS_ATTRS_NOT_SUPPORTED:
        return "Operation is not supported";
    }

    assert(false && "Bad argument in FSAttrsErrorCodeToString()");
    return "";
}

/**
 * @brief Get immutable flag of inode given the filename
 * @param filename The filename
 * @param flag The flag; true if flag is set, otherwise false
 * @return Error code (see FSAttrsResult)
 * @note Only regular files are currently supported
 */
FSAttrsResult FSAttrsGetImmutableFlag(const char *filename, bool *flag);

/**
 * @brief Set/clear immutable flag of inode given the filename
 * @param filename The filename
 * @param flag The flag; true to set the flag, false to clear the flag
 * @return Error code (see FSAttrsResult)
 * @note Only regular files are currently supported
 */
FSAttrsResult FSAttrsUpdateImmutableFlag(const char *filename, bool flag);

#endif /* CFENGINE_FSATTR_H */
