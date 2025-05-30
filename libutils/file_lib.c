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
#include <file_lib.h>
#include <misc_lib.h>
#include <dir.h>
#include <logging.h>

#include <alloc.h>
#include <definitions.h>                               /* CF_PERMS_DEFAULT */
#include <libgen.h>
#include <logging.h>
#include <string_lib.h>                                         /* memcchr */
#include <path.h>

/* below are includes for the fancy efficient file/data copying on Linux */
#ifdef __linux__


#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>  /* FICLONE */
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
#include <sys/stat.h>
#include <unistd.h>    /* copy_file_range(), lseek() */
#endif /* __linux__ */

#ifdef __MINGW32__
#include <windows.h>            /* LockFileEx and friends */
#endif

#define SYMLINK_MAX_DEPTH 32

bool FileCanOpen(const char *path, const char *modes)
{
    FILE *test = NULL;

    if ((test = fopen(path, modes)) != NULL)
    {
        fclose(test);
        return true;
    }
    else
    {
        return false;
    }
}

#define READ_BUFSIZE 4096

Writer *FileRead(const char *filename, size_t max_size, bool *truncated)
{
    int fd = safe_open(filename, O_RDONLY);
    if (fd == -1)
    {
        return NULL;
    }

    Writer *w = FileReadFromFd(fd, max_size, truncated);
    close(fd);
    return w;
}

ssize_t ReadFileStreamToBuffer(FILE *file, size_t max_bytes, char *buf)
{
    size_t bytes_read = 0;
    size_t n = 0;
    while (bytes_read < max_bytes)
    {
        n = fread(buf + bytes_read, 1, max_bytes - bytes_read, file);
        if (ferror(file) && !feof(file))
        {
            return FILE_ERROR_READ;
        }
        else if (n == 0)
        {
            break;
        }
        bytes_read += n;
    }
    if (ferror(file))
    {
        return FILE_ERROR_READ;
    }
    return bytes_read;
}

bool File_Copy(const char *src, const char *dst)
{
    assert(src != NULL);
    assert(dst != NULL);

    Log(LOG_LEVEL_INFO, "Copying: '%s' -> '%s'", src, dst);

    FILE *in = safe_fopen(src, "r");
    if (in == NULL)
    {
        Log(LOG_LEVEL_ERR, "Could not open '%s' (%s)", src, strerror(errno));
        return false;
    }

    FILE *out = safe_fopen_create_perms(dst, "w", CF_PERMS_DEFAULT);
    if (out == NULL)
    {
        Log(LOG_LEVEL_ERR, "Could not open '%s' (%s)", dst, strerror(errno));
        fclose(in);
        return false;
    }

    size_t bytes_in = 0;
    size_t bytes_out = 0;
    bool ret = true;
    do
    {
#define BUFSIZE 1024
        char buf[BUFSIZE] = {0};

        bytes_in = fread(buf, sizeof(char), sizeof(buf), in);
        bytes_out = fwrite(buf, sizeof(char), bytes_in, out);
        while (bytes_out < bytes_in && !ferror(out))
        {
            bytes_out += fwrite(
                buf + bytes_out, sizeof(char), bytes_in - bytes_out, out);
        }
    } while (!feof(in) && !ferror(in) && !ferror(out) &&
             bytes_in == bytes_out);

    if (ferror(in))
    {
        Log(LOG_LEVEL_ERR, "Error encountered while reading '%s'", src);
        ret = false;
    }
    else if (ferror(out))
    {
        Log(LOG_LEVEL_ERR, "Error encountered while writing '%s'", dst);
        ret = false;
    }
    else if (bytes_in != bytes_out)
    {
        Log(LOG_LEVEL_ERR, "Did not copy the whole file");
        ret = false;
    }

    const int i = fclose(in);
    if (i != 0)
    {
        Log(LOG_LEVEL_ERR,
            "Error encountered while closing '%s' (%s)",
            src,
            strerror(errno));
        ret = false;
    }
    const int o = fclose(out);
    if (o != 0)
    {
        Log(LOG_LEVEL_ERR,
            "Error encountered while closing '%s' (%s)",
            dst,
            strerror(errno));
        ret = false;
    }
    return ret;
}

bool File_CopyToDir(const char *src, const char *dst_dir)
{
    assert(src != NULL);
    assert(dst_dir != NULL);
    assert(StringEndsWith(dst_dir, FILE_SEPARATOR_STR));

    const char *filename = Path_Basename(src);
    if (filename == NULL)
    {
        Log(LOG_LEVEL_ERR, "Cannot find filename in '%s'", src);
        return false;
    }

    char dst[PATH_MAX] = {0};
    const int s = snprintf(dst, PATH_MAX, "%s%s", dst_dir, filename);
    if (s >= PATH_MAX)
    {
        Log(LOG_LEVEL_ERR, "Copy destination path too long: '%s...'", dst);
        return false;
    }

    if (!File_Copy(src, dst))
    {
        Log(LOG_LEVEL_ERR, "Copying '%s' failed", filename);
        return false;
    }

    return true;
}

Writer *FileReadFromFd(int fd, size_t max_size, bool *truncated)
{
    if (truncated)
    {
        *truncated = false;
    }

    Writer *w = StringWriter();
    for (;;)
    {
        char buf[READ_BUFSIZE];
        /* Reading more data than needed is deliberate. It is a truncation detection. */
        ssize_t read_ = read(fd, buf, READ_BUFSIZE);

        if (read_ == 0)
        {
            /* Done. */
            return w;
        }
        else if (read_ < 0)
        {
            if (errno != EINTR)
            {
                /* Something went wrong. */
                WriterClose(w);
                return NULL;
            }
            /* Else: interrupted - try again. */
        }
        else if (read_ + StringWriterLength(w) > max_size)
        {
            WriterWriteLen(w, buf, max_size - StringWriterLength(w));
            /* Reached limit - stop. */
            if (truncated)
            {
                *truncated = true;
            }
            return w;
        }
        else /* Filled buffer; copy and ask for more. */
        {
            WriterWriteLen(w, buf, read_);
        }
    }
}

ssize_t FullWrite(int desc, const char *ptr, size_t len)
{
    ssize_t total_written = 0;

    while (len > 0)
    {
        int written = write(desc, ptr, len);

        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return written;
        }

        total_written += written;
        ptr += written;
        len -= written;
    }

    return total_written;
}

ssize_t FullRead(int fd, char *ptr, size_t len)
{
    ssize_t total_read = 0;

    while (len > 0)
    {
        ssize_t bytes_read = read(fd, ptr, len);

        if (bytes_read < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return -1;
        }

        if (bytes_read == 0)
        {
            return total_read;
        }

        total_read += bytes_read;
        ptr += bytes_read;
        len -= bytes_read;
    }

    return total_read;
}

/**
 * @note difference with files_names.h:IsDir() is that this doesn't
 *       follow symlinks, so a symlink is never a directory...
 */
bool IsDirReal(const char *path)
{
    struct stat s;

    if (lstat(path, &s) == -1)
    {
        return false; // Error
    }

    return (S_ISDIR(s.st_mode) != 0);
}

#ifndef __MINGW32__
NewLineMode FileNewLineMode(ARG_UNUSED const char *file)
{
    return NewLineMode_Unix;
}
#endif // !__MINGW32__

