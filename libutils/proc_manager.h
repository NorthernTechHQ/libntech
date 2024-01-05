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

#ifndef CFENGINE_PROC_MANAGER_H
#define CFENGINE_PROC_MANAGER_H

#include <sys/types.h>

/**
 * Struct to hold information about a subprocess.
 */
struct Subprocess_ {
    char *id;             /** unique ID of the subprocess                    [always not %NULL] */
    char *cmd;            /** command used to create the subprocess          [can be %NULL]     */
    char *description;    /** human-friendly description of the subprocess   [can be %NULL]     */
    pid_t pid;            /** PID of the subprocess                          [always >= 0]      */
    FILE *input;          /** stream of the input to the subprocess          [can be %NULL]     */
    FILE *output;         /** stream of the output from the subprocess       [can be %NULL]     */
    char lookup_io;       /** which stream/FD to use for lookup by stream/FD ['i', 'o' or '\0'] */
};
typedef struct Subprocess_ Subprocess;

/**
 * Destroy a subprocess.
 */
void SubprocessDestroy(Subprocess *proc);

typedef struct ProcManager_ ProcManager;

/**
 * Function to terminate a subprocess.
 *
 * @param proc the subprocess to terminate
 * @param data custom data passed to the function through the termination functions
 * @return     whether the termination was successful or not
 */
typedef bool (*ProcessTerminator) (Subprocess *proc, void *data);

/**
 * Get a new ProcManager.
 */
ProcManager *ProcManagerNew();

/**
 * Destroy a ProcManager.
 */
void ProcManagerDestroy();

/**
 * Add a subprocess to a ProcManager.
 *
 * @param manager     the ProcManager to add the process to          [cannot be %NULL]
 * @param id          ID of the process to add                       [string(pid) is used if %NULL]
 * @param cmd         command used to create the subprocess          [can be %NULL]
 * @param description human-friendly description of the subprocess   [can be %NULL]
 * @param pid         PID of the subprocess                          [required to be >=0]
 * @param input       stream of the input to the subprocess          [can be %NULL]
 * @param output      stream of the output from the subprocess       [-1 if N/A]
 * @param lookup_io   which stream/FD to use for lookup by stream/FD ['i', 'o' or '\0']
 *
 * @return whether the subprocess was successfully added or not (e.g. in case of
 *         the process with the same id, PID or lookup stream/FD added before)
 *
 * @note Ownership of #id, #cmd and #description is transferred to the #manager.
 */
bool ProcManagerAddProcess(ProcManager *manager,
                           char *id, char *cmd, char *description, pid_t pid,
                           FILE *input, FILE *output, char lookup_io);

/**
 * Getter functions for #ProcManager.
 *
 * @return %NULL if not found
 */
Subprocess *ProcManagerGetProcessByPID(ProcManager *manager, pid_t pid);
Subprocess *ProcManagerGetProcessById(ProcManager *manager, const char *id);
Subprocess *ProcManagerGetProcessByStream(ProcManager *manager, FILE *stream);
Subprocess *ProcManagerGetProcessByFD(ProcManager *manager, int fd);

/**
 * Functions to get and remove subprocesses from a #ProcManager.
 *
 * @return %NULL if not found
 * @note Destroy the returned #Subprocess with SubprocessDestroy().
 */
Subprocess *ProcManagerPopProcessByPID(ProcManager *manager, pid_t pid);
Subprocess *ProcManagerPopProcessById(ProcManager *manager, const char *id);
Subprocess *ProcManagerPopProcessByStream(ProcManager *manager, FILE *stream);
Subprocess *ProcManagerPopProcessByFD(ProcManager *manager, int fd);

/**
 * Termination functions for ProcManager.
 *
 * @param Terminator terminator function called on the matched process(es)
 * @param data       arbitrary data passed to the #Terminator function
 *
 * @return %true if terminated successfully, %false if not (e.g. when not found, errors are logged)
 *
 * @note If the #Terminator function returns %false, the default hard termination is
 *       attempted (closing FDs and doing kill -9). Once the subprocess is terminated,
 *       or the hard termination is attempted, the subprocess is removed from the
 *       #ProcManager.
 */
bool ProcManagerTerminateProcessByPID(ProcManager *manager, pid_t pid,
                                     ProcessTerminator Terminator, void *data);
bool ProcManagerTerminateProcessById(ProcManager *manager, const char *id,
                                     ProcessTerminator Terminator, void *data);
bool ProcManagerTerminateProcessByStream(ProcManager *manager, FILE *stream,
                                     ProcessTerminator Terminator, void *data);
bool ProcManagerTerminateProcessByFD(ProcManager *manager, int fd,
                                     ProcessTerminator Terminator, void *data);
bool ProcManagerTerminateAllProcesses(ProcManager *manager,
                                      ProcessTerminator Terminator, void *data);

/* TODO: */
/* Functions for spawning processes (see cfengine/core/libpromises/pipes.h) */

#endif  /* CFENGINE_PROC_MANAGER_H */
