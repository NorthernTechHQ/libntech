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

#include <openssl/evp.h>                                       /* EVP_* */
#include <openssl/bn.h>                                        /* BN_bn2bin */

#include <alloc.h>
#include <logging.h>
#include <hash.h>
#include <misc_lib.h>
#include <file_lib.h>
#include <string_lib.h>


static const char *const CF_DIGEST_TYPES[10] =
{
    "md5",
    "sha224",
    "sha256",
    "sha384",
    "sha512",
    "sha1",
    "sha",
    "best",
    "crypt",
    NULL
};

static const int CF_DIGEST_SIZES[10] =
{
    CF_MD5_LEN,
    CF_SHA224_LEN,
    CF_SHA256_LEN,
    CF_SHA384_LEN,
    CF_SHA512_LEN,
    CF_SHA1_LEN,
    CF_SHA_LEN,
    CF_BEST_LEN,
    CF_CRYPT_LEN,
    CF_NO_HASH
};

struct Hash {
    unsigned char digest[EVP_MAX_MD_SIZE];
    char printable[EVP_MAX_MD_SIZE * 4];
    HashMethod method;
    HashSize size;
};

/*
 * These methods are not exported through the public API.
 * These are internal methods used by the constructors of a
 * Hash object. Do not export them since they have very little
 * meaning outside of the constructors.
 */
Hash *HashBasicInit(HashMethod method)
{
    Hash *hash = xcalloc (1, sizeof(Hash));
    hash->size = CF_DIGEST_SIZES[method];
    hash->method = method;
    return hash;
}

void HashCalculatePrintableRepresentation(Hash *hash)
{
    switch (hash->method)
    {
        case HASH_METHOD_MD5:
            strcpy(hash->printable, "MD5=");
            break;
        case HASH_METHOD_SHA224:
        case HASH_METHOD_SHA256:
        case HASH_METHOD_SHA384:
        case HASH_METHOD_SHA512:
        case HASH_METHOD_SHA:
        case HASH_METHOD_SHA1:
            strcpy(hash->printable, "SHA=");
            break;
        default:
            strcpy(hash->printable, "UNK=");
            break;
    }

    unsigned int i;
    for (i = 0; i < hash->size; i++)
    {
        snprintf(hash->printable + 4 + 2 * i,
                 sizeof(hash->printable) - (4 + 2 * i), "%02x",
                 hash->digest[i]);
    }
    hash->printable[4 + 2 * hash->size] = '\0';
}

/*
 * Constructors
 * All constructors call two common methods: HashBasicInit(...) and HashCalculatePrintableRepresentation(...).
 * Each constructor reads the data to create the Hash from different sources so after the basic
 * initialization and up to the point where the hash is computed, each follows its own path.
 */
Hash *HashNew(const char *data, const unsigned int length, HashMethod method)
{
    if (!data || (length == 0))
    {
        return NULL;
    }
    if (method >= HASH_METHOD_NONE)
    {
        return NULL;
    }
    /*
     * OpenSSL documentation marked EVP_DigestInit and EVP_DigestFinal functions as deprecated and
     * recommends moving to EVP_DigestInit_ex and EVP_DigestFinal_ex.
     */
    const EVP_MD *md = NULL;
    md = EVP_get_digestbyname(CF_DIGEST_TYPES[method]);
    if (md == NULL)
    {
        Log(LOG_LEVEL_INFO, "Digest type %s not supported by OpenSSL library", CF_DIGEST_TYPES[method]);
        return NULL;
    }

    EVP_MD_CTX *const context = EVP_MD_CTX_create();
    if (context == NULL)
    {
        Log(LOG_LEVEL_ERR, "Could not allocate openssl hash context");
        return NULL;
    }
    Hash *hash = HashBasicInit(method);
    EVP_DigestInit_ex(context, md, NULL);
    EVP_DigestUpdate(context, data, (size_t) length);
    unsigned int digest_length;
    EVP_DigestFinal_ex(context, hash->digest, &digest_length);
    EVP_MD_CTX_destroy(context);
    /* Update the printable representation */
    HashCalculatePrintableRepresentation(hash);

    return hash;
}