bool IsWindowsNetworkPath(const char *const path)
{
#ifdef _WIN32
    int off = 0;

    while (path[off] == '\"')
    {
        // Bypass quoted strings
        off += 1;
    }

    if (IsFileSep(path[off]) && IsFileSep(path[off + 1]))
    {
        return true;
    }
#else // _WIN32
    UNUSED(path);
#endif // _WIN32
    return false;
}

bool IsWindowsDiskPath(const char *const path)
{
#ifdef _WIN32
    int off = 0;

    while (path[off] == '\"')
    {
        // Bypass quoted strings
        off += 1;
    }

    if (isalpha(path[off]) && path[off + 1] == ':' && IsFileSep(path[off + 2]))
    {
        return true;
    }
#else // _WIN32
    UNUSED(path);
#endif // _WIN32
    return false;
}

bool IsAbsoluteFileName(const char *f)
{
    int off = 0;

// Check for quoted strings

    for (off = 0; f[off] == '\"'; off++)
    {
    }

    if (IsWindowsNetworkPath(f))
    {
        return true;
    }
    if (IsWindowsDiskPath(f))
    {
        return true;
    }
    if (IsFileSep(f[off]))
    {
        return true;
    }

    return false;
}

/* We assume that s is at least MAX_FILENAME large.
 * MapName() is thread-safe, but the argument is modified. */

#ifdef _WIN32
# if defined(__MINGW32__)

char *MapNameCopy(const char *s)
{
    char *str = xstrdup(s);

    char *c = str;
    while ((c = strchr(c, '/')))
    {
        *c = '\\';
    }

    return str;
}

char *MapName(char *s)
{
    char *c = s;

    while ((c = strchr(c, '/')))
    {
        *c = '\\';
    }
    return s;
}

# elif defined(__CYGWIN__)

char *MapNameCopy(const char *s)
{
    Writer *w = StringWriter();

    /* c:\a\b -> /cygdrive/c\a\b */
    if (s[0] && isalpha(s[0]) && s[1] == ':')
    {
        WriterWriteF(w, "/cygdrive/%c", s[0]);
        s += 2;
    }

    for (; *s; s++)
    {
        /* a//b//c -> a/b/c */
        /* a\\b\\c -> a\b\c */
        if (IsFileSep(*s) && IsFileSep(*(s + 1)))
        {
            continue;
        }

        /* a\b\c -> a/b/c */
        WriterWriteChar(w, *s == '\\' ? '/' : *s);
    }

    return StringWriterClose(w);
}

char *MapName(char *s)
{
    char *ret = MapNameCopy(s);

    if (strlcpy(s, ret, MAX_FILENAME) >= MAX_FILENAME)
    {
        FatalError(ctx, "Expanded path (%s) is longer than MAX_FILENAME ("
                   TO_STRING(MAX_FILENAME) ") characters",
                   ret);
    }
    free(ret);

    return s;
}

# else/* !__MINGW32__ && !__CYGWIN__ */
#  error Unknown NT-based compilation environment
# endif/* __MINGW32__ || __CYGWIN__ */
#else /* !_WIN32 */

char *MapName(char *s)
{
    return s;
}

char *MapNameCopy(const char *s)
{
    return xstrdup(s);
}

#endif /* !_WIN32 */

char *MapNameForward(char *s)
/* Like MapName(), but maps all slashes to forward */
{
    while ((s = strchr(s, '\\')))
    {
        *s = '/';
    }
    return s;
}


#ifdef TEST_SYMLINK_ATOMICITY
void switch_symlink_hook();
#define TEST_SYMLINK_SWITCH_POINT switch_symlink_hook();
#else
#define TEST_SYMLINK_SWITCH_POINT
#endif

void PathWalk(
    const char *const path,
    PathWalkFn callback,
    void *const data,
    PathWalkCopyFn copy,
    PathWalkDestroyFn destroy)
{
    assert(path != NULL);
    assert(callback != NULL);

    Seq *const children = ListDir(path, NULL);
    if (children == NULL) {
        Log(
            LOG_LEVEL_DEBUG,
            "Failed to list directory '%s'. Subdirectories will not be "
            "iterated.", path);
        return;
    }
    const size_t n_children = SeqLength(children);

    Seq *const dirnames = SeqNew(1, free);
    Seq *const filenames = SeqNew(1, free);

    for (size_t i = 0; i < n_children; i++)
    {
        char *const child = SeqAt(children, i);

        /* The basename(3) function might potentially mutate the argument.
         * Thus, we make a copy to pass instead. */
        char buf[PATH_MAX];
        const size_t ret = StringCopy(child, buf, PATH_MAX);
        if (ret >= PATH_MAX) {
            Log(LOG_LEVEL_ERR,
                "Failed to copy path: Path too long (%zu >= %d)",
                ret, PATH_MAX);
            SeqDestroy(dirnames);
            SeqDestroy(filenames);
            return;
        }
        const char *const b_name = basename(buf);

        /* We don't iterate the '.' and '..' directory entries as it would cause
         * infinite recursion. */
        if (StringEqual(b_name, ".") || StringEqual(b_name, ".."))
        {
            continue;
        }

        // Note that stat(2) follows symbolic links.
        struct stat sb;
        if (stat(child, &sb) == 0)
        {
            char *const dup = xstrdup(b_name);
            if (sb.st_mode & S_IFDIR)
            {
                SeqAppend(dirnames, dup);
            }
            else
            {
                SeqAppend(filenames, dup);
            }
        }
        else
        {
            Log(LOG_LEVEL_DEBUG,
                "Failed to stat file '%s': %s",
                child,
                GetErrorStr());
        }
    }

    SeqDestroy(children);
    callback(path, dirnames, filenames, data);
    SeqDestroy(filenames);

    // Recursively walk through subdirectories.
    const size_t n_dirs = SeqLength(dirnames);
    for (size_t i = 0; i < n_dirs; i++)
    {
        const char *const dir = SeqAt(dirnames, i);
        if (dir != NULL)
        {
            char *const duplicate = (copy != NULL) ? copy(data) : data;
            if (StringEqual(path, "."))
            {
                PathWalk(dir, callback, duplicate, copy, destroy);
            }
            else
            {
                char *next = Path_JoinAlloc(path, dir);
                PathWalk(next, callback, duplicate, copy, destroy);
                free(next);
            }
            if (copy != NULL && destroy != NULL)
            {
                destroy(duplicate);
            }
        }
    }

    SeqDestroy(dirnames);
}

Seq *ListDir(const char *dir, const char *extension)
{
    Dir *dirh = DirOpen(dir);
    if (dirh == NULL)
    {
        return NULL;
    }

    Seq *contents = SeqNew(10, free);

    const struct dirent *dirp;

    while ((dirp = DirRead(dirh)) != NULL)
    {
        const char *name = dirp->d_name;
        if (extension == NULL || StringEndsWithCase(name, extension, true))
        {
            SeqAppend(contents, Path_JoinAlloc(dir, name));
        }
    }
    DirClose(dirh);

    return contents;
}

mode_t SetUmask(mode_t new_mask)
{
    const mode_t old_mask = umask(new_mask);
    Log(LOG_LEVEL_DEBUG, "Set umask to %o, was %o", new_mask, old_mask);
    return old_mask;
}
void RestoreUmask(mode_t old_mask)
{
    umask(old_mask);
    Log(LOG_LEVEL_DEBUG, "Restored umask to %o", old_mask);
}

