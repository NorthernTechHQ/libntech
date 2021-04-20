/*
  Copyright 2021 Northern.tech AS

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

#include <platform.h>

#include <map.h>                /* Map* */
#include <alloc.h>              /* xmalloc, xasprintf */
#include <string_lib.h>         /* StringHash_untyped, StringEqual_untyped */
#include <logging.h>
#include <proc_manager.h>

#ifndef __MINGW32__
#include <errno.h>
#include <signal.h>             /* kill, SIGKILL */
#endif

#ifdef __MINGW32__
#define fileno _fileno
#endif

#define SIGKILL_TERMINATION_TIMEOUT 5 /* seconds */

struct ProcManager_ {
    Map *procs_by_id;  /** primary storage for the subprocesses info (owns the data)    */
    Map *procs_by_fd;  /** a different view on the data to speed up lookup by FD/stream */
    Map *procs_by_pid; /** a different view on the data to speed up lookup by PID       */
};

static unsigned int HashFD_untyped(const void *fd, ARG_UNUSED unsigned int seed)
{
    assert(fd != NULL);
    return *((unsigned int*) fd);
}

static bool FDEqual_untyped(const void *fd1, const void *fd2)
{
    assert((fd1 != NULL) && (fd2 != NULL));
    return (*((int*) fd1) == *((int*) fd2));
}

static unsigned int HashPID_untyped(const void *pid, ARG_UNUSED unsigned int seed)
{
    assert(pid != NULL);
    return *((unsigned int*) pid);
}

static bool PIDEqual_untyped(const void *pid1, const void *pid2)
{
    assert((pid1 != NULL) && (pid2 != NULL));
    return (*((pid_t*) pid1) == *((pid_t*) pid2));
}

static void NoopDestroy(ARG_UNUSED void *p)
{
    /* not destroying anything here */
}

ProcManager *ProcManagerNew()
{
    ProcManager *manager = xmalloc(sizeof(ProcManager));

    /* keys are just pointers into data, this map actually holds the data */
    manager->procs_by_id = MapNew(StringHash_untyped, StringEqual_untyped,
                                  NoopDestroy, (MapDestroyDataFn) SubprocessDestroy);

    /* keys are allocated, data belongs to the procs_by_id map */
    manager->procs_by_fd = MapNew(HashFD_untyped, FDEqual_untyped,
                                  free, NoopDestroy);

    /* keys are just pointers into data, data belongs to the procs_by_id map */
    manager->procs_by_pid = MapNew(HashPID_untyped, PIDEqual_untyped,
                                   NoopDestroy, NoopDestroy);

    return manager;
}

void ProcManagerDestroy(ProcManager *manager)
{
    if (manager != NULL)
    {
        /* procs_by_id owns the data */
        MapSoftDestroy(manager->procs_by_fd);
        MapSoftDestroy(manager->procs_by_pid);
        MapDestroy(manager->procs_by_id);
        free(manager);
    }
}

static inline Subprocess *SubprocessNew(char *id, char *cmd, char *description,
                                        pid_t pid, FILE *input, FILE *output, char lookup_io)
{
    Subprocess *ret  = xmalloc(sizeof(Subprocess));
    ret->id          = id;
    ret->cmd         = cmd;
    ret->description = description;
    ret->pid         = pid;
    ret->input       = input;
    ret->output      = output;
    ret->lookup_io   = lookup_io;

    return ret;
}

void SubprocessDestroy(Subprocess *proc)
{
    if (proc != NULL)
    {
        free(proc->id);
        free(proc->cmd);
        free(proc->description);
        free(proc);
    }
}

static inline int *DupInt(int val)
{
    int *ret = xmalloc(sizeof(int));
    *ret = val;
    return ret;
}