Hash *HashNewFromDescriptor(const int descriptor, HashMethod method)
{
    if (descriptor < 0)
    {
        return NULL;
    }
    if (method >= HASH_METHOD_NONE)
    {
        return NULL;
    }

    const EVP_MD *const md = HashDigestFromId(method);
    if (md == NULL)
    {
        Log(LOG_LEVEL_INFO, "Digest (type=%d) not supported by OpenSSL library", method);
        return NULL;
    }
    EVP_MD_CTX *const context = EVP_MD_CTX_create();
    if (context == NULL)
    {
        Log(LOG_LEVEL_ERR, "Could not allocate openssl hash context");
        return NULL;
    }

    if (EVP_DigestInit_ex(context, md, NULL) != 1)
    {
        Log(LOG_LEVEL_ERR, "Could not initialize openssl hash context");
        EVP_MD_CTX_destroy(context);
        return NULL;
    }

    ssize_t read_count = 0;
    char buffer[1024];
    do
    {
        read_count = read(descriptor, buffer, 1024);
        EVP_DigestUpdate(context, buffer, (size_t) read_count);
    } while (read_count > 0);

    Hash *const hash = HashBasicInit(method); // xcalloc, cannot be NULL
    unsigned int md_len;
    EVP_DigestFinal_ex(context, hash->digest, &md_len);

    /* Update the printable representation */
    HashCalculatePrintableRepresentation(hash);

    EVP_MD_CTX_destroy(context);
    return hash;
}

Hash *HashNewFromKey(const RSA *rsa, HashMethod method)
{
    if (rsa == NULL)
    {
        return NULL;
    }
    if (method >= HASH_METHOD_NONE)
    {
        return NULL;
    }

    const BIGNUM *n, *e;
    RSA_get0_key(rsa, &n, &e, NULL);

    size_t n_len = (n == NULL) ? 0 : (size_t) BN_num_bytes(n);
    size_t e_len = (e == NULL) ? 0 : (size_t) BN_num_bytes(e);
    size_t buf_len = MAX(n_len, e_len);

    if (buf_len <= 0)
    {
        // Should never happen
        Log(LOG_LEVEL_ERR, "Invalid RSA key, internal OpenSSL related error");
        return NULL;
    }

    const EVP_MD *md = EVP_get_digestbyname(CF_DIGEST_TYPES[method]);
    if (md == NULL)
    {
        Log(LOG_LEVEL_INFO, "Digest type %s not supported by OpenSSL library", CF_DIGEST_TYPES[method]);
        return NULL;
    }

    EVP_MD_CTX *context = EVP_MD_CTX_new();
    if (context == NULL)
    {
        Log(LOG_LEVEL_ERR, "Failed to allocate openssl hashing context");
        return NULL;
    }

    if (EVP_DigestInit_ex(context, md, NULL) != 1)
    {
        EVP_MD_CTX_free(context);
        return NULL;
    }

    unsigned char buffer[buf_len];
    size_t actlen;

    actlen = BN_bn2bin(n, buffer);
    CF_ASSERT(actlen <= buf_len, "Buffer overflow n, %zu > %zu!",
              actlen, buf_len);
    EVP_DigestUpdate(context, buffer, actlen);

    actlen = BN_bn2bin(e, buffer);
    CF_ASSERT(actlen <= buf_len, "Buffer overflow e, %zu > %zu!",
              actlen, buf_len);
    EVP_DigestUpdate(context, buffer, actlen);

    Hash *hash = HashBasicInit(method);
    unsigned int digest_length;
    EVP_DigestFinal_ex(context, hash->digest, &digest_length);

    EVP_MD_CTX_free(context);

    /* Update the printable representation */
    HashCalculatePrintableRepresentation(hash);

    return hash;
}