/**
 * Opens a file safely, with default (strict) permissions on creation.
 * See safe_open_create_perms for more documentation.
 *
 * @param pathname The path to open.
 * @param flags Same flags as for system open().
 * @return Same errors as open().
 */
int safe_open(const char *pathname, int flags)
{
    return safe_open_create_perms(pathname, flags, CF_PERMS_DEFAULT);
}

/**
 * Opens a file safely. It will follow symlinks, but only if the symlink is
 * trusted, that is, if the owner of the symlink and the owner of the target are
 * the same, or if the owner of the symlink is either root or the user running
 * the current process. All components are checked, even symlinks encountered in
 * earlier parts of the path name.
 *
 * It should always be used when opening a file or directory that is not
 * guaranteed to be owned by root.
 *
 * safe_open and safe_fopen both default to secure (0600) file creation perms.
 * The _create_perms variants allow you to explicitly set different permissions.
 *
 * @param pathname The path to open
 * @param flags Same flags as for system open()
 * @param create_perms Permissions for file, only relevant on file creation
 * @return Same errors as open()
 * @see safe_fopen_create_perms()
 * @see safe_open()
 */
int safe_open_create_perms(
    const char *const pathname, int flags, const mode_t create_perms)
{
    if (flags & O_TRUNC)
    {
        /* Undefined behaviour otherwise, according to the standard. */
        assert((flags & O_RDWR) || (flags & O_WRONLY));
    }

    if (!pathname)
    {
        errno = EINVAL;
        return -1;
    }

    if (*pathname == '\0')
    {
        errno = ENOENT;
        return -1;
    }

#ifdef __MINGW32__
    // Windows gets off easy. No symlinks there.
    return open(pathname, flags, create_perms);
#elif defined(__ANDROID__)
    // if effective user is not root then don't try to open
    // all paths from '/' up, might not have permissions.
    uid_t p_euid = geteuid();
    if (p_euid != 0)
    {
        return open(pathname, flags, create_perms);
    }
#else // !__MINGW32__ and !__ANDROID__
    const size_t path_bufsize = strlen(pathname) + 1;
    char path[path_bufsize];
    const size_t res_len = StringCopy(pathname, path, path_bufsize);
    UNUSED(res_len);
    assert(res_len == strlen(pathname));
    int currentfd;
    const char *first_dir;
    bool trunc = false;
    const int orig_flags = flags;
    char *next_component = path;
    bool p_uid;

    if (*next_component == '/')
    {
        first_dir = "/";
        // Eliminate double slashes.
        while (*(++next_component) == '/') { /*noop*/ }
        if (!*next_component)
        {
            next_component = NULL;
        }
    }
    else
    {
        first_dir = ".";
    }
    currentfd = openat(AT_FDCWD, first_dir, O_RDONLY);
    if (currentfd < 0)
    {
        return -1;
    }

    // current process user id
    p_uid = geteuid();

    size_t final_size = (size_t) -1;
    while (next_component)
    {
        char *component = next_component;
        next_component = strchr(component + 1, '/');
        // Used to restore the slashes in the final path component.
        char *restore_slash = NULL;
        if (next_component)
        {
            restore_slash = next_component;
            *next_component = '\0';
            // Eliminate double slashes.
            while (*(++next_component) == '/') { /*noop*/ }
            if (!*next_component)
            {
                next_component = NULL;
            }
            else
            {
                restore_slash = NULL;
            }
        }

        // In cases of a race condition when creating a file, our attempt to open it may fail
        // (see O_EXCL and O_CREAT flags below). However, this can happen even under normal
        // circumstances, if we are unlucky; it does not mean that someone is up to something bad.
        // So retry it a few times before giving up.
        int attempts = 3;
        trunc = false;
        while (true)
        {

            if ((  (orig_flags & O_RDWR) || (orig_flags & O_WRONLY))
                && (orig_flags & O_TRUNC))
            {
                trunc = true;
                /* We need to check after we have opened the file whether we
                 * opened the right one. But if we truncate it, the damage is
                 * already done, we have destroyed the contents, and that file
                 * might have been a symlink to /etc/shadow! So save that flag
                 * and apply the truncation afterwards instead. */
                flags &= ~O_TRUNC;
            }

            if (restore_slash)
            {
                *restore_slash = '\0';
            }

            struct stat stat_before, stat_after;
            int stat_before_result = fstatat(currentfd, component, &stat_before, AT_SYMLINK_NOFOLLOW);
            if (stat_before_result < 0
                && (errno != ENOENT
                    || next_component // Meaning "not a leaf".
                    || !(flags & O_CREAT)))
            {
                close(currentfd);
                return -1;
            }

            /*
             * This testing entry point is about the following real-world
             * scenario: There can be an attacker that at this point
             * overwrites the existing file or writes a file, invalidating
             * basically the previous fstatat().
             *
             * - We make sure that can't happen if the file did not exist, by
             *   creating with O_EXCL.
             * - We make sure that can't happen if the file existed, by
             *   comparing with fstat() result after the open().
             *
             */
            TEST_SYMLINK_SWITCH_POINT

            if (!next_component)                          /* last component */
            {
                if (stat_before_result < 0)
                {
                    assert(flags & O_CREAT);

                    // Doesn't exist. Make sure *we* create it.
                    flags |= O_EXCL;

                    /* No need to ftruncate() the file at the end. */
                    trunc = false;
                }
                else
                {
                    if ((flags & O_CREAT) && (flags & O_EXCL))
                    {
                        close(currentfd);
                        errno = EEXIST;
                        return -1;
                    }

                    // Already exists. Make sure we *don't* create it.
                    flags &= ~O_CREAT;
                }
                if (restore_slash)
                {
                    *restore_slash = '/';
                }
                int filefd = openat(currentfd, component, flags, create_perms);
                if (filefd < 0)
                {
                    if ((stat_before_result < 0  && !(orig_flags & O_EXCL)  && errno == EEXIST) ||
                        (stat_before_result >= 0 &&  (orig_flags & O_CREAT) && errno == ENOENT))
                    {
                        if (--attempts >= 0)
                        {
                            // Might be our fault. Try again.
                            flags = orig_flags;
                            continue;
                        }
                        else
                        {
                            // Too many attempts. Give up.
                            // Most likely a link that is in the way of file creation, but can also
                            // be a file that is constantly created and deleted (race condition).
                            // It is not possible to separate between the two, so return EACCES to
                            // signal that we denied access.
                            errno = EACCES;
                        }
                    }
                    close(currentfd);
                    return -1;
                }
                close(currentfd);
                currentfd = filefd;
            }
            else
            {
                int new_currentfd = openat(currentfd, component, O_RDONLY);
                close(currentfd);
                if (new_currentfd < 0)
                {
                    return -1;
                }
                currentfd = new_currentfd;
            }

            /* If file did exist, we fstat() again and compare with previous. */

            if (stat_before_result == 0)
            {
                if (fstat(currentfd, &stat_after) < 0)
                {
                    close(currentfd);
                    return -1;
                }
                // Some attacks may use symlinks to get higher privileges
                // The safe cases are:
                // * symlinks owned by root
                // * symlinks owned by the user running the process
                // * symlinks that have the same owner and group as the destination
                if (stat_before.st_uid != 0 &&
                    stat_before.st_uid != p_uid &&
                    (stat_before.st_uid != stat_after.st_uid || stat_before.st_gid != stat_after.st_gid))
                {
                    close(currentfd);
                    Log(LOG_LEVEL_ERR, "Cannot follow symlink '%s'; it is not "
                        "owned by root or the user running this process, and "
                        "the target owner and/or group differs from that of "
                        "the symlink itself.", pathname);
                    // Return ENOLINK to signal that the link cannot be followed
                    // ('Link has been severed').
                    errno = ENOLINK;
                    return -1;
                }

                final_size = (size_t) stat_after.st_size;
            }

            // If we got here, we've been successful, so don't try again.
            break;
        }
    }

    /* Truncate if O_CREAT and the file preexisted. */
    if (trunc)
    {
        /* Do not truncate if the size is already zero, some
         * filesystems don't support ftruncate() with offset>=size. */
        assert(final_size != (size_t) -1);

        if (final_size != 0)
        {
            int tr_ret = ftruncate(currentfd, 0);
            if (tr_ret < 0)
            {
                Log(LOG_LEVEL_ERR,
                    "safe_open: unexpected failure (ftruncate: %s)",
                    GetErrorStr());
                close(currentfd);
                return -1;
            }
        }
    }

    return currentfd;
#endif // !__MINGW32__
}