bool ProcManagerAddProcess(ProcManager *manager,
                           char *id, char *cmd, char *description, pid_t pid,
                           FILE *input, FILE *output, char lookup_io)
{
    assert(manager != NULL);

    if (id == NULL)
    {
        NDEBUG_UNUSED int ret = xasprintf(&id, "%jd", (intmax_t) pid);
        assert(ret > 0);
    }

    int *lookup_fd = NULL;
    if (lookup_io == 'i')
    {
        assert(input != NULL);
        int fd = fileno(input);
        if (fd >= 0)
        {
            lookup_fd = DupInt(fd);
        }
        else
        {
            Log(LOG_LEVEL_ERR, "Failed to get FD for input file stream for the process '%s'",
                description != NULL ? description : id);
        }
    }
    else if (lookup_io == 'o')
    {
        assert(output != NULL);
        int fd = fileno(output);
        if (fd >= 0)
        {
            lookup_fd = DupInt(fd);
        }
        else
        {
            Log(LOG_LEVEL_ERR, "Failed to get FD for output file stream for the process '%s'",
                description != NULL ? description : id);
        }
    }
    else
    {
        assert(lookup_io == '\0');
    }

    if (MapHasKey(manager->procs_by_id, id) ||
        MapHasKey(manager->procs_by_pid, &pid) ||
        ((lookup_fd != NULL) && MapHasKey(manager->procs_by_fd, lookup_fd)))
    {
        Log(LOG_LEVEL_ERR,
            "Attempt to insert the process '%s:%jd:%d' ['%s'] ('%s')' twice into the same process manager",
            id, (intmax_t) pid, *lookup_fd, description, cmd);

        free(id);
        free(cmd);
        free(description);
        free(lookup_fd);

        return false;
    }

    Subprocess *proc = SubprocessNew(id, cmd, description, pid, input, output, lookup_io);
    MapInsert(manager->procs_by_id, id, proc);
    MapInsert(manager->procs_by_pid, &(proc->pid), proc);

    if (lookup_fd != NULL)
    {
        MapInsert(manager->procs_by_fd, lookup_fd, proc);
    }

    return true;
}


Subprocess *ProcManagerGetProcessByPID(ProcManager *manager, pid_t pid)
{
    assert(manager != NULL);
    return (Subprocess*) MapGet(manager->procs_by_pid, &pid);
}

Subprocess *ProcManagerGetProcessById(ProcManager *manager, const char *id)
{
    assert(manager != NULL);
    return (Subprocess*) MapGet(manager->procs_by_id, id);
}

Subprocess *ProcManagerGetProcessByFD(ProcManager *manager, int fd)
{
    assert(manager != NULL);
    return (Subprocess*) MapGet(manager->procs_by_fd, &fd);
}

Subprocess *ProcManagerGetProcessByStream(ProcManager *manager, FILE *stream)
{
    assert(manager != NULL);
    int fd = fileno(stream);
    return (Subprocess*) MapGet(manager->procs_by_fd, &fd);
}


static inline void SoftRemoveProcFromMaps(ProcManager *manager, Subprocess *proc)
{
    assert(manager != NULL);
    assert(proc != NULL);

    MapRemoveSoft(manager->procs_by_id, proc->id);
    MapRemoveSoft(manager->procs_by_pid, &(proc->pid));

    int lookup_fd = -1;
    if (proc->lookup_io == 'i')
    {
        lookup_fd = fileno(proc->input);
        assert(lookup_fd != -1);
    }
    else if (proc->lookup_io == 'o')
    {
        lookup_fd = fileno(proc->output);
        assert(lookup_fd != -1);
    }

    if (lookup_fd != -1)
    {
        MapRemoveSoft(manager->procs_by_fd, &lookup_fd);
    }
}

Subprocess *ProcManagerPopProcessByPID(ProcManager *manager, pid_t pid)
{
    assert(manager != NULL);

    Subprocess *proc = MapGet(manager->procs_by_pid, &pid);
    SoftRemoveProcFromMaps(manager, proc);
    return proc;
}

Subprocess *ProcManagerPopProcessById(ProcManager *manager, const char *id)
{
    assert(manager != NULL);

    Subprocess *proc = MapGet(manager->procs_by_id, id);
    SoftRemoveProcFromMaps(manager, proc);
    return proc;
}

Subprocess *ProcManagerPopProcessByFD(ProcManager *manager, int fd)
{
    assert(manager != NULL);

    Subprocess *proc = MapGet(manager->procs_by_fd, &fd);
    SoftRemoveProcFromMaps(manager, proc);
    return proc;
}

Subprocess *ProcManagerPopProcessByStream(ProcManager *manager, FILE *stream)
{
    assert(manager != NULL);

    int fd = fileno(stream);
    Subprocess *proc = MapGet(manager->procs_by_fd, &fd);
    SoftRemoveProcFromMaps(manager, proc);
    return proc;
}


/**
 * @param lookup_fd needed because proc->input and proc->output are expected to be already closed
 *        and thus not valid for fileno()
 */