void HashDestroy(Hash **hash)
{
    if (!hash || !*hash)
    {
        return;
    }
    free (*hash);
    *hash = NULL;
}

int HashCopy(Hash *origin, Hash **destination)
{
    if (!origin || !destination)
    {
        return -1;
    }
    *destination = xmalloc(sizeof(Hash));
    memcpy((*destination)->digest, origin->digest, origin->size);
    strlcpy((*destination)->printable, origin->printable, (EVP_MAX_MD_SIZE * 4));
    (*destination)->method = origin->method;
    (*destination)->size = origin->size;
    return 0;
}

bool HashEqual(const Hash *a, const Hash *b)
{
    if (!a && !b)
    {
        return true;
    }
    if (!a && b)
    {
        return false;
    }
    if (a && !b)
    {
        return false;
    }
    if (a->method != b->method)
    {
        return false;
    }
    size_t i = 0;
    for (i = 0; i < a->size; ++i)
    {
        if (a->digest[i] != b->digest[i])
        {
            return false;
        }
    }
    return true;
}

const unsigned char *HashData(const Hash *hash, unsigned int *length)
{
    if (!hash || !length)
    {
        return NULL;
    }
    *length = hash->size;
    return hash->digest;
}

const char *HashPrintable(const Hash *hash)
{
    return hash ? hash->printable : NULL;
}

HashMethod HashType(const Hash *hash)
{
    return hash ? hash->method : HASH_METHOD_NONE;
}

HashSize HashLength(const Hash *hash)
{
    return hash ? hash->size : CF_NO_HASH;
}

/* Class methods */
HashMethod HashIdFromName(const char *hash_name)
{
    int i;
    for (i = 0; CF_DIGEST_TYPES[i] != NULL; i++)
    {
        if (hash_name && (strcmp(hash_name, CF_DIGEST_TYPES[i]) == 0))
        {
            return (HashMethod) i;
        }
    }

    return HASH_METHOD_NONE;
}

const char *HashNameFromId(HashMethod hash_id)
{
    assert(hash_id >= 0);
    return (hash_id >= HASH_METHOD_NONE) ? NULL : CF_DIGEST_TYPES[hash_id];
}

const EVP_MD *HashDigestFromId(HashMethod type)
{
    const char *const name = HashNameFromId(type);
    if (name == NULL)
    {
        return NULL;
    }
    return EVP_get_digestbyname(name);
}


HashSize HashSizeFromId(HashMethod hash_id)
{
    assert(hash_id >= 0);
    return (hash_id >= HASH_METHOD_NONE) ? CF_NO_HASH : CF_DIGEST_SIZES[hash_id];
}

static void HashFile_Stream(
    FILE *const file,
    unsigned char digest[EVP_MAX_MD_SIZE + 1],
    const HashMethod type)
{
    assert(file != NULL);
    const EVP_MD *const md = HashDigestFromId(type);
    if (md == NULL)
    {
        Log(LOG_LEVEL_ERR,
            "Could not determine function for file hashing (type=%d)",
            (int) type);
        return;
    }

    EVP_MD_CTX *const context = EVP_MD_CTX_new();
    if (context == NULL)
    {
        Log(LOG_LEVEL_ERR, "Failed to allocate openssl hashing context");
        return;
    }

    if (EVP_DigestInit(context, md) == 1)
    {
        unsigned char buffer[1024];
        size_t len;
        while ((len = fread(buffer, 1, 1024, file)))
        {
            EVP_DigestUpdate(context, buffer, len);
        }

        unsigned int digest_length;
        EVP_DigestFinal(context, digest, &digest_length);
    }

    EVP_MD_CTX_free(context);
}

/**
 * @param text_mode whether to read the file in text mode or not (binary mode)
 * @note Reading/writing file in text mode on Windows changes Unix newlines
 *       into Windows newlines.
 */