FILE *safe_fopen(const char *const path, const char *const mode)
{
    return safe_fopen_create_perms(path, mode, CF_PERMS_DEFAULT);
}

/**
 * Opens a file safely. It will follow symlinks, but only if the symlink is trusted,
 * that is, if the owner of the symlink and the owner of the target are the same,
 * or if the owner of the symlink is either root or the user running the current process.
 * All components are checked, even symlinks encountered in earlier parts of the
 * path name.
 *
 * It should always be used when opening a directory that is not guaranteed to be
 * owned by root.
 *
 * @param pathname The path to open.
 * @param flags Same mode as for system fopen().
 * @param create_perms Permissions for file, only relevant on file creation.
 * @return Same errors as fopen().
 */
FILE *safe_fopen_create_perms(
    const char *const path, const char *const mode, const mode_t create_perms)
{
    if (!path || !mode)
    {
        errno = EINVAL;
        return NULL;
    }

    char fdopen_mode_str[3] = {0};
    int fdopen_mode_idx = 0;

    int flags = 0;
    for (int c = 0; mode[c]; c++)
    {
        /* Ignore mode letters not supported in fdopen() for the mode string
         * that we will later use in fdopen(). Mode letters like 'x' will for
         * example cause fdopen() to return error on Windows. According to the
         * man page fdopen(3), the mode must be one of "r", "r+", "w", "w+",
         * "a", "a+". Windows also have additional characters that are
         * Microsoft extensions, but advice against using them to preserve
         * ANSI portability. */
        if (strchr("rwa+", mode[c]) != NULL)
        {
            if (fdopen_mode_idx >= (sizeof(fdopen_mode_str) - 1))
            {
                ProgrammingError("Invalid flag for fdopen: %s", mode);
            }
            fdopen_mode_str[fdopen_mode_idx++] = mode[c];
        }

        switch (mode[c])
        {
        case 'r':
            flags |= O_RDONLY;
            break;
        case 'w':
            flags |= O_WRONLY | O_TRUNC | O_CREAT;
            break;
        case 'a':
            flags |= O_WRONLY | O_CREAT;
            break;
        case '+':
            flags &= ~(O_RDONLY | O_WRONLY);
            flags |= O_RDWR;
            break;
        case 'b':
            flags |= O_BINARY;
            break;
        case 't':
            flags |= O_TEXT;
            break;
        case 'x':
            flags |= O_EXCL;
            break;
        default:
            ProgrammingError("Invalid flag for fopen: %s", mode);
            return NULL;
        }
    }

    int fd = safe_open_create_perms(path, flags, create_perms);
    if (fd < 0)
    {
        return NULL;
    }

    FILE *ret = fdopen(fd, fdopen_mode_str);
    if (!ret)
    {
        close(fd);
        return NULL;
    }

    if (mode[0] == 'a')
    {
        if (fseek(ret, 0, SEEK_END) < 0)
        {
            fclose(ret);
            return NULL;
        }
    }

    return ret;
}

/**
 * Use this instead of chdir(). It changes into the directory safely, using safe_open().
 * @param path Path to change into.
 * @return Same return values as chdir().
 */
