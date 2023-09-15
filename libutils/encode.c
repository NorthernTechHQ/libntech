/*
  Copyright 2023 Northern.tech AS

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

#include <openssl/buffer.h>                                 /* BUF_MEM */
#include <openssl/bio.h>                                    /* BIO_* */
#include <openssl/evp.h>                                    /* BIO_f_base64 */

#include <alloc.h>


char *StringEncodeBase64(const char *str, size_t len)
{
    assert(str);
    if (!str)
    {
        return NULL;
    }

    if (len == 0)
    {
        return xcalloc(1, sizeof(char));
    }

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bio = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bio);
    BIO_write(b64, str, len);
    if (!BIO_flush(b64))
    {
        assert(false && "Unable to encode string to base64" && str);
        return NULL;
    }

    BUF_MEM *buffer = NULL;
    BIO_get_mem_ptr(b64, &buffer);
    char *out = xcalloc(1, buffer->length);
    memcpy(out, buffer->data, buffer->length - 1);
    out[buffer->length - 1] = '\0';

    BIO_free_all(b64);

    return out;
}