void HashFile(
    const char *const filename,
    unsigned char digest[EVP_MAX_MD_SIZE + 1],
    HashMethod type,
    bool text_mode)
{
    assert(filename != NULL);
    assert(digest != NULL);

    memset(digest, 0, EVP_MAX_MD_SIZE + 1);

    FILE *file = NULL;
    if (text_mode)
    {
        file = safe_fopen(filename, "rt");
    }
    else
    {
        file = safe_fopen(filename, "rb");
    }
    if (file == NULL)
    {
        Log(LOG_LEVEL_INFO,
            "Cannot open file for hashing '%s'. (fopen: %s)",
            filename,
            GetErrorStr());
        return;
    }

    HashFile_Stream(file, digest, type);
    fclose(file);
}

/*******************************************************************/

void HashString(
    const char *const buffer,
    const int len,
    unsigned char digest[EVP_MAX_MD_SIZE + 1],
    HashMethod type)
{
    assert(buffer != NULL);
    assert(digest != NULL);
    assert(type != HASH_METHOD_CRYPT);

    memset(digest, 0, EVP_MAX_MD_SIZE + 1);

    if (type == HASH_METHOD_CRYPT)
    {
        Log(LOG_LEVEL_ERR,
            "The crypt support is not presently implemented, please use another algorithm instead");
        return;
    }

    const EVP_MD *const md = HashDigestFromId(type);
    if (md == NULL)
    {
        Log(LOG_LEVEL_ERR,
            "Could not determine function for file hashing (type=%d)",
            (int) type);
        return;
    }

    EVP_MD_CTX *const context = EVP_MD_CTX_new();
    if (context == NULL)
    {
        Log(LOG_LEVEL_ERR, "Failed to allocate openssl hashing context");
        return;
    }

    if (EVP_DigestInit(context, md) == 1)
    {
        EVP_DigestUpdate(context, buffer, len);
        EVP_DigestFinal(context, digest, NULL);
    }
    else
    {
        Log(LOG_LEVEL_ERR,
            "Failed to initialize digest for hashing: '%s'",
            buffer);
    }

    EVP_MD_CTX_free(context);
}

/*******************************************************************/

void HashPubKey(
    const RSA *const key,
    unsigned char digest[EVP_MAX_MD_SIZE + 1],
    const HashMethod type)
{
    assert(key != NULL);
    assert(type != HASH_METHOD_CRYPT);

    memset(digest, 0, EVP_MAX_MD_SIZE + 1);

    if (type == HASH_METHOD_CRYPT)
    {
        Log(LOG_LEVEL_ERR,
            "The crypt support is not presently implemented, please use sha256 instead");
        return;
    }

    const EVP_MD *const md = HashDigestFromId(type);
    if (md == NULL)
    {
        Log(LOG_LEVEL_ERR,
            "Could not determine function for file hashing (type=%d)",
            (int) type);
        return;
    }

    EVP_MD_CTX *const context = EVP_MD_CTX_new();
    if (context == NULL)
    {
        Log(LOG_LEVEL_ERR, "Failed to allocate openssl hashing context");
        return;
    }


    if (EVP_DigestInit(context, md) == 1)
    {
        const BIGNUM *n, *e;
        RSA_get0_key(key, &n, &e, NULL);

        const size_t n_len = (n == NULL) ? 0 : (size_t) BN_num_bytes(n);
        const size_t e_len = (e == NULL) ? 0 : (size_t) BN_num_bytes(e);
        const size_t buf_len = MAX(n_len, e_len);

        unsigned char buffer[buf_len];
        size_t actlen;
        actlen = BN_bn2bin(n, buffer);
        CF_ASSERT(actlen <= buf_len, "Buffer overflow n, %zu > %zu!",
                  actlen, buf_len);
        EVP_DigestUpdate(context, buffer, actlen);

        actlen = BN_bn2bin(e, buffer);
        CF_ASSERT(actlen <= buf_len, "Buffer overflow e, %zu > %zu!",
                  actlen, buf_len);
        EVP_DigestUpdate(context, buffer, actlen);

        unsigned int digest_length;
        EVP_DigestFinal(context, digest, &digest_length);
    }

    EVP_MD_CTX_free(context);
}