int safe_chdir(const char *path)
{
#ifdef __MINGW32__
    return chdir(path);
#else // !__MINGW32__
    int fd = safe_open(path, O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    if (fchdir(fd) < 0)
    {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
#endif // !__MINGW32__
}

#ifndef __MINGW32__

/**
 * Opens the true parent dir of the file in the path given. The notable
 * difference from doing it the naive way (open(dirname(path))) is that it
 * can follow the symlinks of the path, ending up in the true parent dir of the
 * path. It follows the same safe mechanisms as `safe_open()` to do so. If
 * AT_SYMLINK_NOFOLLOW is given, it is equivalent to doing it the naive way (but
 * still following "safe" semantics).
 * @param path           Path to open parent directory of.
 * @param flags          Flags to use for fchownat.
 * @param link_user      If we have traversed a link already, which user was it.
 * @param link_group     If we have traversed a link already, which group was it.
 * @param traversed_link Whether we have traversed a link. If this is false the
 *                       two previus arguments are ignored. This is used enforce
 *                       the correct UID/GID combination when following links.
 *                       Initially this is false, but will be set to true in
 *                       sub invocations if we follow a link.
 * @param loop_countdown Protection against infinite loop following.
 * @param true_leaf      Place to store the true leaf name (basename) *initialized
 *                       to %NULL* or %NULL
 * @return File descriptor pointing to the parent directory of path, or -1 on
 *         error.
 */
static int safe_open_true_parent_dir(const char *path,
                                     int flags,
                                     uid_t link_user,
                                     gid_t link_group,
                                     bool traversed_link,
                                     int loop_countdown,
                                     char **true_leaf)
{
    assert((true_leaf == NULL) || (*true_leaf == NULL));

    int dirfd = -1;
    int ret = -1;

    char *parent_dir_alloc = xstrdup(path);
    char *leaf_alloc = xstrdup(path);
    char *parent_dir = dirname(parent_dir_alloc);
    char *leaf = basename(leaf_alloc);
    struct stat statbuf;
    uid_t p_uid = geteuid();

    if ((dirfd = safe_open(parent_dir, O_RDONLY)) == -1)
    {
        goto cleanup;
    }

    if ((ret = fstatat(dirfd, leaf, &statbuf, AT_SYMLINK_NOFOLLOW)) == -1)
    {
        goto cleanup;
    }

    // Some attacks may use symlinks to get higher privileges
    // The safe cases are:
    // * symlinks owned by root
    // * symlinks owned by the user running the process
    // * symlinks that have the same owner and group as the destination
    if (traversed_link &&
        link_user != 0 &&
        link_user != p_uid &&
        (link_user != statbuf.st_uid || link_group != statbuf.st_gid))
    {
        errno = ENOLINK;
        ret = -1;
        goto cleanup;
    }

    if (S_ISLNK(statbuf.st_mode) && !(flags & AT_SYMLINK_NOFOLLOW))
    {
        if (--loop_countdown <= 0)
        {
            ret = -1;
            errno = ELOOP;
            goto cleanup;
        }

        // Add one byte for '\0', and one byte to make sure size doesn't change
        // in between calls.
        char *link = xmalloc(statbuf.st_size + 2);
        ret = readlinkat(dirfd, leaf, link, statbuf.st_size + 1);
        if (ret < 0 || ret > statbuf.st_size)
        {
            // Link either disappeared or was changed under our feet. Be safe
            // and bail out.
            free(link);
            errno = ENOLINK;
            ret = -1;
            goto cleanup;
        }
        link[ret] = '\0';

        char *resolved_link;
        if (link[0] == FILE_SEPARATOR)
        {
            // Takes ownership of link's memory, so no free().
            resolved_link = link;
        }
        else
        {
            xasprintf(&resolved_link, "%s%c%s", parent_dir,
                      FILE_SEPARATOR, link);
            free(link);
        }

        ret = safe_open_true_parent_dir(resolved_link, flags, statbuf.st_uid,
                                        statbuf.st_gid, true, loop_countdown,
                                        true_leaf);

        free(resolved_link);
        goto cleanup;
    }

    // We now know it either isn't a link, or we don't want to follow it if it
    // is. Return the parent dir.
    ret = dirfd;
    dirfd = -1;

cleanup:
    free(parent_dir_alloc);

    /* We only want to set this once, in the deepest recursive call of this
     * function (see above). */
    if ((true_leaf != NULL) && (*true_leaf == NULL))
    {
        *true_leaf = xstrdup(leaf);
    }

    free(leaf_alloc);

    if (dirfd != -1)
    {
        close(dirfd);
    }

    return ret;
}

/**
 * Implementation of safe_chown.
 * @param path   Path to chown.
 * @param owner  Owner to set on path.
 * @param group  Group to set on path.
 * @param flags  Flags to use for fchownat.
 */
static int safe_chown_impl(const char *path, uid_t owner, gid_t group, int flags)
{
    int dirfd;
    char *leaf_alloc = NULL;
    char *leaf;
    if ((flags & AT_SYMLINK_NOFOLLOW) != 0)
    {
        /* SYMLINK_NOFOLLOW set, we can take leaf name from path */
        dirfd = safe_open_true_parent_dir(path, flags, 0, 0, false, SYMLINK_MAX_DEPTH, NULL);
        leaf_alloc = xstrdup(path);
        leaf = basename(leaf_alloc);
    }
    else
    {
        /* no SYMLINK_NOFOLLOW -> follow symlinks, thus we need the real leaf
         * name */
        dirfd = safe_open_true_parent_dir(path, flags, 0, 0, false, SYMLINK_MAX_DEPTH, &leaf_alloc);
        leaf = leaf_alloc;
    }
    if (dirfd < 0)
    {
        free(leaf_alloc);
        return -1;
    }


    // We now know it either isn't a link, or we don't want to follow it if it
    // is. In either case make sure we don't try to follow it.
    flags |= AT_SYMLINK_NOFOLLOW;

    int ret = fchownat(dirfd, leaf, owner, group, flags);
    free(leaf_alloc);
    close(dirfd);
    return ret;
}

#endif // !__MINGW32__

/**
 * Use this instead of chown(). It changes file owner safely, using safe_open().
 * @param path Path to operate on.
 * @param owner Owner.
 * @param group Group.
 * @return Same return values as chown().
 */
int safe_chown(const char *path, uid_t owner, gid_t group)
{
#ifdef __MINGW32__
    return chown(path, owner, group);
#else // !__MINGW32__
    return safe_chown_impl(path, owner, group, 0);
#endif // !__MINGW32__
}

/**
 * Use this instead of lchown(). It changes file owner safely, using safe_open().
 * @param path Path to operate on.
 * @param owner Owner.
 * @param group Group.
 * @return Same return values as lchown().
 */
#ifndef __MINGW32__
int safe_lchown(const char *path, uid_t owner, gid_t group)
{
    return safe_chown_impl(path, owner, group, AT_SYMLINK_NOFOLLOW);
}
#endif // !__MINGW32__

/**
 * Use this instead of chmod(). It changes file permissions safely, using safe_open().
 * @param path Path to operate on.
 * @param mode Permissions.
 * @return Same return values as chmod().
 */
int safe_chmod(const char *path, mode_t mode)
{
#ifdef __MINGW32__
    return chmod(path, mode);
#else // !__MINGW32__
    int dirfd = -1;
    int ret = -1;

    char *leaf_alloc = NULL;
    if ((dirfd = safe_open_true_parent_dir(path, 0, 0, 0, false, SYMLINK_MAX_DEPTH, &leaf_alloc)) == -1)
    {
        goto cleanup;
    }

    char *leaf = basename(leaf_alloc);
    struct stat statbuf;
    if ((ret = fstatat(dirfd, leaf, &statbuf, AT_SYMLINK_NOFOLLOW)) == -1)
    {
        goto cleanup;
    }

    uid_t olduid = 0;
    if (S_ISFIFO(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode))
    {
        /* For FIFOs/sockets we cannot resort to the method of opening the file
           first, since it might block. But we also cannot use chmod directly,
           because the file may be switched with a symlink to a sensitive file
           under our feet, and there is no way to avoid following it. So
           instead, switch effective UID to the owner of the FIFO, and then use
           chmod.
        */

        /* save old euid */
        olduid = geteuid();

        if ((ret = seteuid(statbuf.st_uid)) == -1)
        {
            goto cleanup;
        }

        ret = fchmodat(dirfd, leaf, mode, 0);

        // Make sure EUID is set back before we check error condition, so that we
        // never return with lowered privileges.
        if (seteuid(olduid) == -1)
        {
            ProgrammingError("safe_chmod: Could not set EUID back. Should never happen.");
        }

        goto cleanup;
    }

    int file_fd = safe_open(path, 0);
    if (file_fd < 0)
    {
        ret = -1;
        goto cleanup;
    }

    ret = fchmod(file_fd, mode);
    close(file_fd);

cleanup:
    free(leaf_alloc);

    if (dirfd != -1)
    {
        close(dirfd);
    }

    return ret;
#endif // !__MINGW32__
}

/**
 * Use this instead of creat(). It creates a file safely, using safe_open().
 * @param path Path to operate on.
 * @param mode Permissions.
 * @return Same return values as creat().
 */
int safe_creat(const char *pathname, mode_t mode)
{
    return safe_open_create_perms(pathname,
                                  O_CREAT | O_WRONLY | O_TRUNC,
                                  mode);
}

// Windows implementation in Enterprise.
#ifndef _WIN32
bool SetCloseOnExec(int fd, bool enable)
{
    int flags = fcntl(fd, F_GETFD);
    if (enable)
    {
        flags |= FD_CLOEXEC;
    }
    else
    {
        flags &= ~FD_CLOEXEC;
    }
    return (fcntl(fd, F_SETFD, flags) == 0);
}
#endif // !_WIN32

static bool DeleteDirectoryTreeInternal(const char *basepath, const char *path)
{
    Dir *dirh = DirOpen(path);
    const struct dirent *dirp;
    bool failed = false;

    if (dirh == NULL)
    {
        if (errno == ENOENT)
        {
            /* Directory disappeared on its own */
            return true;
        }

        Log(LOG_LEVEL_INFO, "Unable to open directory '%s' during purge of directory tree '%s' (opendir: %s)",
            path, basepath, GetErrorStr());
        return false;
    }

    for (dirp = DirRead(dirh); dirp != NULL; dirp = DirRead(dirh))
    {
        if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
        {
            continue;
        }

        char subpath[PATH_MAX];
        snprintf(subpath, sizeof(subpath), "%s" FILE_SEPARATOR_STR "%s", path, dirp->d_name);

        struct stat lsb;
        if (lstat(subpath, &lsb) == -1)
        {
            if (errno == ENOENT)
            {
                /* File disappeared on its own */
                continue;
            }

            Log(LOG_LEVEL_VERBOSE, "Unable to stat file '%s' during purge of directory tree '%s' (lstat: %s)", path, basepath, GetErrorStr());
            failed = true;
        }
        else
        {
            if (S_ISDIR(lsb.st_mode))
            {
                if (!DeleteDirectoryTreeInternal(basepath, subpath))
                {
                    failed = true;
                }

                if (rmdir(subpath) == -1)
                {
                    failed = true;
                }
            }
            else
            {
                if (unlink(subpath) == -1)
                {
                    if (errno == ENOENT)
                    {
                        /* File disappeared on its own */
                        continue;
                    }

                    Log(LOG_LEVEL_VERBOSE, "Unable to remove file '%s' during purge of directory tree '%s'. (unlink: %s)",
                        subpath, basepath, GetErrorStr());
                    failed = true;
                }
            }
        }
    }

    DirClose(dirh);
    return !failed;
}

bool DeleteDirectoryTree(const char *path)
{
    return DeleteDirectoryTreeInternal(path, path);
}

/**
 * @NOTE Better use FileSparseCopy() if you are copying file to file
 *       (that one callse this function).
 *
 * @NOTE Always use FileSparseWrite() to close the file descriptor, to avoid
 *       losing data.
 */
bool FileSparseWrite(int fd, const void *buf, size_t count,
                     bool *wrote_hole)
{
    bool all_zeroes = (memcchr(buf, '\0', count) == NULL);

    if (all_zeroes)                                     /* write a hole */
    {
        off_t seek_ret = lseek(fd, count, SEEK_CUR);
        if (seek_ret == (off_t) -1)
        {
            Log(LOG_LEVEL_ERR,
                "Failed to write a hole in sparse file (lseek: %s)",
                GetErrorStr());
            return false;
        }
    }
    else                                              /* write normally */
    {
        ssize_t w_ret = FullWrite(fd, buf, count);
        if (w_ret < 0)
        {
            Log(LOG_LEVEL_ERR,
                "Failed to write to destination file (write: %s)",
                GetErrorStr());
            return false;
        }
    }

    *wrote_hole = all_zeroes;
    return true;
}

/**
 * Below is a basic implementation of copying sparse files shoveling data from
 * sd to dd detecting blocks of zeroes.
 *
 * On Linux, we try more fancy and efficient approaches than what is done
 * below if any are supported.
 */
#ifdef __linux__
#if HAVE_SENDFILE || HAVE_COPY_FILE_RANGE
#if HAVE_DECL_SEEK_DATA && HAVE_DECL_FALLOC_FL_PUNCH_HOLE
#define SMART_FILE_COPY_SYSCALLS_AVAILABLE 1
#define SMART_SYSCALLS_UNUSED ARG_UNUSED
#endif  /* HAVE_DECL_SEEK_DATA && HAVE_DECL_FALLOC_FL_PUNCH_HOLE */
#endif  /* HAVE_SENDFILE || HAVE_COPY_FILE_RANGE */
#endif  /* __linux__ */
#ifndef SMART_FILE_COPY_SYSCALLS_AVAILABLE
#define SMART_FILE_COPY_SYSCALLS_AVAILABLE 0
#define SMART_SYSCALLS_UNUSED
#endif

SMART_SYSCALLS_UNUSED static bool FileSparseCopyShoveling(int sd, const char *src_name,
                                                          int dd, const char *dst_name,
                                                          size_t blk_size,
                                                          size_t *total_bytes_written,
                                                          bool   *last_write_was_a_hole)
{
    assert(total_bytes_written   != NULL);
    assert(last_write_was_a_hole != NULL);

    const size_t buf_size  = blk_size;
    void *buf              = xmalloc(buf_size);

    size_t n_read_total = 0;
    bool   retval       = false;

    *last_write_was_a_hole = false;

    while (true)
    {
        ssize_t n_read = FullRead(sd, buf, buf_size);
        if (n_read < 0)
        {
            Log(LOG_LEVEL_ERR,
                "Unable to read source file while copying '%s' to '%s'"
                " (read: %s)", src_name, dst_name, GetErrorStr());
            break;
        }
        else if (n_read == 0)                                   /* EOF */
        {
            retval = true;
            break;
        }

        bool ret = FileSparseWrite(dd, buf, n_read,
                                   last_write_was_a_hole);
        if (!ret)
        {
            Log(LOG_LEVEL_ERR, "Failed to copy '%s' to '%s'",
                src_name, dst_name);
            break;
        }

        n_read_total += n_read;
    }

    free(buf);
    *total_bytes_written   = n_read_total;
    return retval;
}

/**
 * Copy data jumping over areas filled by '\0' greater than blk_size, so
 * files automatically become sparse if possible.
 *
 * File descriptors should already be open, the filenames #source and
 * #destination are only for logging purposes.
 *
 * @NOTE Always use FileSparseClose() to close the file descriptor, to avoid
 *       losing data.
 */
bool FileSparseCopy(int sd, const char *src_name,
                    int dd, const char *dst_name,
                    SMART_SYSCALLS_UNUSED size_t blk_size,
                    size_t *total_bytes_written,
                    bool *last_write_was_a_hole)
{
    assert(total_bytes_written != NULL);
    assert(last_write_was_a_hole != NULL);

#if !SMART_FILE_COPY_SYSCALLS_AVAILABLE
    return FileSparseCopyShoveling(sd, src_name, dd, dst_name, blk_size,
                                   total_bytes_written, last_write_was_a_hole);
#else
    /* We rely on the errno value below so make sure it's not spoofed by an
     * error from outside. */
    errno = 0;

    size_t input_size;
    struct stat in_sb;
    if (fstat(sd, &in_sb) == 0)
    {
        input_size = in_sb.st_size;
    }
    else
    {
        Log(LOG_LEVEL_ERR, "Failed to stat() '%s': %m", src_name);
        *total_bytes_written = 0;
        return false;
    }
    bool same_dev = false;
    struct stat out_sb;
    if (fstat(dd, &out_sb) == 0)
    {
        same_dev = in_sb.st_dev == out_sb.st_dev;
    }
    else
    {
        Log(LOG_LEVEL_ERR, "Failed to stat() '%s': %m", dst_name);
        *total_bytes_written = 0;
        return false;
    }
#if HAVE_DECL_FICLONE
    if (same_dev)
    {
        /* If they are on the same device, first try to use FICLONE
         * (man:ioctl_ficlone(2)) if available because nothing can beat that.
         * However, it can easily fail if the FS doesn't support it or if we are
         * making a copy accross multiple file systems.
         */
        if (ioctl(dd, FICLONE, sd) == 0)
        {
            Log(LOG_LEVEL_VERBOSE, "'%s' copied successfully to '%s'", src_name, dst_name);
            *total_bytes_written = input_size;
            *last_write_was_a_hole = false;
            return true;
        }
        else
        {
            Log(LOG_LEVEL_DEBUG, "Failed to use FICLONE to copy '%s' to '%s': %m", src_name, dst_name);
            /* let's try other things */
            errno = 0;
        }
    }
#endif  /* HAVE_DECL_FICLONE */
    if (ftruncate(dd, input_size) != 0)
    {
        Log(LOG_LEVEL_ERR, "Failed to preset size for '%s': %m", dst_name);
        *total_bytes_written = 0;
        return false;
    }
    if (in_sb.st_blocks == 0)
    {
        /* Nothing to copy. */
        Log(LOG_LEVEL_VERBOSE, "'%s' copied successfully to '%s'", src_name, dst_name);
        *total_bytes_written = input_size;
        *last_write_was_a_hole = false;
        return true;
    }

    /* man:inode(7) says about st_blocks:
     * This field indicates the number of blocks allocated to the file, 512-byte
     * units, (This may be smaller than st_size/512 when the file has holes.)
     * The POSIX.1 standard notes that the unit for the st_blocks member of the stat
     * structure is not defined by the standard.  On many implementations it is 512
     * bytes; on a few systems, a different unit is used, such as 1024. Furthermore,
     * the unit may differ on a per-filesystem basis.
     * So using 512 is the best we can do although it might not be perfect.
     */
    if (fallocate(dd, FALLOC_FL_KEEP_SIZE|FALLOC_FL_PUNCH_HOLE, 0, in_sb.st_blocks * 512) != 0)
    {
        Log((errno == EOPNOTSUPP) ? LOG_LEVEL_VERBOSE : LOG_LEVEL_ERR,
            "Failed to pre-allocate space for '%s': %m", dst_name);
        if (errno != EOPNOTSUPP) {
            *total_bytes_written = 0;
            return false;
        }
    }

    off_t in_pos = 0;
    off_t out_pos = 0;
    bool done = false;
    int error = 0;
    while (!done)
    {
        error = 0;
        in_pos = lseek(sd, in_pos, SEEK_DATA);
        if (in_pos == -1)
        {
            if (errno == ENXIO)
            {
                /* ENXIO means we are either seeking past the end of file or
                   that we seek data and there is only a hole till the end of
                   the file. IOW, we are done. */
                done = true;
                break;
            }
            else
            {
                error = errno;
                Log(LOG_LEVEL_ERR, "Failed to seek to next data in '%s'", src_name);
                break;
            }
        }
        if (in_pos != out_pos)
        {
            out_pos = lseek(dd, in_pos - out_pos, SEEK_CUR);
            if (out_pos == -1) {
                error = errno;
                Log(LOG_LEVEL_ERR, "Failed to advance descriptor in '%s'", dst_name);
                break;
            }
        }
        off_t next_hole = lseek(sd, in_pos, SEEK_HOLE);
        if (next_hole == -1)
        {
            error = errno;
            Log(LOG_LEVEL_ERR, "Failed to find next hole in '%s'", src_name);
            break;
        }
        if (lseek(sd, in_pos, SEEK_SET) == -1)
        {
            error = errno;
            Log(LOG_LEVEL_ERR, "Failed to seek back to data in '%s'", src_name);
            break;
        }
        ssize_t n_copied;
#ifdef HAVE_COPY_FILE_RANGE
        /* copy_file_range() only works on the same device (file system), but
         * unlike sendfile() it can actually create reflinks instead of copying
         * the data. */
        if (same_dev)
        {
            n_copied = copy_file_range(sd, NULL, dd, NULL, next_hole - in_pos, 0);
            error = errno;
        }
        else
#endif /* HAVE_COPY_FILE_RANGE */
        {
            n_copied = sendfile(dd, sd, NULL, next_hole - in_pos);
            error = errno;
        }
        if (n_copied > 0)
        {
            in_pos += n_copied;
            out_pos += n_copied;
            *total_bytes_written += n_copied;
        }
        done = ((n_copied <= 0) || ((size_t) in_pos == input_size));
    }
    *last_write_was_a_hole = false;
    return (error == 0);
#endif  /* SMART_FILE_COPY_SYSCALLS_AVAILABLE */
}
#undef SMART_FILE_COPY_SYSCALLS_AVAILABLE

/**
 * Always close a written sparse file using this function, else truncation
 * might occur if the last part was a hole.
 *
 * If the tail of the file was a hole (and hence lseek(2)ed on destination
 * instead of being written), do a ftruncate(2) here to ensure the whole file
 * is written to the disc. But ftruncate() fails with EPERM on non-native
 * Linux filesystems (e.g. vfat, vboxfs) when the count is >= than the
 * size of the file. So we write() one byte and then ftruncate() it back.
 *
 * No need for this function to return anything, since the filedescriptor is
 * (attempted to) closed in either success or failure.
 *
 * TODO? instead of needing the #total_bytes_written parameter, we could
 * figure the offset after writing the one byte using lseek(fd,0,SEEK_CUR) and
 * truncate -1 from that offset. It's probably not worth adding an extra
 * system call for simplifying code.
 */
bool FileSparseClose(int fd, const char *filename,
                     bool do_sync,
                     SMART_SYSCALLS_UNUSED size_t total_bytes_written,
                     SMART_SYSCALLS_UNUSED bool last_write_was_hole)
{
#if SMART_FILE_COPY_SYSCALLS_AVAILABLE
    /* Not much to do here with smart syscalls (see above). */
    bool success = true;
    if (do_sync && (fsync(fd) != 0))
    {
        Log(LOG_LEVEL_WARNING, "Could not sync to disk file '%s': %m)", filename);
        success = false;
    }
    close(fd);
    return success;
#else
    if (last_write_was_hole)
    {
        ssize_t ret1 = FullWrite(fd, "", 1);
        if (ret1 == -1)
        {
            Log(LOG_LEVEL_ERR,
                "Failed to close sparse file '%s' (write: %s)",
                filename, GetErrorStr());
            close(fd);
            return false;
        }

        int ret2 = ftruncate(fd, total_bytes_written);
        if (ret2 == -1)
        {
            Log(LOG_LEVEL_ERR,
                "Failed to close sparse file '%s' (ftruncate: %s)",
                filename, GetErrorStr());
            close(fd);
            return false;
        }
    }

    if (do_sync)
    {
        if (fsync(fd) != 0)
        {
            Log(LOG_LEVEL_WARNING,
                "Could not sync to disk file '%s' (fsync: %s)",
                filename, GetErrorStr());
        }
    }

    int ret3 = close(fd);
    if (ret3 == -1)
    {
        Log(LOG_LEVEL_ERR,
            "Failed to close file '%s' (close: %s)",
            filename, GetErrorStr());
        return false;
    }

    return true;
#endif /* SMART_FILE_COPY_SYSCALLS_AVAILABLE */
}
#undef SMART_FILE_COPY_SYSCALLS_AVAILABLE

ssize_t CfReadLine(char **buff, size_t *size, FILE *fp)
{
    ssize_t b = getline(buff, size, fp);
    assert(b != 0 && "To the best of my knowledge, getline never returns zero");

    if (b > 0)
    {
        if ((*buff)[b - 1] == '\n')
        {
            (*buff)[b - 1] = '\0';
            b--;
        }
    }

    return b;
}

ssize_t CfReadLines(char **buff, size_t *size, FILE *fp, Seq *lines)
{
    assert(size != NULL);
    assert(fp != NULL);
    assert(lines != NULL);
    assert((buff != NULL) || *size == 0);

    ssize_t appended = 0;

    ssize_t ret;
    bool free_buff = (buff == NULL);
    while (!feof(fp))
    {
        assert((buff != NULL) || *size == 0);
        ret = CfReadLine(buff, size, fp);
        if (ret == -1)
        {
            if (!feof(fp))
            {
                if (free_buff)
                {
                    free(*buff);
                }
                return -1;
            }
        }
        else
        {
            SeqAppend(lines, xstrdup(*buff));
            appended++;
        }
    }
    if (free_buff)
    {
        free(*buff);
    }

    return appended;
}

/*******************************************************************/


#if !defined(__MINGW32__)

static int LockFD(int fd, short int lock_type, bool wait)
{
    struct flock lock_spec = {
        .l_type = lock_type,
        .l_whence = SEEK_SET,
        .l_start = 0, /* start of the region to which the lock applies */
        .l_len = 0    /* till EOF */
    };

    if (wait)
    {
        while (fcntl(fd, F_SETLKW, &lock_spec) == -1)
        {
            if (errno != EINTR)
            {
                Log(LOG_LEVEL_DEBUG, "Failed to acquire file lock for FD %d: %s",
                    fd, GetErrorStr());
                return -1;
            }
        }
        return 0;
    }
    else
    {
        if (fcntl(fd, F_SETLK, &lock_spec) == -1)
        {
            Log(LOG_LEVEL_DEBUG, "Failed to acquire file lock for FD %d: %s",
                fd, GetErrorStr());
            return -1;
        }
        /* else */
        return 0;
    }
}

static int UnlockFD(int fd)
{
    struct flock lock_spec = {
        .l_type = F_UNLCK,
        .l_whence = SEEK_SET,
        .l_start = 0, /* start of the region to which the lock applies */
        .l_len = 0    /* till EOF */
    };

    if (fcntl(fd, F_SETLK, &lock_spec) == -1)
    {
        Log(LOG_LEVEL_DEBUG, "Failed to release file lock for FD %d: %s",
            fd, GetErrorStr());
        return -1;
    }
    /* else */
    return 0;
}

int ExclusiveFileLock(FileLock *lock, bool wait)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    return LockFD(lock->fd, F_WRLCK, wait);
}

int SharedFileLock(FileLock *lock, bool wait)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    return LockFD(lock->fd, F_RDLCK, wait);
}

