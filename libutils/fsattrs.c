#include <platform.h>
#include <fsattrs.h>
#include <logging.h>
#include <file_lib.h>

#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h> /* FS_* constants */
#endif                /* HAVE_LINUX_FS_H */


/**
 * @brief Get / Set immutable flag for file
 * @param filename The filename
 * @param flag The flag
 * @param get_flag Whether to get (i.e., true) or set (i.e., false) the flag
 * @return Error code (see FSAttrsResult)
 * @note Only regular files are currently supported
 */
static FSAttrsResult ImmutableFlag(
    const char *filename, bool *flag, bool get_flag)
{
    assert(filename != NULL);
    assert(flag != NULL);

#if defined(HAVE_LINUX_FS_H)

    /* Since we have to open the file, to use ioctl() we must make sure that
     * it is in fact a regular file before doing so. E.g., a FIFO file could
     * cause open(2) to block indefinitely. */

    struct stat sb;
    if (lstat(filename, &sb) == -1)
    {
        return (errno == ENOENT) ? FS_ATTRS_DOES_NOT_EXIST : FS_ATTRS_FAILURE;
    }
    if (!S_ISREG(sb.st_mode))
    {
        return FS_ATTRS_NOT_SUPPORTED;
    }

    int fd = safe_open(filename, O_RDONLY);
    if (fd == -1)
    {
        return (errno == ENOENT) ? FS_ATTRS_DOES_NOT_EXIST : FS_ATTRS_FAILURE;
    }

    int attrs;
    int ret = ioctl(fd, FS_IOC_GETFLAGS, &attrs);
    if (ret < 0)
    {
        close(fd);
        return (errno == ENOTTY) ? FS_ATTRS_NOT_SUPPORTED : FS_ATTRS_FAILURE;
    }

    if (get_flag)
    {
        close(fd);
        *flag = attrs & FS_IMMUTABLE_FL;
        return FS_ATTRS_SUCCESS;
    }

    if (*flag)
    {
        if (attrs & FS_IMMUTABLE_FL)
        {
            Log(LOG_LEVEL_DEBUG,
                "The immutable bit is already set in inode flags for file '%s'",
                filename);
            close(fd);
            return FS_ATTRS_SUCCESS;
        }

        Log(LOG_LEVEL_DEBUG,
            "Setting immutable bit in inode flags for file '%s'",
            filename);
        attrs |= FS_IMMUTABLE_FL;
    }
    else
    {
        if (!(attrs & FS_IMMUTABLE_FL))
        {
            Log(LOG_LEVEL_DEBUG,
                "The immutable bit is already cleared in inode flags for file '%s'",
                filename);
            close(fd);
            return FS_ATTRS_SUCCESS;
        }

        Log(LOG_LEVEL_DEBUG,
            "Clearing immutable bit in inode flags for file '%s'",
            filename);
        attrs &= ~FS_IMMUTABLE_FL;
    }

    ret = ioctl(fd, FS_IOC_SETFLAGS, &attrs);
    close(fd);
    if (ret < 0)
    {
        return (errno == ENOTTY) ? FS_ATTRS_NOT_SUPPORTED : FS_ATTRS_FAILURE;
    }
    return FS_ATTRS_SUCCESS;

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) \
    || defined(__NetBSD__)

    struct stat sb;
    int ret = lstat(filename, &sb);
    if (ret == -1)
    {
        return (errno == ENOENT) ? FS_ATTRS_DOES_NOT_EXIST : FS_ATTRS_FAILURE;
    }

    if (get_flag)
    {
        *flag = sb.st_flags & SF_IMMUTABLE;
        return FS_ATTRS_SUCCESS;
    }

    if (*flag)
    {
        if (sb.st_flags & SF_IMMUTABLE)
        {
            Log(LOG_LEVEL_DEBUG,
                "The immutable bit is already set in inode flags for file '%s'",
                filename);
            return FS_ATTRS_SUCCESS;
        }

        Log(LOG_LEVEL_DEBUG,
            "Setting immutable bit in inode flags for file '%s'",
            filename);
        sb.st_flags |= SF_IMMUTABLE;
    }
    else
    {
        if (!(sb.st_flags & SF_IMMUTABLE))
        {
            Log(LOG_LEVEL_DEBUG,
                "The immutable bit is already cleared in inode flags for file '%s'",
                filename);
            return FS_ATTRS_SUCCESS;
        }

        Log(LOG_LEVEL_DEBUG,
            "Clearing immutable bit in inode flags for file '%s'",
            filename);
        sb.st_flags &= ~SF_IMMUTABLE;
    }

    ret = chflags(filename, sb.st_flags);
    if (ret == -1)
    {
        return (errno == ENOTSUP) ? FS_ATTRS_NOT_SUPPORTED : FS_ATTRS_FAILURE;
    }

    return FS_ATTRS_SUCCESS;

#else
    /* Windows, AIX, HP-UX does not have file system flags as far as I can
       tell */
    UNUSED(filename);
    UNUSED(flag);
    UNUSED(get_flag);
    return FS_ATTRS_NOT_SUPPORTED;
#endif
}


FSAttrsResult FSAttrsGetImmutableFlag(const char *filename, bool *flag)
{
    return ImmutableFlag(filename, flag, true);
}

FSAttrsResult FSAttrsUpdateImmutableFlag(const char *filename, bool flag)
{
    return ImmutableFlag(filename, &flag, false);
}