/*******************************************************************/

bool HashesMatch(
    const unsigned char digest1[EVP_MAX_MD_SIZE + 1],
    const unsigned char digest2[EVP_MAX_MD_SIZE + 1],
    HashMethod type)
{
    const HashSize size = HashSizeFromId(type);
    if (size <= 0) // HashSize is an enum (so int)
    {
        return false;
    }

    return (memcmp(digest1, digest2, size) == 0);
}

/* TODO rewrite this ugliness, currently it's not safe, it truncates! */
/**
 * @WARNING #dst must have enough space to hold the result!
 */
char *HashPrintSafe(char *dst, size_t dst_size, const unsigned char *digest,
                    HashMethod type, bool use_prefix)
{
    const char *prefix;

    if (use_prefix)
    {
        prefix = type == HASH_METHOD_MD5 ? "MD5=" : "SHA=";
    }
    else
    {
        prefix = "";
    }

    size_t dst_len = MIN(dst_size - 1, strlen(prefix));
    memcpy(dst, prefix, dst_len);

    size_t digest_len = HashSizeFromId(type);
    assert(dst_size >= strlen(prefix) + digest_len*2 + 1);

#ifndef NDEBUG // Avoids warning.
    size_t ret =
#endif
        StringBytesToHex(&dst[dst_len], dst_size - dst_len,
                         digest, digest_len);
    assert(ret == 2 * digest_len);

#if 0 /* TODO return proper exit status and check it in the callers */
    if (ret < 2 * digest_len)
    {
        return NULL;
    }
#endif

    return dst;
}


char *SkipHashType(char *hash)
{
    char *str = hash;

    if(STARTSWITH(hash, "MD5=") || STARTSWITH(hash, "SHA="))
    {
        str = hash + 4;
    }

    return str;
}

size_t StringCopyTruncateAndHashIfNecessary(
    const char *const src, char *const dst, size_t dst_size)
{
    assert(src != NULL);
    assert(dst != NULL);
    assert(dst_size > (32 + 5)); // Must be big enough for MD5 hex and prefix

    const size_t length = StringCopy(src, dst, dst_size);
    if (length < dst_size)
    {
        // String length smaller than destination buffer size
        // safe to return, no truncation/hashing necessary
        return length;
    }
    assert(length == dst_size);

    const char md5_prefix[] = "#MD5=";
    const size_t md5_prefix_length = sizeof(md5_prefix) - 1;
    const size_t md5_hex_length = 32;
    const size_t md5_string_length = md5_hex_length + md5_prefix_length;
    assert(dst_size > md5_string_length);
    assert(md5_prefix_length == strlen(md5_prefix));

    // Hash the original string using MD5:
    unsigned char digest[EVP_MAX_MD_SIZE + 1];
    HashString(src, strlen(src), digest, HASH_METHOD_MD5);

    // Calculate where the hash should start:
    char *const terminator = dst + dst_size - 1;
    assert(*terminator == '\0');
    char *const hash_prefix_start = terminator - md5_string_length;
    assert(hash_prefix_start >= dst);
    char *const hash_start = hash_prefix_start + sizeof(md5_prefix) - 1;
    assert(hash_start > hash_prefix_start);

    // Insert the "#MD5=" part into dst;
    memcpy(hash_prefix_start, md5_prefix, md5_prefix_length);

    // Produce the hex string representation of MD5 hash:
    // (Overwrite the last part of dst)
    const char lookup[]="0123456789abcdef";
    assert((md5_hex_length % 2) == 0);
    for (size_t i = 0; i < md5_hex_length / 2; i++)
    {
        hash_start[i * 2]     = lookup[digest[i] >> 4];
        hash_start[i * 2 + 1] = lookup[digest[i] & 0xf];
    }
    assert(hash_start[md5_hex_length] == '\0');
    assert(hash_start + md5_hex_length == terminator);

    return dst_size;
}