bool ExclusiveFileLockCheck(FileLock *lock)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    struct flock lock_spec = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0, /* start of the region to which the lock applies */
        .l_len = 0    /* till EOF */
    };
    if (fcntl(lock->fd, F_GETLK, &lock_spec) == -1)
    {
        /* should never happen */
        Log(LOG_LEVEL_ERR, "Error when checking locks on FD %d", lock->fd);
        return false;
    }
    return (lock_spec.l_type == F_UNLCK);
}

int ExclusiveFileUnlock(FileLock *lock, bool close_fd)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    if (close_fd)
    {
        /* also releases the lock */
        int ret = close(lock->fd);
        if (ret != 0)
        {
            Log(LOG_LEVEL_ERR, "Failed to close lock file with FD %d: %s",
                lock->fd, GetErrorStr());
            lock->fd = -1;
            return -1;
        }
        /* else*/
        lock->fd = -1;
        return 0;
    }
    else
    {
        return UnlockFD(lock->fd);
    }
}

int SharedFileUnlock(FileLock *lock, bool close_fd)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    /* unlocking is the same for both kinds of locks */
    return ExclusiveFileUnlock(lock, close_fd);
}

#else  /* __MINGW32__ */

static int LockFD(int fd, DWORD flags, bool wait)
{
    OVERLAPPED ol = { 0 };
    ol.Offset = INT_MAX;

    if (!wait)
    {
        flags |= LOCKFILE_FAIL_IMMEDIATELY;
    }

    HANDLE fh = (HANDLE)_get_osfhandle(fd);

    if (!LockFileEx(fh, flags, 0, 1, 0, &ol))
    {
        Log(LOG_LEVEL_DEBUG, "Failed to acquire file lock for FD %d: %s",
            fd, GetErrorStr());
        return -1;
    }

    return 0;
}