static inline void RemoveProcFromMaps(ProcManager *manager, Subprocess *proc, int lookup_fd)
{
    assert(manager != NULL);
    assert(proc != NULL);

    MapRemoveSoft(manager->procs_by_pid, &(proc->pid));

    if (lookup_fd != -1)
    {
        MapRemoveSoft(manager->procs_by_fd, &lookup_fd);
    }
    MapRemove(manager->procs_by_id, proc->id);
}

static bool ForceTermination(Subprocess *proc)
{
    assert(proc != NULL);

    if (proc->input != NULL)
    {
        int ret = fclose(proc->input);
        if (ret != 0)
        {
            Log(LOG_LEVEL_ERR, "Failed to close input to the process '%s:%jd' (FD: %d)",
                proc->id, (intmax_t) proc->pid, fileno(proc->input));
        }
    }
    if (proc->output != NULL)
    {
        int ret = fclose(proc->output);
        if (ret != 0)
        {
            Log(LOG_LEVEL_ERR, "Failed to close output from the process '%s:%jd' (FD: %d)",
                proc->id, (intmax_t) proc->pid, fileno(proc->output));
        }
    }

#ifndef __MINGW32__
    int ret = kill(proc->pid, SIGKILL);
    if (ret != 0)
    {
        Log(LOG_LEVEL_ERR, "Failed to send SIGKILL to the process '%s:%jd'",
            proc->id, (intmax_t) proc->pid);
    }

    bool retry = true;
    const int started = time(NULL);
    while (retry)
    {
        pid_t pid = waitpid(proc->pid, NULL, WNOHANG);
        if (pid > 0)
        {
            /* successfully reaped */
            retry = false;
        }
        else if (pid == 0)
        {
            /* still not terminated */
            int now = time(NULL);
            if ((now - started) > SIGKILL_TERMINATION_TIMEOUT)
            {
                Log(LOG_LEVEL_ERR, "Failed to terminate process '%s:%jd'",
                    proc->id, (intmax_t) proc->pid);
                return false;
            }
            /* else retry */
        }
        else if ((pid == -1) && (errno != EINTR))
        {
            Log(LOG_LEVEL_ERR, "Failed to wait for the process '%s:%jd'",
                proc->id, (intmax_t) proc->pid);
            return false;
        }
        /* else retry */
    }
    return true;
#else
    /* TODO: This will require spawning to happen via the ProcManager too. */
    Log(LOG_LEVEL_NOTICE, "Forceful termination of processes not implemented on Windows");
    return false;
#endif
}

bool ProcManagerTerminateProcessByPID(ProcManager *manager, pid_t pid,
                                     ProcessTerminator Terminator, void *data)
{
    assert(manager != NULL);

    Subprocess *proc = ProcManagerGetProcessByPID(manager, pid);
    if (proc == NULL)
    {
        Log(LOG_LEVEL_ERR, "No process with PID '%jd' to terminate", (intmax_t) pid);
        return false;
    }

    /* Determine lookup_fd now before termination closes the streams. */
    int lookup_fd = -1;
    if (proc->lookup_io == 'i')
    {
        lookup_fd = fileno(proc->input);
        assert(lookup_fd != -1);
    }
    else if (proc->lookup_io == 'o')
    {
        lookup_fd = fileno(proc->output);
        assert(lookup_fd != -1);
    }

    bool terminated = Terminator(proc, data);
    if (!terminated)
    {
        Log(LOG_LEVEL_NOTICE, "Failed to terminate the process '%s:%jd' gracefully",
            proc->id, (intmax_t) proc->pid);
        terminated = ForceTermination(proc);
    }

    if (terminated)
    {
        RemoveProcFromMaps(manager, proc, lookup_fd);
    }

    return terminated;
}

bool ProcManagerTerminateProcessById(ProcManager *manager, const char *id,
                                     ProcessTerminator Terminator, void *data)
{
    assert(manager != NULL);

    Subprocess *proc = ProcManagerGetProcessById(manager, id);
    if (proc == NULL)
    {
        Log(LOG_LEVEL_ERR, "No process with ID '%s' to terminate", id);
        return false;
    }

    /* Determine lookup_fd now before termination closes the streams. */
    int lookup_fd = -1;
    if (proc->lookup_io == 'i')
    {
        lookup_fd = fileno(proc->input);
        assert(lookup_fd != -1);
    }
    else if (proc->lookup_io == 'o')
    {
        lookup_fd = fileno(proc->output);
        assert(lookup_fd != -1);
    }

    bool terminated = Terminator(proc, data);
    if (!terminated)
    {
        Log(LOG_LEVEL_NOTICE, "Failed to terminate the process '%s:%jd' gracefully",
            proc->id, (intmax_t) proc->pid);
        terminated = ForceTermination(proc);
    }

    if (terminated)
    {
        RemoveProcFromMaps(manager, proc, lookup_fd);
    }

    return terminated;
}

