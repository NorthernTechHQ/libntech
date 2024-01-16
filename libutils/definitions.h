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

#ifndef CFENGINE_DEFINITIONS_H
#define CFENGINE_DEFINITIONS_H

/*****************************************************************************
 * Metric prefixes
 *****************************************************************************/
#define KIBIBYTE(n) (n * 1024UL)
#define MEBIBYTE(n) (n * 1024UL * 1024UL)
#define GIBIBYTE(n) (n * 1024ULL * 1024ULL * 1024ULL)
#define TEBIBYTE(n) (n * 1024ULL * 1024ULL * 1024ULL * 1024ULL)

/*****************************************************************************
 * Size related defines							     *
 *****************************************************************************/
#define CF_MAXSIZE  102400000
#define CF_BILLION 1000000000L

#define CF_BLOWFISHSIZE    16
#define CF_BUFFERMARGIN   128
#define CF_MAXVARSIZE    1024
#define CF_MAXSIDSIZE    2048      /* Windows only: Max size (bytes) of
                                     security identifiers */

// Limit database names and table names so they can be combined
// into CF_MAXVARSIZE without truncation
// NOTE: Databases like PostgreSQL and MySQL actually have stricter limits
#define CF_MAXTABLENAMESIZE 256
#define CF_MAXDBNAMESIZE    256

/* Max size of plaintext in one transaction, see net.c:SendTransaction(),
   leave space for encryption padding (assuming max 64*8 = 512-bit cipher
   block size). */
#define CF_SMALLBUF     128
#define CF_BUFSIZE     4096
#define CF_EXPANDSIZE (2 * CF_BUFSIZE)


/*****************************************************************************
 *                          File permissions                                 *
 *****************************************************************************/
// 0600 - Read/Write for owner
#define CF_PERMS_DEFAULT  S_IRUSR | S_IWUSR
// 0644 - World readable
#define CF_PERMS_SHARED   CF_PERMS_DEFAULT | S_IRGRP | S_IROTH

#endif // CFENGINE_DEFINITIONS_H