int ExclusiveFileLock(FileLock *lock, bool wait)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    return LockFD(lock->fd, LOCKFILE_EXCLUSIVE_LOCK, wait);
}

int SharedFileLock(FileLock *lock, bool wait)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    return LockFD(lock->fd, 0, wait);
}

static int UnlockFD(int fd)
{
    OVERLAPPED ol = { 0 };
    ol.Offset = INT_MAX;

    HANDLE fh = (HANDLE)_get_osfhandle(fd);

    if (!UnlockFileEx(fh, 0, 1, 0, &ol))
    {
        Log(LOG_LEVEL_DEBUG, "Failed to release file lock for FD %d: %s",
            fd, GetErrorStr());
        return -1;
    }

    return 0;
}

bool ExclusiveFileLockCheck(FileLock *lock)
{
    /* XXX: there seems to be no way to check if the current process is holding
     * a lock on a file */
    return false;
}

int ExclusiveFileUnlock(FileLock *lock, bool close_fd)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    int ret = UnlockFD(lock->fd);
    if (close_fd)
    {
        close(lock->fd);
        lock->fd = -1;
    }
    return ret;
}

int SharedFileUnlock(FileLock *lock, bool close_fd)
{
    assert(lock != NULL);
    assert(lock->fd >= 0);

    /* unlocking is the same for both kinds of locks */
    return ExclusiveFileUnlock(lock, close_fd);
}

#endif  /* __MINGW32__ */

int ExclusiveFileLockPath(FileLock *lock, const char *fpath, bool wait)
{
    assert(lock != NULL);
    assert(lock->fd < 0);

    int fd = safe_open(fpath, O_CREAT|O_RDWR);
    if (fd < 0)
    {
        Log(LOG_LEVEL_ERR, "Failed to open '%s' for locking", fpath);
        return -2;
    }

    lock->fd = fd;
    int ret = ExclusiveFileLock(lock, wait);
    if (ret != 0)
    {
        lock->fd = -1;
    }
    return ret;
}

int SharedFileLockPath(FileLock *lock, const char *fpath, bool wait)
{
    assert(lock != NULL);
    assert(lock->fd < 0);

    int fd = safe_open(fpath, O_CREAT|O_RDONLY);
    if (fd < 0)
    {
        Log(LOG_LEVEL_ERR, "Failed to open '%s' for locking", fpath);
        return -2;
    }

    lock->fd = fd;
    int ret = SharedFileLock(lock, wait);
    if (ret != 0)
    {
        lock->fd = -1;
    }
    return ret;
}