bool ProcManagerTerminateProcessByStream(ProcManager *manager, FILE *stream,
                                         ProcessTerminator Terminator, void *data)
{
    assert(manager != NULL);

    Subprocess *proc = ProcManagerGetProcessByStream(manager, stream);
    if (proc == NULL)
    {
        Log(LOG_LEVEL_ERR, "No process to terminate found for given stream (fd: %d)",
            fileno(stream));
        return false;
    }

    /* Determine lookup_fd now before termination closes the streams. */
    int lookup_fd = -1;
    if (proc->lookup_io == 'i')
    {
        lookup_fd = fileno(proc->input);
        assert(lookup_fd != -1);
    }
    else if (proc->lookup_io == 'o')
    {
        lookup_fd = fileno(proc->output);
        assert(lookup_fd != -1);
    }

    bool terminated = Terminator(proc, data);
    if (!terminated)
    {
        Log(LOG_LEVEL_NOTICE, "Failed to terminate the process '%s:%jd' gracefully",
            proc->id, (intmax_t) proc->pid);
        terminated = ForceTermination(proc);
    }

    if (terminated)
    {
        RemoveProcFromMaps(manager, proc, lookup_fd);
    }

    return terminated;
}

bool ProcManagerTerminateProcessByFD(ProcManager *manager, int fd,
                                     ProcessTerminator Terminator, void *data)
{
    assert(manager != NULL);

    Subprocess *proc = ProcManagerGetProcessByFD(manager, fd);
    if (proc == NULL)
    {
        Log(LOG_LEVEL_ERR, "No process to terminate found for FD %d", fd);
        return false;
    }

    /* Determine lookup_fd now before termination closes the streams. */
    int lookup_fd = -1;
    if (proc->lookup_io == 'i')
    {
        lookup_fd = fileno(proc->input);
        assert(lookup_fd != -1);
    }
    else if (proc->lookup_io == 'o')
    {
        lookup_fd = fileno(proc->output);
        assert(lookup_fd != -1);
    }

    bool terminated = Terminator(proc, data);
    if (!terminated)
    {
        Log(LOG_LEVEL_NOTICE, "Failed to terminate the process '%s:%jd' gracefully",
            proc->id, (intmax_t) proc->pid);
        terminated = ForceTermination(proc);
    }

    if (terminated)
    {
        RemoveProcFromMaps(manager, proc, lookup_fd);
    }

    return terminated;
}

bool ProcManagerTerminateAllProcesses(ProcManager *manager,
                                      ProcessTerminator Terminator, void *data)
{
    assert(manager != NULL);

    /* Avoid removing items from the map while iterating over it. */
    Seq *to_remove = SeqNew(MapSize(manager->procs_by_id), NULL);

    bool all_terminated = true;
    MapIterator iter = MapIteratorInit(manager->procs_by_id);
    MapKeyValue *item;
    while ((item = MapIteratorNext(&iter)) != NULL)
    {
        Subprocess *proc = item->value;

        /* Determine lookup_fd now before termination closes the streams. */
        int lookup_fd = -1;
        if (proc->lookup_io == 'i')
        {
            lookup_fd = fileno(proc->input);
            assert(lookup_fd != -1);
        }
        else if (proc->lookup_io == 'o')
        {
            lookup_fd = fileno(proc->output);
            assert(lookup_fd != -1);
        }

        bool terminated = Terminator(proc, data);
        if (!terminated)
        {
            Log(LOG_LEVEL_NOTICE, "Failed to terminate the process '%s:%jd' gracefully",
                proc->id, (intmax_t) proc->pid);
            terminated = ForceTermination(proc);
        }

        if (terminated)
        {
            SeqAppend(to_remove, proc);

            /* Remove from map by FD here so that we don't have to remember the
             * FDs anywhere. */
            MapRemoveSoft(manager->procs_by_fd, &lookup_fd);
        }
        else
        {
            all_terminated = false;
        }
    }

    const size_t length = SeqLength(to_remove);
    for (size_t i = 0; i < length; i++)
    {
        RemoveProcFromMaps(manager, (Subprocess *) SeqAt(to_remove, i), -1);
    }
    SeqDestroy(to_remove);

    return all_terminated;
}
